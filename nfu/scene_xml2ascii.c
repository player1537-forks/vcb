/*
 * scene_xml2ascii.c -- convert XML scene setting to ascii string.
 *
 * Written by Huadong Liu, http://www.cs.utk.edu/~hliu/
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <libxml/parser.h>
#include "scene_xml2ascii.h"
#include "base64.h"
#include "rcs.h"

RCS_ID("@(#) $Id: scene_xml2ascii.c,v 1.3 2005/09/21 15:42:49 hliu Exp $")

int have_mcube=0;
int have_raycasting=0;
int have_streamline=0;

/**
 * get an xml tag element or property value.
 */ 
void 
get_xml_val(xmlDocPtr doc, xmlNodePtr node, char mode, 
		char val_type, void *val, char *xml_tag)
{
	char *tmp;
	
	switch (mode) {
	case 'e': /* for tag element. */
		assert(doc);
		tmp = xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
		break;
	case 'p': /* for tag property. */
		tmp = xmlGetProp(node, xml_tag);
		break;
	default:
		fprintf(stderr, "Invalide mode.\n");
		break;
	}

	if (!tmp) {
		fprintf(stderr, "Empty %s.\n", xml_tag);
		exit(1);
	}

	switch (val_type) {
	case 'i':
		*((int *)val) = atoi(tmp);
		break;
	case 'f':
	case 'd':
		*((float *)val) = atof(tmp);
		break;
	default:
		fprintf(stderr, "Invalide return type.\n");
		break;
	}
}

/**
 * Compose a string using a suffix and the original string. 
 */ 
char *
append_str(const char *org, const char *suffix)
{
	char *new;
	
	new = calloc(strlen(org)+strlen(suffix)+1, sizeof(char));
	strcpy(new, org);
	strcat(new, suffix);
	
	return new;
}

/**
 * get n triple values of tag prefix.
 */ 
void
get_triple_vals(xmlDocPtr doc, xmlNodePtr node, tripleValue *vals, 
		int n, const char *prefix)
{
	xmlNodePtr child, gchild;
	char *tag_x, *tag_y, *tag_z;
	int cnt=0;
	
	tag_x = append_str(prefix, "_x");
	tag_y = append_str(prefix, "_y");
	tag_z = append_str(prefix, "_z");
	
	child = node->xmlChildrenNode;
	while (child) {
		if (!xmlStrcmp(child->name, (const xmlChar *)prefix)) {
			gchild = child->xmlChildrenNode;
			while (gchild) {
				if (!xmlStrcmp(gchild->name, (const xmlChar *)tag_x)) {
					get_xml_val(doc, gchild, 'e', 'f', &vals[cnt].x, tag_x);
				}
				else if (!xmlStrcmp(gchild->name, (const xmlChar *)tag_y)) {
					get_xml_val(doc, gchild, 'e', 'f', &vals[cnt].y, tag_y);
				}
				else if (!xmlStrcmp(gchild->name, (const xmlChar *)tag_z)) {
					get_xml_val(doc, gchild, 'e', 'f', &vals[cnt].z, tag_z);
				}
				gchild = gchild->next;
			}
			cnt++;
		}
		child = child->next;
	}
	if (cnt < n) {
		fprintf(stderr, "Less than %d %s.\n", n, prefix);
		exit(1);
	}
}

void
get_lighting(xmlDocPtr doc, xmlNodePtr node, sceneViewing *psv)
{
	xmlNodePtr child;
	int cnt_ambient=0;
	int cnt_diffuse=0;
	int cnt_specular=0;

	get_xml_val(0, node, 'p', 'i', &psv->numLights, "num_lights");

	psv->lights = calloc(psv->numLights, sizeof(tripleValue));
	psv->halves = calloc(psv->numLights, sizeof(tripleValue));
	psv->lightVec = calloc(psv->numLights, sizeof(tripleValue));
	psv->ambient = calloc(psv->numLights, sizeof(float));
	psv->diffuse = calloc(psv->numLights, sizeof(float));
	psv->specular = calloc(psv->numLights, sizeof(float));

	child = node->xmlChildrenNode;
	while (child) {
		if (!xmlStrcmp(child->name, (const xmlChar *)"light")) {
			get_triple_vals(doc, node, psv->lights, psv->numLights, "light");
		}
		else if (!xmlStrcmp(child->name, (const xmlChar *)"half")) {
			get_triple_vals(doc, node, psv->halves, psv->numLights, "half");
		}
		else if (!xmlStrcmp(child->name, (const xmlChar *)"vector")) {
			get_triple_vals(doc, node, psv->lightVec, psv->numLights, "vector");
		}
		else if (!xmlStrcmp(child->name, (const xmlChar *)"ambient")) {
			get_xml_val(doc, child, 'e', 'f', 
					psv->ambient+cnt_ambient, "ambient");
			cnt_ambient++;
			if (cnt_ambient > psv->numLights) break;
		}
		else if (!xmlStrcmp(child->name, (const xmlChar *)"ambient")) {
			get_xml_val(doc, child, 'e', 'f', 
					psv->ambient+cnt_ambient, "ambient");
			cnt_ambient++;
			if (cnt_ambient > psv->numLights) break;
		}
		else if (!xmlStrcmp(child->name, (const xmlChar *)"diffuse")) {
			get_xml_val(doc, child, 'e', 'f', 
					psv->diffuse+cnt_diffuse, "diffuse");
			cnt_diffuse++;
			if (cnt_diffuse > psv->numLights) break;
		}
		else if (!xmlStrcmp(child->name, (const xmlChar *)"specular")) {
			get_xml_val(doc, child, 'e', 'f', 
					psv->specular+cnt_specular, "specular");
			cnt_specular++;
			if (cnt_specular > psv->numLights) break;
		}
		child = child->next;
	}
}

void
get_model_view_matrix(xmlDocPtr doc, xmlNodePtr node, float *matrix)
{
	xmlNodePtr child, gchild;
	int cnt=0;
	int m, n;
	float v;
	
	child = node->xmlChildrenNode;
	while (child) {
		if (!xmlStrcmp(child->name, (const xmlChar *)"element")) {
			gchild = child->xmlChildrenNode;
			while (gchild) {
				if (!xmlStrcmp(gchild->name, (const xmlChar *)"m")) {
					get_xml_val(doc, gchild, 'e', 'i', &m, "m");
				}
				else if (!xmlStrcmp(gchild->name, (const xmlChar *)"n")) {
					get_xml_val(doc, gchild, 'e', 'i', &n, "n");
				}
				else if (!xmlStrcmp(gchild->name, (const xmlChar *)"value")) {
					get_xml_val(doc, gchild, 'e', 'f', &v, "n");
				}
				gchild = gchild->next;
			}
			/* Todo: check m, n, v. */
			matrix[m*MODEL_VIEW_MATRIX_SZ+n] = v;
			cnt++;
		}
		child = child->next;
	}
	if (cnt < MODEL_VIEW_MATRIX_SZ * MODEL_VIEW_MATRIX_SZ) {
		fprintf(stderr, "Invalid matrix.\n");
		exit(1);
	}
}

/**
 * Get scene settings -- viewing parameters part.
 */ 
void 
parse_scene_viewing(xmlDocPtr doc, xmlNodePtr node, sceneViewing *psv)
{
	xmlNodePtr child, gchild;
		
	child = node->xmlChildrenNode;
	while (child) {
		if (!xmlStrcmp(child->name, (const xmlChar *)"lighting")) {
			get_lighting(doc, child, psv);
		}
		else if (!xmlStrcmp(child->name, 
					(const xmlChar *)"model_view_matrix")) {
			get_model_view_matrix(doc, child, psv->modelViewMatrix);
		}
		else if (!xmlStrcmp(child->name, 
					(const xmlChar *)"viewing_transform")) {
			get_triple_vals(doc, child, &psv->eye, 1, "eye");
			get_triple_vals(doc, child, &psv->center, 1, "center");
			get_triple_vals(doc, child, &psv->up, 1, "up");
		}
		else if (!xmlStrcmp(child->name, 
					(const xmlChar *)"projection_transform")) {
			gchild = child->xmlChildrenNode;
			while (gchild) {
				if (!xmlStrcmp(gchild->name, (const xmlChar *)"mode")) {
					get_xml_val(doc, gchild, 'e', 'i', &psv->projMode, "mode");
				}
				else if (!xmlStrcmp(gchild->name, (const xmlChar *)"near")) {
					get_xml_val(doc, gchild, 'e', 'f', &psv->near, "near");
				}
				else if (!xmlStrcmp(gchild->name, (const xmlChar *)"far")) {
					get_xml_val(doc, gchild, 'e', 'f', &psv->far, "far");
				}
				gchild = gchild->next;
			}
		}
		else if (!xmlStrcmp(child->name, 
					(const xmlChar *)"perspective_projection")) {
			gchild = child->xmlChildrenNode;
			while (gchild) {
				if (!xmlStrcmp(gchild->name, (const xmlChar *)"fov")) {
					get_xml_val(doc, gchild, 'e', 'f', &psv->fov, "fov");
				}
				else if (!xmlStrcmp(gchild->name, (const xmlChar *)"aspect")) {
					get_xml_val(doc, gchild, 'e', 'f', &psv->aspect, "aspect");
				}
				else if (!xmlStrcmp(gchild->name, (const xmlChar *)"hither")) {
					get_xml_val(doc, gchild, 'e', 'f', &psv->hither, "hither");
				}
				else if (!xmlStrcmp(gchild->name, (const xmlChar *)"yon")) {
					get_xml_val(doc, gchild, 'e', 'f', &psv->yon, "yon");
				}
				gchild = gchild->next;
			}
		}
		else if (!xmlStrcmp(child->name, 
					(const xmlChar *)"orthographic_projection")) {
			gchild = child->xmlChildrenNode;
			while (gchild) {
				if (!xmlStrcmp(gchild->name, (const xmlChar *)"left")) {
					get_xml_val(doc, gchild, 'e', 'f', &psv->left, "left");
				}
				else if (!xmlStrcmp(gchild->name, (const xmlChar *)"right")) {
					get_xml_val(doc, gchild, 'e', 'f', &psv->right, "right");
				}
				else if (!xmlStrcmp(gchild->name, (const xmlChar *)"bottom")) {
					get_xml_val(doc, gchild, 'e', 'f', &psv->bottom, "bottom");
				}
				else if (!xmlStrcmp(gchild->name, (const xmlChar *)"top")) {
					get_xml_val(doc, gchild, 'e', 'f', &psv->top, "top");
				}
				gchild = gchild->next;
			}
		}
		else if (!xmlStrcmp(child->name, 
					(const xmlChar *)"viewport_transform")) {
			gchild = child->xmlChildrenNode;
			while (gchild) {
				if (!xmlStrcmp(gchild->name, (const xmlChar *)"width")) {
					get_xml_val(doc, gchild, 'e', 'i', &psv->width, "width");
				}
				else if (!xmlStrcmp(gchild->name, (const xmlChar *)"height")) {
					get_xml_val(doc, gchild, 'e', 'i', &psv->height, "height");
				}
				gchild = gchild->next;
			}
		}

		child = child->next;
	}
}

/**
 * Get scene settings -- mcube part.
 */ 
void 
parse_scene_mcube(xmlDocPtr doc, xmlNodePtr node, sceneMcube *psm)
{
	xmlNodePtr child;
		
	child = node->xmlChildrenNode;
	while (child) {
		if (!xmlStrcmp(child->name, (const xmlChar *)"iso_value")) {
			get_xml_val(doc, child, 'e', 'f', &psm->isoValue, "iso_value");
		}

		child = child->next;
	}
}

/**
 * Get scene settings -- raycasting part.
 */ 
void 
parse_scene_raycast(xmlDocPtr doc, xmlNodePtr node, sceneRaycast *psr)
{
	xmlNodePtr child, gchild;

	child = node->xmlChildrenNode;
	if (! child) {
		fprintf(stderr, "Empty raycasting.\n");
		exit(1);
	}
	
	child = node->xmlChildrenNode;
	while (child) {
		if (!xmlStrcmp(child->name, (const xmlChar *)"scale")) {
			get_triple_vals(doc, node, &psr->scale, 1, "scale");
		}
		else if (!xmlStrcmp(child->name, (const xmlChar *)"color_scheme")) {
			get_xml_val(doc, child, 'e', 'i', &psr->colorScheme,"color_scheme");
		}
		else if (!xmlStrcmp(child->name, (const xmlChar *)"iso_range")) {
			get_xml_val(0, child, 'p', 'i', &psr->numIsoRange, "num_iso_range");
			
			psr->posIsoRange = calloc(psr->numIsoRange, sizeof(float));
			psr->sizeIsoRange = calloc(psr->numIsoRange, sizeof(float));
			psr->opaIsoRange = calloc(psr->numIsoRange, sizeof(float));
			assert(psr->posIsoRange && psr->sizeIsoRange && psr->opaIsoRange);
			
			gchild = child->xmlChildrenNode;
			int pos_cnt=0;
			int size_cnt=0;
			int opa_cnt=0;
			while (gchild) {
				if (!xmlStrcmp(gchild->name, (const xmlChar *)"pos")) {
					get_xml_val(doc, gchild, 'e', 'f', 
							psr->posIsoRange+pos_cnt, "pos");
					pos_cnt++;
					if (pos_cnt > psr->numIsoRange) break;
				}
				else if (!xmlStrcmp(gchild->name, (const xmlChar *)"size")) {
					get_xml_val(doc, gchild, 'e', 'f', 
							psr->sizeIsoRange+size_cnt, "size");
					size_cnt++;
					if (size_cnt > psr->numIsoRange) break;
				}
				else if (!xmlStrcmp(gchild->name, (const xmlChar *)"opa")) {
					get_xml_val(doc, gchild, 'e', 'f', 
							psr->opaIsoRange+opa_cnt, "opa");
					opa_cnt++;
					if (opa_cnt > psr->numIsoRange) break;
				}
				gchild = gchild->next;
			}

			if ((pos_cnt != psr->numIsoRange) || 
				(size_cnt != psr->numIsoRange) || 
				(opa_cnt != psr->numIsoRange)) {
				fprintf(stderr, "Invalid number of pos, size or opa.\n");
				exit(1);
			}
		}
		child = child->next;
	}
}

/**
 * Get scene settings -- streamline part.
 */ 
void 
parse_scene_streamline(xmlDocPtr doc, xmlNodePtr node, sceneStreamline *pss)
{
	xmlNodePtr child;

	child = node->xmlChildrenNode;
	if (! child) {
		fprintf(stderr, "Empty streamline setting.\n");
		exit(1);
	}
	
	child = node->xmlChildrenNode;
	while (child) {
		if (!xmlStrcmp(child->name, (const xmlChar *)"seeds")) {
			get_xml_val(0, child, 'p', 'i', &pss->numSeeds, "num_seeds");
				
			pss->seeds = calloc(pss->numSeeds, sizeof(tripleValue));
			assert(pss->seeds);

			get_triple_vals(doc, child, pss->seeds, pss->numSeeds, "seed");
		}
		else if (!xmlStrcmp(child->name, (const xmlChar *)"step_size")) {
			get_xml_val(doc, child, 'e', 'f', &pss->stepSize, "step_size");
		}
		else if (!xmlStrcmp(child->name, (const xmlChar *)"num_verts")) {
			get_xml_val(doc, child, 'e', 'i', &pss->numVerts, "num_verts");
		}
		
		child = child->next;
	}
}

/**
 * Encode scene settings to a ascii string.
 */ 
void
dumpstr(sceneViewing *psv, sceneMcube *psm, 
		sceneRaycast *psr, sceneStreamline *pss)
{
	int size;
	int tmp;
	int offset=0;
	void *input, *output;

	/* 
	 * Calculate size of input buffer. Note that pointers need to be 
	 * replaced with actual data. 
	 */

	/* viewing parameters are required. */
	size = sizeof(sceneViewing) -  6*sizeof(void *);
	size += 3*(sizeof(tripleValue) + sizeof(float)) * psv->numLights;

	/* showing whether mcube is included. */
	size += sizeof(int);
	/* mcube is optional. */
	if (have_mcube) {
		size += sizeof(sceneMcube);
	}

	/* showing whether raycasting is included. */
	size += sizeof(int);
	/* raycasting is optional. */
	if (have_raycasting) {
		size += sizeof(sceneRaycast) - 3*sizeof(void *);
		size += 3*sizeof(float) * psr->numIsoRange;
	}

	/* showing whether raycasting is included. */
	size += sizeof(int);
	/* streamline is optional. */
	if (have_streamline) {
		size += sizeof(sceneStreamline) - sizeof(void *);
		size += sizeof(tripleValue) * pss->numSeeds;
	}
	
	input = calloc(size, 1);

	/* base64 encoding only increases length of input by 4/3. */
	output = calloc(2*size, 1);
	
	memcpy(input+offset, &psv->numLights, sizeof(int));
	offset += sizeof(int);
	
	memcpy(input+offset, psv->lights, sizeof(tripleValue) * psv->numLights);
	offset += sizeof(tripleValue) * psv->numLights;
	
	memcpy(input+offset, psv->halves, sizeof(tripleValue) * psv->numLights);
	offset += sizeof(tripleValue) * psv->numLights;
		
	memcpy(input+offset, psv->lightVec, sizeof(tripleValue) * psv->numLights);
	offset += sizeof(tripleValue) * psv->numLights;

	memcpy(input+offset, psv->ambient, sizeof(float) * psv->numLights);
	offset += sizeof(float) * psv->numLights;
	
	memcpy(input+offset, psv->diffuse, sizeof(float) * psv->numLights);
	offset += sizeof(float) * psv->numLights;

	memcpy(input+offset, psv->specular, sizeof(float) * psv->numLights);
	offset += sizeof(float) * psv->numLights;

	tmp = sizeof(sceneViewing) - 6*sizeof(void *) - sizeof(int);
	memcpy(input+offset, psv->modelViewMatrix, tmp);
	offset += tmp;

	memcpy(input+offset, &have_mcube, sizeof(int));
	offset += sizeof(int);

	if (have_mcube) {
		memcpy(input+offset, psm, sizeof(sceneMcube));
		offset += sizeof(sceneMcube);
	}

	memcpy(input+offset, &have_raycasting, sizeof(int));
	offset += sizeof(int);
	
	if (have_raycasting) {
		tmp = sizeof(sceneRaycast) - 3*sizeof(void *);
		memcpy(input+offset, psr, tmp);
		offset += tmp;

		memcpy(input+offset, psr->posIsoRange, sizeof(float)*psr->numIsoRange);
		offset += sizeof(float)*psr->numIsoRange;
	
		memcpy(input+offset, psr->sizeIsoRange, sizeof(float)*psr->numIsoRange);
		offset += sizeof(float)*psr->numIsoRange;

		memcpy(input+offset, psr->opaIsoRange, sizeof(float)*psr->numIsoRange);
		offset += sizeof(float)*psr->numIsoRange;
	}

	memcpy(input+offset, &have_streamline, sizeof(int));
	offset += sizeof(int);

	if (have_streamline) {
		memcpy(input+offset, &pss->numSeeds, sizeof(int));
		offset += sizeof(int);

		memcpy(input+offset, pss->seeds, sizeof(tripleValue) * pss->numSeeds);
		offset += sizeof(tripleValue) * pss->numSeeds;
	
		memcpy(input+offset, &pss->stepSize, sizeof(float));
		offset += sizeof(float);
	
		memcpy(input+offset, &pss->numVerts, sizeof(int));
		offset += sizeof(int);
	}

	assert(offset == size);

	encode(input, size, output);
	printf("%s", (char *)output);
	fflush(stdout);
}

void 
print_xml(sceneViewing *psv, sceneMcube *psm, 
		sceneRaycast *psr, sceneStreamline *pss)
{
	int i, j;

	printf("Scene settings for viewing parameters\n");
	printf("    lighting\n");
	printf("\tnumLights=%d\n", psv->numLights);
	for (i=0; i<psv->numLights; i++) {
		printf("\tlight[%d]=(%f %f %f)\n", i, 
				psv->lights[i].x, psv->lights[i].y, psv->lights[i].z);
	}
	for (i=0; i<psv->numLights; i++) {
		printf("\thalf[%d]=(%f %f %f)\n", i, 
				psv->halves[i].x, psv->halves[i].y, psv->halves[i].z);
	}
	for (i=0; i<psv->numLights; i++) {
		printf("\tlightVec[%d]=(%f %f %f)\n", i, 
				psv->lightVec[i].x, psv->lightVec[i].y, psv->lightVec[i].z);
	}
	for (i=0; i<psv->numLights; i++) {
		printf("\tambient[%d]=%f\n", i, psv->ambient[i]);
	}
	for (i=0; i<psv->numLights; i++) {
		printf("\tdiffuse[%d]=%f\n", i, psv->diffuse[i]);
	}
	for (i=0; i<psv->numLights; i++) {
		printf("\tspecular[%d]=%f\n", i, psv->specular[i]);
	}
	
	printf("    model view matrix\n");
	for (i=0; i<MODEL_VIEW_MATRIX_SZ; i++) {
		printf("\t");
		for (j=0; j<MODEL_VIEW_MATRIX_SZ; j++) {
			printf("%f ", psv->modelViewMatrix[i*MODEL_VIEW_MATRIX_SZ+j]);
		}
		printf("\n");
	}
	
	printf("    viewing transformation\n");
	printf("\teye=(%f %f %f)\n", psv->eye.x, psv->eye.y, psv->eye.z);
	printf("\tcenter=(%f %f %f)\n", psv->center.x, psv->center.y,psv->center.z);
	printf("\tup=(%f %f %f)\n", psv->up.x, psv->up.y, psv->up.z);
	
	printf("    projection transformation\n");
	printf("\tprojMode=%d\n", psv->projMode);
	printf("\tnear=%f\n", psv->near);
	printf("\tfar=%f\n", psv->far);
	
	printf("    perspective projection\n");
	printf("\tfov=%f\n", psv->fov);
	printf("\taspect=%f\n", psv->aspect);
	printf("\thither=%f\n", psv->hither);
	printf("\tyon=%f\n", psv->yon);

	printf("    orthographic projection\n");
	printf("\tleft=%f\n", psv->left);
	printf("\tright=%f\n", psv->right);
	printf("\tbottom=%f\n", psv->bottom);
	printf("\ttop=%f\n", psv->top);

	printf("    viewport transoformation\n");
	printf("\twidth=%d\n", psv->width);
	printf("\theight=%d\n", psv->height);

	
	printf("Scene settings for marching cubes\n");
	printf("\tisoValue=%f\n", psm->isoValue);

	printf("Scene settings for raycasting\n");
	printf("\tscale=(%f %f %f)\n", psr->scale.x, psr->scale.y, psr->scale.z);
	printf("\tcolorScheme=%d\n", psr->colorScheme);
	printf("\tnumIsoRange=%d\n", psr->numIsoRange);
	for (i=0; i<psr->numIsoRange; i++) {
		printf("\tpos[%d]=%f, size[%d]=%f, ops[%d]=%f\n", 
				i, psr->posIsoRange[i], 
				i, psr->sizeIsoRange[i], 
				i, psr->opaIsoRange[i]);
	}
	
	printf("Scene settings for streamline\n");
	printf("\tnumSeeds=%d\n", pss->numSeeds);
	for (i=0; i<pss->numSeeds; i++) {
		printf("\tseed[%d]=(%f, %f, %f)\n", i,
				pss->seeds[i].x, pss->seeds[i].y, pss->seeds[i].z);
	}
	printf("\tstepSize=%f\n", pss->stepSize);
	printf("\tnumVerts=%d\n", pss->numVerts);
}

/**
 * Scene settings to a string.
 * Memory is not freed explicitly on error because the program exits anyway.
 */ 
void 
ss2str(char *xmlfile, sceneViewing *psv, sceneMcube *psm, 
		sceneRaycast *psr, sceneStreamline *pss)
{
	xmlDocPtr doc;
	xmlNodePtr node;
	int have_viewing=0;

	doc=xmlParseFile(xmlfile);
	if (! doc) {
		fprintf(stderr, "Cannot parse xml document %s.\n", xmlfile);
		exit(1);
	}

	node=xmlDocGetRootElement(doc);
	if (! node) {
		fprintf(stderr, "Empty xml document %s.\n", xmlfile);
		exit(1);
	}

	if (xmlStrcmp(node->name, (const xmlChar *)"scene_setting")){
		fprintf(stderr, "xml root node is not scene_setting.\n");
		exit(1);
	}

	if (strcmp("0.0.1", xmlGetProp(node,"version"))) {
		fprintf(stderr, "xml version does not match.\n");
		exit(1);
	}

	node = node->xmlChildrenNode;
	/**
	 * Assumes that scene setting xml is checked so that the following tags
	 * do not appear more than once.
	 */ 
	while (node) {
		if (!xmlStrcmp(node->name, (const xmlChar *)"viewing_parameters")) {
			parse_scene_viewing(doc, node, psv);
			have_viewing = 1;
		}
		else if (!xmlStrcmp(node->name, (const xmlChar *)"mcube")) {
			parse_scene_mcube(doc, node, psm);
			have_mcube = 1;
		}
		else if (!xmlStrcmp(node->name, (const xmlChar *)"raycasting")) {
			parse_scene_raycast(doc, node, psr);
			have_raycasting = 1;
		}
		else if (!xmlStrcmp(node->name, (const xmlChar *)"streamline")) {
			parse_scene_streamline(doc, node, pss);
			have_streamline = 1;
		}
		node = node->next;
	}
	
	if (!have_viewing) {
		fprintf(stderr, "viewing parameters are required.\n");
		exit(1);
	}

	if (!have_mcube && !have_raycasting && !have_streamline) {
		fprintf(stderr, "at least one of mcube, raycasting or streamlne "
				"is required.\n");
		exit(1);
	}
	
}

int 
main(int argc, char **argv)
{
	sceneViewing sv;
	sceneMcube sm;
	sceneRaycast sr;
	sceneStreamline ss;
	
	if (argc != 2) {
		fprintf(stderr, "Usage: %s xml_scene_setting\n", argv[0]);
		exit(1);
	}

	ss2str(argv[1], &sv, &sm, &sr, &ss);

#ifdef VERBOSE	
	print_xml(&sv, &sm, &sr, &ss);
#endif

	dumpstr(&sv, &sm, &sr, &ss);

	return 0;
}
