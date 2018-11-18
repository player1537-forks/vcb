/*
 * ascii2val_test.c -- testing functions defined in scene_ascii2val.h.
 *
 * Written by Huadong Liu, http://www.cs.utk.edu/~hliu/
 */

#include <stdio.h>
#include <string.h>
#include "rcs.h"
#include "scene_ascii2val.h"

RCS_ID("@(#) $Id: ascii2val_test.c,v 1.1 2005/09/21 15:42:48 hliu Exp $")

int main()
{
	int i;
	int numLights;
	int numIsoRange;
	int numSeeds;
	char *input;
	float *fp;
	int *ip;

	/* 
	 * The decoded buffer overwrites the original string without causing 
	 * any memory problem because base64 encoding increases length of the 
	 * source buffer by 4/3.
	 */
	input = strdup("AgAAADMz3kIzM15DZqamQzPjikQz4wpFzVRQRTMz3kMA4ApEZqYmRDPjikUAnK1FzVTQRc1sQkQzM15Emvl5RJoN80Uz4wpGmj8cRmYGekRmRnpEZgb6RGZG+kTNhDtFzbQ7RTPjikRm5opEmumKRM3sikQA8IpEM/OKRGb2ikSa+YpEmuEKRTPjCkXN5ApFZuYKRQDoCkWa6QpFM+sKRc3sCkWa4QpFM+MKRc3kCkVm5gpFAOgKRZrpCkUz6wpFzewKRWbuCkUJAwAAZkZCRM1MQkRmBl5EzQxeRDMTXkSaGV5EZsZ5RM3MeUQz03lEmtl5RHgDAADnAwAAAQAAADPjikQBAAAAmuEKRTPjCkXN5ApFAwAAAAIAAABm5gpFCucKRQDoCkXN6ApFmukKRY/qCkUBAAAAAgAAAJpRUEUzU1BFzVRQRWZWUEUAWFBFmllQRTNbUEUFDQAA");

	/* always call this function before inquiry for any setting. */
	scene_ascii2val_init(input);

	numLights = scene_get_num_lights();
	printf("numLights=%d\n", scene_get_num_lights());
	
	fp = scene_get_lights();
	for (i=0; i<numLights; i++) {
		printf("light[%d]=(%f %f %f)\n", i, *fp, *(fp+1), *(fp+2));
		fp += 3;
	}
	
	fp = scene_get_halves();
	for (i=0; i<numLights; i++) {
		printf("half[%d]=(%f %f %f)\n", i, *fp, *(fp+1), *(fp+2));
		fp += 3;
	}

	fp = scene_get_lightvecs();
	for (i=0; i<numLights; i++) {
		printf("lightVec[%d]=(%f %f %f)\n", i, *fp, *(fp+1), *(fp+2));
		fp += 3;
	}

	fp = scene_get_ambient();
	for (i=0; i<numLights; i++) {
		printf("ambient[%d]=%f\n", i, *fp++);
	}

	fp = scene_get_diffuse();
	for (i=0; i<numLights; i++) {
		printf("diffuse[%d]=%f\n", i, *fp++);
	}

	fp = scene_get_specular();
	for (i=0; i<numLights; i++) {
		printf("specular[%d]=%f\n", i, *fp++);
	}
	
	fp = scene_get_model_view_matrix();
	printf("model view matrix\n");
	for (i=0; i<4; i++) {
		printf("%f %f %f %f\n", *fp, *(fp+1), *(fp+2), *(fp+3));
		fp+=4;
	}
	
	fp = scene_get_viewing_trans();
	printf("eye=(%f %f %f)\n", *fp, *(fp+1), *(fp+2));
	fp+=3;
	printf("center=(%f %f %f)\n", *fp, *(fp+1), *(fp+2));
	fp+=3;
	printf("up=(%f %f %f)\n", *fp, *(fp+1), *(fp+2));

	ip = (int *)scene_get_proj_trans();
	printf("projMode=%d\n", *ip++);
	fp = (float *)ip;
	printf("near=%f\n", *fp++);
	printf("far=%f\n", *fp);

	fp = scene_get_perspective_proj();
	printf("fov=%f\n", *fp++);
	printf("aspect=%f\n", *fp++);
	printf("hither=%f\n", *fp++);
	printf("yon=%f\n", *fp++);
	
	fp = scene_get_orthographic_proj();
	printf("left=%f\n", *fp++);
	printf("right=%f\n", *fp++);
	printf("bottom=%f\n", *fp++);
	printf("top=%f\n", *fp++);
	
	ip = (int *)scene_get_viewport_trans();
	printf("width=%d\n", *ip++);
	printf("height=%d\n", *ip++);

	
	fp = (float *)scene_get_mcube();
	if (fp)
		printf("isoValue=%f\n", *fp);
	else 
		printf("no mcube setting\n");

	
	fp = (float *)scene_get_raycasting();
	printf("scales=(%f %f %f)\n", *fp, *(fp+1), *(fp+2));
	ip = (int *)(fp+3);
	printf("colorScheme=%d\n", *ip++);
	numIsoRange=*ip++;
	printf("numIsoRange=%d\n", numIsoRange);
	fp = (float *)ip;
	for (i=0; i<numIsoRange; i++) {
		printf("posIsoRange[%d]=%f\n", i, *fp++);
	}
	for (i=0; i<numIsoRange; i++) {
		printf("sizeIsoRange[%d]=%f\n", i, *fp++);
	}
	for (i=0; i<numIsoRange; i++) {
		printf("opaIsoRange[%d]=%f\n", i, *fp++);
	}

	
	ip = (int *)scene_get_streamline();
	numSeeds = *ip++;
	printf("numSeeds=%d\n", numSeeds);
	fp = (float *)ip;
	for (i=0; i<numSeeds; i++) {
		printf("seed[%d]=(%f %f %f)\n", i, *fp, *(fp+1), *(fp+2));
		fp += 3;
	}
	printf("stepSize=%f\n", *fp++);
	ip = (int *)fp;
	printf("numVerts=%d\n", *ip);
	
	return 0;
}
