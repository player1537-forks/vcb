#define __USE_LARGEFILE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "vcbutils.h"

#include <math.h>

#define MAXDIMS 4

int readHeaderBinm(char * filename, vcbdatatype * file_datatype, int * dims, int * orig, int * dsize, int * nattribs)
{
#ifndef VCB_USECUR_ENDIAN
#define VCB_USECUR_ENDIAN 0
#endif
#ifndef VCB_USEINV_ENDIAN
#define VCB_USEINV_ENDIAN 1
#endif

        int nvals, nbytes, nread, i;
        int file_endian = 1, change_endian;
        char errormsg [100];
        FILE * datain;

        datain = fopen64(filename,"rb");
        if (datain == NULL) {
               sprintf(errormsg,"cannot open %s to read\n", filename);
                perror(errormsg);
                return -1;
        }

        fread(&file_endian, sizeof(int), 1, datain);
        change_endian = (1 == file_endian) ? VCB_USECUR_ENDIAN : VCB_USEINV_ENDIAN;
        fread(file_datatype, sizeof(int), 1, datain);
	if (change_endian == VCB_USEINV_ENDIAN)
	      vcbToggleEndian(file_datatype,sizeof(int));

	fread(dims, sizeof(int), 1, datain);
	if (change_endian == VCB_USEINV_ENDIAN)
	      vcbToggleEndian(dims,sizeof(int));

	fread(orig, sizeof(int), *dims, datain);
	if (change_endian == VCB_USEINV_ENDIAN)
	for (i = 0; i < *dims; vcbToggleEndian(&orig[i], sizeof(int)), i++);

	fread(dsize, sizeof(int), *dims, datain);
	if (change_endian == VCB_USEINV_ENDIAN)
        for (i = 0; i < *dims; vcbToggleEndian(&dsize[i], sizeof(int)), i++);

       fread(nattribs, sizeof(int), 1, datain);
        if (change_endian == VCB_USEINV_ENDIAN) vcbToggleEndian(nattribs, sizeof(int));
        fclose(datain);
        return 0;
}

int main(int argc, char ** argv)
{
  vcbdatatype datatype;
  int ndims, orig[MAXDIMS], dsize[MAXDIMS], nattribs, i;
  int lbounds[MAXDIMS], ubounds[MAXDIMS];
  int ncuts, k, which[MAXDIMS], cutsize[MAXDIMS];
  float full;
  int interval, total;
  void * outdata;

  FILE * fp;

  int headlen;

  /* See if user passed an argument. */
  if (argc != 5) {
    fprintf(stderr, "Usage: %s datafile n k outrootname\n", argv[0]);
    exit(-1);
  }

   readHeaderBinm(argv[1], &datatype, &ndims, orig, dsize, &nattribs);

   ncuts = atoi(argv[2]);
   k     = atoi(argv[3]);

   for (i = 0; i < ndims; k /= ncuts, i ++)
     which[i] = k % ncuts;

   printf("vcbcut outputs"); 
   for (i = 0; i < ndims; i ++) {
     full = dsize[i];
     interval = (int)ceil(full/ncuts);
     lbounds[i] = interval * which[i];
     ubounds[i] = interval * (which[i]+1);/*if don't want overlap, then -1*/ 
     if (ubounds[i] >= dsize[i]) ubounds[i] = dsize[i]-1;

     cutsize[i] = ubounds[i] - lbounds[i] + 1;
     printf(": x%d [%d, %d] ",i, lbounds[i],ubounds[i]);
   }
   printf("\n");

   for (total = 1, i = 0; i < ndims; total *= cutsize[i], i++);
   outdata = malloc(total*nattribs*vcbSizeOf(datatype));

   fp = fopen64(argv[1],"rb");
   headlen = (3+2*ndims)*sizeof(int)+sizeof(datatype);/*vcbSizeOf(datatype);*/

   vcbFileGrablk(fp, headlen, outdata, 
		 nattribs,
		 vcbSizeOf(datatype),
		 ndims,
		 dsize,
		 lbounds, 
		 ubounds, 
		 NULL);

   for (i = 0; i < ndims; orig[i]+=lbounds[i], i++);
   vcbGenBinm(argv[4], datatype, ndims, orig, cutsize, nattribs, outdata);
 
   free(outdata);

   fclose(fp);

   return 0;
}
