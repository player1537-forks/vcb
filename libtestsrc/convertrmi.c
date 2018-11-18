#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "vcbutils.h"

int rmiIndex(int ndims, int * sz, int * coords)
{
   int i, id = coords[ndims-1];
   for (i = ndims - 2; i >= 0; i--)
	   id = id * sz[i] + coords[i];
   return id;
}

int load_volume_ub(char *volume_file_name,int *dsizes, unsigned char **dataVol)
{
  /* -1 for failure. 0 for success */

  FILE *fp;
  int voln;
  int headlen = 12;
  int lb[3], ub[3];

  if ((fp = fopen(volume_file_name,"rb")) == NULL) {
    perror(volume_file_name);
    return -1;
  }
  
  fread(&dsizes[0], sizeof(int), 1, fp);
  fread(&dsizes[1], sizeof(int), 1, fp);
  fread(&dsizes[2], sizeof(int), 1, fp);

  printf("loading xdim, ydim, zdim: %d %d %d\n",dsizes[0],dsizes[1],dsizes[2]);
  voln = dsizes[0] * dsizes[1] * dsizes[2];
  
  (*dataVol) = (unsigned char *) malloc (sizeof(unsigned char) * voln);
  
  lb[0] = lb[1] = lb[2] = 0;
  ub[0] = dsizes[0] - 1;
  ub[1] = dsizes[1] - 1;
  ub[2] = dsizes[2] - 1;
  vcbFileGrablk(fp, headlen, (*dataVol), 1, sizeof(unsigned char),
		  3, dsizes, lb, ub, rmiIndex);

  //fread((*dataVol), sizeof(unsigned char), voln, fp);

  fclose(fp);
 
}


int main (int argc, char **argv) 
{
  unsigned char *rawdata;
  int dsizes[3], orig[3];
  
   /* See if user passed an argument. */
   if (argc != 3) {
      fprintf(stderr, "Usage: %s rawfile outputfile\n", argv[0]);
      exit(1);
   }

   if (load_volume_ub(argv[1], dsizes,&rawdata) < 0) 
     return -1;

   orig[0] = dsizes[2];
   dsizes[2] = dsizes[0];
   dsizes[0] = orig[0];
   orig[0] = orig[1] = orig[2] = 0;
   vcbGenBinm(argv[2], VCB_UNSIGNEDBYTE, 3, orig, dsizes, 1, rawdata);

   free(rawdata);
   return 0;
}
