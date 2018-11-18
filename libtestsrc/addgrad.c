#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "vcbutils.h"
#include "vcbvolren.h"

#define MAXDIMS 4

int i, j, k;

void * computegrad_ub(void * data, int * sz)
{
  unsigned char * fvox, * voxels;
  voxels = (unsigned char *) data;

  fvox = (unsigned char *) calloc (sz[0]*sz[1]*sz[2]*4,sizeof(unsigned char)); 

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
	
	fvox[vcbid3(i,j,k,sz,1,4)] = (unsigned char)((char)(diff[0]*128));
	fvox[vcbid3(i,j,k,sz,2,4)] = (unsigned char)((char)(diff[1]*128));
	fvox[vcbid3(i,j,k,sz,3,4)] = (unsigned char)((char)(diff[2]*128));
      }

  return fvox;
}

void * computegrad_f(void * data, int * sz)
{
  float * fvox, * voxels;
  voxels = (float *) data;

  fvox = (float *) calloc (sz[0]*sz[1]*sz[2]*4,sizeof(float)); 

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
	
	fvox[vcbid3(i,j,k,sz,1,4)] = diff[0];
	fvox[vcbid3(i,j,k,sz,2,4)] = diff[1];
	fvox[vcbid3(i,j,k,sz,3,4)] = diff[2];
      }

  return fvox;
}

void * computegrad_us(void * data, int * sz)
{
  unsigned short * fvox, * voxels;
  voxels = (unsigned short *) data;

  fvox = (unsigned short *) calloc (sz[0]*sz[1]*sz[2]*4,sizeof(unsigned short)); 

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
	
	fvox[vcbid3(i,j,k,sz,1,4)] = (unsigned short)((short)(diff[0]*32767));
	fvox[vcbid3(i,j,k,sz,2,4)] = (unsigned short)((short)(diff[1]*32767));
	fvox[vcbid3(i,j,k,sz,3,4)] = (unsigned short)((short)(diff[2]*32767));
      }

  return fvox;
}

typedef void * (*computegrad_funcptr) (void * data, int * sz);

computegrad_funcptr func_tab[9] = {computegrad_ub, /* unsigned byte */
				   NULL,           /* byte */
				   computegrad_us, /* unsigned short */
				   NULL,           /* short */
				   NULL,           /* unsigned int */
				   NULL,           /* int */
				   computegrad_f,  /* float */
				   NULL,           /* double */
				   NULL};          /* bitfield */

int main(int argc, char ** argv)
{
  vcbdatatype datatype;
  int ndims, orig[MAXDIMS], dsize[MAXDIMS], nattribs, i;
  void * rawdata;
  void * data_withgrad;

  computegrad_funcptr computegrad_func;

  /* See if user passed an argument. */
   if (argc != 3) {
      fprintf(stderr, "Usage: %s in out\n", argv[0]);
      exit(1);
   }

   rawdata = vcbReadBinm(argv[1], &datatype, &ndims, orig, dsize, &nattribs);
   if (nattribs != 1) {
     fprintf(stderr,"addgrad expects data file to be uni-variate. quitting ...\n");
     free(rawdata);
     return -1;
   }

   computegrad_func = func_tab[datatype];
   if (computegrad_func == NULL) {
     fprintf(stderr,"addgrad only handles uchar, ushort and floats. quitting ...\n");
     free(rawdata);
     return -1;
   }

   data_withgrad = computegrad_func(rawdata,dsize);
   free(rawdata);

   vcbGenBinm(argv[2], datatype, ndims, orig, dsize, 4, data_withgrad);
   free(data_withgrad);

   return 0;
}
