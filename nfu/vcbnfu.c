#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>

#include "vcbnfu.h"
#include "vcbutils.h"
#include "scene_ascii2val.h"

#include "vcbmcube.h"
#include "vbutrcast.h"

#define MAXDIMS 4

int NFUvcbMcube(int npara, NFU_PARA *paras)
{

  /* declared for NFU */
  vcbdatatype datatype;
  void * raw, * block;
  char * input;
  float * isoval;
  int ndims, orig[MAXDIMS],dsizes[MAXDIMS], nattribs, nverts, nfacets, base;
  float  forig[MAXDIMS];
  float * varray_v;
  int * varray_i;
  int len_varray_v, len_varray_i;
  int i;
  int network_tmp;

  /*
   * paras[0] - input:  mmap'ed file
   *      [1] - input:  scenesetting
   *      [2] - output: nverts
   *      [3] - output: nfacets
   *      [4] - output: vertex array
   *      [5] - output: triangle array
   */

  /* set args for NFU */
  raw = (void *)(paras[0].data);
  block = vcbLoadBinm(raw, &datatype,&ndims,orig,dsizes,&nattribs);

  if (ndims != 3)     return -1;  /* wrong number of dimensions */
  if (nattribs != 4)  return -2;  /* wrong number of attributes */

  /* start to parse input arguments for vcb call */
  input = (char *)(paras[1].data);
  scene_ascii2val_init(input, paras[1].len);
  isoval = (float *)scene_get_mcube();

  for (i = 0; i < ndims; forig[i] = orig[i], i++);

  if (isoval == NULL) return -3;  /* no mcube settings */

  nfacets = vcbMarchingCubeBlk(datatype, block, *isoval, dsizes, 1, &nverts, &varray_v, &varray_i);

  for (i = 0; i < nverts; i++) {
    varray_v[i*6+0] += forig[0];
    varray_v[i*6+1] += forig[1];
    varray_v[i*6+2] += forig[2];
  }

  network_tmp = htonl(nverts);
  memcpy(paras[2].data, &network_tmp, sizeof(int));   paras[2].len = sizeof(int);
  
  network_tmp = htonl(nfacets);
  memcpy(paras[3].data, &network_tmp, sizeof(int));  paras[3].len = sizeof(int);

  base = 0;
  len_varray_v = nverts * 6 * sizeof(float);
  len_varray_i = nfacets * 3 * sizeof(int);

  vcbMakeBinm(paras[4].data, VCB_FLOAT, 1, &base, &nverts, 6, varray_v);
  paras[4].len = len_varray_v + 24;

  vcbMakeBinm(paras[5].data, VCB_INT, 1, &base, &nfacets, 3, varray_i);
  paras[5].len = len_varray_i + 24;

  return 0;  /* successful */

}


int NFUvcbRcast(int npara, NFU_PARA * paras)
{

  char * input;
  float * fp;
  int   * ip;

  float ipos[MAXNUMISORANGE], irange[MAXNUMISORANGE], iopa[MAXNUMISORANGE];
  void  * raw, * block;
  vcbdatatype datatype;
  int   ndims, orig[4], sz[4], nattribs;
  float minval, maxval;
  unsigned char * fbuffer, * frame;
  float eye[3], coi[3], up[3];
  int   numlights = 1, numiso, colorscheme;
  float lightpos[3];
  int   w, h, imgbounds[4], i, j;
  float hither, yon, fov, aspect, xmin, xmax, ymin, ymax;

  int   imgsz[2], regionsz[2];

  /*
   * paras[0] - input:  mmap'ed file
   *      [1] - input:  scenesetting
   *      [2] - output: fbuffer
   *      [3] - output: imgbounds
   */

  /* set args for NFU */
  raw = (void *)(paras[0].data);
  block = vcbLoadBinm(raw, &datatype,&ndims,orig,sz,&nattribs);

  if (ndims != 3)     return -1;  /* wrong number of dimensions */
  if (nattribs != 4)  return -2;  /* wrong number of attributes */

  /* start to parse input arguments for vcb call */
  input = (char *)(paras[1].data);
  scene_ascii2val_init(input, paras[1].len);

  fp = (float *) scene_get_raycasting();
  ip = (int *) (fp + 3);
  colorscheme = *ip++;
  numiso = *ip++;
  fp = (float *) ip;
  for (i = 0; i < numiso; ipos[i]   = *fp++, i ++);
  for (i = 0; i < numiso; irange[i] = *fp++, i ++);
  for (i = 0; i < numiso; iopa[i]   = *fp++, i ++);
  if (numiso < 1) {
    fprintf(stderr,"cannot proceed since number of intervals is: %d\n",numiso);
    exit(1);
  }

  fp = scene_get_viewing_trans();
  for (i = 0; i < 3; eye[i] = *fp++, i ++);
  for (i = 0; i < 3; coi[i] = *fp++, i ++);  
  for (i = 0; i < 3; up[i]  = *fp++, i ++);

  minval      = 0.f;
  maxval      = 10.f;
  if (minval >= maxval) {
    fprintf(stderr,"minval needs to be smaller than maxval. quitting ...\n");
    exit(1);
  }

  numlights = scene_get_num_lights();	
  fp = scene_get_lights();
  for (i=0; i<numlights; i++)
    for (j = 0; j < 3; lightpos[i*3+j] = *fp++, j++);

  vbutDefineTransfunc(colorscheme, numiso, minval, maxval-minval, ipos, irange, iopa);

  ip = (int *)scene_get_viewport_trans();
  w  = *ip++;
  h  = *ip++;

  ip = (int *)scene_get_proj_trans();
  printf("projMode=%d\n", *ip++);
  fp = (float *)(ip+1);
  hither = *fp++;
  yon = *fp++;

  fp = scene_get_orthographic_proj();
  xmin = *fp++;
  xmax = *fp++;
  ymin = *fp++;
  ymax = *fp++;

  fbuffer = vbutrcastOrtho(block,datatype, 
			   orig, sz, 
			   eye, coi, up, 
			   numlights, lightpos, 
			   w, h, 
			   imgbounds, 
			   hither, yon, xmin, xmax, ymin, ymax);

  imgsz[0] = w;
  imgsz[1] = h;
  regionsz[0] = imgbounds[1] - imgbounds[0] + 1;
  regionsz[1] = imgbounds[3] - imgbounds[2] + 1;

  frame = (unsigned char *) malloc(regionsz[0]*regionsz[1]*4);
  vcbGrablk(fbuffer, frame, 4, 1, 2, imgsz, imgbounds, imgbounds+2, NULL);
  free(fbuffer);

  vcbMakeBinm(paras[2].data, VCB_UNSIGNEDBYTE,2,imgbounds, regionsz,4,frame);
  paras[2].len = regionsz[0]*regionsz[1]*4 + 32;
  free(frame);

  memcpy(paras[3].data, imgbounds, sizeof(int)*4);

  return 0; /* normal exit */
}
