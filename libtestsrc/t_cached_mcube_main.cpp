#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

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

#define SHIFT_DELTA 10

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
typedef struct {
   int     nverts, nfacets;
   float * vdata;
   int   * fdata;
   float * fnormals;
} mesh_t;

typedef struct {
   int low[3];
   int high[3];
} bounds_t;

mesh_t *mesh;

int sz[3];
unsigned char *voxels;
bounds_t bounds;

vcbCache cache;

/* end of application specific data */

bbox abbox;

/* ------------------------------------------------------------------------- */

void print_key(vcbCacheKeyData *key_data) {

   printf("%s", (char *) key_data->key);

}

/* ------------------------------------------------------------------------- */

int load_vol (char* fname, int * dsizes, unsigned char ** dataVol) {
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

/* ------------------------------------------------------------------------- */

int x_then_y_then_z(int ndims, int *dsizes, int *coords) {

   int i;
   int id = coords[0];
   int num_in_prev_dims = 1;

   for (i = 1; i < ndims; i++) {
      num_in_prev_dims *= dsizes[i - 1];
      id += coords[i] * num_in_prev_dims;
   }

   return id;

}

/* ------------------------------------------------------------------------- */

void *load_block(vcbCacheKeyData *key_data) {

   int sz2[3];
	unsigned char *block, *fvox;
	int i, j, k;
	float isoval = 30.f;
	clock_t time1, time2;
   bounds_t bounds;
   mesh_t *mesh;

   sscanf((char *) key_data->key, "%d %d %d %d %d %d",
          &(bounds.low[0]), &(bounds.low[1]), &(bounds.low[2]),
          &(bounds.high[0]), &(bounds.high[1]), &(bounds.high[2]));

   mesh = (mesh_t *) malloc(sizeof(mesh_t));

   sz2[0] = bounds.high[0] - bounds.low[0] + 1,
   sz2[1] = bounds.high[1] - bounds.low[1] + 1,
   sz2[2] = bounds.high[2] - bounds.low[2] + 1;

   block = (unsigned char *) malloc(sz2[0] * sz2[1] * sz2[2] * 
                                    sizeof(unsigned char));
   vcbGrablk((void *) voxels, (void *) block, 1, sizeof(unsigned char), 3, sz,
             bounds.low, bounds.high, x_then_y_then_z);

	fvox = (unsigned char *) calloc (sz2[0]*sz2[1]*sz2[2],sizeof(unsigned char));
	int cnt = 0;
	for (i = 0; i < sz2[0]; i ++)
		for (j = 0; j <sz2[1]; j ++)
			for (k = 0; k <sz2[2]; k ++) {
				fvox[cnt] = block[(k*sz2[1]+j)*sz2[0]+i];
				cnt ++;
			}
	free(block);
	block = fvox;

	/* printf("in load file\n"); */

#define _id3(x,y,z, dsize,i,na) ((((x)*dsize[1] + (y)) * dsize[2] + (z))*(na) + (i))

	fvox = (unsigned char *) calloc (sz2[0]*sz2[1]*sz2[2]*4,sizeof(unsigned char));		

	for (i = 0; i < sz2[0]; i ++)
		for (j = 0; j <sz2[1]; j ++)
			for (k = 0; k <sz2[2]; k ++) {
				int lo_i, hi_i, lo_j,hi_j, lo_k, hi_k;
				float diff[3];

				fvox[_id3(i,j,k,sz2,0,4)] = block[_id3(i,j,k,sz2,0,1)];

				lo_i = (i == 0)? i : (i - 1);
				hi_i = (i == (sz2[0] -1))? i : (i + 1);
				lo_j = (j == 0)? j : (j - 1);
				hi_j = (j == (sz2[1] -1))? j : (j + 1);
				lo_k = (k == 0)? k : (k - 1);
				hi_k = (k == (sz2[2] -1))? k : (k + 1);

				diff[0] = ((float)(block[_id3(hi_i,j,k,sz2,0,1)] - block[_id3(lo_i,j,k,sz2,0,1)]))/(lo_i-hi_i);
				diff[1] = ((float)(block[_id3(i,hi_j,k,sz2,0,1)] - block[_id3(i,lo_j,k,sz2,0,1)]))/(lo_j-hi_j);
				diff[2] = ((float)(block[_id3(i,j,hi_k,sz2,0,1)] - block[_id3(i,j,lo_k,sz2,0,1)]))/(lo_k-hi_k);
				la_normalize3(diff);

				fvox[_id3(i,j,k,sz2,1,4)] = (unsigned char)((char)(diff[0]*128));
				fvox[_id3(i,j,k,sz2,2,4)] = (unsigned char)((char)(diff[1]*128));
				fvox[_id3(i,j,k,sz2,3,4)] = (unsigned char)((char)(diff[2]*128));
			}

	free(block);
	block = fvox;

	time1 = clock();

	if ((mesh->nfacets = vcbMarchingCubeBlk(VCB_UNSIGNEDBYTE, block, isoval, sz2, VCB_WITHNORMAL, &(mesh->nverts), &(mesh->vdata), &(mesh->fdata)))<=0) {
		fprintf(stderr,"vcbMarchingCubeBlk: failed to extract any triangles at isovalue %f\n",isoval);
		return NULL;
	}

	time2 = clock();
	/* printf("marchng cube done in %f seconds\n",(float)(time2-time1)/CLOCKS_PER_SEC); */

	free(block);

	/* abbox.low[0] = abbox.low[1] = abbox.low[2] = 1e6; */
	/* abbox.high[0] = abbox.high[1] = abbox.high[2] = -1e6; */

	/* for (i = 0; i < nverts * 3; i ++) { */
		/* abbox.low[i%3] = VCB_MINVAL(abbox.low[i%3],vdata[i]); */
		/* abbox.high[i%3] = VCB_MAXVAL(abbox.high[i%3],vdata[i]); */
	/* } */

	/* printf("load_data obtained bounding box: (%f %f %f) (%f %f %f)\n",abbox.low[0], abbox.low[1], */
		/* abbox.low[2], abbox.high[0],abbox.high[1],abbox.high[2]); */

   return (void *) mesh;
}

/* ------------------------------------------------------------------------- */

void get_blocks() {

   vcbCacheKeyData **keys;
   int i;

   /* No prefetching is done.  Uncomment next line to prefetch. */
   /* int num_of_keys = 1; */

   /* Prefetch the chunks to the right.  A good scheme would also prefetch
    * above, to the left, below, closer, and farther in. */
   int num_of_keys = 5;

   keys = (vcbCacheKeyData **) malloc(sizeof(vcbCacheKeyData *) * num_of_keys);
   keys[0] = (vcbCacheKeyData *) malloc(sizeof(vcbCacheKeyData));
   keys[0]->len = asprintf((char **) &(keys[0]->key), "%d %d %d %d %d %d",
                           bounds.low[0], bounds.low[1], bounds.low[2],
                           bounds.high[0], bounds.high[1], bounds.high[2]) + 1;

   /* printf ("bounds: (%d %d %d) - (%d %d %d)\n", */
           /* bounds.low[0], bounds.low[1], bounds.low[2], */
           /* bounds.high[0], bounds.high[1], bounds.high[2]); */

   for (i = 1; i < num_of_keys; i++) {
      keys[i] = (vcbCacheKeyData *) malloc(sizeof(vcbCacheKeyData));
      keys[i]->len =
         asprintf((char **) &(keys[i]->key), "%d %d %d %d %d %d",
                  bounds.low[0] + i * SHIFT_DELTA, bounds.low[1], bounds.low[2],
                  bounds.high[0] + i * SHIFT_DELTA, bounds.high[1],
                  bounds.high[2]) + 1;
   }

   mesh = (mesh_t *) vcbCacheFetch(cache, keys, num_of_keys);
   /* vcbCacheStats(cache); */
   free(keys);

}

/* ------------------------------------------------------------------------- */

void load_data(void) {

	char * dataname = "datasets/vol/uncbrain.vol";
	
	if (load_vol(dataname, sz, &voxels) < 0) {
		fprintf(stderr, "load_data: error loading datafile\n");
		exit(-1);
	}

   abbox.low[0] = abbox.low[1] = abbox.low[2] = 0.0f;
   abbox.high[0] = sz[0];
   abbox.high[1] = sz[1];
   abbox.high[2] = sz[2];

   bounds.low[0] = bounds.low[1] = bounds.low[2] = 50;
   bounds.high[0] = bounds.high[1] = bounds.high[2] = 144;

   get_blocks();

}

/* ------------------------------------------------------------------------- */

void cleanup (void) {
   vcbCacheDestroy(cache);

	printf("finished cleaning up ...\n");
	fflush(stdout);
}

/* ------------------------------------------------------------------------- */

void cache_keys(unsigned char c, int x, int y) {
	float fzin = 0.95;
	float fzout = 1.05;
	unsigned char * pixels;

	switch(c) {
      case 'x': case 'y': case 'z':
         bounds.low[c - 'x'] += SHIFT_DELTA;
         bounds.high[c - 'x'] += SHIFT_DELTA;
         get_blocks();
         reshape(iwinWidth,iwinHeight);
         break;
      case 'X': case 'Y': case 'Z':
         bounds.low[c - 'X'] -= SHIFT_DELTA;
         bounds.high[c - 'X'] -= SHIFT_DELTA;
         get_blocks();
		   reshape(iwinWidth,iwinHeight);
         break;
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
     case 'c':
        vcbCacheStats(cache);
        vcbCacheReset(cache);
        vcbCacheStats(cache);
        mesh = NULL;
        break;
     case 's':
        vcbCacheStats(cache);
        break;
	  case 'q':
		  exit(0);
		  break;
	  default:
		  break;
  }
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

	glBegin(GL_TRIANGLES);

   if (mesh != NULL) {
      for (i = 0; i < mesh->nfacets; i ++) {
         glNormal3fv(&mesh->vdata[mesh->fdata[i*3+0]*6+3]);
         glVertex3fv(&mesh->vdata[mesh->fdata[i*3+0]*6+0]);
         glNormal3fv(&mesh->vdata[mesh->fdata[i*3+1]*6+3]);
         glVertex3fv(&mesh->vdata[mesh->fdata[i*3+1]*6+0]);
         glNormal3fv(&mesh->vdata[mesh->fdata[i*3+2]*6+3]);
         glVertex3fv(&mesh->vdata[mesh->fdata[i*3+2]*6+0]);
      }
   }

	glEnd();


	glutSwapBuffers();
	glPopMatrix();
}

/* ------------------------------------------------------------------------- */

void free_data(void *data) {

   mesh_t *tmp;

   tmp = (mesh_t *) data;

   free(tmp->vdata);
   free(tmp->fdata);
   free(tmp);

}

/* ------------------------------------------------------------------------- */

void free_key(vcbCacheKeyData *key_data) {

   free(key_data->key);
   free(key_data);

}

/* ------------------------------------------------------------------------- */

int main(int argc, char ** argv) {
	float oglverno;
   int cache_capacity;
   int num_to_purge;

   printf("This program takes an optional argument specifying the cache's\n"
          "maximum capacity.  To see how the program behaves without\n"
          "caching, run: \n\n   %s 1\n\n"
          "If no argument is specified, the capacity defaults to 20.\n\n",
          argv[0]);

   if (argc >= 2) {
      cache_capacity = atoi(argv[1]);
      num_to_purge = 1;
   } else {
      cache_capacity = 20;
      num_to_purge = 5;
   }

   cache = vcbCacheMake(load_block, free_data, free_key, print_key,
                        cache_capacity, num_to_purge, 2);

	load_data();

	/* printf("after reading file got %d %d\n",mesh->nverts,mesh->nfacets); */
	/* printf("bounding box: (%f %f %f) (%f %f %f)\n",abbox.low[0], abbox.low[1], */
		/* abbox.low[2], abbox.high[0],abbox.high[1],abbox.high[2]); */

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
	/* printf("using ogl version %f\n",oglverno); */

   glutKeyboardFunc(cache_keys);
	glutMainLoop();

	return 0;
}

/* ------------------------------------------------------------------------- */

