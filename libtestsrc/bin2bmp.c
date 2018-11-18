#include <stdlib.h>
#include <stdio.h>

#include "vcbimage.h"
#include "vcbutils.h"

int main (int argc, char ** argv)
{

	unsigned char * pixels;

	int             sz[3], orig[3];
	int             ndims,nattribs;
	vcbdatatype     rawdatatype;
	
	if (argc != 3) {
	  fprintf(stderr,"usage: %s input_binfile output_image\n",argv[0]);
	  exit(1);
	}

	if ((pixels = vcbReadBinm(argv[1], &rawdatatype, &ndims, orig, sz, &nattribs)) 
	    == NULL) {
		fprintf(stderr, "load_data: error loading datafile\n");
		exit(-1);
	}

	printf("Reading %s succeeded\n", argv[1]);
	printf("imgbounds: %d %d %d %d, nchannels: %d\n",orig[0],orig[1],sz[0],sz[1],nattribs);
	
	vcbImgWriteBMP(argv[2],pixels, 4, sz[1], sz[0]);
	printf("Wrote %s to %s\n",argv[1],argv[2]);

	free(pixels);

	return 0;
}
