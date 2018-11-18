#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "vcbutils.h"

typedef struct xyz_struct {float x,y,z;} xyz_td;

void load_volume_vox(
  char           *rootname,
  xyz_td         *dataSize,
  unsigned char  **dataVol,
  int            z_asp)
{
  char volumename[70], infoname[70];
  FILE *fp;
  char str[300];
  int nx, ny, nz, voln;
  float ax,ay,az;
  char junk[30];

  sprintf(volumename,"%s.vox",rootname);
  sprintf(infoname,"%s.txt",rootname);

  printf("working on %s %s\n",volumename,infoname);

  if(!(fp=fopen(infoname,"r")))
  {
    printf("Cannot open %s\n",infoname);
    exit(1);
  }
  fgets(str,100,fp);

  strtok(str," "); strtok(NULL," ");
  nx = atoi(strtok(NULL,"x"));
  ny = atoi(strtok(NULL,"x"));
  nz = atoi(strtok(NULL," "));
  printf("loading a volume of size %d %d %d\n",nx,ny,nz);
  voln= nx*ny*nz;

  fgets(str,100,fp);
  sscanf(str,"%s%s%f:%f:%f",junk,junk,&ax,&ay,&az);
  printf("aspect ratio = %f %f %f\n",ax,ay,az);
  z_asp = (int)az;

  fclose(fp);

  if(!(fp=fopen(volumename,"rb")))
  {
    printf("Cannot open %s\n",volumename);
    exit(1);
  }

  (*dataVol) = (unsigned char*)malloc(voln*sizeof(unsigned char));

  fread((*dataVol),sizeof(unsigned char),voln,fp);
  (*dataSize).x = (float)nx;
  (*dataSize).y = (float)ny;
  (*dataSize).z = (float)nz;

  fclose(fp);
}

int main (int argc, char **argv) {
	unsigned char *rawdata;
	char filename[30];
	int dsize[3], dummy, na;
	xyz_td xyz;
	char root[30];

	sprintf(root, "datasets/vox/dataset1");
    sprintf(filename,"convertvol.bin");

	load_volume_vox(root, &xyz, &rawdata, dummy);

	dsize[0] = (int)xyz.x;
	dsize[1] = (int)xyz.y;
	dsize[2] = (int)xyz.z;
	vcbWriteBinf(filename, VCB_UNSIGNEDBYTE, vcbBigEndian(), 3, dsize, 1, rawdata);

	dummy = 0;
	dsize[0]=0;
	dsize[1]=0;
	dsize[2]=0;
	printf("\n\nverifying data:");
    sprintf(filename,"%s.ub", filename);
	vcbReadBinf(filename, VCB_UNSIGNEDBYTE, vcbBigEndian(), &dummy, dsize, &na);

	printf("\ndimensions read: %d\ndimension sizes: %d %d %d\n",dummy,dsize[0],dsize[1],dsize[2]);
	
	return 0;
}
