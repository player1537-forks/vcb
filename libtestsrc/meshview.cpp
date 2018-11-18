#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <time.h>

#include "glew.h"
#include <GL/glut.h>

#include "vcbutils.h"
#include "ArcBall.h"
#include "vcbmath.h"
#include "vcbglutils.h"
#include "vcblinalg.h"
#include "vcbmcube.h"
#include "vcbimage.h"

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

/* end of application specific data */

bbox abbox;



void display (void)
{
	int i;

	spaceball.Update();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslated(view.center[0],view.center[1],view.center[2]);
	glMultMatrixf(spaceball.GetMatrix());
	glTranslated(-view.center[0],-view.center[1],-view.center[2]);
	/*
	vcbEnableOGLlight(0.1f);
	vcbSetOGLlight(GL_LIGHT0, 0.1, 0.8, light0.lightpos[0], light0.lightpos[1], light0.lightpos[2], 40.f);
	vcbSetOGLMaterial(GL_FRONT_AND_BACK, MATERIAL_COPPER, 1.0);
	*/

	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);

        glEnableClientState(GL_NORMAL_ARRAY);
        glEnableClientState(GL_VERTEX_ARRAY);

        glNormalPointer(GL_FLOAT, 6*sizeof(float), &vdata[3]);
        glVertexPointer(3, GL_FLOAT, 6*sizeof(float), vdata);

        glDrawElements(GL_TRIANGLES, nfacets*3, GL_UNSIGNED_INT, fdata);
/*
	glBegin(GL_TRIANGLES);
	for (i = 0; i < nfacets; i ++) {
		glNormal3fv(&vdata[fdata[i*3+0]*6+3]);
		glVertex3fv(&vdata[fdata[i*3+0]*6+0]);
		glNormal3fv(&vdata[fdata[i*3+1]*6+3]);
		glVertex3fv(&vdata[fdata[i*3+1]*6+0]);
		glNormal3fv(&vdata[fdata[i*3+2]*6+3]);
		glVertex3fv(&vdata[fdata[i*3+2]*6+0]);
	}
	glEnd();
*/


	glutSwapBuffers();
	glPopMatrix();

	glutSetWindowTitle("test1");
}


void meshviewkeys(unsigned char c, int x, int y)
{
  float fzin = 0.95;
  float fzout = 1.05;
  unsigned char * pixels;
  int i;
  
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
  case 's': /* save framebuffer */
    pixels = (unsigned char *) malloc(iwinWidth*iwinHeight*4*sizeof(unsigned char));
    glReadPixels(0,0,iwinWidth,iwinHeight,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
    vcbImgWriteBMP("pbufres.bmp",pixels,4, iwinWidth, iwinHeight);
    free(pixels);
    break;
  case 'q':
    exit(0);
    break;
  case 'n':
    for (i = 0; i < nverts; i ++) {
      vdata[i*6+3] *= -1.f;
      vdata[i*6+4] *= -1.f;
      vdata[i*6+5] *= -1.f;
    }
    break;
  default:
    break;
  }
}

void cleanup (void)
{
	free(vdata);
	free(fdata);

	printf("finished cleaning up ...\n");
	fflush(stdout);
}

int main(int argc, char ** argv)
{
  float oglverno;
  vcbdatatype dtype;
  int   i, ndims, orig, nvals, dummy;

  /* See if user passed an argument. */
  if (argc != 3) {
    fprintf(stderr, "Usage: %s varray.bin.f triarray.bin.i\n", argv[0]);
    exit(1);
  }

  vdata = (float *) vcbReadBinm(argv[1], &dtype, &ndims, &orig, &nverts, &nvals);
  if ((vdata == NULL) || (dtype != VCB_FLOAT)) {
    fprintf(stderr, "The vertex array file does not seem right. quitting ...\n");
    exit(2);
  }

  printf("%s: %d type, %d %d %d %d\n",argv[1], dtype, ndims,orig,nverts,nvals);

  fdata = (int *)  vcbReadBinm(argv[2], &dtype, &ndims, &orig, &nfacets, &nvals);
  if ((fdata == NULL) || (dtype != VCB_INT)) {
    fprintf(stderr, "The triangle array file does not seem right. quitting ...\n");
    exit(3);
  }

  printf("after reading file got %d %d\n",nverts,nfacets);
  abbox.low[0] = abbox.low[1] = abbox.low[2] = 1e6;
  abbox.high[0] = abbox.high[1] = abbox.high[2] = -1e6;

  for (i = 0; i < nverts * 6; i += 6) {
    abbox.low [i%3]     = VCB_MINVAL(abbox.low [i%3],    vdata[i]);
    abbox.high[i%3]     = VCB_MAXVAL(abbox.high[i%3],    vdata[i]);
    abbox.low [(i+1)%3] = VCB_MINVAL(abbox.low [(i+1)%3],vdata[i+1]);
    abbox.high[(i+1)%3] = VCB_MAXVAL(abbox.high[(i+1)%3],vdata[i+1]);
    abbox.low [(i+2)%3] = VCB_MINVAL(abbox.low [(i+2)%3],vdata[i+2]);
    abbox.high[(i+2)%3] = VCB_MAXVAL(abbox.high[(i+2)%3],vdata[i+2]);
  }

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

  glutKeyboardFunc(meshviewkeys);
  //glutKeyboardFunc(keys);
  oglverno = vcbGetOGLVersion();
  printf("using ogl version %f\n",oglverno);
  
  vcbEnableOGLlight(0.1f);
  vcbSetOGLlight(GL_LIGHT0, 0.1, 0.8, light0.lightpos[0], light0.lightpos[1], light0.lightpos[2], 40.f);
  vcbSetOGLMaterial(GL_FRONT_AND_BACK, MATERIAL_COPPER, 1.0);

  glutMainLoop();
  
  return 0;
}
