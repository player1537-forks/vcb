#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "vr_ibp_nfu.h"
#include "vr_mcube.h"
#include "vru.h"
#include "vr_trimesh.h"
#include "linalg.h"
#include "dmem.h"
#include "vr_mcube.h"
#include "vr_volren.h"

void voxtransf (float * rgba, void * vox)
{
   float voxel = *(float *)vox;

   voxel /= 75.f;

   rgba[0] = voxel;
   rgba[1] = 1.f - voxel;
   rgba[2] = fabs(voxel - 0.5f) * 2.f;
   rgba[3] = (float) ((voxel > 0.4f && voxel < 0.6f)
                 ? (1.f - fabs(voxel - 0.5f) * 10.f)
                 : 0);
}

int vr_nfu_raycaster(int npara, NFU_PARA *paras)
{
	//App vars
	VR_volren volren;
	unsigned char * framebuffer;
	float * voxels;

	//Vars from paras
	vrOpticalModel opticalmodel;
	VR_volren volr;
	float fov, aspect, hither, yon;
	float xmin, xmax, ymin, ymax;
	float center[3], eye[3], up[3],light[3];
	int light_id;
	int w, h;
	VR_TRANSFUNC transf;
	vrdatatype vrtype;
	int nval, origin[3], dsizes[3];
	float scales[3], stepsz;
	int imgbounds[4];
	vrVolrenDst dst;
	vrdatatype  fbtype;

	//Typecast vars from paras structure

	//printf("1111111111111111111111\n");

	opticalmodel = *(int *)paras[0].data;
	fov = *(float *)paras[1].data;
	aspect = *(float *)paras[2].data;
	hither = *(float *)paras[3].data;
	yon = *(float *) paras[4].data;
	xmin = *(float *)paras[5].data;
	xmax = *(float *)paras[6].data;
	ymin = *(float *)paras[7].data;
	ymax = *(float *)paras[8].data;
	memcpy(center,   paras[9].data,  3*sizeof(float));
	memcpy(eye,      paras[10].data, 3*sizeof(float));
	memcpy(up,       paras[11].data, 3*sizeof(float));
	memcpy(light,    paras[12].data, 3*sizeof(float));
	light_id =*(int*)paras[13].data;
	w = *(int *)     paras[14].data;
	h = *(int *)     paras[15].data;
	dst = *(vrVolrenDst*)paras[16].data;
	fbtype = *(int *)paras[17].data;
	vrtype = *(int *)paras[18].data;
	nval = *(int *) paras[19].data;

	memcpy(origin,  paras[20].data, 3*sizeof(int));
	memcpy(dsizes,  paras[21].data, 3*sizeof(int));
	memcpy(scales,  paras[22].data, 3*sizeof(int));
	stepsz = *(float *)paras[23].data;
	voxels = (void *)(paras[26].data);

	//printf("%f %f %f %f\n", fov, aspect,hither,yon);
	//printf("%f %f %f %f\n", xmin,xmax,ymin,ymax);
	//printf("%d %d %d\n",light_id,w,h);
	//printf("%d %d %d\n",dsizes[0],dsizes[1],dsizes[2]);

	//Malloc framebuffer
	framebuffer = (unsigned char *) malloc (w*h*4*sizeof(unsigned char));

	//Call Raycasting API
	volren = vr_newVolR(opticalmodel);
	vr_volrPerspective(volren, fov, aspect, hither, yon);
	vr_volrOrtho(volren, hither, yon, xmin, xmax, ymin, ymax);
	vr_volrViewPort(volren, w, h);
	vr_volrTransf(volren, voxtransf);
	vr_volrFramebuffer(volren, dst, fbtype, framebuffer);
	vr_volrSetLight(volren, light_id, light[0], light[1], light[2]);
	vr_volrLookAt(volren, center[0], center[1], center[2], \
		      eye[0], eye[1], eye[2], \
		      up[0], up[1], up[2]);

	vr_volrApplyTransformation(volren);
	vr_volrenR(volren, vrtype, voxels, nval, \
		   origin, dsizes, scales, stepsz, imgbounds);

	//output framebuffer into paras
	memcpy(paras[24].data, imgbounds, 4 *sizeof(int));
	memcpy(paras[25].data, framebuffer, w*h*4*sizeof(unsigned char));
	paras[24].len = 4 *sizeof(int);
	paras[25].len = w*h*4*sizeof(unsigned char);

	return 0;
}





int NFU_vr_mcblock(int npara, NFU_PARA *paras)
{

	//declared for NFU
	vrdatatype datatype;
	void * block;
	float isoval;
	int *dsizes;
	int wn;
	int nverts;
	int nfacets;
	float * varray_v;
	int * varray_i;
	int len_varray_v;
	int len_varray_i;

	//set args for NFU
	datatype = *(int *)(paras[0].data);
	block = (void *)(paras[1].data);
	isoval = *(float *)(paras[2].data);
	dsizes = (int *)(paras[3].data);
	wn = *(int *)(paras[4].data);

	nfacets = vr_mcblock( datatype, block, isoval, dsizes, wn, &nverts, &varray_v, &varray_i);

	len_varray_v = nverts * 6 * sizeof(float);
	len_varray_i = nfacets * 3 * sizeof(int);

	memcpy(paras[5].data, &nverts, sizeof(int));
	memcpy(paras[6].data, &nfacets, sizeof(int));
	memcpy(paras[7].data, varray_v, len_varray_v);
	memcpy(paras[8].data, varray_i, len_varray_i);

	paras[7].len = len_varray_v;
	paras[8].len = len_varray_i;

	return 0;
}




int load_file(char *filename, IBP_depot depot, IBP_attributes att, IBP_timer timeout, IBP_set_of_caps *caps)
{
	int fd, length;
	struct stat statbuf;
	char *buf;

	/* read file to buffer */
	if((fd = open(filename, O_RDONLY)) == -1){
		fprintf(stderr,"Can not open file %s\n", filename);
		return 0;
	}
	if(fstat(fd, &statbuf) < 0){
		fprintf(stderr, "Fstat %s error\n", filename);
		return 0;
	}
	length = statbuf.st_size;
	if(NULL == (buf = (char *)malloc(length))){
		fprintf(stderr, "Malloc buffer for %s error\n", filename);
		return 0;
	}
	if(length != read(fd, buf, length)){
		fprintf(stderr, "Read %s error\n", filename);
		return 0;
	}

	/* allocate space on IBP depot */
	if(NULL == (*caps = IBP_allocate(depot, timeout, length, att))){
		fprintf(stderr, "IBP_allocate failed %d\n", IBP_errno);
		return 0;
	}
	else{
		fprintf(stderr, "Allocate space for %s done!\n", filename);
	}
	/* store buffer to an IBP allocation */
	if(length != IBP_store((*caps)->writeCap, timeout, buf, length)){
        fprintf(stderr,"IBP store failed %d\n", IBP_errno);
		return 0;
	}
	else{
		fprintf(stderr, "Store %s done!\n", filename);
	}
	close(fd);
	free(buf);

	return length;
}


int load_buf(char *buf, int length, IBP_depot depot, IBP_attributes att, IBP_timer timeout, IBP_set_of_caps *caps)
{
	if(buf == NULL){
		fprintf(stderr, "Cannot store, NULL buffer\n");
		return 0;
	}
	if(length == 0){
		fprintf(stderr, "Cannot store buffer of length zero.\n");
		return 0;
	}
	/* allocate space on IBP depot */
	if(NULL == (*caps = IBP_allocate(depot, timeout, length, att))){
		fprintf(stderr, "IBP_allocate failed %d\n", IBP_errno);
		return 0;
	}
	/* store buffer to an IBP allocation */
	if(length != IBP_store((*caps)->writeCap, timeout, buf, length)){
        fprintf(stderr,"IBP store failed %d\n", IBP_errno);
		return 0;
	}

	return 0;
}
