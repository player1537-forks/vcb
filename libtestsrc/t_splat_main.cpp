#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <time.h>

#include "glew.h"
#include <GL/glut.h>

#include "vcbutils.h"
#include "ArcBall.h"
#include "vcbglutils.h"
#include "vcbmath.h"
#include "atypes.h"

#include "vcbvolren.h"
#include "vcblinalg.h"

char projmode = 'O'; /* P for perspective, O for orthogonal */
projStruct proj;
viewStruct view;
lightStruct light0;

ArcBall spaceball;
int iwinWidth;
int iwinHeight;

GLuint  texobj;
int     mainwin, sidewin;

clock_t time1, time2, time3, time4;

bbox abbox;

unsigned char * voxels;
int             sz[3], nvox;
float           tlut[256*4];

#define FTB 1

int load_vox(char *rootname, int * dsizes, unsigned char **dataVol)
/* -1 for failure. 0 for success */
{
	char volumename[30], infoname[30], str[300], junk[30];  
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

int load_classify(char * filename, float * transflut)
/*
 * -1 for failure, 0 for success 
 * transflut is assumed to have valid memory allocation
 */
{
	int i, j;
	FILE * fp;
	float * tmparray, junk;
 
	if ((fp=fopen(filename,"r"))==NULL) {
		fprintf(stderr,"load_classify, cannot open classify file %s\n", filename);
		return -1;
	}

	for (i = 0; i < 256; i ++) {
		tmparray = & transflut[i * 4];
		for (j = 0; j < 4; j ++)
			fscanf(fp,"%f",&tmparray[j]);
		fscanf(fp,"%f",&junk);
		fscanf(fp,"%f",&junk);
		fscanf(fp,"%f",&junk);
    }

	fclose(fp);
	return 0;
}

void voxtransf (float * rgba, void * vox)
{
	unsigned char * voxel = (unsigned char *) vox;
	int id = voxel[0] * 4;
	rgba[0] = tlut[id+0];
	rgba[1] = tlut[id+1];
	rgba[2] = tlut[id+2];
	rgba[3] = tlut[id+3];
}

void load_data(void)
{
	unsigned char * fvox;
	unsigned short * cvox;
	int i, j, k, numvox;

	char * dataname = "datasets/vox/dataset2";
	char * tlutname = "datasets/vox/dataset2_trans.txt";
	
	if (load_vox(dataname, sz, &voxels) < 0) {
		fprintf(stderr, "load_data: error loading datafile\n");
		exit(-1);
	}

	numvox = sz[0]*sz[1]*sz[2];
	fvox = (unsigned char *) calloc (numvox*4,sizeof(unsigned char));		

	for (i = 0; i < sz[0]; i ++)
		for (j = 0; j <sz[1]; j ++)
			for (k = 0; k <sz[2]; k ++) {
				int lo_i, hi_i, lo_j,hi_j, lo_k, hi_k;
				float diff[3];

				fvox[vcbid3(i,j,k,sz,0,4)] = voxels[vcbid3(i,j,k,sz,0,1)];

				lo_i = (i == 0)? i : (i - 1);
				hi_i = (i == (sz[0] -1))? i : (i + 1);
				lo_j = (j == 0)? j : (j - 1);
				hi_j = (j == (sz[1] -1))? j : (j + 1);
				lo_k = (k == 0)? k : (k - 1);
				hi_k = (k == (sz[2] -1))? k : (k + 1);

				diff[0] = ((float)(voxels[vcbid3(hi_i,j,k,sz,0,1)] - voxels[vcbid3(lo_i,j,k,sz,0,1)]))/(lo_i-hi_i);
				diff[1] = ((float)(voxels[vcbid3(i,hi_j,k,sz,0,1)] - voxels[vcbid3(i,lo_j,k,sz,0,1)]))/(lo_j-hi_j);
				diff[2] = ((float)(voxels[vcbid3(i,j,hi_k,sz,0,1)] - voxels[vcbid3(i,j,lo_k,sz,0,1)]))/(lo_k-hi_k);
				la_normalize3(diff);

				fvox[vcbid3(i,j,k,sz,1,4)] = (unsigned char)((char)(diff[0]*127));
				fvox[vcbid3(i,j,k,sz,2,4)] = (unsigned char)((char)(diff[1]*127));
				fvox[vcbid3(i,j,k,sz,3,4)] = (unsigned char)((char)(diff[2]*127));
			}

	free(voxels);
	voxels = fvox;

	if (load_classify(tlutname, tlut) < 0) {
		fprintf(stderr, "load_classify: error loading transfer lut\n");
		exit(-1);
	}

	for (i = 0, nvox = 0; i < numvox; i ++)
		if (tlut[voxels[i*4]*4+3] > 0.0039) /* (1/256) */
			nvox ++;

	printf("among %d raw voxels, %d have none-zero opacity\n", numvox, nvox);

	fvox = (unsigned char *) malloc(nvox*10*sizeof(unsigned char));
	for (i = 0, nvox = 0; i < numvox; i ++)
		if (tlut[voxels[i*4]*4+3] > 0.005) {
			cvox = (unsigned short*)&fvox[nvox*10+4];
			fvox[nvox*10+0] = voxels[i*4+0];	
			fvox[nvox*10+1] = voxels[i*4+1];
			fvox[nvox*10+2] = voxels[i*4+2];
			fvox[nvox*10+3] = voxels[i*4+3];
			cvox[0] = (unsigned short)(i/(sz[1]*sz[2]));	
			cvox[1] = (unsigned short)((i/sz[2])%sz[1]);
			cvox[2] = (unsigned short)(i%sz[2]);

			nvox ++;
		}
	free(voxels);
	voxels = fvox;

	abbox.low[0] = abbox.low[1] = abbox.low[2] = 0.f;
	abbox.high[0] = sz[0];
	abbox.high[1] = sz[1];
	abbox.high[2] = sz[2];

	printf("bounding box: (%f %f %f) (%f %f %f)\n",abbox.low[0], abbox.low[1],
		abbox.low[2], abbox.high[0],abbox.high[1],abbox.high[2]);

}

void display (void)
{
	char echofps[80];
	float * m_spaceball;

	time1 = clock();

	spaceball.Update();

	m_spaceball = spaceball.GetMatrix();

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslated(view.center[0],view.center[1],view.center[2]);
	glMultMatrixf(spaceball.GetMatrix());
	glTranslated(-view.center[0],-view.center[1],-view.center[2]);

	glClear(GL_COLOR_BUFFER_BIT);

	vcbVolrSsplats(voxels, nvox, sz);//, m_spaceball);
	glPopMatrix();

	time2 = clock();
	sprintf(echofps,"fps: %6.2f",(float)CLOCKS_PER_SEC/(time2-time1));

	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_1D);
	glActiveTexture(GL_TEXTURE1);
	glDisable(GL_TEXTURE_2D);
	vcbOGLprint(0.01f,0.01f, 1.f, 1.f, 0.f, echofps);
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_1D);

	glutSwapBuffers();
}



void cleanup (void)
{
	vcbVolrCloseS();
	printf("finished cleaning up ...\n");
	fflush(stdout);
}

int main(int argc, char ** argv)
{
	float oglverno;
	int   ntexunits;

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
	vcbEnableOGLlight(0.1f); 
	vcbSetOGLlight(GL_LIGHT0, 0.1f, 1.0f, light0.lightpos[0], light0.lightpos[1], light0.lightpos[2], 40.f);

	atexit(cleanup);
	oglverno = vcbGetOGLVersion();
	printf("using ogl version %f",oglverno);
	glGetIntegerv(GL_MAX_TEXTURE_UNITS, & ntexunits);
	printf(" with %d texture units\n", ntexunits);

	vcbVolrInitS(voxtransf);


	glutMainLoop();

	return 0;
}
