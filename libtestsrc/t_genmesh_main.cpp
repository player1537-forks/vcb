#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "glew.h"
#include <GL/glut.h>

#include "vcbutils.h"
#include "ArcBall.h"
#include "vcbmath.h"
#include "vcbglutils.h"
#include "vcbtrimesh.h"

#include "atypes.h"

char projmode = 'P'; /* P for perspective, O for orthogonal */
projStruct proj;
viewStruct view;
lightStruct light0;

ArcBall spaceball;
int iwinWidth;
int iwinHeight;

GLuint  texobj;
int     mainwin, sidewin;

/* application specific data starts here */
int     nverts, nfacets;
float * vdata;
int   * fdata;
float * fnormals;

/* end of application specific data */

bbox abbox;

void load_data(void)
{
	int i;

	printf("in load file\n");

#if 1
	vcbTrimeshStl("datasets/STL/rfxs4h_19d646_a_pram.stl", &nverts, &nfacets, &vdata, &fdata, &fnormals);
#else
	vcbTrimeshStl("datasets/STL/brevi.stl", &nverts, &nfacets, &vdata, &fdata, &fnormals);
#endif

	abbox.low[0] = abbox.low[1] = abbox.low[2] = 1e6;
	abbox.high[0] = abbox.high[1] = abbox.high[2] = -1e6;

	for (i = 0; i < nverts * 3; i ++) {
		abbox.low[i%3] = VCB_MINVAL(abbox.low[i%3],vdata[i]);
		abbox.high[i%3] = VCB_MAXVAL(abbox.high[i%3],vdata[i]);
	}

	printf("load_data obtained bounding box: (%f %f %f) (%f %f %f)\n",abbox.low[0], abbox.low[1],
		abbox.low[2], abbox.high[0],abbox.high[1],abbox.high[2]);
}


void display (void)
{
	int i;
	vcbEnableOGLlight(0.1f);
	vcbSetOGLlight(GL_LIGHT0, 0.1, 0.8, light0.lightpos[0], light0.lightpos[1], light0.lightpos[2], 40.f);
	vcbSetOGLMaterial(GL_FRONT_AND_BACK, MATERIAL_COPPER, 1.0);

	spaceball.Update();
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


	/*
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glNormalPointer(GL_FLOAT, 3*sizeof(float), fnormals);
  	glVertexPointer(3, GL_FLOAT, 3*sizeof(float), vdata);
	glDrawElements(GL_TRIANGLES, nfacets*3, GL_UNSIGNED_INT, fdata);
	*/
	glBegin(GL_TRIANGLES);
	for (i = 0; i < nfacets; i ++) {
		glNormal3fv(&fnormals[i*3]);
		glVertex3fv(&vdata[fdata[i*3+0]*3]);
		glVertex3fv(&vdata[fdata[i*3+1]*3]);
		glVertex3fv(&vdata[fdata[i*3+2]*3]);
	}
	glEnd();

	glutSwapBuffers();
	glPopMatrix();
}



void cleanup (void)
{
	free(vdata);
	free(fdata);
	free(fnormals);

	printf("finished cleaning up ...\n");
	fflush(stdout);
}

int main(int argc, char ** argv)
{
	float oglverno;
	load_data();

	printf("after reading file got %d %d\n",nverts,nfacets);
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
