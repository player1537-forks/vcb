#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <time.h>

#include "glew.h"
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include "vcbutils.h"
#include "ArcBall.h"
#include "vcbmath.h"
#include "vcbglutils.h"
#include "vcblinalg.h"
#include "vcbmcube.h"
#include "vcbcache.h"

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

int *ids;
int *dsizes;
int nvoxels;
int ndims;
float *p1;
float *p2;

/* end of application specific data */
bbox abbox;

/* ------------------------------------------------------------------------- */

void cleanup (void) {
   free(ids);
	printf("finished cleaning up ...\n");
	fflush(stdout);
}

/* ------------------------------------------------------------------------- */

void display (void) {
	int i;
	vcbEnableOGLlight(0.1f);
	vcbSetOGLlight(GL_LIGHT0, 0.1, 0.8, light0.lightpos[0], light0.lightpos[1],
                  light0.lightpos[2], 40.f);
	vcbSetOGLMaterial(GL_FRONT_AND_BACK, MATERIAL_COPPER, 1.0);

	spaceball.Update();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslated(view.center[0],view.center[1],view.center[2]);
	glMultMatrixf(spaceball.GetMatrix());
	glTranslated(-view.center[0],-view.center[1],-view.center[2]);

	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);

   glEnable(GL_COLOR_MATERIAL);
   glColor4f(1.0, 1.0, 1.0, 1.0);
	glBegin(GL_LINES);
      glVertex3fv(p1);
      glVertex3fv(p2);
	glEnd();

   glColor4f(1.0, 1.0, 1.0, 1.0);
   if (ndims == 3 || ndims == 4) {
      glPushMatrix();
         glTranslatef(dsizes[0] / 2.0, dsizes[1] / 2.0, dsizes[2] / 2.0);
         glScalef(dsizes[0], dsizes[1], dsizes[2]);
         glutWireCube(1.0);
      glPopMatrix();
   } else if (ndims == 2) {
      glBegin(GL_LINE_LOOP);
         glVertex2f(0.0f, 0.0f);
         glVertex2f(dsizes[0], 0.0f);
         glVertex2f(dsizes[0], dsizes[1]);
         glVertex2f(0.0f, dsizes[1]);
      glEnd();
   }

   for (i = 0; i < nvoxels; i++) {
      if (ndims == 3) {
         glColor4f(1.0, 0.0, 0.0, 0.7);
         glPushMatrix();
         glTranslatef(ids[i] % dsizes[0] + 0.5,
                      (ids[i] % (dsizes[0] * dsizes[1])) / dsizes[0] + 0.5,
                      ids[i] / (dsizes[0] * dsizes[1]) + 0.5);
         glutSolidCube(1.0);
         glPopMatrix();
      } else if (ndims == 4) {
         /* printf("%d - %d,%d - %d - %f\n", ids[i], dsizes[0]*dsizes[1]*dsizes[2], ids[i]/(dsizes[0]*dsizes[1]*dsizes[2]), dsizes[3], (float) ids[i] / */
                                   /* (dsizes[0] * dsizes[1] * dsizes[2]) /  */
                                   /* dsizes[3]); */
         glColor4f(1.0,
                   (float) ids[i] / (dsizes[0] * dsizes[1] * dsizes[2]) /
                      dsizes[3],
                   0.0,
                   0.7);
         glPushMatrix();
         glTranslatef(ids[i] % dsizes[0] + 0.5,
                      (ids[i] % (dsizes[0] * dsizes[1])) / dsizes[0] + 0.5,
                      (ids[i] % (dsizes[0] * dsizes[1] * dsizes[2])) /
                         (dsizes[0] * dsizes[1]) + 0.5);
         glutSolidCube(1.0);
         glPopMatrix();
      } else if (ndims == 2) {
         glColor4f(1.0, 0.0, 0.0, 0.7);
         glBegin(GL_POLYGON);
            glNormal3f(0.0, 0.0, -1.0);
            glVertex2f(ids[i] % dsizes[0], ids[i] / dsizes[0]);
            glVertex2f(ids[i] % dsizes[0] + 1, ids[i] / dsizes[0]);
            glVertex2f(ids[i] % dsizes[0] + 1, ids[i] / dsizes[0] + 1);
            glVertex2f(ids[i] % dsizes[0], ids[i] / dsizes[0] + 1);
         glEnd();
      } else {
         printf("No display registered for %d dimensions.\n", ndims);
      }
   }

	glutSwapBuffers();
	glPopMatrix();
}

/* ------------------------------------------------------------------------- */

int main(int argc, char ** argv) {
	float oglverno;
   int i;

   ndims = (argc - 1) / 2;
   dsizes = (int *) malloc(sizeof(int) * ndims);
   if (dsizes == NULL) {
      fprintf(stderr, "Couldn't allocate memory.\n");
      exit(1);
   }

   p1 = (float *) malloc(sizeof(float) * ndims);
   if (p1 == NULL) {
      fprintf(stderr, "Couldn't allocate memory.\n");
      exit(1);
   }

   p2 = (float *) malloc(sizeof(float) * ndims);
   if (p2 == NULL) {
      fprintf(stderr, "Couldn't allocate memory.\n");
      exit(1);
   }

   printf ("ndims: %d\n", ndims);

   if (argc == 5 || argc == 7 || argc == 9) {
      for (i = 0; i < ndims; i++) {
         dsizes[i] = 51;
         p1[i] = atof(argv[i + 1]);
         p2[i] = atof(argv[ndims + i + 1]);
      }
   } else {
      fprintf(stderr, "Invalid arguments.\n\n"
                      "Usage: %s x1 y1 [z1 [t1]] x2 y2 [z2 [t2]]\n", argv[0]);
      exit(1);
   }

   abbox.low[0] = abbox.low[1] = abbox.low[2] = 0;
   abbox.high[0] = abbox.high[1] = abbox.high[2] = 50;

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

   glLineWidth(3.0);
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   ids = vcbVoxelsBetweenPoints(ndims, dsizes, p1, p2, &nvoxels,
                                vcbIndexXYZEtc);

	glutMainLoop();

	return 0;
}

/* ------------------------------------------------------------------------- */

