#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "vcbutils.h"
#include "vcbvolren.h"

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


int main (int argc, char **argv) 
{
  unsigned char *voxels, *fvox;
  int i, j, k;
  char filename[30];
  int sz[3], dummy, na;
  xyz_td xyz;
  char root[30];
  int orig[3];

  sprintf(root, "datasets/vox/dataset2");
  sprintf(filename,"%s.bin",root);
  
  load_volume_vox(root, &xyz, &voxels, dummy);
  sz[0] = (int)xyz.x;
  sz[1] = (int)xyz.y;
  sz[2] = (int)xyz.z;

  orig[0] = orig[1] = orig[2] = 0;

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
  
  free(voxels);
  voxels = fvox;
  
  return vcbGenBinm(filename, VCB_UNSIGNEDBYTE, 3, orig, sz, 4, voxels);
  
}
