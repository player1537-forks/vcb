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
#include <sys/types.h>
#include <dirent.h>

#define INTSIZE 10


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

extern int winid;
char datasetname[200];
char dataname[200];
char setname[80];
extern int tstep;
extern int numoftsteps;
extern int numofparts;
extern int cycle;

int oldtstep = 1;

int tstepcomp;
char perdatacomp[100];
char pertstepcomp[100];

/* end of application specific data */

bbox abbox;


int scandirsorted(char *dir, char *contains) {
  DIR *directory;  struct dirent *entry;  int n = 0;
	 
  if (!(directory = opendir(dir))) return -1;
  while((entry = readdir(directory)) != NULL)
    if(   (strcmp(entry->d_name,"..")!=0)
       && (strcmp(entry->d_name,".")!=0)
       && (strstr(entry->d_name, contains)))
      n++;
  closedir(directory);
  
  return n;
}



void load_data(char * rootname,
	       unsigned char * framebuffer,
	       int * dsize, int ncuts, 
	       float * eye, float * coi, 
	       int imgw, int imgh)
{
  int i, j, k;

  tstepcomp = 2 * numofparts;

  memset(framebuffer, 0, imgw*imgh*4);
  
  for (nparts = 1, i = 0; i < 3; i ++)
    nparts *= ncuts;
  
  //printf("%s has %d partitions\n",rootname, nparts);

  for (i = 0; i < nparts; i ++) {
    if(strcmp(setname, "vorts") == 0) {
      sprintf(filename, "%s.%d.bin.f", rootname,i);
    }
    if((strcmp(setname, "jet") == 0) || (strcmp(setname, "tsi") == 0)) {
      sprintf(filename, "%s.%d.bin.ub", rootname, i);
      //printf("Filename = %s\n", filename);
    }
    if (NULL == (pixels = (unsigned char *) vcbReadBinm(filename, &rawdatatype, &ndims, 
							orig, sz, &nattribs))){
      printf("Did not load partiton %d of timestep %d\n", i, tstep);
      tstepcomp--;
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

  sprintf(pertstepcomp,"Timestep %.2d completion = %6.2f", tstep, 100.0f* (float)tstepcomp/(float)(2*numofparts));
  
}

void ibpstat (int dummy)
{
  int tmp, tmp1;
  tmp = scandirsorted(dataname, setname);
  tmp1 = (2*numofparts*numoftsteps);

  sprintf(perdatacomp,"Dataset completion = %6.2f", 100.0f*((float)tmp/(float)tmp1));
}

void display (void)
{
  int i;
  char temp[6];
  
  if(glutGetWindow() != winid)
    glutSetWindow(winid);
  

  if(!cycle) {
    if(oldtstep != tstep){
	sprintf(temp, "%.2d",tstep);
	strcpy(datasetname, dataname);
	strncat(datasetname, setname, strlen(setname));
	strncat(datasetname, temp, strlen(temp));
	printf("Loading %s\n", datasetname);
	load_data(datasetname,framebuffer,dsize,ncuts,eye,coi,imgw,imgh);
	oldtstep = tstep;
    }
  }
  else{
    if(tstep >= numoftsteps)
      tstep = 1;
    else
      tstep++;

    sprintf(temp,"%.2d",tstep);
    strcpy(datasetname, dataname);
    strncat(datasetname, setname, strlen(setname));
    strncat(datasetname, temp, strlen(temp));
    printf("Loading %s\n", datasetname);
    load_data(datasetname,framebuffer,dsize,ncuts,eye,coi,imgw,imgh);
  }
  
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0., 1., 0., 1., -1., 1.);
  glRasterPos3f(0.f, 0.f, 1.f);

  glClear(GL_COLOR_BUFFER_BIT);
  glClear(GL_DEPTH_BUFFER_BIT);

  glDrawPixels(imgw, imgh, GL_RGBA, GL_UNSIGNED_BYTE, framebuffer);

  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  vcbOGLprint(0.01f,0.9f, 1.f, 1.f, 0.f,perdatacomp);
  vcbOGLprint(0.4f,0.9f, 1.f,1.f,0.f, pertstepcomp);

  glutSwapBuffers();

}


void cleanup (void)
{
  int i;
  for (i = 0; i < nparts; i ++)
    free(imgs[i].fb);
  
  free(framebuffer);
  
  if(strcmp(setname, "jet") == 0)
    system("/bin/rm gs_jet_*");
  if(strcmp(setname, "vorts") == 0)
    system("/bin/rm gs_vorts_*"); 

  system("killall lors_compute");
  system("killall lors_launch");

  printf("finished cleaning up ...\n");

}


int main(int argc, char ** argv)
{
  float oglverno;
  int i;
  
  if (argc != 18) {
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
  
  strcpy(datasetname, argv[1]);
  strcpy(dataname, argv[15]);
  strcpy(setname, argv[16]);
  numoftsteps = atoi(argv[17]);

  for (numofparts = 1, i = 0; i < 3; i ++)
    numofparts *= ncuts;
  
  load_data(datasetname, framebuffer, dsize, ncuts, eye, coi, imgw, imgh);
  
  /*printf("%s completed\n",argv[0]);*/
  // vcbImgWriteBMP(argv[14],framebuffer, 4, imgw, imgh);
  
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
  glutTimerFunc(2000, ibpstat, 0);

  oglverno = vcbGetOGLVersion();
  /*printf("using ogl version %f\n",oglverno);*/

  
  glutMainLoop();
  
  return 0;
}

