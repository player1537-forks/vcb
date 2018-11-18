#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "glew.h"
#include <GL/glut.h>

#include "vcbutils.h"
#include "ArcBall.h"
#include "vcbglutils.h"
#include "vcbmath.h"
#include "vcbspcurve.h"
#include "atypes.h"

char projmode = 'O'; /* P for perspective, O for orthogonal */
projStruct proj;
viewStruct view;
lightStruct light0;

ArcBall spaceball;
int iwinWidth;
int iwinHeight;

GLuint  texobj;
int     mainwin, sidewin;

VCBdata3d tensor;  /* space filling curve */
int numTime, numVert, numEle;
bbox abbox;


void load_data(void)
{
	int ndims, sizes[3], nattribs;
	int i, j, k;
	float * tensordata = NULL, * fptr;
	float minval, maxval;

	tensordata = (float *)vcbReadBinf("datasets/ORNLtensor/AlGG6full.binf", 
		VCB_FLOAT, VCB_LITTLEENDIAN, & ndims, sizes, & nattribs);

	if (tensordata == NULL) {
		fprintf(stderr, "failure to load tensor data\n");
		exit(0);
	}

	tensor = new_data3dstruct(VCB_FLOAT, sizes[0],sizes[1],sizes[2],nattribs,tensordata);
	printf("in load file, vcbBigEndian = %d\n", vcbBigEndian());

	minval = 1e6; maxval = -1e6;
	for (i = 0; i < sizes[0]*sizes[1]*sizes[2]*nattribs; i++) {
		minval = VCB_MINVAL(minval,vcb_p_f(tensor->v_list)[i]);
		maxval = VCB_MAXVAL(maxval,vcb_p_f(tensor->v_list)[i]);
	}

	printf("minval = %f, maxval = %f\n", minval, maxval);

	fptr = vcb_p_f(tensor->v_list);
	for (i = 0; i < tensor->isize1stDim; i ++)
		for (j = 0; j < tensor->isize2ndDim; j ++)
			for (k = 0; k < tensor->isize3rdDim; k ++) {
				vcbNormalize3(fptr); fptr += 3;
				vcbNormalize3(fptr); fptr += 3;
				vcbNormalize3(fptr); fptr += 3;	
			}

	abbox.low[0] = abbox.low[1] = abbox.low[2] = 0.f;
	abbox.high[0] = sizes[0];
	abbox.high[1] = sizes[1];
	abbox.high[2] = sizes[2];

	printf("bounding box: (%f %f %f) (%f %f %f)\n",abbox.low[0], abbox.low[1],
		abbox.low[2], abbox.high[0],abbox.high[1],abbox.high[2]);
}




void display (void)
{
	static int curtime; 

	int i,j,k;
	float * fptr, curcoor[3];
	float cur_view[3], cur_light[3];
	float m_shadedstreamline [16];

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

	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);

	glColor4f(1.f,0.f,0.f,1.f);
	glPointSize(10.f);
	glBegin(GL_POINTS);
	  glVertex3fv(view.center);
    glEnd();

	glColor4f(0.f, 1.f,0.f,1.f);

	glClientActiveTexture(GL_TEXTURE0_ARB);

	glColor4f(0.8, 0.8,0.8,1.0);

		fptr = vcb_p_f(tensor->v_list);

		glBegin(GL_LINES);

		for (i = 0; i < tensor->isize1stDim; i ++)
			for (j = 0; j < tensor->isize2ndDim; j ++)
				for (k = 0; k < tensor->isize3rdDim; k ++) {
					curcoor[0] = i + 0.5f;
					curcoor[1] = j + 0.5f;
					curcoor[2] = k + 0.5f;
					glColor4f(1.f,0.f,0.f,1.f);
					glVertex3fv(curcoor);
					glVertex3f(curcoor[0] + fptr[0], curcoor[1] + fptr[3], curcoor[2] + fptr[6]);
					fptr += 3;
					glColor4f(1.f,1.f,0.f,1.f);
					glVertex3fv(curcoor);
					glVertex3f(curcoor[0] + fptr[1], curcoor[1] + fptr[4], curcoor[2] + fptr[7]);
					fptr += 3;
					glColor4f(0.f,1.f,0.f,1.f);
					glVertex3fv(curcoor);
					glVertex3f(curcoor[0] + fptr[2], curcoor[1] + fptr[5], curcoor[2] + fptr[8]);
					fptr += 3;
				}

		glEnd();


	glutSwapBuffers();
	glPopMatrix();
}



void cleanup (void)
{
	free_data3dstruct(tensor);
	printf("finished cleaning up ...\n");
	fflush(stdout);
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

	oglverno = vcbGetOGLVersion();
	printf("using ogl version %f\n",oglverno);

	glutMainLoop();

	return 0;
}
