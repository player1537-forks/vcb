#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "glew.h"
#include <GL/glut.h>

#include "vcbutils.h"
#include "ArcBall.h"
#include "vcbglutils.h"
#include "atypes.h"
#include "vcbmath.h"


char projmode = 'O'; /* P for perspective, O for orthogonal */
projStruct proj;
viewStruct view;
lightStruct light0;

ArcBall spaceball;
int iwinWidth;
int iwinHeight;

GLuint  texobj;
int     mainwin;

VCBdata3d vectors;  /* flow field */

int sizes[3], nattribs;

int numSTRMLN, numVert;
bbox abbox;

VCBdata1d * strmln;
VCBdata1d * tangents;
float * vectordata = NULL, * seeds, * curloc;
float  seed_x = 24.f;
float  seed_y = 24.f;
float  seed_z = 24.f;
float  stepsz = 10.f;

void gen_seeds(float * seeds, int numVert)
{
	int i, j, k, incr;
	incr = 0;
	for (i = -2; i < 3; i ++)
		for (j = -2; j < 3; j ++) 
			for (k = 0; k < 2; k ++) {
				seeds[incr*3  ] = seed_x + i;
				seeds[incr*3+1] = seed_y + j;
				seeds[incr*3+2] = seed_z + k;
				incr ++;
			}
}

int offsets[8];
#define myid3(x,y,z, dsize,i,na) ((((z)*dsize[1] + (y)) * dsize[0] + (x))*(na) + (i))

void get_vector(float t_val, float * coord, float * vector)
{
	/* trilinear interpolation */
	int   i,x, y, z, id0, off;
	float r, s, t, u, v, w, fac;
	float * voxels;
	static float fsam0, fsam1, fsam2;
	static float factors[8];

	if (coord[0] >= 47.f || coord[0] <= 0.f || coord[1] >= 47.f || coord[1] <= 0.f || coord[2] >= 47.f || coord[2] <= 0.f) {
		vector[0] = 0.f;
		vector[1] = 0.f;
		vector[2] = 0.f;
		return;
	}/* an example for using procedural flow field, instead of a flow field read in 
	else {
		dydx[0] = -coord[1] + 24.f;	
		dydx[1] = coord[0] - 24.f;
		dydx[2] = coord[2] - 24.f;
		vcbNormalize3(dydx);
		return;
	}*/

	voxels = vcb_p_f(vectors->v_list);

	x = (int) coord[0];
	y = (int) coord[1];
	z = (int) coord[2];
	r = coord[0] - x;
	s = coord[1] - y; 
	t = coord[2] - z;

	id0 = myid3(x,y,z,sizes,0,nattribs);
	u = 1.f - r;
	v = 1.f - s;
	w = 1.f - t;
                                           
	factors[0] = u*v*w;
	factors[1] = r*v*w;
	factors[2] = u*s*w;
	factors[3] = r*s*w;
	factors[4] = u*v*t;
	factors[5] = r*v*t; 
	factors[6] = u*s*t; 
	factors[7] = r*s*t;
 
	for (i = 0, fsam0 = fsam1 = fsam2 = 0.f; i < 8; i ++) {
		fac = factors[i];
		off = offsets[i];
		fsam0 += fac * voxels[id0 + off + 0];
		fsam1 += fac * voxels[id0 + off + 1];
		fsam2 += fac * voxels[id0 + off + 2];
	}

	vector[0] = fsam0;
	vector[1] = fsam1;
	vector[2] = fsam2;
}


void load_data(void)
{
	int i, j, k;
	float * fptr;
	float minval, maxval;
	FILE * file;

	printf("in load file\n");

	sizes[0] = sizes[1] = sizes[2] = 48;
	nattribs = 3;

	offsets[0] = 0;
	offsets[1] = 1;
	offsets[2] = sizes[0];
	offsets[3] = sizes[0]+1;
	offsets[4] = sizes[1]*sizes[0];
	offsets[5] = offsets[4] + 1;
	offsets[6] = offsets[4] + sizes[0];
	offsets[7] = offsets[4] + sizes[0] + 1;
	for (i = 0; i < 8; offsets[i]*= nattribs, i ++);

	/* load flow data */
	vectors = new_data3dstruct(VCB_FLOAT,sizes[0],sizes[1],sizes[2],nattribs,NULL);
	file = fopen("datasets/tornado/ftornado.bin","rb");
	fread(vectors->v_list,sizeof(float),sizes[0]*sizes[1]*sizes[2]*nattribs,file);
	fclose(file);

	numSTRMLN = 50;
	numVert   = 250;

	strmln   = (VCBdata1d *) malloc (sizeof(VCBdata1d)*numSTRMLN);
	tangents = (VCBdata1d *) malloc (sizeof(VCBdata1d)*numSTRMLN);
	seeds    = (float *) malloc (sizeof(float)*3*numSTRMLN);
	curloc   = (float *) malloc (sizeof(float)*3);

	/* compute streamlines */
	for (i = 0; i < numSTRMLN; i ++) {
		strmln[i]   = new_data1dstruct(VCB_FLOAT, numVert, 3, NULL);
		tangents[i] = new_data1dstruct(VCB_FLOAT, numVert, 3, NULL);
	}

	minval = 1e6; maxval = -1e6;
	for (i = 0; i < sizes[0]*sizes[1]*sizes[2]*nattribs; i++) {
		vcbToggleEndian(&vcb_p_f(vectors->v_list)[i],4);
		minval = VCB_MINVAL(minval,vcb_p_f(vectors->v_list)[i]);
		maxval = VCB_MAXVAL(maxval,vcb_p_f(vectors->v_list)[i]);
	}
	printf("minval = %f, maxval = %f\n", minval, maxval);

	abbox.low[0] = abbox.low[1] = abbox.low[2] = 0.f;
	abbox.high[0] = sizes[0];
	abbox.high[1] = sizes[1];
	abbox.high[2] = sizes[2];

	/* flip the y direction */
	fptr = vcb_p_f(vectors->v_list);
	float ftmp;
	for (k = 0; k < sizes[2]; k ++)
		for (j = 0; j < sizes[1]/2; j ++)
			for (i = 0; i < sizes[0]; i ++) {
				int id1, id2;
				id1 = myid3(i,j,k, sizes,0,nattribs);
				id2 = myid3(i,(sizes[1] - j - 1),k, sizes,0,nattribs);
				ftmp = fptr[id1]; fptr[id1] = fptr[id2]; fptr[id2] = ftmp;
				ftmp = fptr[id1+1]; fptr[id1+1] = fptr[id2+1]; fptr[id2+1] = ftmp;
				ftmp = fptr[id1+2]; fptr[id1+2] = fptr[id2+2]; fptr[id2+2] = ftmp;
			}

	printf("bounding box: (%f %f %f) (%f %f %f)\n",abbox.low[0], abbox.low[1],
		abbox.low[2], abbox.high[0],abbox.high[1],abbox.high[2]);	

}

void display (void)
{
	int i, j;
	static int curtime = 0;
	float * fptr, * tptr;
	float cur_view[3], cur_light[3];
	float m_shadedstreamline [16];
	float lbounds[4], ubounds[4];

	lbounds[0] = lbounds[1] = lbounds[2] = 0.f;
	ubounds[0] = (float)sizes[0];
	ubounds[1] = (float)sizes[1];
	ubounds[2] = (float)sizes[2];

	spaceball.Update();

	vcbCalcViewLightVecs(spaceball.GetMatrix(),view.eye,view.center,light0.lightpos, 
		cur_view, cur_light);

	vcbUpdateTexMatShadedStreamline(m_shadedstreamline,cur_view,cur_light);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMultMatrixf(m_shadedstreamline);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslated(view.center[0],view.center[1],view.center[2]);
	glMultMatrixf(spaceball.GetMatrix());
	glTranslated(-view.center[0],-view.center[1],-view.center[2]);

	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	glColor4f(1.f,0.f,0.f,1.f);
	glPointSize(10.f);
	glBegin(GL_POINTS);
	  glVertex3fv(view.center);
    glEnd();

	glColor4f(0.f, 1.f,0.f,1.f);

	gen_seeds(seeds, numSTRMLN);
	/* compute streamlines */
	for (i = 0; i < numSTRMLN; i ++) {

		fptr = vcb_p_f(strmln[i]->v_list);
		memcpy(curloc, &seeds[i*3], sizeof(float)*3);

		for (j = 0; j < numVert; j ++) {

			if (vcbAdvectFlow(curloc, 3, lbounds, ubounds, stepsz, 1e-3f, 0.5f, 1e-4f, get_vector) < 0) {
				strmln[i]->isize1stDim = j;
				break;
			}

			memcpy(&fptr[j*3], curloc, sizeof(float)*3);

		}

		tptr = (float*)tangents[i]->v_list;
		vcbData1DGradient(fptr, 3, strmln[i]->isize1stDim, 6,  tptr);

		for (j = 0; j < strmln[i]->isize1stDim; j ++)
			vcbNormalize3(&tptr[j*3]);
	}

	for (i = 0; i < numSTRMLN; i ++)
	{
		int numV = strmln[i]->isize1stDim;

		fptr = (float*) strmln[i]->v_list;
		tptr = (float*)tangents[i]->v_list;

		glBegin(GL_LINE_STRIP);
			for (j = 0; j < numV; glTexCoord3fv(tptr), glVertex3fv(fptr), tptr +=3, fptr+= 3, j ++)
			   ;
		glEnd();

	}

	/* compute streamlines */
	for (i = 0; i < numSTRMLN; i ++) {

		fptr = vcb_p_f(strmln[i]->v_list);
		memcpy(curloc, &seeds[i*3], sizeof(float)*3);

		for (j = 0; j < numVert; j ++) {

			if (vcbAdvectFlow(curloc, 3, lbounds, ubounds, -stepsz, 1e-3f, 0.5f, 1e-4f, get_vector) < 0) {
				strmln[i]->isize1stDim = j;
				break;
			}

			memcpy(&fptr[j*3], curloc, sizeof(float)*3);

		}

		tptr = (float*)tangents[i]->v_list;
		vcbData1DGradient(fptr, 3, strmln[i]->isize1stDim, 6,  tptr);

		for (j = 0; j < strmln[i]->isize1stDim; j ++)
			vcbNormalize3(&tptr[j*3]);
	}

	for (i = 0; i < numSTRMLN; i ++)
	{
		int numV = strmln[i]->isize1stDim;

		fptr = (float*) strmln[i]->v_list;
		tptr = (float*)tangents[i]->v_list;

		glBegin(GL_LINE_STRIP);
			for (j = 0; j < numV; glTexCoord3fv(tptr), glVertex3fv(fptr), tptr +=3, fptr+= 3, j ++)
			   ;
		glEnd();

	}

	glutSwapBuffers();
	glPopMatrix();
	curtime ++;
}



void cleanup (void)
{
	int i;
	for (i = 0; i < numSTRMLN; i ++) {
		free_data1dstruct(strmln[i]);
		free_data1dstruct(tangents[i]);
	}
	free(strmln);
	free(tangents);

	free(seeds);
	free(curloc);

	printf("finished cleaning up ...\n");
	fflush(stdout);
}

void strmline_keys(unsigned char c, int x, int y)
{
	float fzin = 0.95;
	float fzout = 1.05;
	switch(c) {
      case 'i':
		  proj.xmin *= fzin;
		  proj.xmax *= fzin;
		  proj.ymin *= fzin;
		  proj.ymax *= fzin;
		  proj.fov  *= fzin;
		  reshape(iwinWidth,iwinHeight);
		  break;
	  case 'o':
		  proj.xmin *= fzout;
		  proj.xmax *= fzout;
		  proj.ymin *= fzout;
		  proj.ymax *= fzout;
		  proj.fov  *= fzout;
		  reshape(iwinWidth,iwinHeight);
		  break;
	  case 'u':
		  if (seed_z < 47.f) seed_z += 1.f;
		  break;
	  case 'd':
		  if (seed_z > 0.f) seed_z -= 1.f;
		  break;
	  case 'l':
		  if (seed_x < 47.f) seed_x += 1.f;
		  break;
	  case 'r':
		  if (seed_x > 0.f) seed_x -= 1.f;
		  break;
	  case 'f':
		  if (seed_y < 47.f) seed_y += 1.f;
		  break;
	  case 'b':
		  if (seed_y > 0.f) seed_y -= 1.f;
		  break;
	  case 'z':
		  stepsz *= 1.2f;
		  break;
	  case 'x':
		  stepsz /= 1.2f;
		  break;
	  case 'q':
		  exit(0);
		  break;
	  default:
		  break;
  }
}

int main(int argc, char ** argv)
{
	float oglverno;
	load_data();

	printf("bounding box: (%f %f %f) (%f %f %f)\n",abbox.low[0], abbox.low[1],
		abbox.low[2], abbox.high[0],abbox.high[1],abbox.high[2]);

	initApp(); /* initialize the application */

	mainwin = initGLUT(argc,argv);

    GLenum err = glewInit();
    if (GLEW_OK != err) {
		fprintf(stderr,"glew error, initialization failed\n");
        fprintf(stderr,"Glew Error: %s\n", glewGetErrorString(err)); 
    } 

	initGLsettings();
	atexit(cleanup);

	glutKeyboardFunc(strmline_keys);

	oglverno = vcbGetOGLVersion();
	printf("using ogl version %f\n",oglverno);
	texobj = vcbInitShadedStreamline(GL_TEXTURE0,MATERIAL_GOLD);

	glutMainLoop();

	return 0;
}
