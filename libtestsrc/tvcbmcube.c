#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "vcbutils.h"
#include "vcbmcube.h"

unsigned char * voxels;
int             sz[3], orig[3];
int             ndims,nattribs;
vcbdatatype     rawdatatype;

char *          binfile;
int             binfile_len;

/* application specific data starts here */
int     nverts, nfacets;
float * vdata;
int   * fdata;
float * fnormals;
/* end of application specific data */



void load_data(char * dataname)
{
	unsigned char * fvox;
	int i, j, k;

	if ((voxels = vcbReadBinm(dataname, &rawdatatype, &ndims, orig, sz, &nattribs)) 
	    == NULL) {
		fprintf(stderr, "load_data: error loading datafile\n");
		exit(-1);
	}

	printf("after vcbReadBinm %d datatype, %d dims, %d sz, %d attribs\n",
	       rawdatatype,ndims,sz[0],nattribs);

}


int main (int argc, char ** argv) 
{
  clock_t time1, time2;
  float isoval = 30.f;
  int   base = 0;
  int   i;
  char * dst;

  if (argc != 3) {
	 fprintf(stderr,"usage: %s datafile isoval\n",argv[0]);
	 exit(1);
  }

  load_data(argv[1]);
  isoval = atof(argv[2]);

  time1 = clock();

  if ((nfacets = vcbMarchingCubeBlk(rawdatatype, voxels, isoval, sz, 
				    VCB_WITHNORMAL, &nverts, &vdata, &fdata))<=0) {
    fprintf(stderr,
	    "vcbMarchingCubeBlk: failed to extract any triangles at isovalue %f\n",
	    isoval);
    return -1;
  }

  time2 = clock();
  printf("marchng cube done in %f seconds\n",(float)(time2-time1)/CLOCKS_PER_SEC);

  /* saving vertex array and triangle array into endian safe formats */
  for (i = 0; i < nverts; i ++) {
      vdata[i*6+0] += orig[0];
      vdata[i*6+1] += orig[1];
      vdata[i*6+2] += orig[2];
  }
  vcbGenBinm("vertexarray.bin", VCB_FLOAT, 1, &base, &nverts, 6, vdata);
  vcbGenBinm("triangarray.bin", VCB_INT, 1, &base, &nfacets, 3, fdata);

  return 0;
}
