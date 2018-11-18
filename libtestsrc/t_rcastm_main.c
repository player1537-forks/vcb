#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "vcbutils.h"
#include "vcbvolren.h"
#include "vcblinalg.h"
#include "vcbimage.h"

/* headers used for mmap */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h> /* mmap() is defined in this header */
#include <fcntl.h>
/* end of headers used for mmap */

unsigned char * voxels;
int             sz[3], orig[3];
float           tlut[256*4];

char *          binfile;
size_t          binfile_len;

char * mmap_binfile(char * filename)
{
  int fd;
  char * data;
  struct stat statbuf;

  if ((fd = open (filename, O_RDWR)) < 0){
     perror("mmap_binfile can't open file for reading");
     return NULL;
  }
 
  /* find size of input file */
  if (fstat (fd,&statbuf) < 0){
     perror("fstat error");
     return NULL;
  }

  binfile_len = statbuf.st_size;
  /* mmap the input file */
  if ((data = mmap (0, binfile_len, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0))
      == (caddr_t) -1){
     perror("mmap error");
     return NULL;
  }

  return data;

}



int load_classify(char * filename, float * transflut)
/*
 * -1 for failure, 0 for success 
 * transflut is assumed to have valid memory allocation
 */
{
	int i, j;
	FILE * fp;
	float * tmparray, junk;
 
	if ((fp=fopen(filename,"r"))==NULL) {
		fprintf(stderr,"load_classify, cannot open classify file %s\n", filename);
		return -1;
	}

	for (i = 0; i < 256; i ++) {
		tmparray = & transflut[i * 4];
		for (j = 0; j < 4; j ++)
			fscanf(fp,"%f",&tmparray[j]);
		fscanf(fp,"%f",&junk);
		fscanf(fp,"%f",&junk);
		fscanf(fp,"%f",&junk);
    }

	fclose(fp);
	return 0;
}

vcbdatatype rawdatatype;

void load_data(void)
{
	unsigned char * fvox;
	int i, j, k;
	int ndims,nattribs;

	char * dataname = "datasets/vox/dataset2.bin.ub";
	char * tlutname = "datasets/vox/dataset2_trans.txt";
	
	if ((binfile = mmap_binfile(dataname)) == NULL) {
	  fprintf(stderr,"load_data: faiure while doing mmap\n");
	  exit(-1);
	}

	if ((voxels = vcbLoadBinm(binfile, &rawdatatype, &ndims, orig, sz, &nattribs)) == NULL) {
		fprintf(stderr, "load_data: error loading datafile\n");
		exit(-1);
	}

	printf("after vcbLoadBinm %d datatype, %d dims, %d sz, %d attribs\n",rawdatatype,ndims,sz[0],nattribs);


	if (load_classify(tlutname, tlut) < 0) {
		fprintf(stderr, "load_classify: error loading transfer lut\n");
		exit(-1);
	}
}

void voxtransf (float * rgba, void * vox)
{
	unsigned char * voxel = (unsigned char *) vox;
	int id = voxel[0] * 4;
	rgba[0] = tlut[id+0];
	rgba[1] = tlut[id+1];
	rgba[2] = tlut[id+2];
	rgba[3] = tlut[id+3];
}

int main ()
{
	VCB_volren volren;
	unsigned char * framebuffer;
	float scales[3] = {0.5f, 0.5f, 0.5f};
	float translate1[3] = {-32.f, -32.f, -32.f}, translate2[3] = {32.f, 32.f, 32.f};
	float rotate[16], obj2scr[16];
	int   origin[3] = {0, 0, 0};
	clock_t time1, time2;
	int w, h;
	int imgbounds[4];

	load_data();
	w = h = 512;
	framebuffer = (unsigned char *) malloc (w*h*4*sizeof(unsigned char));

	volren = vcbNewVolRenderer(VCB_ABSEMI);

#if 0
    vcbVolrPerspective(volren, 30.f, 1.f, 1.f, 100000.f);
#else
    vcbVolrOrtho(volren, 1.0f, 1000.f, -50.f, 50.f, -50.f, 50.f);
#endif

    vcbVolrLookAt(volren, 32.f, 32.f, 32.f, 160.f, 32.f, 160.f, 1.f, 0.f, 0.f);
	vcbVolrSetLight(volren, 0, 32.f, 32.f, 160.f);
	vcbVolrViewPort(volren, w, h);
	vcbVolrTransFunc(volren, voxtransf);
	vcbVolrFramebuffer(volren, VCB_MEMORY, VCB_UNSIGNEDBYTE, framebuffer);

	time1 = clock();
    vcbVolrApplyTransformation(volren);
	vcbVolrRC(volren, VCB_UNSIGNEDBYTE, voxels, 4, origin, sz, scales, 0.5f, imgbounds);
	time2 = clock();

	printf("done in %f seconds\n",(float)(time2-time1)/CLOCKS_PER_SEC);
	printf("imgbounds are: %d %d %d %d\n",imgbounds[0], imgbounds[1], imgbounds[2], imgbounds[3]);
	vcbImgWriteBMP("rcast.bmp",framebuffer, 4, w, h);

    
	/* test the rotation */
	printf("rotate the data for 45 degree along y axis...\n");	
	memcpy(obj2scr, volren->obj2scr, 16 * sizeof(float));
	vcbRotate3f(rotate, 0, 1, 0, 45);
	vcbVolrUpdateTransformTransl(volren, translate2);    
    vcbVolrUpdateTransformRot(volren, rotate);
	vcbVolrUpdateTransformTransl(volren, translate1);    
	vcbVolrRC(volren, VCB_UNSIGNEDBYTE, voxels, 4, origin, sz, scales, 0.5f, imgbounds);
    vcbImgWriteBMP("rcast_rotate.bmp",framebuffer, 4, w, h);
	memcpy(volren->obj2scr, obj2scr, 16 * sizeof(float));

	/* test the translation */
	printf("move the data along x axis...\n");	
	memcpy(obj2scr, volren->obj2scr, 16 * sizeof(float));
	vcbVolrUpdateTransformTransl(volren, translate1);    
	vcbVolrRC(volren, VCB_UNSIGNEDBYTE, voxels, 4, origin, sz, scales, 0.5f, imgbounds);
    vcbImgWriteBMP("rcast_translate.bmp",framebuffer, 4, w, h);
	memcpy(volren->obj2scr, obj2scr, 16 * sizeof(float));

	/* test the scaling */
	printf("scale the data in half...\n");	
	memcpy(obj2scr, volren->obj2scr, 16 * sizeof(float));
	vcbVolrUpdateTransformScale(volren, scales);    
	vcbVolrRC(volren, VCB_UNSIGNEDBYTE, voxels, 4, origin, sz, scales, 0.5f, imgbounds);
    vcbImgWriteBMP("rcast_scale.bmp",framebuffer, 4, w, h);
	memcpy(volren->obj2scr, obj2scr, 16 * sizeof(float));

	vcbVolrCloseRC(volren);
	vcbFreeVolRenderer(volren);
	free(framebuffer);

	munmap(binfile, binfile_len);
	return 0;
}
