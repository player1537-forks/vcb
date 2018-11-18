#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "vcbutils.h"
#include "vcbvolren.h"
#include "vcblinalg.h"
#include "vcbimage.h"

unsigned char * voxels;
int             sz[3];
float           tlut[256*4];

int load_vox(char *rootname, int * dsizes, unsigned char **dataVol)
/* -1 for failure. 0 for success */
{
	char volumename[70], infoname[70], str[300], junk[30];  
	float ax,ay,az;
	FILE *fp;
	int nx, ny, nz, voln;
	
	sprintf(volumename,"%s.vox",rootname);
	sprintf(infoname,"%s.txt",rootname);
	
	if((fp=fopen(infoname,"r"))==NULL) {
		fprintf(stderr,"load_vox, cannot open infofile %s\n",infoname);
		return -1;
	}
  
	fgets(str,100,fp);
	strtok(str," "); strtok(NULL," ");
	nx = atoi(strtok(NULL,"x"));
	ny = atoi(strtok(NULL,"x"));
	nz = atoi(strtok(NULL," "));
  
	printf("loading a volume of size %d x %d x%d\n", nx, ny, nz);
	voln= nx*ny*nz;

	fgets(str,100,fp);
	sscanf(str,"%s%s%f:%f:%f",junk,junk,&ax,&ay,&az);
	printf("aspect ratio = %f:%f:%f\n",ax, ay, az);
	fclose(fp);

	if((fp=fopen(volumename,"rb"))==NULL) {
		fprintf(stderr,"load_vox, cannot open volume file %s\n", volumename);
		return -1;
	}

	(*dataVol) = (unsigned char *) malloc (sizeof(unsigned char) * voln);

	fread((*dataVol),sizeof(unsigned char),voln,fp);
	dsizes[0] = nx;
	dsizes[1] = ny;
	dsizes[2] = nz;

	fclose(fp);

	return 0;
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

void load_data(void)
{
	unsigned char * fvox;
	int i, j, k;

	char * dataname = "datasets/vox/dataset2";
	char * tlutname = "datasets/vox/dataset2_trans.txt";
	
	if (load_vox(dataname, sz, &voxels) < 0) {
		fprintf(stderr, "load_data: error loading datafile\n");
		exit(-1);
	}

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

	return 0;
}
