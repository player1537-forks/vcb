#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "glew.h"
#include <GL/glut.h>

#include "vcbutils.h"
#include "vcbmath.h"
#include "ArcBall.h"
#include "vcbglutils.h"
#include "atypes.h"
#include "vcblic.h"

char projmode = 'O'; /* P for perspective, O for orthogonal */
projStruct proj;
viewStruct view;
lightStruct light0;

ArcBall spaceball;
int iwinWidth;
int iwinHeight;

GLuint  texobj;
int     mainwin, sidewin;

VCBdata3d vectors;  /* flow field */
VCBdata3d licvol;   /* lic volume */
VCBdata2d tex2d;    /* a slice to show */

int numTime, numVert, numEle;
bbox abbox;
int sliceid = 24;


void load_data(void)
{
	int sizes[3], nattribs;
	int i;
	float * vectordata = NULL;
	float minval, maxval;
	FILE * file;

	printf("in load file\n");

	sizes[0] = sizes[1] = sizes[2] = 48;
	nattribs = 3;
	vectors = new_data3dstruct(VCB_FLOAT,48,48,48,3,NULL);
	licvol  = new_data3dstruct(VCB_UNSIGNEDBYTE, 48, 48, 48, 1, NULL);
	tex2d   = new_data2dstruct(VCB_UNSIGNEDBYTE, 32, 32, 1, NULL);

	/* load flow data */
	file = fopen("datasets/tornado/ftornado.bin","rb");
	fread(vectors->v_list,sizeof(float),48*48*48*3,file);
	fclose(file);

	minval = 1e6; maxval = -1e6;
	for (i = 0; i < sizes[0]*sizes[1]*sizes[2]*nattribs; i++) {
		vcbToggleEndian(&vcb_p_f(vectors->v_list)[i],4);
		minval = VCB_MINVAL(minval,vcb_p_f(vectors->v_list)[i]);
		maxval = VCB_MAXVAL(maxval,vcb_p_f(vectors->v_list)[i]);
	}
	printf("minval = %f, maxval = %f\n", minval, maxval);

	/* compute LIC */
	vcbGenLicTex3D (vcb_p_f(vectors->v_list), vcb_p_ub(licvol->v_list), 48,48,48);

	minval = 1e6; maxval = -1e6;
	for (i = 0; i < sizes[0]*sizes[1]*sizes[2]; i++) {
		minval = VCB_MINVAL(minval,vcb_p_ub(licvol->v_list)[i]);
		maxval = VCB_MAXVAL(maxval,vcb_p_ub(licvol->v_list)[i]);
	}
	printf("minval = %f, maxval = %f\n", minval, maxval);


	abbox.low[0] = abbox.low[1] = abbox.low[2] = 0.f;
	abbox.high[0] = sizes[0];
	abbox.high[1] = sizes[1];
	abbox.high[2] = sizes[2];

	printf("bounding box: (%f %f %f) (%f %f %f)\n",abbox.low[0], abbox.low[1],
		abbox.low[2], abbox.high[0],abbox.high[1],abbox.high[2]);	
}

void lickeys(unsigned char c, int x, int y)
{
	float fzin = 0.95;
	float fzout = 1.05;
	switch(c) {
      case 'i':
		  proj.xmin *= fzin;
		  proj.xmax *= fzin;
		  proj.ymin *= fzin;
		  proj.ymax *= fzin;
		  proj.fov  *= fzin;
		  reshape(iwinWidth,iwinHeight);
		  break;
	  case 'o':
		  proj.xmin *= fzout;
		  proj.xmax *= fzout;
		  proj.ymin *= fzout;
		  proj.ymax *= fzout;
		  proj.fov  *= fzout;
		  reshape(iwinWidth,iwinHeight);
		  break;
	  case 'u':
		  if (sliceid < 47) sliceid ++;
		  break;
	  case 'd':
		  if (sliceid > 0) sliceid --;
		  break;
	  case 'q':
		  exit(0);
		  break;
	  default:
		  break;
  }
}


void display (void)
{
	static int curtime = 0; 
	int dsize[3], ub[3], lb[3];

	spaceball.Update();

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslated(view.center[0],view.center[1],view.center[2]);
	glMultMatrixf(spaceball.GetMatrix());
	glTranslated(-view.center[0],-view.center[1],-view.center[2]);

	glClear(GL_COLOR_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);

	glClientActiveTexture(GL_TEXTURE0_ARB);

	glColor4f(0.8, 0.8,0.8,1.0);

	dsize[0] = dsize[1] = dsize[2] = 48;
	lb[0] = sliceid; lb[1] = 8; lb[2] = 8;
	ub[0] = sliceid; ub[1] = 39; ub[2] = 39;
	vcbGrablk(licvol->v_list, tex2d->v_list,1,1,3,dsize,lb,ub, NULL);

#if 0
	glRasterPos2f(1.f, 1.f);
	glDrawPixels(32,32,GL_LUMINANCE,GL_UNSIGNED_BYTE,tex2d->v_list);
#else
	
	if (curtime == 0)
		texobj = vcbBindOGLTexture2D(GL_TEXTURE0_ARB, GL_LINEAR, GL_LUMINANCE, GL_LUMINANCE, GL_UNSIGNED_BYTE, 
			32, 32, tex2d->v_list, GL_REPLACE, NULL);
	else
		vcbUpdateOGLTexture2D(texobj,GL_TEXTURE0_ARB, GL_LINEAR, GL_LUMINANCE, GL_LUMINANCE, GL_UNSIGNED_BYTE, 
			32, 32, tex2d->v_list, GL_REPLACE, NULL);

	glBegin(GL_QUADS);

	    glTexCoord2f(0.f, 0.f);
		glVertex3f(0.f, 0.f, 0.f);
	    glTexCoord2f(0.f, 1.f);
		glVertex3f(0.f, 32.f, 0.f);
	    glTexCoord2f(1.f, 1.f);
		glVertex3f(32.f, 32.f, 0.f);
	    glTexCoord2f(1.f, 0.f);
		glVertex3f(32.f, 0.f, 0.f);

	glEnd();
#endif

	glutSwapBuffers();
	glPopMatrix();

	curtime ++;
}



void cleanup (void)
{
	free_data3dstruct(vectors);
	free_data3dstruct(licvol);
	free_data2dstruct(tex2d);

	printf("finished cleaning up ...\n");
	fflush(stdout);
}

int main(int argc, char ** argv)
{
	float oglverno;
	load_data();

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

	glutKeyboardFunc(lickeys);

	oglverno = vcbGetOGLVersion();
	printf("using ogl version %f\n",oglverno);

	glutMainLoop();

	return 0;
}
