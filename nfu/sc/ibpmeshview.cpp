#include <stdio.h>
//#include "glui.h"
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include "glew.h"
#include <GL/glut.h>
#include "vcbutils.h"
#include "ArcBall.h"
#include "vcbglutils.h"
#include "vcblinalg.h"
#include "vcbimage.h"
#include "atypes.h"

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

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

int nverts[64];
int nfacets[64];
float * vdata[64];
int   * fdata[64];
int totalfacets = 0;
int totalverts = 0;

//extern GLUI *glui;

int tstepcomp;
/* end of application specific data */

bbox abbox;


//globals for make func
vcbdatatype dtype;
int   i, ndims, orig, nvals, dummy;
char datasetname[200];
char dataname[200];
char setname[200];
char titlebar[200];

int oldtstep = 0;
extern int tstep;
extern int numoftsteps;
extern int numofparts;
extern int cycle;

void makevfdata(char *, char *);
extern int winid;
//END 

extern vcbMaterial color;
extern float intensity;

char perdatacomp[100];
char pertstepcomp[100];

/* functions to read directory list */
int scandirsorted(char *dir, char *contains) {
  DIR *directory;  struct dirent *entry;  int n = 0;

  if (!(directory = opendir(dir))) return -1;
  while((entry = readdir(directory)) != NULL)
    if((   strcmp(entry->d_name,"..")!=0)
       && (strcmp(entry->d_name,".")!=0)
       && (strstr(entry->d_name, contains)))
      n++;
  closedir(directory);
  
  return n;
}

void ibpstat (int dummy)
{
  int tmp, tmp1;
  tmp = scandirsorted(dataname, setname);
  tmp1 = (2*numofparts*numoftsteps);

  //printf("tmp = %d, tmp1 = %d\n",tmp, tmp1);
  sprintf(perdatacomp,"Dataset completion = %6.2f", 100.0f*((float)tmp/(float)tmp1));
}

void display (void)
{
  int i, q;
  char temp[8];
  
  if(glutGetWindow() != winid)
    glutSetWindow(winid);
  
  if(!cycle) { 
    if(oldtstep != tstep){
	sprintf(temp, "%.2d",tstep);
	makevfdata(datasetname, (char*)temp);
      }
    oldtstep = tstep;
  }
  else{
    if(tstep >= numoftsteps)
      tstep = 1;
    else
      tstep++;

    sprintf(temp,"%.2d",tstep);
    makevfdata(datasetname, (char *)temp);
  }

  spaceball.Update();
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glTranslated(view.center[0],view.center[1],view.center[2]);
  glMultMatrixf(spaceball.GetMatrix());
  glTranslated(-view.center[0],-view.center[1],-view.center[2]);

  glClear(GL_COLOR_BUFFER_BIT);
  glClear(GL_DEPTH_BUFFER_BIT);
  
  vcbOGLprint(0.01f,0.01f, 1.f, 1.f, 0.f, perdatacomp);
  vcbOGLprint(0.4f,0.01f, 1.f,1.f,0.f, pertstepcomp); 
  
  vcbSetOGLMaterial(GL_FRONT_AND_BACK, color, 1.0);

  glEnableClientState(GL_NORMAL_ARRAY);
  glEnableClientState(GL_VERTEX_ARRAY);
  
  for(q = 0; q < numofparts; q++) {
    if((nfacets[q] > 0) && (nverts[q] > 0)) {
      glNormalPointer(GL_FLOAT, 6*sizeof(float), &vdata[q][3]);
      glVertexPointer(3, GL_FLOAT, 6*sizeof(float), vdata[q]);
      glDrawElements(GL_TRIANGLES, nfacets[q]*3, GL_UNSIGNED_INT, fdata[q]);
    }
  }

  glutSwapBuffers();
  glPopMatrix();

}


void cleanup (void)
{
  
  for(i = 0; i < numofparts; i++) { 
    free(vdata[i]);
    free(fdata[i]);
  }
#if 1
  if(strcmp(setname, "jet") == 0)
    system("/bin/rm gs_jet_*");
  if(strcmp(setname, "vorts") == 0)
    system("/bin/rm gs_vorts_*");
#endif

  system("killall lors_compute");
  system("killall lors_launch");
  printf("finished cleaning up ...\n");

}

void makevfdata(char *datasetname, char *tsteps)
{
  char varray[100];
  char tarray[100];
  int i, q;
  char temp[4];
  struct stat mbuf;
  
  tstepcomp = 2 * numofparts;
  
  for(i = 0; i < numofparts; i++) {
    totalverts = 0;
    nverts[i] = 0;
    totalfacets = 0;
    nfacets[i] = 0;
  }
  
  for(i = 0; i < numofparts; i++) {
    
    strcpy(varray, datasetname);
    strcpy(tarray, datasetname);
    
    strncat(varray, tsteps, strlen(tsteps));
    strncat(tarray, tsteps, strlen(tsteps));
    
    strncat(varray, ".", strlen("."));
    strncat(tarray, ".", strlen("."));
 
    sprintf(varray,"%s%d",varray,i);
    sprintf(tarray,"%s%d",tarray,i);

    //FOR VORTS
    if (!strcmp("vorts", setname)) {
      strncat(varray, ".bin.f.v.bin.f", 15);
      strncat(tarray, ".bin.f.t.bin.i", 15);
    }

    //FOR JET
    if ((!strcmp("jet", setname)) || !(strcmp("tsi", setname))) {
      strncat(varray, ".bin.ub.v.bin.f", 15);
      strncat(tarray, ".bin.ub.t.bin.i", 15);
    }

    memset(&mbuf, 0, sizeof(mbuf));
    stat(varray, &mbuf) ;
    
    if(mbuf.st_size != 0) {
      vdata[i] = (float *) vcbReadBinm(varray, &dtype, &ndims, &orig, &nverts[i], &nvals);
      if ((vdata[i] == NULL) || (dtype != VCB_FLOAT)) {
	tstepcomp--;
	printf("Did not get vdata of partition %d of timestep %d\n", i, tstep);
	nfacets[i] = 0;
      }

      printf("%s: %d type, %d %d %d %d\n",varray, dtype, ndims,orig,nverts,nvals);
      
      fdata[i] = (int *)  vcbReadBinm(tarray, &dtype, &ndims, &orig, &nfacets[i], &nvals);
      if ((fdata[i] == NULL) || (dtype != VCB_INT)) {
	tstepcomp--;
	printf("Did not get fdata of partition %d of timestep %d\n", i, tstep);
	nfacets[i] = 0;
      }
    }
    else {
      printf("Did not get vdata or fdata from partition %d of timestep %d\n", i, tstep);
      nfacets[i] = 0;
    }
  }
  
  for(i = 0; i < numofparts; i++) {
    totalverts += nverts[i];
    totalfacets += nfacets[i];
  }
  printf("Got %d vertices and %d triangles\n", totalverts, totalfacets);
  
  sprintf(pertstepcomp,"Timestep %.2d completion = %6.2f", tstep, 100.0f*(float)tstepcomp/(float)(2*numofparts));

}

int main(int argc, char ** argv)
{
  float oglverno;
  
  /* See if user passed an argument. */
  if (argc != 5) {
    fprintf(stderr, "Usage: %s varray.bin.f triarray.bin.i\n", argv[0]);
    exit(1);
  }

  numoftsteps = atoi(argv[3]);
  numofparts = atoi(argv[4]);

  printf("argv[1] = %s\n", argv[1]);

  strcpy(dataname, argv[1]);
  strcpy(setname, argv[2]);
  
  strcpy(datasetname, argv[1]);
  strncat(datasetname, argv[2], strlen(argv[2]));
  
  printf("datasetname = %s\n", datasetname);
  
  makevfdata(datasetname, "01");
  
  if(strcmp(setname, "vorts") == 0) {
    abbox.low[0] = abbox.low[1] = abbox.low[2] = 0;
    abbox.high[0] = abbox.high[1] = abbox.high[2] = 128;
  }
  
  if(strcmp(setname, "jet") == 0) {
    abbox.low[0] = abbox.low[1] = abbox.low[2] = 0;
    abbox.high[0] = abbox.high[1] = abbox.high[2] = 256;
  }
  
  if(strcmp(setname, "tsi") == 0) {
    abbox.low[0] = abbox.low[1] = abbox.low[2] = 0;
    abbox.high[0] = abbox.high[1] = abbox.high[2] = 864;
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
  
  glutKeyboardFunc(keys);
  glutTimerFunc(2000, ibpstat, 0);

  oglverno = vcbGetOGLVersion();
  printf("using ogl version %f\n",oglverno);
  
  vcbEnableOGLlight(0.1f);
  vcbSetOGLlight(GL_LIGHT0, 0.1, intensity, light0.lightpos[0], light0.lightpos[1], light0.lightpos[2], 40.f);

  glutMainLoop();
  
  return 0;
}

