#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <time.h>
#include "vcbmath.h"
#include "vcbbtree.h"

#define DEBUG 0
#define MAX_OPERANDS 9
#define LENGTH_HYP 4
//
//These are used to compute the median
//
#define ELEM_SWAP(a,b) { register float t=(a);(a)=(b);(b)=t; }
#define median(a,n) kth_smallest(a,n,(((n)&1)?((n)/2):(((n)/2)-1)))

typedef struct myFiberStruct fiberStruct;
typedef struct myFiberBundle fiberBundle;
typedef struct myFiberStats fiberStats;

/* Bounding box for volume */
typedef struct {
	float minX;
	float minY;
	float minZ;
	float maxX;
	float maxY;
	float maxZ;
	float centerX;
	float centerY;
	float centerZ;
} boundingBox;

//Usable for clustering
struct myFiberStats {
	float* avgFeature;
	float* median;
	float* variance;
	float length;
	float* avgPosition;
};

/* MRI data structures */
struct myFiberStruct {
	int id;
	int numVerts;
	float* vertexArray;
	float* smoothedVertexArray;
	float* tangentVertexArray;
	int numFeatures; //Only used for rendering
	float* featureArray;
	float* normalizedFeatureArray;
	fiberBundle* parent;
	fiberStats stats;
};

struct myFiberBundle {
	int numFibers;
	int numFeatures;
	fiberStruct* fiber;

	boundingBox bbox;
	float offsetX;
	float offsetY;
	float offsetZ;
};

/* MRI-data structures */
typedef struct {
	VCB_mntree root;
	int numRecords;
	float* data;
	fiberStruct** fiber;
	int numClusters;
	int* cluster;
	float clusterThresh;
	int numPass;
	int* queryResults;
} bTree;

//------------------------------------------------------------------------
// Reads in features for corresponding fibers:
// File format . Numverts,newline,Fiber1XYZ1Feature1 Fiber1XYZ1Feature2...,newline...
//------------------------------------------------------------------------
int ReadFeatures(char* filename, fiberBundle* main, int numFeatures)
{
	int i, j, k, numVertsPerFiber, lineNumber;
	float currentFeature;
	i = j = k = numVertsPerFiber = lineNumber = 0;

	if (DEBUG) printf("ENTER ReadFeatures\n");
	FILE* fin;
	fin = fopen(filename,"r");

	if(fin==NULL) {
		printf("Error: can't access %s.\n", filename);
		return 0;
	}
	else { //we can open the file and proceed loading the data
		main->numFeatures = numFeatures;
		while(!feof(fin))
		{
			fscanf(fin, "%d", &numVertsPerFiber);
			lineNumber++;

			if (numVertsPerFiber !=  main->fiber[i].numVerts) //Mismatched data sets
			{
				printf("Error: line %d is not registered with the vertex data . %d vs. %d\n"
					"Check for file errors in the fiber before the given line", lineNumber, numVertsPerFiber, main->fiber[i].numVerts);
				if (DEBUG) printf("EXIT ReadFeatures\n");
				return 0;
			}
			else //It's ok to continue
			{
				//Allocate space for a list of features for each vertex
				main->fiber[i].featureArray = new float[main->numFeatures*main->fiber[i].numVerts];
				main->fiber[i].numFeatures = main->numFeatures;
				main->fiber[i].parent = main;

				for(j=0;j<numVertsPerFiber;j++)//For each fiber
				{
					lineNumber++;
					for(k=0;k<main->numFeatures;k++)//For each feature in the list
					{
						fscanf(fin,"%f ", &currentFeature);
						//printf("%f ", currentFeature);

						//Load the data into the next float
						main->fiber[i].featureArray[main->numFeatures*j+k] = currentFeature;
					}
				}
				
				fscanf(fin, "");
				lineNumber++;
			}

			i++;
		}
		fclose(fin);
		if (DEBUG) printf("EXIT ReadFeatures\n");
		return 1;
	}
}
//------------------------------------------------------------------------
// Find the number of fibers in the file
// Helper function for ReadVertices
//------------------------------------------------------------------------
int FindNumFibers(char* filename) {
	int numFibers, numVertsPerFiber;
	char temp;
	numFibers=numVertsPerFiber = 0;

	if (DEBUG) printf("ENTER FindNumFibers\n");
	FILE* fin;
	fin = fopen(filename,"r");
	if (fin==NULL) {
		printf("Error: can't access %s.\n",filename);
		numFibers = 0;
	}
	else {
		while (!feof(fin)) 
		{
			fscanf(fin, "%d", &numVertsPerFiber);
			for (int j=0; j<=numVertsPerFiber; j++) {			
				   while ((temp = fgetc(fin)) != '\n' && temp != EOF);

			}
			numFibers++;
		}
	}

	fclose(fin);
	if (DEBUG) printf("EXIT FindNumFibers\n");
	return numFibers;
}

//------------------------------------------------------------------------
// Reads in fiber data in file format
// File format . Numverts,newline,xyz,newline...
//------------------------------------------------------------------------
fiberBundle ReadVertices(char* filename)
{
	int i, j;
	float x,y,z;
	int numFibers, numVertsPerFiber, numVertsThreshold;
	i = j = numVertsPerFiber = 0;
	fiberBundle bundle;

	if (DEBUG) printf("ENTER ReadVertices\n");
	//Find number of fibers in the specified file
	numFibers = FindNumFibers(filename);

	//Only load fibers with more than this many vertices
	numVertsThreshold = 50;//Will need to create and not load features upon change

	FILE* fin;
	fin = fopen(filename,"r");

	if(fin==NULL) {
		printf("Error: can't access %s.\n", filename);
	}
	else {
		bundle.fiber = (fiberStruct*)malloc(numFibers*sizeof(fiberStruct));
		//Load the vertex data into memory
		while(!feof(fin))
		{
			fscanf(fin, "%d", &numVertsPerFiber);
			if(numVertsPerFiber > numVertsThreshold) {

				bundle.fiber[i].numVerts = numVertsPerFiber;
				bundle.fiber[i].vertexArray = new float[3*numVertsPerFiber];

				for(j=0;j<numVertsPerFiber;j++)
				{
					fscanf(fin,"%f %f %f", &x, &y, &z);
					bundle.fiber[i].vertexArray[3*j+0] = x;
					bundle.fiber[i].vertexArray[3*j+1] = y;
					bundle.fiber[i].vertexArray[3*j+2] = z;

					if(x>500 || y>500 || z>500)
						printf("  Serious Warning: Fiber %d, Vert %d out of bounds\n",i,j);

					//Initialize bounding box
					if (i==0 && j==0) {
						bundle.bbox.minX=x; bundle.bbox.minY=y; bundle.bbox.minZ=z;
						bundle.bbox.maxX=x; bundle.bbox.maxY=y; bundle.bbox.maxZ=z;
						//printf("Min=(%0.2f,%0.2f,%0.2f) Max=(%0.2f,%0.2f,%0.2f)\n",
						//	bundle.bbox.minX, bundle.bbox.minY, bundle.bbox.minZ,
						//	bundle.bbox.maxX, bundle.bbox.maxY, bundle.bbox.maxZ);
					}

					//See if vertice qualifies as bounding box point
					if(x<bundle.bbox.minX) bundle.bbox.minX=x;
					if(y<bundle.bbox.minY) bundle.bbox.minY=y;
					if(z<bundle.bbox.minZ) bundle.bbox.minZ=z;
					if(x>bundle.bbox.maxX) bundle.bbox.maxX=x;
					if(y>bundle.bbox.maxY) bundle.bbox.maxY=y;
					if(z>bundle.bbox.maxZ) bundle.bbox.maxZ=z;
				}

				fscanf(fin, "");
				i++;
				bundle.numFibers = i-1;
			}
			else {
				for(j=0;j<numVertsPerFiber;j++)
				{
					fscanf(fin,"%f %f %f", &x, &y, &z);

				}

				fscanf(fin, "");
			}
		}
	}
	fclose(fin);
	
	//Calculate center of bounding box	
	bundle.bbox.centerX=(float)((bundle.bbox.minX+bundle.bbox.maxX)/2.0);
	bundle.bbox.centerY=(float)((bundle.bbox.minY+bundle.bbox.maxY)/2.0);
	bundle.bbox.centerZ=(float)((bundle.bbox.minZ+bundle.bbox.maxZ)/2.0);


	if (DEBUG) printf("EXIT ReadVertices\n");
	//Will be blank if failed to open file
	return bundle;
}

//------------------------------------------------------------------------
// For outputting the fiber equation to the screen
//------------------------------------------------------------------------
char* GetFiberEquation(int numFeatures,float* myHyp){
	int i;
	static char equation[500]={0};
	char tmp[500]={0};
	sprintf(equation, "Fiber Equation (%d features): ", numFeatures);
	for(i=0;i<numFeatures;i++) {
		if (i!=0) strcat(equation," | ");
		sprintf(tmp,"F%1.0f [%.2f,%.2f]", myHyp[i*LENGTH_HYP+1],
						myHyp[i*LENGTH_HYP+(LENGTH_HYP-2)],myHyp[i*LENGTH_HYP+(LENGTH_HYP-1)]);
		strcat(equation,tmp);
	}

	return equation;
}

void SmoothGeometry(fiberBundle& main) {
	int i;
	int numDims=3;
	int numFeatures=6;
	float** tmpAvg;

	if (DEBUG) printf("ENTER SmoothGeometry\n");

	tmpAvg = (float**)malloc(main.numFibers*sizeof(float*));

//---GET AVERAGE FIRST
	//For each fiber in the bundle, temporarily store the average of the vertices
	for(i=0; i<main.numFibers; i++) {
		tmpAvg[i] = (float*)malloc(numDims*main.fiber[i].numVerts*sizeof(float));
		vcbAvgData1D(main.fiber[i].vertexArray, numDims, main.fiber[i].numVerts, 7, tmpAvg[i]);
	}

	//For each fiber in the bundle, fit a polynomial to the smoothed data
	for(i=0; i<main.numFibers; i++) {
		main.fiber[i].smoothedVertexArray = (float*)malloc(numDims*main.fiber[i].numVerts*sizeof(float));
		main.fiber[i].tangentVertexArray = (float*)malloc(numDims*main.fiber[i].numVerts*sizeof(float));

		//Store smooth geometry and tangents
		vcbSmoothData1D(tmpAvg[i], numDims, main.fiber[i].numVerts, 8, 4,
			main.fiber[i].smoothedVertexArray, main.fiber[i].tangentVertexArray);
	}

//---DON'T AVERAGE FIRST
/*	//For each fiber in the bundle
	for(i=0; i<main.numFibers; i++) {
		main.fiber[i].smoothedVertexArray = (float*)malloc(numDims*main.fiber[i].numVerts*sizeof(float));
		main.fiber[i].tangentVertexArray = (float*)malloc(numDims*main.fiber[i].numVerts*sizeof(float));

		//Store smooth geometry and tangents
		vcbSmoothData1D(main.fiber[i].vertexArray, numDims, main.fiber[i].numVerts, 5, 3, 
			main.fiber[i].smoothedVertexArray, main.fiber[i].tangentVertexArray);
	}
*/
	//For each fiber in the bundle, temporarily store the average of the vertices
	for(i=0; i<main.numFibers; i++) {
		free(tmpAvg[i]);
	}
	free(tmpAvg);

	if (DEBUG) printf("EXIT SmoothGeometry\n");
}

//------------------------------------------------------------------------
//This function creates a color code index based upon normalized fiber features
//------------------------------------------------------------------------
void NormalizeFeaturesCohort(fiberBundle& main, float minRange, float maxRange)
{
	int i, j, k;
	float* max;
	float* min;
	
	if (DEBUG) printf("ENTER NormalizeFeaturesCohort\n");
	max = (float*)malloc(main.numFeatures*sizeof(float));
	min = (float*)malloc(main.numFeatures*sizeof(float));

	//Initialize max and min
	for(k=0; k<main.numFeatures; k++) {
		max[k] = FLT_MIN;
		min[k] = FLT_MAX;
	}

	for(i=0; i<main.numFibers; i++) {
		for(j=0; j<(main.fiber[i].numVerts); j++) {
			for(k=0; k<main.numFeatures; k++) {
				//Collect statistics (max/min) for scaling on each feature
				if(main.fiber[i].featureArray[j*main.numFeatures+k] < min[k])
					min[k] = main.fiber[i].featureArray[j*main.numFeatures+k];
				if(main.fiber[i].featureArray[j*main.numFeatures+k] > max[k])
					max[k] = main.fiber[i].featureArray[j*main.numFeatures+k];
			}
		}
	}

	//Error check before attempting to normalize
	for(k=0; k<main.numFeatures; k++) {
		if( (max[k]-min[k]) == 0.0) {
			printf("Error: All values in Feature %d are the same\n",k);
		}
	}
	
	for(i=0; i<main.numFibers; i++) {
		main.fiber[i].normalizedFeatureArray = (float*)malloc(main.fiber[i].numVerts*main.numFeatures*sizeof(float));
		for(j=0; j<main.fiber[i].numVerts; j++) {
			for(k=0; k<main.numFeatures; k++) {

				//Copy data to the new array - normalization equation
				main.fiber[i].normalizedFeatureArray[j*main.numFeatures+k] = 
					maxRange*(main.fiber[i].featureArray[j*main.numFeatures+k]-min[k])/(max[k]-min[k]);

				//Make sure values are in the expected vcbRange
				if( (main.fiber[i].normalizedFeatureArray[j*main.numFeatures+k]>maxRange) ||
					(main.fiber[i].normalizedFeatureArray[j*main.numFeatures+k]<minRange) )
				{
					printf("Error: Normalization for Feature %d failed: Fiber %d, Vertex %d\n", k,i,j);
					printf("       Feature = %f, with max=%f, min=%f\n",main.fiber[i].normalizedFeatureArray[j*main.numFeatures+k],max[k],min[k]);
				}
			}
		}
	}

	free(max); free(min);
	if (DEBUG) printf("EXIT NormalizeFeaturesCohort\n");
}

//------------------------------------------------------------------------
// Used to compute the median
//------------------------------------------------------------------------
float kth_smallest(float a[], int n, int k)
{
	register int i,j,l,m;
	register float x;
	l=0; m=n-1;
	while (l<m) {
		x=a[k];
		i=l;
		j=m;
		do {
			while (a[i]<x) i++;
			while (x<a[j]) j--;
			if (i<=j) {
				ELEM_SWAP(a[i],a[j]);
				i++; j--;
			}
		} while (i<=j) ;
		if (j<k) l=i ;
		if (k<i) m=j ;
	}
	return a[k] ;
}

//------------------------------------------------------------------------
// Compute fiber statistics on all fibers for the given patients
//------------------------------------------------------------------------
void InitializeFiberStats(fiberBundle& patient) {
	int fi,fe,v;
	float* medianList;
	float varianceTmp;
	float x1,y1,z1,x2,y2,z2;
	//float* varianceList;

	for(fi=0;fi<patient.numFibers;fi++) {

		//MEAN (aka average)
		patient.fiber[fi].stats.avgFeature = (float*)malloc(patient.numFeatures*sizeof(float));
		for(fe=0;fe<patient.numFeatures;fe++) {//Initialize
			patient.fiber[fi].stats.avgFeature[fe]=0.0;
		}
		for(v=0;v<patient.fiber[fi].numVerts;v++) {//Compute running sum
			for(fe=0;fe<patient.numFeatures;fe++) {
				patient.fiber[fi].stats.avgFeature[fe] +=
					patient.fiber[fi].normalizedFeatureArray[v*patient.numFeatures+fe];
			}
		}
		for(fe=0;fe<patient.numFeatures;fe++) {//Normalize
			patient.fiber[fi].stats.avgFeature[fe] = patient.fiber[fi].stats.avgFeature[fe] /
				patient.fiber[fi].numVerts;
		}

		//MEDIAN
		patient.fiber[fi].stats.median = (float*)malloc(patient.numFeatures*sizeof(float));
		medianList=(float*)malloc(patient.fiber[fi].numVerts*sizeof(float));
		for(fe=0;fe<patient.numFeatures;fe++) {
			for(v=0;v<patient.fiber[fi].numVerts;v++) {//Create list of data for a feature
				medianList[v]=patient.fiber[fi].normalizedFeatureArray[v*patient.numFeatures+fe];
			}
			//Record the median of the list
			patient.fiber[fi].stats.median[fe] = median(medianList,patient.fiber[fi].numVerts);
		}
		free(medianList);


		//VARIANCE
		patient.fiber[fi].stats.variance = (float*)malloc(patient.numFeatures*sizeof(float));
		for(fe=0;fe<patient.numFeatures;fe++) {
			varianceTmp=0.0f;
			for(v=0;v<patient.fiber[fi].numVerts;v++) {//Calculate Sum
				varianceTmp+= (patient.fiber[fi].normalizedFeatureArray[v*patient.numFeatures+fe] -
					patient.fiber[fi].stats.avgFeature[fe])*(patient.fiber[fi].normalizedFeatureArray[v*patient.numFeatures+fe] -
					patient.fiber[fi].stats.avgFeature[fe]);
			}
			//Normalize and record
			patient.fiber[fi].stats.variance[fe] = varianceTmp/patient.fiber[fi].numVerts;
		}

		//AVERAGE FIBER POSITION (CENTROID)
		patient.fiber[fi].stats.avgPosition = (float*)malloc(3*sizeof(float));
		x1=y1=z1=0.0;
		for(v=0;v<patient.fiber[fi].numVerts;v++) {
			x1+=patient.fiber[fi].smoothedVertexArray[v*3+0];
			y1+=patient.fiber[fi].smoothedVertexArray[v*3+1];
			z1+=patient.fiber[fi].smoothedVertexArray[v*3+2];
		}
		patient.fiber[fi].stats.avgPosition[0]=x1/patient.fiber[fi].numVerts;
		patient.fiber[fi].stats.avgPosition[1]=y1/patient.fiber[fi].numVerts;
		patient.fiber[fi].stats.avgPosition[2]=z1/patient.fiber[fi].numVerts;


		//FIBER LENGTH
		patient.fiber[fi].stats.length=0.0;
		for(v=0;v<patient.fiber[fi].numVerts;v++) {//Initialize
			if(v!=0) {
				x1=patient.fiber[fi].vertexArray[(v-1)*3+0];
				y1=patient.fiber[fi].vertexArray[(v-1)*3+1];
				z1=patient.fiber[fi].vertexArray[(v-1)*3+2];
				x2=patient.fiber[fi].vertexArray[(v)*3+0];
				y2=patient.fiber[fi].vertexArray[(v)*3+1];
				z2=patient.fiber[fi].vertexArray[(v)*3+2];
				patient.fiber[fi].stats.length+=(float)sqrt( (x2-x1)*(x2-x1)+(y2-y1)*(y2-y1)+(z2-z1)*(z2-z1) );

			}
		}

		//Compute other statistics? Add to globals.h stats structure
	}
}

//------------------------------------------------------------------------
//This function creates a btree for all given datasets
//------------------------------------------------------------------------
void BuildTree(int numPatients, fiberBundle* patient, bTree& tree) {
	int p,fi,fe,index, numFibers;

	numFibers=0;
	for(p=0;p<numPatients;p++) {
		numFibers+=patient[p].numFibers;
	}
	tree.numRecords=numFibers;
	//Assumes all have same number of features
	tree.data = (float*)malloc(numFibers*(patient[0].numFeatures+1)*sizeof(float));
	tree.fiber = (fiberStruct**)malloc(numFibers*sizeof(fiberStruct*));

	index=0; numFibers=0;
	for(p=0;p<numPatients;p++) {
		for(fi=0; fi<patient[p].numFibers; fi++) {
			for(fe=0; fe<patient[p].fiber[fi].numFeatures; fe++) {
				tree.data[index] = patient[p].fiber[fi].stats.avgFeature[fe];
				index++;
			}
			//tree.data[index] = (float)numFibers;
			memcpy(&tree.data[index], &numFibers, sizeof(int));
			patient[p].fiber[fi].id=numFibers;
			tree.fiber[numFibers]=&patient[p].fiber[fi];//&patient[p].fiber[fi]; Why does this not work?

			/*
			if(numFibers>522)
				printf("Offset fiber %d = %f %f %f\n",numFibers,tree.fiber[numFibers]->parent->offsetX,index,
						tree.fiber[numFibers]->parent->offsetY,index,tree.fiber[numFibers]->parent->offsetZ);*/
			//printf("{%0.3f,%0.3f,%0.3f,%0.3f,%0.3f,%0.3f} = ID %0.1f tree.data[%d]\n",
			//	tree.data[index-6],tree.data[index-5],tree.data[index-4],tree.data[index-3],
			//	tree.data[index-2],tree.data[index-1],tree.data[index],index);
			index++;
			numFibers++;
		}
	}
	tree.queryResults=NULL;
	tree.cluster=NULL;
	tree.root=(VCB_mntree)vcbBuildBtree(numFibers,patient[0].numFeatures,(void*)tree.data,4,2);
}

//------------------------------------------------------------------------
// This function is the find function for clustering
//------------------------------------------------------------------------
int ClusterFind(int x, int *s) {
	if(s[x]<=0)
		return x;
	else{
		s[x]=ClusterFind(s[x],s);
		return s[x];
	}
}

//------------------------------------------------------------------------
// This function clusters the data for querying
//------------------------------------------------------------------------
int Cluster(void **items, int *clustered, int nitems, int (* similarityfunc)(void *, void *, float thresh), float thresh) {
	int *s,i,j,k,l,m,n=nitems,u,*c=clustered,f1,f2;

	s = (int *)calloc(n, sizeof(int));
	for(i=n-1;i>=0;i--) s[i]=0;
	for(k=0;k<n;k++)
		for(j=0;j<n;j++) {
		f1 = ClusterFind(k,s);
		f2 = ClusterFind(j,s);
		if ((f1!=f2)&&(similarityfunc(items[k],items[j],thresh))) {
			if(s[f2]<s[f1]) s[f1]=f2;
			else {if(s[f1]==s[f2]) s[f1]--;s[f2]=f1;}
		}
	}
	for(i=0;i<n;i++) c[i] = ClusterFind(i,s);
	for(i=0;i<n;i++) {
		for(u=0,l=0;l<n;l++) u|=c[l]==i;
		if(!u) {
			for(m=0,l=0;l<n;l++) if(c[l]>m) m=c[l];
			if (i<m) for(l=0;l<n;l++) if(c[l]==m) c[l]=i;
		}
	}
	free(s);
	for(m=0,l=0;l<n;l++) if(c[l]>m) m=c[l];
	return m+1;
}

//------------------------------------------------------------------------
// This function is the clustering function for specifying how to cluster
//------------------------------------------------------------------------
int ClusterFunc(void *a, void *b, float thresh) {
	float difference;
	fiberStruct *f1 = (fiberStruct*)a;
	fiberStruct *f2 = (fiberStruct*)b;

	//Fiber distance difference
	difference = f2->stats.length - f1->stats.length;

	if ( (difference*difference)<(thresh*thresh) ) return 1; else return 0;//0 if you don't want to cluster these two
}

//------------------------------------------------------------------------
// This function is the clustering function for specifying how to cluster
//------------------------------------------------------------------------
int ClusterFuncFeatureDistance(void *a, void *b, float thresh) {
	int i;
	float distance;
	fiberStruct *f1 = (fiberStruct*)a;
	fiberStruct *f2 = (fiberStruct*)b;

	//Euclidean distance of feature variance
	distance=0.0;
	for(i=0;i<f1->numFeatures;i++)
		distance+=((f2->stats.variance[i]-f1->stats.variance[i])*(f2->stats.variance[i]-f1->stats.variance[i]));

	if (distance<(thresh*thresh)) return 1; else return 0;//0 if you don't want to cluster these two
}

//------------------------------------------------------------------------
//This function will run the b-tree query to see which vertices meet the criteria
//------------------------------------------------------------------------
void RunQuery(int numFeatures, float* myHyp, bTree& tree) {
	int i;
	float *lb, *ub;
	// struct for the time value
	clock_t currentTime, startTime;

	printf("Running b-tree query...\n");

	lb = (float*)malloc(numFeatures*sizeof(float));
	ub = (float*)malloc(numFeatures*sizeof(float));

	//Testing/debug printout
	for(i=0; i<numFeatures; i++) {
		lb[i]=myHyp[i*LENGTH_HYP+2];//Min
		ub[i]=myHyp[i*LENGTH_HYP+3];//Max
		//printf("lb[%d]=%f,up[%d]=%f\n",i,lb[i],i,ub[i]);
	}

	tree.numPass=vcbQueryBtree(tree.root,lb,ub,&tree.queryResults);
	printf("  numFibers=%d, numPassed=%d <- should be %d\n",tree.numRecords,tree.numPass,tree.numRecords);
	free(lb); free(ub);
	//View all results
	/*for(i=0;i<tree.numPass;i++)
		printf("%d ",tree.queryResults[i]);*/

	//Clustering
	printf("Clustering %d items...\n",tree.numPass);
	if (tree.cluster != NULL) free(tree.cluster);
	tree.cluster = (int*)malloc(tree.numPass*sizeof(int));
    // gets the microseconds passed since app started
    startTime=clock();
	tree.numClusters=Cluster((void **)tree.fiber, tree.cluster, tree.numPass, ClusterFuncFeatureDistance,tree.clusterThresh);
    currentTime=clock();
	printf("  %d clusters in %d ms\n",tree.numClusters,currentTime-startTime);

	//View all results
	/*for(i=0;i<tree.numPass;i++)
		printf("%d=%d ",i,tree.cluster[i]);*/
}

int main(int argc, char** argv)
{
	int i;
	fiberBundle patient;
	bTree tree;
	char printFiberEq[500];//Char equation to be printed
	char printClusterEq[500];//Char equation to be printed
    clock_t currentTime, startTime;
	float* fiberHyp;
	int* clusterHyp = (int*)malloc(2*sizeof(int));


	//---Read in data
	printf("\nReading data...\n");
	patient=ReadVertices("datasets/t_btree/geometry.txt");
	printf("  Fiber Set = %d fibers\n",patient.numFibers);
	printf("\nReading features...\n");
	ReadFeatures("datasets/t_btree/features.txt", &patient, 9);
	printf("  Fiber Set centered at (%0.2f,%0.2f,%0.2f)\n",
		patient.bbox.centerX,patient.bbox.centerY,patient.bbox.centerZ);
	//Smooth Geometry & Normalize Features
	printf("\nNormalizing features...\n");
	SmoothGeometry(patient);
	NormalizeFeaturesCohort(patient, 0.0, 1.0);

	//---Initialization
	if(MAX_OPERANDS>patient.numFeatures)
		fiberHyp = (float*)malloc(MAX_OPERANDS*LENGTH_HYP*sizeof(float));
	else
		fiberHyp = (float*)malloc(patient.numFeatures*LENGTH_HYP*sizeof(float));

	//Initialize hypothesis
	for(i=0; i<patient.numFeatures; i++) {
		fiberHyp[i*LENGTH_HYP+0] = 3.0;//Operator
		fiberHyp[i*LENGTH_HYP+1] = (float)i;//Feature number
		fiberHyp[i*LENGTH_HYP+2] = 0.0;//Min
		fiberHyp[i*LENGTH_HYP+3] = 1.0;//Max
	}

	//Initialize clustering statistics
	printf("\nInitializing fiber statistics...\n");
	InitializeFiberStats(patient);
	printf("  FS Fib0 Avg = {%0.3f,%0.3f,%0.3f,%0.3f,%0.3f,%0.3f,%0.3f,%0.3f,%0.3f}\n",patient.fiber[0].stats.avgFeature[0]
		,patient.fiber[0].stats.avgFeature[1],patient.fiber[0].stats.avgFeature[2],patient.fiber[0].stats.avgFeature[3]
		,patient.fiber[0].stats.avgFeature[4],patient.fiber[0].stats.avgFeature[5],patient.fiber[0].stats.avgFeature[6]
		,patient.fiber[0].stats.avgFeature[7],patient.fiber[0].stats.avgFeature[8]);
	printf("  FS Fib0 Med = {%0.3f,%0.3f,%0.3f,%0.3f,%0.3f,%0.3f,%0.3f,%0.3f,%0.3f}\n",patient.fiber[0].stats.median[0]
		,patient.fiber[0].stats.median[1],patient.fiber[0].stats.median[2],patient.fiber[0].stats.median[3]
		,patient.fiber[0].stats.median[4],patient.fiber[0].stats.median[5],patient.fiber[0].stats.median[6]
		,patient.fiber[0].stats.median[7],patient.fiber[0].stats.median[8]);
	printf("  FS Fib0 Var = {%0.3f,%0.3f,%0.3f,%0.3f,%0.3f,%0.3f,%0.3f,%0.3f,%0.3f}\n",patient.fiber[0].stats.variance[0]
		,patient.fiber[0].stats.variance[1],patient.fiber[0].stats.variance[2],patient.fiber[0].stats.variance[3]
		,patient.fiber[0].stats.variance[4],patient.fiber[0].stats.variance[5],patient.fiber[0].stats.variance[6]
		,patient.fiber[0].stats.variance[7],patient.fiber[0].stats.variance[8]);
	printf("  FS Fib0 Pos = (%0.2f,%0.2f,%0.2f)\n",patient.fiber[0].stats.avgPosition[0],
		patient.fiber[0].stats.avgPosition[1],patient.fiber[0].stats.avgPosition[2]);
	printf("  FS Fib0 Length = %f\n",patient.fiber[0].stats.length);
	//Initialize b-tree
	printf("\nBuilding b-tree...\n");
	BuildTree(1,&patient,tree);
	tree.clusterThresh=(float)0.01;

	float* max = (float*)malloc(patient.numFeatures*sizeof(float));
	float* min = (float*)malloc(patient.numFeatures*sizeof(float));
	//Initialize max and min
	for(int k=0; k<patient.numFeatures; k++) {
		max[k] = FLT_MIN;
		min[k] = FLT_MAX;
	}
	for(i=0; i<patient.numFibers; i++) {
		for(int j=0; j<(patient.fiber[i].numVerts); j++) {
			for(int k=0; k<patient.numFeatures; k++) {
				//Collect statistics (max/min) for scaling on each feature
				if(patient.fiber[i].normalizedFeatureArray[j*patient.numFeatures+k] < 0.0)
					printf("Normalization Error at (%d,%d,%d)=%f\n",i,j,k,patient.fiber[i].normalizedFeatureArray[j*patient.numFeatures+k]);
				if(patient.fiber[i].normalizedFeatureArray[j*patient.numFeatures+k] > 1.0)
					printf("Normalization Error at (%d,%d,%d)=%f\n",i,j,k,patient.fiber[i].normalizedFeatureArray[j*patient.numFeatures+k]);
				//if(patient.fiber[i].normalizedFeatureArray[j*patient.numFeatures+k] == 1.0)
					//printf("Max at (%d,%d,%d)=%f\n",i,j,k,patient.fiber[i].normalizedFeatureArray[j*patient.numFeatures+k]);
			}
		}
	}

	//---Query
	printf("\nRunning fiber query...\n");
	strcpy(printFiberEq,GetFiberEquation(patient.numFeatures,fiberHyp));
	printf("  %s\n",printFiberEq);
	RunQuery(patient.numFeatures,fiberHyp,tree);
	clusterHyp[0]=0; clusterHyp[1]=tree.numClusters;
	sprintf(printClusterEq,"Has Clusters: [%d,%d] <- should be [0,8]\n",clusterHyp[0],clusterHyp[1]);
	printf("  %s\n",printClusterEq);

	//Clustering
	printf("Clustering %d items...\n",tree.numPass);
	if (tree.cluster != NULL) free(tree.cluster);
	tree.cluster = (int*)malloc(tree.numPass*sizeof(int));
    // gets the microseconds passed since app started
    startTime=clock();
	tree.numClusters=Cluster((void **)tree.fiber, tree.cluster, tree.numPass, ClusterFuncFeatureDistance,(float)0.005);
	currentTime=clock();
	printf("  %d clusters in %d ms\n",tree.numClusters,currentTime-startTime);
	clusterHyp[0]=0; clusterHyp[1]=tree.numClusters;
	sprintf(printClusterEq,"Has Clusters: [%d,%d] <- should be [0,84]\n",clusterHyp[0],clusterHyp[1]);
	printf("  %s\n",printClusterEq);

	//Clustering
	printf("Clustering %d items...\n",tree.numPass);
	if (tree.cluster != NULL) free(tree.cluster);
	tree.cluster = (int*)malloc(tree.numPass*sizeof(int));
    // gets the microseconds passed since app started
    startTime=clock();
	tree.numClusters=Cluster((void **)tree.fiber, tree.cluster, tree.numPass, ClusterFuncFeatureDistance,0.0);
	currentTime=clock();
	printf("  %d clusters in %d ms\n",tree.numClusters,currentTime-startTime);
	clusterHyp[0]=0; clusterHyp[1]=tree.numClusters;
	sprintf(printClusterEq,"Has Clusters: [%d,%d] <- should be [0,%d]\n",clusterHyp[0],clusterHyp[1],tree.numPass);
	printf("  %s\n",printClusterEq);

	//Clustering
	printf("Clustering %d items...\n",tree.numPass);
	if (tree.cluster != NULL) free(tree.cluster);
	tree.cluster = (int*)malloc(tree.numPass*sizeof(int));
    // gets the microseconds passed since app started
    startTime=clock();
	tree.numClusters=Cluster((void **)tree.fiber, tree.cluster, tree.numPass, ClusterFuncFeatureDistance,1.0);
	currentTime=clock();
	printf("  %d clusters in %d ms\n",tree.numClusters,currentTime-startTime);
	clusterHyp[0]=0; clusterHyp[1]=tree.numClusters;
	sprintf(printClusterEq,"Has Clusters: [%d,%d] <- should be [0,1]\n",clusterHyp[0],clusterHyp[1]);
	printf("  %s\n",printClusterEq);

	//Cleanup
	free(fiberHyp);
	free(clusterHyp);
	for(i=0;i<patient.numFibers;i++) {
		if( patient.fiber[i].vertexArray ) delete [] patient.fiber[i].vertexArray;
		free(patient.fiber[i].smoothedVertexArray);
		free(patient.fiber[i].tangentVertexArray);
		if( patient.fiber[i].featureArray) delete [] patient.fiber[i].featureArray;
		free(patient.fiber[i].normalizedFeatureArray);
	}
	vcbFreeBtree(tree.root);
	free(patient.fiber);
	free(tree.data);
	free(tree.fiber);
	free(tree.cluster);
	free(tree.queryResults);

	return 0;
}

