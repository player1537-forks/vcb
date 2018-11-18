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


char projmode = 'P'; /* P for perspective, O for orthogonal */
projStruct proj;
viewStruct view;
lightStruct light0;

ArcBall spaceball;
int iwinWidth;
int iwinHeight;

GLuint  texobj;
int     mainwin, sidewin;

VCBdata1d spacecurve;  /* space filling curve */
int numTime, numVert, numEle;
bbox abbox;

//#define NUM_COMP 3
void load_data(void)
{
	int i, j, cnt;

	unsigned int nDims = 3;
	unsigned int nBits = 4;
	int size = vcbScurveSize(nDims, nBits);

	spacecurve = new_data1dstruct(VCB_INT, size, nDims, NULL);
	printf("in load file\n");


#if 0
	/*vcbGenHscurve(nDims, nBits, vcb_p_i(spacecurve->v_list));*/

	vcbBeginIncrHscurve(nDims, nBits);
	  for (i = 0; i < size; i ++) 
		  vcbIncrHscurve(& vcb_p_i(spacecurve->v_list)[i*nDims]);
	vcbEndIncrHscurve();
#else
	/*vcbGenZscurve(nDims, nBits, vcb_p_i(spacecurve->v_list));*/
	
	vcbBeginIncrZscurve(nDims, nBits);
	  for (i = 0; i < size; i ++) 
		  vcbIncrZscurve(& vcb_p_i(spacecurve->v_list)[i*nDims]);
	vcbEndIncrZscurve();
	
#endif

	abbox.low[0] = abbox.low[1] = abbox.low[2] = 1e6;
	abbox.high[0] = abbox.high[1] = abbox.high[2] = -1e6;

	cnt = 0;
	for (j = 0; j < spacecurve->inumAttribs*size; cnt++, j ++){
		abbox.low[cnt%3] = VCB_MINVAL(abbox.low[cnt%3],vcb_p_i(spacecurve->v_list)[j]);
		abbox.high[cnt%3] = VCB_MAXVAL(abbox.high[cnt%3],vcb_p_i(spacecurve->v_list)[j]);
	}

	printf("bounding box: (%f %f %f) (%f %f %f)\n",abbox.low[0], abbox.low[1],
		abbox.low[2], abbox.high[0],abbox.high[1],abbox.high[2]);
}




int curtime = 0;

void display (void)
{
	static int curtime; 

	int j;
	int * iptr;
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

		iptr = vcb_p_i(spacecurve->v_list);

		glBegin(GL_LINE_STRIP);
		for (j = 0; j < spacecurve->isize1stDim; glVertex3iv(iptr), iptr+= spacecurve->inumAttribs, j ++)
			   ;
		glEnd();


	glutSwapBuffers();
	glPopMatrix();
}

void cleanup (void)
{
	free_data1dstruct(spacecurve);
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
