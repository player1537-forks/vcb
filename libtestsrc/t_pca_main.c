#include <stdlib.h>
#include <stdio.h>
#include "vcbmath.h"

int main(int args, char ** argv) 
{
	float means[2], eval[2];
	float evec[4];
	int i, j;
	int npnts = 10;
	int nattribs = 2;

	/* p0_1: stands for attribute #1 of point 0
	 *
	 * the below goes in this sequence:
	 *   p0_0 p0_1 p1_0 p1_1 p2_0 p2_1 ...
	 */
	double d[20] = { 2.5, 2.4, 0.5, 0.7, 2.2, 2.9, 1.9, 2.2, \
		3.1, 3.0, 2.3, 2.7, 2, 1.6, 1, 1.1, 1.5, 1.6, 1.1, 0.9};

	vcbDataPCA(d, VCB_DOUBLE, npnts, nattribs, means, evec, eval);

	for (i = 0; i < npnts; i ++)
		for (j = 0; j < nattribs; j ++)
			d[i*nattribs+j] -= means[j];

	printf("transformed results \n");
	printf("eigenvalues: %f %f\n", eval[0], eval[1]);

	for (i = 0; i < npnts; i ++) {
		printf("%f ", d[i*nattribs]*evec[0]+d[i*nattribs+1]*evec[1]);
		printf("%f \n", d[i*nattribs]*evec[2]+d[i*nattribs+1]*evec[3]);
	}

	return 1;
}
