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
#include "vbutrcast.h"
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
#define MAXNUMPARTITIONS 512

unsigned char * framebuffer, * pixels;
int imgw, imgh;
int             dsize[3], sz[3], orig[3], centroid[3];
float           eye[3], coi[3];
int             ndims,nattribs;
vcbdatatype     rawdatatype;

vcbVolumeImg    imgs[MAXNUMPARTITIONS];
int             ncuts, nparts, which[4];
char  filename[256];


/* end of application specific data */

bbox abbox;

void load_data(char * rootname,
	       unsigned char * framebuffer,
	       int * dsize, int ncuts, 
	       float * eye, float * coi, 
	       int imgw, int imgh)
{
  int i, j, k;

  memset(framebuffer, 0, imgw*imgh*4);
  
  for (nparts = 1, i = 0; i < 3; i ++)
    nparts *= ncuts;
  
  printf("%s has %d partitions\n",rootname, nparts);

  for (i = 0; i < nparts; i ++) {
    sprintf(filename, "%s.%d.bin.f", rootname,i);
    if (NULL == (pixels = (unsigned char *) vcbReadBinm(filename, &rawdatatype, &ndims, 
				      orig, sz, &nattribs))){
      fprintf(stderr, "load_data: error loading datafile\n");
      exit(-1);
    }
    
    imgs[i].fb = pixels;
    imgs[i].origx = orig[1];
    imgs[i].origy = orig[0];
    imgs[i].w     = sz[1];
    imgs[i].h     = sz[0];
    
    for (j = 0, k = i; j < 3; k /= ncuts, j ++)
      which[j] = k % ncuts;
    
    for (j = 0; j < 3; j ++) {
      float interval = (dsize[j]*1.f)/ncuts;
      imgs[i].centroid[j] = (which[j] +0.5) * interval;
    }
    /*printf("cen: %f %f %f\n",imgs[i].centroid[0],imgs[i].centroid[1],imgs[i].centroid[2]);*/
  }
  
  vcbCompositeRegions(eye, coi, framebuffer, 4, imgw, imgh, nparts, imgs);

}

void display (void)
{
  int i;

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0., 1., 0., 1., -1., 1.);
  glRasterPos3f(0.f, 0.f, 1.f);

  glDrawPixels(imgw, imgh, GL_RGBA, GL_UNSIGNED_BYTE, framebuffer);

  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  glutSwapBuffers();
}


void cleanup (void)
{
  int i;
  for (i = 0; i < nparts; i ++)
    free(imgs[i].fb);
  
  free(framebuffer);
	
  printf("finished cleaning up ...\n");
  fflush(stdout);
}



int main(int argc, char ** argv)
{
  float oglverno;

  if (argc != 15) {
    fprintf(stderr,"usage: %s input xdim ydim zdim ncuts ex ey ez cx cy cz imgszx imgszy output_image\n",argv[0]);
    /* vcbcomp vort.01 128 128 128 2 300 300 300 64 64 64 512 512 vorts01.bmp */
    exit(1);
  }
  
  dsize[0] = atoi(argv[2]);
  dsize[1] = atoi(argv[3]);
  dsize[2] = atoi(argv[4]);
  ncuts    = atoi(argv[5]);
  eye[0]   = atof(argv[6]);
  eye[1]   = atof(argv[7]);
  eye[2]   = atof(argv[8]);
  coi[0]   = atof(argv[9]);
  coi[1]   = atof(argv[10]);
  coi[2]   = atof(argv[11]);
  imgw     = atoi(argv[12]);
  imgh     = atoi(argv[13]);

  /*  
  printf("dsize: %d %d %d\n",dsize[0],dsize[1],dsize[2]);
  printf("ncuts: %d\n",ncuts);
  printf("eye: %f %f %f, coi: %f %f %f\n",eye[0],eye[1],eye[2],coi[0],coi[1],coi[2]);
  printf("imgw: %d, imgh: %d\n",imgw,  imgh);
  */

  framebuffer = (unsigned char *)malloc(imgw*imgh*4);

  load_data(argv[1], framebuffer, dsize, ncuts, eye, coi, imgw, imgh);

  /*printf("%s completed\n",argv[0]);*/
  vcbImgWriteBMP(argv[14],framebuffer, 4, imgw, imgh);
  
  initApp(); /* initialize the application */

  mainwin = initGLUT(argc,argv);

  GLenum err = glewInit();
  if (GLEW_OK != err) {
    fprintf(stderr,"glew error, initialization failed\n");
    fprintf(stderr,"Glew Error: %s\n", glewGetErrorString(err)); 
  } 

  initGLsettings();
  atexit(cleanup);

  glutKeyboardFunc(keys);
  oglverno = vcbGetOGLVersion();
  /*printf("using ogl version %f\n",oglverno);*/
  
  glutMainLoop();
  
  return 0;
}
