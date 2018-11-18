#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "glew.h"
#include <GL/glut.h>

#include "vcbutils.h"
#include "ArcBall.h"
#include "vcbmath.h"
#include "vcbglutils.h"

#include "atypes.h"

char projmode = 'P'; /* P for perspective, O for orthogonal */
projStruct proj;
viewStruct view;
lightStruct light0;

ArcBall spaceball;
int iwinWidth;
int iwinHeight;

GLuint  texobj;
int     mainwin, sidewin;

VCBdata1d * artery;  /* original artery data */
VCBdata1d * sartery; /* smoothed artery data */
VCBdata1d * dartery; /* derivative artery data */
VCBdata1d * Whlu;    /* turtle geometry coefficients */

int numTime, numVert, numEle;
bbox abbox;

#define NUM_COMP 3
void load_data(void)
{
	int i, j, cnt;
	float * fdata, * fptr;
	char filename [] = "datasets/artery/artery_tb7.bin";
	char errormsg [80];

	FILE * filep;
	
	printf("in load file\n");

	if ((filep = fopen(filename,"rb")) == NULL) {
		sprintf(errormsg,"Can't open %s in load_data\n",filename);
		perror(errormsg);
		exit(1);
	}

	fread(&numTime,sizeof(int),1,filep);
	fread(&numVert,sizeof(int),1,filep);
	fread(&numEle,sizeof(int),1,filep);

	artery = (VCBdata1d *) malloc (sizeof(VCBdata1d)*numTime);
	sartery = (VCBdata1d *) malloc (sizeof(VCBdata1d)*numTime);
	dartery = (VCBdata1d *) malloc (sizeof(VCBdata1d)*numTime);
	Whlu = (VCBdata1d *) malloc (sizeof(VCBdata1d)*numTime);
	fdata = (float *) malloc (sizeof(float)*numVert*3);

	for (i = 0; i < numTime; i ++) {

		fptr = fdata;

		for (j = 0; j < numVert; fptr += NUM_COMP, j ++) {

			if (fseek(filep,8L,SEEK_CUR)) {
				sprintf(errormsg,"Error in fseek %s in load_data\n",filename);
				perror(errormsg);
				exit(1);
			}

			if (fread(fptr,sizeof(float),NUM_COMP, filep) != NUM_COMP) {
				sprintf(errormsg,"Error in fread %s in load_data\n",filename);
				perror(errormsg);
				exit(1);
			}
		}

		if ((artery[i] = new_data1dstruct (VCB_FLOAT,numVert,NUM_COMP,fdata)) == NULL) {
			sprintf(errormsg,"new_mfistruct failed\n");
			perror(errormsg);
			exit(1);
		}

		vcbAvgData1D((float*)artery[i]->v_list,NUM_COMP,numVert,2,fdata);
		vcbAvgData1D(fdata,NUM_COMP,numVert,2,(float*)artery[i]->v_list);
		vcbAvgData1D((float*)artery[i]->v_list,NUM_COMP,numVert,2,fdata);

		sartery[i] = new_data1dstruct(VCB_FLOAT,numVert, NUM_COMP, NULL);
		dartery[i] = new_data1dstruct(VCB_FLOAT,numVert, NUM_COMP, NULL);
		vcbSmoothData1D(fdata, NUM_COMP, numVert, 10,4, (float*)sartery[i]->v_list,(float*)dartery[i]->v_list);

#define FINITEDIF 0
#define SMOOSSING 0

#if FINITEDIF
		for (j = 0; j < numVert*NUM_COMP-NUM_COMP; j ++)
			dartery[i]->v_list[j] = sartery[i]->v_list[j+3] - sartery[i]->v_list[j];
#endif

		Whlu[i] = new_data1dstruct(VCB_FLOAT,numVert, NUM_COMP, NULL);
		vcbTurtleGeometry(dartery[i]->isize1stDim, (float*)dartery[i]->v_list, (float*)Whlu[i]->v_list);

#if SMOOSSING
		vcbAvgData1D(Whlu[i]->v_list,3,numVert,10,fdata);
		for (j = 0; j < numVert*3; Whlu[i]->v_list[j]=fdata[j], j++)
			;
#endif

		for (j = 0; j < numVert; j ++)
			vcbNormalize3(&((float*)dartery[i]->v_list)[j*NUM_COMP]);
		
	}

	free(fdata);
	fclose(filep);

	filep = fopen("turtleL.txt","w");
	for (i = 0; i < numVert; i ++) {
		for (j = 0; j < numTime; j ++)
			fprintf(filep,"%f ",((float*)Whlu[j]->v_list)[i*NUM_COMP+1]);
		fprintf(filep,"\n");
	}
	fclose(filep);

	filep = fopen("turtleU.txt","w");
	for (i = 0; i < numVert; i ++) {
		for (j = 0; j < numTime; j ++)
			fprintf(filep,"%f ",((float*)Whlu[j]->v_list)[i*NUM_COMP+2]);
		fprintf(filep,"\n");
	}
	fclose(filep);

	filep = fopen("scoor.txt","w");
	for (i = 0; i < numVert; i ++) {
		for (j = 0; j < numTime; j ++)
			fprintf(filep,"%f %f %f ",((float*)sartery[j]->v_list)[i*NUM_COMP],((float*)sartery[j]->v_list)[i*NUM_COMP+1],((float*)sartery[j]->v_list)[i*NUM_COMP+2]);
		fprintf(filep,"\n");
	}
	fclose(filep);

	abbox.low[0] = abbox.low[1] = abbox.low[2] = 1e6;
	abbox.high[0] = abbox.high[1] = abbox.high[2] = -1e6;

	for (i = 0; i < numTime; i ++) {
		cnt = 0;
		for (j = 0; j < 3*numVert; cnt++, j ++){
			abbox.low[cnt%3] = VCB_MINVAL(abbox.low[cnt%3],((float*)sartery[i]->v_list)[j]);
			abbox.high[cnt%3] = VCB_MAXVAL(abbox.high[cnt%3],((float*)sartery[i]->v_list)[j]);
		}
	}

	printf("after smoothing bounding box: (%f %f %f) (%f %f %f)\n",abbox.low[0], abbox.low[1],
		abbox.low[2], abbox.high[0],abbox.high[1],abbox.high[2]);

	abbox.low[0] = abbox.low[1] = abbox.low[2] = 1e6;
	abbox.high[0] = abbox.high[1] = abbox.high[2] = -1e6;

	for (i = 0; i < numTime; i ++) {
		cnt = 0;
		for (j = 0; j < 3*numVert; cnt++, j ++){
			abbox.low[cnt%3] = VCB_MINVAL(abbox.low[cnt%3],((float*)artery[i]->v_list)[j]);
			abbox.high[cnt%3] = VCB_MAXVAL(abbox.high[cnt%3],((float*)artery[i]->v_list)[j]);
		}
	}
}


int curtime = 0;

void display (void)
{
	static int curtime; 

	int i, j;
	float * fptr, * tptr;
	float cur_view[3], cur_light[3];
	float m_shadedstreamline [16];

	spaceball.Update();

	vcbCalcViewLightVecs(spaceball.GetMatrix(),view.eye,view.center,light0.lightpos, 
		cur_view, cur_light);

	texobj = vcbInitShadedStreamline(GL_TEXTURE0_ARB,MATERIAL_GOLD);
	vcbUpdateTexMatShadedStreamline(m_shadedstreamline,cur_view,cur_light);
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMultMatrixf(m_shadedstreamline);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslated(view.center[0],view.center[1],view.center[2]);
	glMultMatrixf(spaceball.GetMatrix());
	glTranslated(-view.center[0],-view.center[1],-view.center[2]);

	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);

	glColor4f(1.f,0.f,0.f,1.f);
	glPointSize(10.f);
	glBegin(GL_POINTS);
	  glVertex3fv(view.center);
    glEnd();

	glColor4f(0.f, 1.f,0.f,1.f);

	glClientActiveTexture(GL_TEXTURE0_ARB);
	for (i = 0; i < numTime; i ++) 
	{
	
		if (i == 0)
			texobj = vcbInitShadedStreamline(GL_TEXTURE0_ARB,MATERIAL_GOLD);
		else if (i == 15)
			texobj = vcbInitShadedStreamline(GL_TEXTURE0_ARB,MATERIAL_BLACK_PLASTIC);

		//i = curtime/50;
		int numV = sartery[i]->isize1stDim;

		//i = 0;
		fptr = (float*)sartery[i]->v_list;
		tptr = (float*)dartery[i]->v_list;

		glBegin(GL_LINE_STRIP);
			for (j = 0; j < numV; glTexCoord3fv(tptr), glVertex3fv(fptr), tptr +=3, fptr+= 3, j ++)
			   ;
		glEnd();

		curtime ++; curtime = curtime % (numTime*50);
	}

	glutSwapBuffers();
	glPopMatrix();
}



void cleanup (void)
{
	int i;
	for (i = 0; i < numTime; i ++) {
		free_data1dstruct(artery[i]);
		free_data1dstruct(sartery[i]);
		free_data1dstruct(dartery[i]);
		free_data1dstruct(Whlu[i]);
	}

	free(artery);
	free(sartery);
	free(dartery);
	free(Whlu);

	printf("finished cleaning up ...\n");
	fflush(stdout);
}

int main(int argc, char ** argv)
{
	float oglverno;
	load_data();

	printf("after reading file got %d %d %d\n",numTime,numVert,numEle);
	printf("bounding box: (%f %f %f) (%f %f %f)\n",abbox.low[0], abbox.low[1],
		abbox.low[2], abbox.high[0],abbox.high[1],abbox.high[2]);

	initApp(); /* initialize the application */

	mainwin = initGLUT(argc,argv);

    GLenum err = glewInit();
    if (GLEW_OK != err) {
		fprintf(stderr,"glew error, initialization failed\n");
        fprintf(stderr,"Glew Error: %s\n", glewGetErrorString(err)); 
    }

	initGLsettings();
	atexit(cleanup);

	oglverno = vcbGetOGLVersion();
	printf("using ogl version %f\n",oglverno);

	texobj = vcbInitShadedStreamline(GL_TEXTURE0_ARB,MATERIAL_BRASS);

	glutMainLoop();

	return 0;
}
