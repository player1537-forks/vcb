#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "vcbutils.h"
#include "vbutrcast.h"
#include "vcbimage.h"

#include "mmaplib.h"

/*unsigned char * voxels;*/
float  *voxels;
int             sz[3], orig[3];
int             ndims,nattribs;
vcbdatatype     rawdatatype;

char *          binfile;
int             binfile_len;

/* application specific data starts here */


/* end of application specific data */



void load_data(char * dataname)
{
	int i, j, k;
	float minv, maxv;
	
	if ((binfile = mmap_infile(dataname,&binfile_len)) == NULL) {
	  fprintf(stderr,"load_data: faiure while doing mmap\n");
	  exit(-1);
	}

	if ((voxels = vcbLoadBinm(binfile, &rawdatatype, &ndims, orig, sz, &nattribs)) 
	    == NULL) {
		fprintf(stderr, "load_data: error loading datafile\n");
		exit(-1);
	}


	/*printf("loadBinm succeeded\n");*/

	/*
	minv = 1e6;
	maxv = -1e6;
	for (i = 0; i < 128*128*128; i++) {
	  if (minv > voxels[i]) minv = voxels[i];
	  if (maxv < voxels[i]) maxv = voxels[i];
	}
	*/
	/*
	printf("in load_data: minv %f, maxv %f\n",minv, maxv);
	printf("after vcbLoadBinm %d datatype, %d dims, %d sz, %d attribs\n",
	       rawdatatype,ndims,sz[0],nattribs);
	*/

}


int main (int argc, char ** argv)
{
  clock_t time1, time2;
  int   base = 0;
  char * dst;

  float ipos[MAXNUMISORANGE], irange[MAXNUMISORANGE], iopa[MAXNUMISORANGE];
  float minval, maxval;
  unsigned char * fbuffer;
  float eye[3] = {160.f, 32.f, 160.f};
  float coi[3] = {32.f, 32.f, 32.f};
  float up[3]  = {0.f, 1.f, 0.f};
  int   numlights = 1, numiso, colorscheme;
  float lightpos[3] = {32.f, 32.f, 160.f};
  int   w, h, imgbounds[4];
  int   i, cnt;
  float hither, yon, fov, aspect, xmin, xmax, ymin, ymax;
  int   whichview;
  float radius;

  if (argc < 11) {
    fprintf(stderr,"usage: %s dataset color minv maxv ex ey ez [ipos irange ipa]\n",argv[0]);
    exit(1);
  }

  load_data(argv[1]);
  colorscheme = atoi(argv[2]);
  minval      = atof(argv[3]);
  maxval      = atof(argv[4]);
  eye[0]      = atoi(argv[5]);
  eye[1]      = atof(argv[6]);
  eye[2]      = atof(argv[7]);
  if (minval >= maxval) {
    fprintf(stderr,"minval needs to be smaller than maxval. quitting ...\n");
    exit(1);
  }

  for (i = 8, cnt = 0; (i+2) < argc; i += 3) {
    ipos[cnt]   = atof(argv[i  ]);
    irange[cnt] = atof(argv[i+1]);
    iopa[cnt]   = atof(argv[i+2]);
    cnt ++;
    if (cnt >= MAXNUMISORANGE)
      break;
  }
  numiso = cnt;
  if (numiso < 1) {
    fprintf(stderr,"cannot proceed since number of intervals is: %d\n",numiso);
    exit(1);
  }

  printf("%s, maxval %f, %d intervals: ", argv[1],maxval, numiso);
  fflush(stdout);

  vbutDefineTransfunc(colorscheme, numiso, minval, maxval-minval, ipos, irange, iopa);

  coi[0] = sz[0]/2; coi[1] = sz[1]/2; coi[2] = sz[2]/2;
  radius = sz[0];
  if (radius < sz[1]) radius = sz[1];
  if (radius < sz[2]) radius = sz[2];
  lightpos[0] = eye[0];
  lightpos[1] = eye[1];
  lightpos[2] = eye[2];
  time1 = clock();

  w = h = 512;
  hither = 1.f;   yon    = 1000.f;
  xmin   = -radius*0.8f; xmax   = radius*0.8f;
  ymin   = -radius*0.8f; ymax   = radius*0.8f;

  orig[0] = orig[1] = orig[2] = 0;

  fbuffer = vbutrcastOrtho(voxels,rawdatatype, 
			   orig, sz, 
			   eye, coi, up, 
			   numlights, lightpos, 
			   w, h, 
			   imgbounds, 
			   hither, yon, xmin, xmax, ymin, ymax);

  if (fbuffer != NULL)
    vcbImgWriteBMP("rcast.bmp",fbuffer, 4, w, h);

  time2 = clock();
  printf(" done in %f seconds\n",(float)(time2-time1)/CLOCKS_PER_SEC);

  munmap(binfile, binfile_len);

  free(fbuffer);
  return 0;
}
