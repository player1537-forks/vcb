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
#include "vcbbtree.h"

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

unsigned char * voxels;
int sz[3];
float isoval = 30.f;
float * pool;
int  changed;

VCB_mntree btree;

int load_vol (char* fname, int * dsizes, unsigned char ** dataVol)
{
	FILE* in;
	int size;

	in = fopen(fname,"rb"); 
	if (in == NULL) {
		printf(" can't open file %s\n", fname); 
		return -1;
	}

	fscanf(in,"%d",&dsizes[0]);
	fscanf(in,"%d",&dsizes[1]);
	fscanf(in,"%d",&dsizes[2]);  

	size = dsizes[0]*dsizes[1]*dsizes[2];
	(*dataVol) = (unsigned char *) malloc(size*sizeof(unsigned char));
	if ((*dataVol) == NULL) {
		fprintf(stderr,"load_vol: failed to allocation memory\n");
		return -1;
	}

	fread((*dataVol), sizeof(unsigned char), size, in);

	fclose(in);
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

int nattribs = 2;

int poolcnt;

void load_data(void)
{
	int i, j, k, cnt;
	unsigned char * fvox;
	clock_t time1, time2;
	float cellmin, cellmax;

	/* vr_mcube library assumes [(i*ydim+j)*zdim+k] voxel order */
	/* vox data formats are indexed by [(i*ydim+j)*zdim+k] */

	char * dataname = "datasets/vox/dataset2";
	
	if (load_vox(dataname, sz, &voxels) < 0) {
		fprintf(stderr, "load_data: error loading datafile\n");
		exit(-1);
	}

	printf("in load file\n");

#define _id3(x,y,z, dsize,i,na) ((((x)*dsize[1] + (y)) * dsize[2] + (z))*(na) + (i))

	time1 = clock();

	pool = (float *) malloc((sz[0]-1)*(sz[1]-1)*(sz[2]-1)*3*sizeof(float));
	poolcnt = 0;
	for (i = 0; i < sz[0]-1; i ++)
		for (j = 0; j <sz[1]-1; j ++)
			for (k = 0; k <sz[2]-1; k ++) {
				cellmin = 255;
				cellmax = 0;
				cellmin = VCB_MINVAL(cellmin, voxels[_id3(i,j,k,sz,0,1)]);
				cellmax = VCB_MAXVAL(cellmax, voxels[_id3(i,j,k,sz,0,1)]);
				cellmin = VCB_MINVAL(cellmin, voxels[_id3(i,j,k+1,sz,0,1)]);
				cellmax = VCB_MAXVAL(cellmax, voxels[_id3(i,j,k+1,sz,0,1)]);
				cellmin = VCB_MINVAL(cellmin, voxels[_id3(i,j+1,k,sz,0,1)]);
				cellmax = VCB_MAXVAL(cellmax, voxels[_id3(i,j+1,k,sz,0,1)]);
				cellmin = VCB_MINVAL(cellmin, voxels[_id3(i,j+1,k+1,sz,0,1)]);
				cellmax = VCB_MAXVAL(cellmax, voxels[_id3(i,j+1,k+1,sz,0,1)]);
				cellmin = VCB_MINVAL(cellmin, voxels[_id3(i+1,j,k,sz,0,1)]);
				cellmax = VCB_MAXVAL(cellmax, voxels[_id3(i+1,j,k,sz,0,1)]);
				cellmin = VCB_MINVAL(cellmin, voxels[_id3(i+1,j,k+1,sz,0,1)]);
				cellmax = VCB_MAXVAL(cellmax, voxels[_id3(i+1,j,k+1,sz,0,1)]);
				cellmin = VCB_MINVAL(cellmin, voxels[_id3(i+1,j+1,k,sz,0,1)]);
				cellmax = VCB_MAXVAL(cellmax, voxels[_id3(i+1,j+1,k,sz,0,1)]);
				cellmin = VCB_MINVAL(cellmin, voxels[_id3(i+1,j+1,k+1,sz,0,1)]);
				cellmax = VCB_MAXVAL(cellmax, voxels[_id3(i+1,j+1,k+1,sz,0,1)]);
				if (cellmax > 0) {
					pool[poolcnt * 3    ] = cellmax;
					pool[poolcnt * 3 + 1] = cellmin;
					cnt = (i * 64 + j)*64 + k;
					memcpy(pool+poolcnt*3+2, &cnt, sizeof(int));
					poolcnt ++;
				}
			}


	btree = (VCB_mntree)vcbBuildBtree(poolcnt, nattribs, pool, 32, 3);

	time2 = clock();
	printf("mntree constructed in %f seconds\n",(float)(time2-time1)/CLOCKS_PER_SEC);

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

	abbox.low[0] = abbox.low[1] = abbox.low[2] = 1e6f;
	abbox.high[0] = abbox.high[1] = abbox.high[2] = -1e6f;

		abbox.low[0] = VCB_MINVAL(abbox.low[0],0);
        abbox.high[0] = VCB_MAXVAL(abbox.high[0],sz[0]);
        abbox.low[1] = VCB_MINVAL(abbox.low[1],0);
        abbox.high[1] = VCB_MAXVAL(abbox.high[1],sz[1]);
        abbox.low[2] = VCB_MINVAL(abbox.low[2],0);
        abbox.high[2] = VCB_MAXVAL(abbox.high[2],sz[2]);    

	printf("getmesh obtained bounding box: (%f %f %f) (%f %f %f)\n",abbox.low[0], abbox.low[1],
		abbox.low[2], abbox.high[0],abbox.high[1],abbox.high[2]);

}

void getmesh(void)
{
	float lb[5], ub[5];
	int ncells, i, yzslice;
	int * q_ids;
	short * cellids;
	int cellid;
	clock_t time1, time2;

	time1 = clock();

	lb[0] = isoval-0.5f; ub[0] = 255;
	lb[1] = 0;      ub[1] = isoval+0.5f;
	lb[2] = lb[3] = lb[4] = 0;
	ub[2] = ub[3] = ub[4] = 63;

	ncells = vcbQueryBtree(btree, lb, ub, &q_ids);

	printf("ncells = %d\n",ncells);

	cellids = (short *) malloc(ncells * 3 * sizeof(short));

	yzslice = sz[1] * sz[2];
	for (i = 0; i < ncells; i ++) {
		cellid = q_ids[i];
		cellids[i*3  ] = (short)(cellid/yzslice); cellid = cellid% yzslice;
		cellids[i*3+1] = (short)(cellid/64);
		cellids[i*3+2] = (short)(cellid%64);
	}

	time2 = clock();
	printf("amcube search done in %f seconds\n",(float)(time2-time1)/CLOCKS_PER_SEC);

	if (vdata  != NULL) free(vdata);
	if (fdata  != NULL) free(fdata);

	if ((nfacets = vcbMarchingCubeList(VCB_UNSIGNEDBYTE, voxels, isoval, sz, ncells, cellids, VCB_WITHNORMAL, &nverts, &vdata, &fdata))<=0) {
		fprintf(stderr,"vcbMarchingCubeList: failed to extract any triangles at isovalue %f\n",isoval);
		return;
	}

	free(cellids);
	free(q_ids);
	printf("amcblock done with %d triangles, %d verts\n", nfacets, nverts);

}

void display (void)
{
	int i;
	vcbEnableOGLlight(0.1f);
	vcbSetOGLlight(GL_LIGHT0, 0.1, 0.8, light0.lightpos[0], light0.lightpos[1], light0.lightpos[2], 40.f);
	vcbSetOGLMaterial(GL_FRONT_AND_BACK, MATERIAL_COPPER, 1.0);

	spaceball.Update();

	if (changed > 0) {
		changed = 0;
		getmesh();
	}

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


	glutSwapBuffers();
	glPopMatrix();
}

void amcubekeys(unsigned char c, int x, int y)
{
	float fzin = 0.95f;
	float fzout = 1.05f;
	int rzoom = 2;

	changed = 1;

	switch(c) {
      case 'a':
		  proj.xmin *= fzin;
		  proj.xmax *= fzin;
		  proj.ymin *= fzin;
		  proj.ymax *= fzin;
		  proj.fov  *= fzin;
		  reshape(iwinWidth,iwinHeight);
		  changed = 0;
		  break;
	  case 'z':
		  proj.xmin *= fzout;
		  proj.xmax *= fzout;
		  proj.ymin *= fzout;
		  proj.ymax *= fzout;
		  proj.fov  *= fzout;
		  reshape(iwinWidth,iwinHeight);
		  changed = 0;
		  break;


	  /* controls for attribute */
	  case 'u': if (isoval < 255)    isoval += 1.f;  break;
	  case 'd': if (isoval > 1)      isoval -= 1.f;  break;

	  case 'q':
		  exit(0);
		  break;
	  default:
		  break;
  }
}


void cleanup (void)
{
	vcbFreeBtree(btree);
	free(pool);
	free(voxels);
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
	changed = 1;

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
	
	glutKeyboardFunc(amcubekeys);

	oglverno = vcbGetOGLVersion();
	printf("using ogl version %f\n",oglverno);

	glutMainLoop();

	return 0;
}
