#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vcbutils.h"
#include "vcbimage.h"


int main(int argc, char ** argv)
{
	int nvals, i, j, imgsz[2];
	int change_endian;
	unsigned char file_endian;
	char errormsg [100];
	FILE * datain, * outfp;
	float * tex2d;

	long stride;
	int  dimid, sliceid, dsizes[3];
	int  ndims, nattribs, valsize, withbmp;
	char * filename, *outroot;
	int *ub, *lb;
	char outputname[20];
	unsigned char * img;
	float minval, maxval;
	int slice_dim, slice_no;
/*
	if (argc != 6) {
	  fprintf(stderr,"USAGE: slicer filename slice_dim slice_no rootname_output withbmps\n");
	  exit(-1);
	}
*/
	/*fprintf(stderr,"\nslicer assumes volumes containing floating point values\n");*/

	filename = "datasets/ttooth/ttooth.bin.f";
	valsize  = sizeof(float);
	outroot  = "slice_of_tooth";
	slice_dim = 2;
	slice_no = 30;
	withbmp  = 1; /* 0 - no bmp's, 1- generate bmp for the slice too */

	i = strlen(filename);

	datain = fopen(filename,"rb");
	if (datain == NULL) {
	  sprintf(errormsg,"cannot open %s to read\n", filename);
	  perror(errormsg);
	  return -1;
	}

	fread(&file_endian, sizeof(unsigned char), 1, datain);
	change_endian = (vcbBigEndian() == file_endian) ? 0 : 1;

	fread(&ndims, sizeof(int), 1, datain);
	if (change_endian == 1)
	  vcbToggleEndian(&ndims,sizeof(int));

	fread(dsizes, sizeof(int), ndims, datain);
	if (change_endian == 1)
	  for (i = 0; i < ndims; vcbToggleEndian(&dsizes[i], sizeof(int)), i++)
			;

	fread(&nattribs, sizeof(int), 1, datain);
	if (change_endian == 1) vcbToggleEndian(&nattribs, sizeof(int));

	if (nattribs != 1) fprintf(stderr, "More than one attribute: Slicer assumes volumes containing a SINGLE floating point attribute\n"), exit(1);

	printf("reading %s expecting %d dimensions, %d by %d by %d voxels, %d attributes per voxel\n", filename, ndims,dsizes[0],dsizes[1],dsizes[2],nattribs);
	dimid = slice_dim;
	sliceid = slice_no;

	if (dimid > ndims-1) fprintf(stderr, "Dimension for slicing is out of range\n"), exit(1);
	if (sliceid > dsizes[dimid]-1) fprintf(stderr, "slice number if out of range\n"), exit(1);
/*
	printf("which dimension to slice [0-%d]: ",ndims-1);
	fflush(stdout);
	scanf("%d", &dimid);

	printf("which slice [0-%d]: ", dsizes[dimid]-1);
	fflush(stdout);
	scanf("%d", &sliceid);
*/
	for (i = 0, nvals = 1; i < ndims; i ++)
	  nvals *= (i!= dimid) ? dsizes[i] : 1;

	tex2d = (float *) malloc (nvals * sizeof(float));
	lb = (int *)malloc(ndims*sizeof(int));
	ub = (int *)malloc(ndims*sizeof(int));

	stride = 1 + sizeof(int) + ndims * sizeof(int) + sizeof(int);

	for (i = 0; i < ndims; lb[i] = 0, ub[i] = dsizes[i] - 1, i ++);
	lb[dimid] = ub[dimid] = sliceid;

	vcbFileGrablk(datain, stride, tex2d, nattribs, sizeof(float),\
		    ndims, dsizes, lb, ub, NULL);

	fclose(datain);

	sprintf(outputname,"%s%.3d.2bin",outroot, sliceid);

	outfp = fopen(outputname,"wb");
	fwrite(tex2d, sizeof(float), nvals, outfp);
	fclose(outfp);

	if (!withbmp) return 0;

	sprintf(outputname,"%s%.3d.bmp",outroot, sliceid);
	minval = 1000000.f; maxval = -minval;
	for (i = 0; i < nvals; i ++) {
	  if (minval > tex2d[i]) minval = tex2d[i];
	  if (maxval < tex2d[i]) maxval = tex2d[i];
	}
	img = (unsigned char *) malloc(nvals*3);
	for (i = 0; i < nvals; i ++)
	  img[i*3] = img[i*3+1] = img[i*3+2] = \
	    (unsigned char)((tex2d[i]-minval)/(maxval-minval)*255);

	for (i = 0, j = 0; i < ndims; i ++) {
	  if (i != dimid) {
	    imgsz[j] = dsizes[i];
	    j ++;
	  }
	  if (j > 1)
	    break;
	}
/*	printf("imagesize %d %d\n",imgsz[1], imgsz[0]);*/

	vcbImgWriteBMP(outputname, img, 3, imgsz[1], imgsz[0]);

	return 0;
}

