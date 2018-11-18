#include "vcbops.h"

int dfindex(int ndims, int * dsizes, int * coords)
/* VR default indexing func, coords values start from zero */
{
	int i, id = coords[0];
	for (i = 1; i < ndims; i ++)
		id = id*dsizes[i] + coords[i];

	return id;
}

void printtf(short *testfield, int *dims) {
	int c[2];
	for (c[0]=0;c[0]<5;c[0]++) {
		for (c[1]=0;c[1]<5;c[1]++) {
			printf("%d ",testfield[dfindex(2,dims,c)]);
		}
		printf("\n");
	}
	printf("\n");
}

void printtff(VCBfloat *testfield, int *dims) {
	int c[2];
	for (c[0]=0;c[0]<5;c[0]++) {
		for (c[1]=0;c[1]<5;c[1]++) {
			printf("%f ",testfield[dfindex(2,dims,c)]);
		}
		printf("\n");
	}
	printf("\n");
}

int main(int args, char **argv) {
	/*... test vcbDerivative and vcbAverage */
	short * testfield = (short *) calloc(5*5,sizeof(short));
	VCBfloat * results = (VCBfloat *) calloc(5*5,sizeof(VCBfloat));
	int dims[2];
	int c[2];
	dims[0] = 5;
	dims[1] = 5;


	for (c[0]=0;c[0]<5;c[0]++) {
		for (c[1]=0;c[1]<5;c[1]++) {
			testfield[dfindex(2,dims,c)] = ((short)c[0])*2;
		}
	}

	printtf(testfield,dims);

	vcbDerivative(2, dims, 1, 0, VCB_SHORT, 0, testfield, results);

	printtff(results,dims);

	vcbDerivative(2, dims, 1, 1, VCB_SHORT, 0, testfield, results);

	printtff(results,dims);

	vcbAverage(2, dims, 1, VCB_SHORT, 0, testfield, results);

	printtff(results,dims);

	vcbLinearityError(2, dims, 1, VCB_SHORT, 0, testfield, results);

	printtff(results,dims);

	return 0;

}

