#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "vcbutils.h"

int load_volume_ub(char *volume_file_name,int *dsizes, unsigned char **dataVol)
{
  /* -1 for failure. 0 for success */

  FILE *fp;
  int voln;

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
  
  fread((*dataVol), sizeof(unsigned char), voln, fp);
  
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

   orig[0] = orig[1] = orig[2] = 0;
   vcbGenBinm(argv[2], VCB_UNSIGNEDBYTE, 3, orig, dsizes, 1, rawdata);

   free(rawdata);
   return 0;
}
