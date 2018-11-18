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

void dst_src_indices(int dst_ndims, int *dst_dsizes, int *dst_coords,
                     int src_ndims, int *src_dsizes, int *src_coords,
                     int *dst_index, int *src_index) {

   float len;
   int tmp;
   int i;

   len = 0.0f;
   for (i = 0; i < dst_ndims; i++) {
      tmp = dst_coords[i] - dst_dsizes[i] / 2;
      len += tmp * tmp;
   }
   len = sqrt(len);

   if (len > 25) {
      *dst_index = -1;
      *src_index = -1;
      return;
   }

   *dst_index = vcbIndexXYZEtc(dst_ndims, dst_dsizes, dst_coords);
   src_coords[0] = dst_coords[0];
   src_coords[1] = dst_coords[1];
   src_coords[2] = dst_coords[2];
   /* src_coords[3] = (int) (((float) dst_coords[0] / dst_dsizes[0]) * */
                           /* src_dsizes[3]); */
   src_coords[3] =
      (int) ((dst_dsizes[0] - 1 - fabs(2 * dst_coords[0] - dst_dsizes[0])) /
             dst_dsizes[0] * src_dsizes[3]);
   *src_index = vcbIndexXYZEtc(src_ndims, src_dsizes, src_coords);

}

/* ------------------------------------------------------------------------- */

int load_vol(char *fmt, int start, int end, int step, int *dsizes,
             unsigned char **dataVol) {
	FILE* in;
	int size;
   char fname[256];
   int i;
   int src_dsizes[4];
   unsigned char *timeVol;

   sprintf(fname, fmt, start);

	in = fopen(fname,"rb"); 
	if (in == NULL) {
		printf(" can't open file %s\n", fname); 
		return -1;
	}

   fread(&dsizes[0], sizeof(int), 1, in);
   fread(&dsizes[1], sizeof(int), 1, in);
   fread(&dsizes[2], sizeof(int), 1, in);
   if( vcbBigEndian() ){
	   vcbToggleEndian(&dsizes[0], sizeof(int));
	   vcbToggleEndian(&dsizes[1], sizeof(int));
	   vcbToggleEndian(&dsizes[2], sizeof(int));
   }

   src_dsizes[0] = dsizes[0];
   src_dsizes[1] = dsizes[1];
   src_dsizes[2] = dsizes[2];
   src_dsizes[3] = (end - start) / step + 1;

	size = dsizes[0]*dsizes[1]*dsizes[2];
	(*dataVol) = (unsigned char *) calloc(size, sizeof(unsigned char));
	if ((*dataVol) == NULL) {
		fprintf(stderr,"load_vol: failed to allocation memory\n");
		return -1;
	}

	timeVol = (unsigned char *) malloc(size * src_dsizes[3] *
                                      sizeof(unsigned char));
	if (timeVol == NULL) {
		fprintf(stderr,"load_vol: failed to allocation memory\n");
		return -1;
	}

   i = start;
   do {
      fread(&(timeVol[(i - start) / step * size]), sizeof(unsigned char),
            size, in);
      fclose(in);

      i = i + step;
      if (i <= end) {
         sprintf(fname, fmt, i);
         in = fopen(fname,"rb"); 
         if (in == NULL) {
            printf(" can't open file %s\n", fname); 
            return -1;
         }
         fread(&dsizes[0], sizeof(int), 1, in);
         fread(&dsizes[1], sizeof(int), 1, in);
         fread(&dsizes[2], sizeof(int), 1, in);
         if( vcbBigEndian() ){
			vcbToggleEndian(&dsizes[0], sizeof(int));
			vcbToggleEndian(&dsizes[1], sizeof(int));
			vcbToggleEndian(&dsizes[2], sizeof(int));
		 }
      }
   } while (i <= end);

   vcbVolumeFromFunction(4, src_dsizes, timeVol, 3, dsizes, *dataVol,
                         sizeof(unsigned char), dst_src_indices);

	return 0; 
}

int load_vox(char *rootname, int * dsizes, unsigned char **dataVol)
/* -1 for failure. 0 for success */
{
	char volumename[70], infoname[70], str[300], junk[30];  
	float ax,ay,az;
	FILE *fp;
	int nx, ny, nz, voln;
	
	sprintf(volumename,"%s.vox",rootname);
	sprintf(infoname,"%s.txt",rootname);
	
	if((fp=fopen(infoname,"r"))==NULL) {
		fprintf(stderr,"load_vox, cannot open infofile %s\n",infoname);
		return -1;
	}
  
	fgets(str,100,fp);
	strtok(str," "); strtok(NULL," ");
	nx = atoi(strtok(NULL,"x"));
	ny = atoi(strtok(NULL,"x"));
	nz = atoi(strtok(NULL," "));
  
	printf("loading a volume of size %d x %d x%d\n", nx, ny, nz);
	voln= nx*ny*nz;

	fgets(str,100,fp);
	sscanf(str,"%s%s%f:%f:%f",junk,junk,&ax,&ay,&az);
	printf("aspect ratio = %f:%f:%f\n",ax, ay, az);
	fclose(fp);

	if((fp=fopen(volumename,"rb"))==NULL) {
		fprintf(stderr,"load_vox, cannot open volume file %s\n", volumename);
		return -1;
	}

	(*dataVol) = (unsigned char *) malloc (sizeof(unsigned char) * voln);

	fread((*dataVol),sizeof(unsigned char),voln,fp);
	dsizes[0] = nx;
	dsizes[1] = ny;
	dsizes[2] = nz;

	fclose(fp);

	return 0;
}

void load_data(char *fmt, int start, int end, int step) {
	int i, j, k, sz[3];
	unsigned char * voxels, * fvox;
	float isoval = 30.f;
	clock_t time1, time2;

	if (load_vol(fmt, start, end, step, sz, &voxels) < 0) {
		fprintf(stderr, "load_data: error loading datafile\n");
		exit(-1);
	}

	fvox = (unsigned char *) calloc (sz[0]*sz[1]*sz[2],sizeof(unsigned char));
	int cnt = 0;
	for (i = 0; i < sz[0]; i ++)
		for (j = 0; j <sz[1]; j ++)
			for (k = 0; k <sz[2]; k ++) {
				fvox[cnt] = voxels[(k*sz[1]+j)*sz[0]+i];
				cnt ++;
			}
	free(voxels);
	voxels = fvox;

	printf("in load file\n");

#define _id3(x,y,z, dsize,i,na) ((((x)*dsize[1] + (y)) * dsize[2] + (z))*(na) + (i))

	fvox = (unsigned char *) calloc (sz[0]*sz[1]*sz[2]*4,sizeof(unsigned char));		

	for (i = 0; i < sz[0]; i ++)
		for (j = 0; j <sz[1]; j ++)
			for (k = 0; k <sz[2]; k ++) {
				int lo_i, hi_i, lo_j,hi_j, lo_k, hi_k;
				float diff[3];

				fvox[_id3(i,j,k,sz,0,4)] = voxels[_id3(i,j,k,sz,0,1)];

				lo_i = (i == 0)? i : (i - 1);
				hi_i = (i == (sz[0] -1))? i : (i + 1);
				lo_j = (j == 0)? j : (j - 1);
				hi_j = (j == (sz[1] -1))? j : (j + 1);
				lo_k = (k == 0)? k : (k - 1);
				hi_k = (k == (sz[2] -1))? k : (k + 1);

				diff[0] = ((float)(voxels[_id3(hi_i,j,k,sz,0,1)] - voxels[_id3(lo_i,j,k,sz,0,1)]))/(lo_i-hi_i);
				diff[1] = ((float)(voxels[_id3(i,hi_j,k,sz,0,1)] - voxels[_id3(i,lo_j,k,sz,0,1)]))/(lo_j-hi_j);
				diff[2] = ((float)(voxels[_id3(i,j,hi_k,sz,0,1)] - voxels[_id3(i,j,lo_k,sz,0,1)]))/(lo_k-hi_k);
				la_normalize3(diff);

				fvox[_id3(i,j,k,sz,1,4)] = (unsigned char)((char)(diff[0]*128));
				fvox[_id3(i,j,k,sz,2,4)] = (unsigned char)((char)(diff[1]*128));
				fvox[_id3(i,j,k,sz,3,4)] = (unsigned char)((char)(diff[2]*128));
			}

	free(voxels);
	voxels = fvox;

	time1 = clock();

	if ((nfacets = vcbMarchingCubeBlk(VCB_UNSIGNEDBYTE, voxels, isoval, sz, VCB_WITHNORMAL, &nverts, &vdata, &fdata))<=0) {
		fprintf(stderr,"vcbMarchingCubeBlk: failed to extract any triangles at isovalue %f\n",isoval);
		return;
	}

	time2 = clock();
	printf("marchng cube done in %f seconds\n",(float)(time2-time1)/CLOCKS_PER_SEC);

	free(voxels);

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

   /* glEnable(GL_COLOR_MATERIAL); */
   /* glEnable(GL_BLEND); */
   /* glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); */
   /* glColor4f(0.0, 0.0, 1.0, 0.2); */
   /* vcbGrid2d(1, 5, 251, 253, 21, 10); */
   /* vcbGrid3d(0, 0, 0, 256, 256, 256, 21, 33, 19); */
   /* glDisable(GL_BLEND); */
   /* glDisable(GL_COLOR_MATERIAL); */

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

   if (argc != 5) {
      fprintf(stderr, "Usage: %s file_format_%%02d start_time end_time "
                      "interval\n\nFor example,\n\n   %s jet%%02d 1 99 10\n\n"
                      "runs %s on jet01, jet11, jet21, ..., jet91.\n", argv[0],
                      argv[0], argv[0]);
      exit(1);
   }
	load_data(argv[1], atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));

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
