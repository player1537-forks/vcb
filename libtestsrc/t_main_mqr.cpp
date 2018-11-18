#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <time.h>

#include "glew.h"
#include <GL/glut.h>

#include "vcbutils.h"
#include "ArcBall.h"
#include "vcbglutils.h"
#include "vcbmath.h"
#include "atypes.h"

#include "vcbimage.h"
#include "vcbvolren.h"
#include "vcblinalg.h"
#include "vcbcolor.h"
#include "vcbmntree.h"

extern void display(void);

char projmode = 'O'; /* P for perspective, O for orthogonal */
projStruct proj;
viewStruct view;
lightStruct light0;

ArcBall spaceball;
int iwinWidth;
int iwinHeight;

float TRIANGLE_LOW  = 0.0f;
float TRIANGLE_HIGH = 1.0f;
#define CWIDTH 128

GLuint  texobj;
GLubyte colorBar[512][CWIDTH+10][4];
int     mainwin, sidewin;

clock_t time1, time2, time3, time4;

bbox abbox;
int  changed;
int  pic=0;

#define FTB 1
#define NQUANTS 10
#define MAXQUERIES 5

unsigned char * pixels;
unsigned char * voxels = NULL;
unsigned char * curvoxels[MAXQUERIES];
unsigned int    active[MAXQUERIES];
unsigned int    sliceid[MAXQUERIES][NQUANTS];
unsigned int    slicerange[MAXQUERIES][NQUANTS];
unsigned int    nQueries = 1;
unsigned int    curQ = 0;
unsigned int    doClustering = 0;
unsigned int    triAttrib[MAXQUERIES];
unsigned int    useT[MAXQUERIES];
unsigned int    selC[NQUANTS];
unsigned int	onscreen = 1;

int             sz[3], nvox = 0;
float         **  tlut;
int             ndims, nattribs = NQUANTS-3;


VCB_mntree root;

void initApp (void)
{
	float mradius = 0.f;

	iwinWidth = 800;
	iwinHeight = 600;

	spaceball.SetWidth(iwinWidth);
	spaceball.SetHeight(iwinHeight);

	view.center[0] = 50.f + (abbox.low[0] + abbox.high[0])/2.;
	view.center[1] = (abbox.low[1] + abbox.high[1])/2.;
	view.center[2] = (abbox.low[2] + abbox.high[2])/2.;

	if ((abbox.high[0]-abbox.low[0]) > mradius) mradius = abbox.high[0]-abbox.low[0];
	if ((abbox.high[1]-abbox.low[1]) > mradius) mradius = abbox.high[1]-abbox.low[1];
	if ((abbox.high[2]-abbox.low[2]) > mradius) mradius = abbox.high[2]-abbox.low[2];

	printf("view.center = (%f %f %f)\n",view.center[0],view.center[1],view.center[2]);

	view.eye[0] = view.center[0];
	view.eye[1] = view.center[1];
	view.eye[2] = view.center[2];
	view.eye[2] += mradius*10.f;

	view.up[0] = 0.;
	view.up[1] = 1.;
	view.up[2] = 0.;

	proj.aspect = 1.0;
	proj.fov = 10.;
	proj.hither = 1.0;
	proj.yon = 1e6;
	proj.xmin = mradius*(-1.);
	proj.xmax = mradius*1.;
	proj.ymin = mradius*(-1.);
	proj.ymax = mradius*1.;

	light0.lightpos[0] = view.eye[0]; /* using headlight */
	light0.lightpos[1] = view.eye[1];
	light0.lightpos[2] = view.eye[2];
}

void initGLsettings(void)
{
	glClearColor(0.,0.,0.,1.);

	glViewport(0,0,iwinWidth,iwinHeight);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glLoadIdentity();
	if (projmode == 'P')
		gluPerspective(proj.fov,
					   proj.aspect,
					   proj.hither,
					   proj.yon);
	else
		glOrtho(proj.xmin,proj.xmax,
				proj.ymin,proj.ymax,
				proj.hither,proj.yon);

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glLoadIdentity();
	gluLookAt(view.eye[0],view.eye[1],view.eye[2],
			  view.center[0],view.center[1],view.center[2],
			  view.up[0],view.up[1],view.up[2]);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(1);
	glClearDepth(1.);

}

void reshape(int width,int height)
{
	if (width <= 0 || height <= 0)
		return;

	iwinWidth  = width;
	iwinHeight = height;
	spaceball.SetWidth(width);
	spaceball.SetHeight(height);

	double aspect_ratio = width*1.0/height;
	proj.aspect = aspect_ratio;

	glViewport(0,0,width,height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	if (projmode == 'P') {
		gluPerspective(proj.fov,
					   proj.aspect,
					   proj.hither,
					   proj.yon);
	}
	else {
		double x_center, y_center, x_rad, y_rad;
		x_center = (proj.xmin + proj.xmax)/2.;
		y_center = (proj.ymin + proj.ymax)/2.;
		x_rad    = (proj.xmax - proj.xmin)/2.;
		y_rad    = (proj.ymax - proj.ymin)/2.;

		if (width < height)
			y_rad = x_rad/proj.aspect;
		else
			x_rad = y_rad * proj.aspect;

		proj.xmin = x_center - x_rad;
		proj.xmax = x_center + x_rad;
		proj.ymin = y_center - y_rad;
		proj.ymax = y_center + y_rad;

		glOrtho(proj.xmin,proj.xmax,
				proj.ymin,proj.ymax,
				proj.hither,proj.yon);

	}

	display();
}

void keys(unsigned char c, int x, int y)
{
	float fzin = 0.95;
	float fzout = 1.05;
	unsigned char * pixels;

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
	  case 's': /* save framebuffer */
		  pixels = (unsigned char *) malloc(iwinWidth*iwinHeight*4*sizeof(unsigned char));
		  glReadPixels(0,0,iwinWidth,iwinHeight,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
		  vcbImgWriteBMP("pbufres.bmp",pixels,4, iwinWidth, iwinHeight);
		  free(pixels);
		  break;
	  case 'q':
		  exit(0);
		  break;
	  default:
		  break;
  }
}

void mouse_handler(int button, int bstate, int x, int y)
{   
	//printf("in mouse_handler\n");

	if (button == GLUT_LEFT_BUTTON) {
		if (bstate == GLUT_DOWN) {
		  spaceball.StartMotion( x, y, glutGet(GLUT_ELAPSED_TIME));
		} else if (bstate == GLUT_UP) {
			spaceball.StopMotion( x, y, glutGet(GLUT_ELAPSED_TIME));
		}
	}
}

void trackMotion(int x, int y) 
{
	//printf("in trackMotion\n");

    if (spaceball.Track())
      spaceball.TrackMotion( x, y, glutGet(GLUT_ELAPSED_TIME)); 
}

int initGLUT(int argc, char **argv)
{
	int winid; 

	glutInit( &argc, argv);
	glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE|GLUT_DEPTH);

    glutInitWindowSize(iwinWidth,iwinHeight);
    glutInitWindowPosition(50,50);
    winid = glutCreateWindow(argv[0]);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(display);
    glutMouseFunc(mouse_handler);
	glutMotionFunc(trackMotion);
	glutKeyboardFunc(keys);
    //glutSpecialFunc(SpecialKeys);

	return winid;
}

void voxtransf (float * rgba, void * vox)
{
  unsigned char * voxel = (unsigned char *) vox;
  int id = voxel[0];
  rgba[0] = tlut[id][0];
  rgba[1] = tlut[id][1];
  rgba[2] = tlut[id][2];
  rgba[3] = tlut[id][3];
}

#define BASE 256.f

#define USEFLOAT 0  /* changing this would require varying types in 6 functions */

void  sparseKeyFunc(unsigned char * key, void * dptr)
{
#if USEFLOAT
	int i;
	float ret;

	unsigned char * d = (unsigned char *) dptr;
	ret = d[0];
	for (i = 1; i < 7; i ++)
		ret = ret * BASE + d[i];
	memcpy(key, &ret, sizeof(float));
#else
	key[0] = 0;
	memcpy(key+1, dptr, 7);
#endif
}



void   boundKeyFunc(unsigned char * key, int * data)
{
	int i;

#if USEFLOAT
	float ret;

	ret = data[0];
	for (i = 1; i < 7; i ++)
		ret = ret*BASE + data[i];

	memcpy(key, & ret, sizeof(float));
#else
	key[0] = 0;
	key ++;
	for (i = 0; i < 7; i ++)
		key[i] = (unsigned char)data[i];

#endif
}

void  decompKeyFunc(int nattrib, unsigned char * key, int * cl)
{
	int i;
#if USEFLOAT
	float k, f;

	memcpy(&k, key, sizeof(float));
	for (i = nattrib - 1; i >= 0; i --) {
		f = (float)floor(k/256.);
		cl[i] = (k - f*256.f);
		k = f;
	}
#else
	key ++;
	for (i = 0; i < 7; i ++)
		cl[i] = key[i];
#endif
}

int  compareKeyFunc(unsigned char * k1, unsigned char * k2)
{
#if USEFLOAT
	float * f1, * f2;
	f1 = (float *) k1;
	f2 = (float *) k2;

	if ((*f1) < (*f2)) return -1;
	if ((*f1) > (*f2)) return 1;
	return 0;
#else
	int i;
	for (i = 1; i < 8; i ++){
		if (k1[i] < k2[i]) return -1;
		if (k1[i] > k2[i]) return  1;
	}
	return 0;
#endif
}

void buildColorBar() {
	static int ff=0;
	int i,j,k, g1, g2;
	int vg, vf, rg, rf, g, f;
	float m1;

	for(i=0;i<256;i++) tlut[i][3] = 0.0f;
	for(k=0;k<512;k++) for(j=0 ;j<CWIDTH;j++) colorBar[k][j][3] = 0;

	for(i=0;i<nQueries;i++) {
		if (!active[i]) continue;

		for(j=0;j<256;j++) {
			if(abs((int)sliceid[i][triAttrib[i]] - j) <= slicerange[i][triAttrib[i]]) {
				m1 = TRIANGLE_LOW + (TRIANGLE_HIGH - TRIANGLE_LOW) * pow((1.f - fabs(j - (int)sliceid[i][triAttrib[i]])/(float)(slicerange[i][triAttrib[i]]+1.0f)),2.5f);
				if (tlut[j][3] < m1) tlut[j][3] = m1;
			}
		}

		g1 = (int)((float)((int)sliceid[i][6] - (int)slicerange[i][6]) * (float)CWIDTH / 256.0); if (g1<=0) g1=1;
		g2 = (int)((float)((int)sliceid[i][6] + (int)slicerange[i][6]) * (float)CWIDTH / 256.0); if (g2>CWIDTH) g2=CWIDTH;
		for(k=0;k<512;k++) {
			if(abs((int)sliceid[i][triAttrib[i]] - k/2) <= slicerange[i][triAttrib[i]]) {
				if (useT[i]) {
					rf = slicerange[i][triAttrib[i]];
					rg = slicerange[i][6];
					vf = sliceid[i][triAttrib[i]];
					vg = sliceid[i][6];
					m1 = ((float)vg+(float)rg)/((float)rf);
					for(j=g1;j<g2;j++) {
						f  = (int)(k/2);
						g  = (int)(j / ((float)CWIDTH / 256.0));
						if((-m1*(((float)f)-((float)vf)+((float)rf))+(float)vg+(float)rg>(float)g) || (m1*(((float)f)-((float)vf)-((float)rf))+(float)vg+(float)rg>(float)g))
						{} else if (colorBar[k][CWIDTH-j][3] < (GLubyte)(tlut[k/2][3]*255)) colorBar[k][CWIDTH-j][3] = (GLubyte)(tlut[k/2][3]*255);
					}
				} else for(j=g1;j<g2;j++) if(colorBar[k][CWIDTH-j][3] < (GLubyte)(tlut[k/2][3]*255)) colorBar[k][CWIDTH-j][3] = (GLubyte)(tlut[k/2][3]*255);

			}
		}
	}

	for(k=0;k<512;k++) colorBar[k][0][0] = colorBar[k][0][1] = colorBar[k][0][2] = colorBar[k][0][3] = colorBar[k][CWIDTH+9][0] = colorBar[k][CWIDTH+9][1] = colorBar[k][CWIDTH+9][2] = colorBar[k][CWIDTH+9][3] = 255;
	for(k=0;k<CWIDTH+10;k++) colorBar[0][k][0] = colorBar[0][k][1] = colorBar[0][k][2] = colorBar[0][k][3] = colorBar[511][k][0] = colorBar[511][k][1] = colorBar[511][k][2] = colorBar[511][k][3] = 255;

	if (ff++) vcbVolrInitS(voxtransf);
}

void load_data(char *fn)
{
  unsigned short * cvox;
  float t;
  int i, j, k, ret, itemsz;

  char filename[300];
  char fullname[300];
  char *tmp;
  float * buffs[NQUANTS], minval[NQUANTS], maxval[NQUANTS];
  unsigned char item[3];
  int   keysz, na;
  float gradient[3];
  FILE * fp;

  if(!(fp = fopen(fn,"r"))) perror(fn), exit(1);

  for (i = 0; i < NQUANTS; i ++) minval[i] = -(maxval[i] = -1e6f);
  itemsz = NQUANTS*sizeof(unsigned char) + 3*sizeof(short);

  for (i = 0; i < nattribs && fgets(filename, 300, fp); i++) {
	ret = (int)strlen(filename);
	filename[ret-1] = '\0';
	sprintf(fullname,"%s",filename);
	buffs[i] = (float *)vcbReadBinf(fullname, VCB_FLOAT, vcbBigEndian(), &ndims, sz, &na);

	if (!nvox) for(nvox=1,j=0;j<ndims;j++) nvox *= sz[j];
	if (!voxels) voxels = (unsigned char *) malloc (nvox * itemsz);

	/* set up the load data process */

	/* read everything in once to get ranges */
	for (j = 0; j < nvox; j ++) {
		minval[i] = VCB_MINVAL(minval[i],buffs[i][j]);
		maxval[i] = VCB_MAXVAL(maxval[i],buffs[i][j]);
	}

	printf("Quantity %d in vcbRange: %f - %f\n",i, minval[i], maxval[i]);

	/* read everything in once again to quantize everything */
	for (j = 0; j < nvox; j++) {
		item[0] = (unsigned char)((buffs[i][j]-minval[i])/(maxval[i]-minval[i])*255+0.5);
		memcpy(&voxels[j * itemsz + i], &item[0], sizeof(unsigned char));
	}

	free(buffs[i]);
  }

  /* read in gradient files */
  for (i = nattribs; i < NQUANTS && fgets(filename, 300, fp); i ++) {
	ret = (int)strlen(filename);
	filename[ret-1] = '\0';
	sprintf(fullname,"%s",filename);
	buffs[i] = (float *)vcbReadBinf(fullname, VCB_FLOAT, vcbBigEndian(), &ndims, sz, &na);
}

  for(j=0;j<nvox;j++) {
	/* always assume the last three are gradients, without which no shading :) */

	gradient[0] = buffs[NQUANTS - 3][j];
	gradient[1] = buffs[NQUANTS - 2][j];
	gradient[2] = buffs[NQUANTS - 1][j];
	la_normalize3(gradient);
	tmp = (char *)&item[nattribs];
	tmp[0] = (char)(gradient[0]*127);
	tmp[1] = (char)(gradient[1]*127);
	tmp[2] = (char)(gradient[2]*127);
	/*
	item[NQUANTS - 3] = (unsigned char)((char)(gradient[0]*127));
	item[NQUANTS - 2] = (unsigned char)((char)(gradient[1]*127));
	item[NQUANTS - 1] = (unsigned char)((char)(gradient[2]*127));
	*/
	cvox = (unsigned short*)&item[10];
	cvox[0] = (unsigned short)( j / (sz[1]*sz[2]));
	cvox[1] = (unsigned short)((j % (sz[1]*sz[2])) / sz[2]);
	cvox[2] = (unsigned short)( j % sz[2]);

	memcpy(&voxels[j * itemsz + nattribs], &item[nattribs], 9);
  }

  fclose(fp);

  for (i = nattribs; i < NQUANTS; i ++) free(buffs[i]);

  printf("got %d voxels\n",nvox);

#if USEFLOAT
  keysz = sizeof(float);
#else
  keysz = 8;
#endif
  printf("now building mntree..."); fflush(stdout);
  time1 = clock();
  root = vcbMntreeBuildub(32, 3, keysz, nvox, itemsz, voxels, nattribs, \
	                     sparseKeyFunc,  \
						 compareKeyFunc, \
						 decompKeyFunc,  \
						 boundKeyFunc);
  time2 = clock();
  t = (float)(time2-time1)/CLOCKS_PER_SEC;
  printf("building done in %4.3f seconds, %.3f mil-vox/sec, %d voxs.\n",t, nvox/t/1e6f, nvox);
  printf("done!\n"); fflush(stdout);

  abbox.low[0] = abbox.low[1] = abbox.low[2] = 0.f;
  abbox.high[0] = sz[0]-1.f;
  abbox.high[1] = sz[1]-1.f;
  abbox.high[2] = sz[2]-1.f;

  printf("bounding box: (%f %f %f) (%f %f %f)\n",abbox.low[0], abbox.low[1],
	 abbox.low[2], abbox.high[0],abbox.high[1],abbox.high[2]);

  for (i = 0; i < MAXQUERIES; i ++) {
  	triAttrib[i] = 0;
  	active[i] = 1;
	useT[i] = 0;
	for (j = 0; j < NQUANTS; j ++) {
		selC[i] = 0;
		sliceid[i][j] = (j?128:1);
		slicerange[i][j] = (j?128:0);
	}
  }
  selC[nattribs] = 0;

}

int makeCurVoxelist(int q) {
	int i, voxnum = 0, offset;
	int * lb, * ub;
	void ** results;
	lb = (int *) malloc(nattribs * sizeof(int));
	ub = (int *) malloc(nattribs * sizeof(int));

	for (i = 0; i < 7; i ++) {
		lb[i] = sliceid[q][i] - slicerange[q][i];
		ub[i] = sliceid[q][i] + slicerange[q][i];
	}

 	if (curvoxels[q] != NULL)
		free(curvoxels[q]);

	voxnum = vcbMntreeQueryub(root, nattribs, lb, ub, &results);

	curvoxels[q] = (unsigned char *) malloc(voxnum * 10);
	offset = (255/nQueries)*q;
	for (i = 0; i < voxnum; i ++) {
		curvoxels[q][i * 10] = ((unsigned char *)results[i])[0];///nQueries+offset;
		memcpy(&curvoxels[q][i*10+1], ((unsigned char *)results[i])+7, 9);
	}

	free(results);
	free(lb);
	free(ub);
	return voxnum;
}

void oglDrawImage(float x, float y, GLubyte ***image, int width, int height, int blending) {
	GLint matrixMode;
	GLboolean lightingOn, blendOn, atestOn;

	/* setup opengl states for image output, saving modified states too */
	if (lightingOn = glIsEnabled(GL_LIGHTING))
		glDisable(GL_LIGHTING);
	if (blendOn    = glIsEnabled(GL_BLEND)) ;
		if (blending) glDisable(GL_BLEND);
	if (atestOn = glIsEnabled(GL_ALPHA_TEST))
		glDisable(GL_ALPHA_TEST);
	glGetIntegerv(GL_MATRIX_MODE, &matrixMode);

	glPushAttrib(GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0., 1., 0., 1., -1., 1.);
	/* end set up */

	glRasterPos3f(x, y, 0.f);
	glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, image);

	/* restore opengl state machine */
	glPopAttrib();
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(matrixMode);
	if (lightingOn) glEnable(GL_LIGHTING);
	if (blendOn)    glEnable(GL_BLEND);
	if (atestOn)    glEnable(GL_ALPHA_TEST);

	glEnable(GL_BLEND);
	glEnable(GL_ALPHA_TEST);
}

int dclusterfunc(void *a, void *b) {
	int x, y, z, d = 2;
	unsigned char *A = (unsigned char *)a;
	unsigned char *B = (unsigned char *)b;

	x = (*((unsigned short *)&A[4])) - (*((unsigned short *)&B[4]));
	y = (*((unsigned short *)&A[6])) - (*((unsigned short *)&B[6]));
	z = (*((unsigned short *)&A[8])) - (*((unsigned short *)&B[8]));

	if (x*x+y*y+z*z<d*d) return 1; else return 0;

	return 1;
}

int vclusterfunc(void *a, void *b) {
	unsigned char *A = (unsigned char *)a;
	unsigned char *B = (unsigned char *)b;

	//TODO
	int k1 = (*((unsigned short *)&A[4]))*sz[1]*sz[2]+(*((unsigned short *)&A[6]))*sz[2]+(*((unsigned short *)&A[8]));
	int k2 = (*((unsigned short *)&B[4]))*sz[1]*sz[2]+(*((unsigned short *)&B[6]))*sz[2]+(*((unsigned short *)&B[8]));

	int i, j;
	char sum=0;

	A = &voxels[k1*16]; B = &voxels[k2*16];
	//END TODO

	for(i=0;i<nattribs;i++) {
		j = selC[i]*(A[i] - B[i]);
		sum += j*j;
	}

	if (sum<selC[nattribs]*selC[nattribs]) return 1; else return 0;
}

int _find(int x, int *s) {return(s[x]<=0?x:(s[x]=_find(s[x],s)));}

int cluster(void **items, int *clustered, int nitems, int (* similarityfunc)(void *, void *)) {
	int *s,i,j,k,l,m,n=nitems,u,*c=clustered,f1,f2;

	s = (int *)malloc(sizeof(int)*n);
	for(i=n;i>0;i--) s[i]=0;
	for(k=0;k<n;k++) for(j=0;j<n;j++) {
		f1 = _find(k,s); f2 = _find(j,s);
		if ((f1!=f2)&&(similarityfunc(items[k],items[j]))) {
			if(s[f2]<s[f1]) s[f1]=f2;
			else {if(s[f1]==s[f2]) s[f1]--;s[f2]=f1;}
		}
	}
	for(i=0;i<n;i++) c[i] = _find(i,s);
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

void display (void)
{
  int i, k, cnt, *clustered, m,j, itemsz, vg, vf, rg, rf, g, f;
  char echofps[120];
  float * m_spaceball, m1;
  static int  ncurvox[MAXQUERIES], sumcurvox;
  float t;
  unsigned char * dispvoxels;
  unsigned char **voxp;

  itemsz = NQUANTS*sizeof(unsigned char) + 3*sizeof(short);
  time1 = clock();

  if (doClustering==1) {
  	vcbOGLprint(0.3f,0.5f, 1.f, 0.f, 0.f, "clustering... Please wait!");
  	glutSwapBuffers();
  }

  spaceball.Update();
  m_spaceball = spaceball.GetMatrix();

  if (changed) {
  	buildColorBar();
  	if (doClustering) doClustering=1;
	sumcurvox = 0;
	for(i=0;i<nQueries;i++) {
		ncurvox[i] = 0;
		if (active[i]) sumcurvox += ncurvox[i] = makeCurVoxelist(i);
		for(j=0;useT[i]&&j<ncurvox[i];j++) {
			/* APPLY TRIANGLE FUNCTION FOR OPACITY */
			//TODO
			k = (*((unsigned short *)&curvoxels[i][j*10+4]))*sz[1]*sz[2]+(*((unsigned short *)&curvoxels[i][j*10+6]))*sz[2]+(*((unsigned short *)&curvoxels[i][j*10+8]));
			f = (int)voxels[k*itemsz+triAttrib[i]];
			g = (int)voxels[k*itemsz+6];
			//END TODO
			rf = slicerange[i][triAttrib[i]];
			rg = slicerange[i][6];
			vf = sliceid[i][triAttrib[i]];
			vg = sliceid[i][6];
			m1 = ((float)vg+(float)rg)/((float)rf);
			if((-m1*(((float)f)-((float)vf)+((float)rf))+(float)vg+(float)rg>(float)g) || (m1*(((float)f)-((float)vf)-((float)rf))+(float)vg+(float)rg>(float)g)) {
				sumcurvox--;
				memcpy(&curvoxels[i][j--*10], &curvoxels[i][--ncurvox[i]*10], 10);
			}
		}
	}
	time2 = clock();
	t = (float)(time2-time1)/CLOCKS_PER_SEC;
	printf("query done in %4.3f seconds, %.3f mil-vox/sec, %d voxs.\n",t, sumcurvox/t/1e6f, sumcurvox);

	if (doClustering==1) {

		time1 = clock();

		voxp = (unsigned char **)malloc(ncurvox[0]*sizeof(unsigned char *));
		clustered = (int *)malloc(ncurvox[0]*sizeof(int));
		for(i=0;i<ncurvox[0];i++) voxp[i]=&curvoxels[0][i*10];

		m = cluster((void **)voxp, clustered, ncurvox[0], vclusterfunc);

		for (i = 0; i < ncurvox[0]; i ++) curvoxels[0][i * 10] = (clustered[i]+1)*255/(m+1);

		time2 = clock();
		t = (float)(time2-time1)/CLOCKS_PER_SEC;
		printf("clustering done in %4.3f seconds, %.3f vox/sec, %d voxs, %d cluster(s).\n",t, sumcurvox/t, sumcurvox, m);
		doClustering=2;

	}
  }

  time1 = clock();

  dispvoxels = (unsigned char *) malloc(sumcurvox * 10);
  if (!dispvoxels) perror("malloc"), exit(1);
  for(i=0,cnt=0;i<nQueries;i++) {
	memcpy(&dispvoxels[cnt], curvoxels[i], ncurvox[i] * 10);
	cnt += ncurvox[i] * 10;
  }


  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glTranslated(view.center[0],view.center[1],view.center[2]);
  glMultMatrixf(spaceball.GetMatrix());
  glTranslated(-view.center[0],-view.center[1],-view.center[2]);

  glClear(GL_COLOR_BUFFER_BIT);
  vcbVolrSsplats(dispvoxels, sumcurvox, sz);
  glPopMatrix();

  time2 = clock();

  glActiveTexture(GL_TEXTURE0);
  glDisable(GL_TEXTURE_1D);
  glActiveTexture(GL_TEXTURE1);
  glDisable(GL_TEXTURE_2D);
  t = (float)CLOCKS_PER_SEC/(float)(time2-time1);
  if (t>10000.0) t=100.f;
  sprintf(echofps,"fps: %7.2f on %.7d voxels",t, sumcurvox);
  if (onscreen) vcbOGLprint(0.01f,0.01f, 1.f, 1.f, 0.f, echofps);
  for(i=0;onscreen&&(i<nQueries);i++) {
	sprintf(echofps,"%c", (active[i]?'*':' '));
	vcbOGLprint(0.01f,0.95f-i*.025f, 1.f, 1.f, (curQ==i?1.f:0.f), echofps);
	for(j=0;j<7;j++) {
		sprintf(echofps,"%.3d-%.3d%s",sliceid[i][j],slicerange[i][j],(j!=6?", ":""));
		vcbOGLprint(0.04f+0.10f*j,0.95f-i*.025f, (triAttrib[i]==j?0.f:1.f), 1.f, (curQ==i?1.f:0.f), echofps);
	}
	if (useT[i]) vcbOGLprint(0.04f+0.10f*7,0.95f-i*.025f, 1.f, 0.3f, (curQ==i?1.f:0.f), "TRI");

  }
  if (doClustering&&onscreen) vcbOGLprint(0.04f,0.925f, 1.f, 0.f, 0.f, "clustering activated");
  oglDrawImage(.8f, .08f, (GLubyte ***)colorBar, CWIDTH+10, 512, 0);

  glEnable(GL_TEXTURE_2D);
  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_1D);

  glutSwapBuffers();

  free(dispvoxels);
  changed = 0;
}


void cleanup (void)
{
  vcbVolrCloseS();
  free(voxels);
  free(tlut);
  vcbMntreeFree(root);
  printf("finished cleaning up ...\n");
  fflush(stdout);
}

void sparsekeys(unsigned char c, int x, int y)
{
	char o[300];
	char *jj;
	float fzin = 0.95f;
	float fzout = 1.05f;
	int rzoom = 2;
	int i,j,k,l;

	changed = 1;

	switch(c) {
	  case 'a':
		  proj.xmin *= fzin;
		  proj.xmax *= fzin;
		  proj.ymin *= fzin;
		  proj.ymax *= fzin;
		  proj.fov  *= fzin;
		  changed = 0;
		  reshape(iwinWidth,iwinHeight);
		  break;
	  case 'z':
		  proj.xmin *= fzout;
		  proj.xmax *= fzout;
		  proj.ymin *= fzout;
		  proj.ymax *= fzout;
		  proj.fov  *= fzout;
		  changed = 0;
		  reshape(iwinWidth,iwinHeight);
		  break;

	  /* controls for attribute 0 */
	  case '2': if (sliceid[curQ][0] < 255)    sliceid[curQ][0] ++;           break;
	  case 'w': if (sliceid[curQ][0] > 1)      sliceid[curQ][0] --;           break;
	  case 's': if (slicerange[curQ][0] < 128) slicerange[curQ][0] += rzoom;  break;
	  case 'x': if (slicerange[curQ][0] > 0)   slicerange[curQ][0] -= rzoom;  break;

	  /* controls for attribute 1 */
	  case '3': if (sliceid[curQ][1] < 255)    sliceid[curQ][1] ++;           break;
	  case 'e': if (sliceid[curQ][1] > 1)      sliceid[curQ][1] --;           break;
	  case 'd': if (slicerange[curQ][1] < 128) slicerange[curQ][1] += rzoom;  break;
	  case 'c': if (slicerange[curQ][1] > 0)   slicerange[curQ][1] -= rzoom;  break;

	  /* controls for attribute 2 */
	  case '4': if (sliceid[curQ][2] < 255)    sliceid[curQ][2] ++;           break;
	  case 'r': if (sliceid[curQ][2] > 1)      sliceid[curQ][2] --;           break;
	  case 'f': if (slicerange[curQ][2] < 128) slicerange[curQ][2] += rzoom;  break;
	  case 'v': if (slicerange[curQ][2] > 0)   slicerange[curQ][2] -= rzoom;  break;

	  /* controls for attribute 3 */
	  case '5': if (sliceid[curQ][3] < 255)    sliceid[curQ][3] ++;           break;
	  case 't': if (sliceid[curQ][3] > 1)      sliceid[curQ][3] --;           break;
	  case 'g': if (slicerange[curQ][3] < 128) slicerange[curQ][3] += rzoom;  break;
	  case 'b': if (slicerange[curQ][3] > 0)   slicerange[curQ][3] -= rzoom;  break;

	  /* controls for attribute 4 */
	  case '6': if (sliceid[curQ][4] < 255)    sliceid[curQ][4] ++;           break;
	  case 'y': if (sliceid[curQ][4] > 1)      sliceid[curQ][4] --;           break;
	  case 'h': if (slicerange[curQ][4] < 128) slicerange[curQ][4] += rzoom;  break;
	  case 'n': if (slicerange[curQ][4] > 0)   slicerange[curQ][4] -= rzoom;  break;

	  /* controls for attribute 5 */
	  case '7': if (sliceid[curQ][5] < 255)    sliceid[curQ][5] ++;           break;
	  case 'u': if (sliceid[curQ][5] > 1)      sliceid[curQ][5] --;           break;
	  case 'j': if (slicerange[curQ][5] < 128) slicerange[curQ][5] += rzoom;  break;
	  case 'm': if (slicerange[curQ][5] > 0)   slicerange[curQ][5] -= rzoom;  break;

	  /* controls for attribute 6 */
	  case '8': if (sliceid[curQ][6] < 255)    sliceid[curQ][6] ++;           break;
	  case 'i': if (sliceid[curQ][6] > 1)      sliceid[curQ][6] --;           break;
	  case 'k': if (slicerange[curQ][6] < 128) slicerange[curQ][6] += rzoom;  break;
	  case ',': if (slicerange[curQ][6] > 0)   slicerange[curQ][6] -= rzoom;  break;

	  case 'q':
		  exit(0);
		  break;
	  case '`':
	  	  curQ++;
		  if(curQ>=nQueries) curQ=0;
		  break;
	  case '+':
	  	  doClustering=0;
	  	  nQueries+=2; // attention, fall-through!
	  case '-':
	  	  nQueries--;
		  if (nQueries > MAXQUERIES) nQueries = MAXQUERIES;
		  if (nQueries < 1) nQueries = 1;
		  while (curQ >= nQueries) curQ--;
		  break;
	  case '*':
	  	  active[curQ] = 1 - active[curQ];
	  	  break;
	  case '1':
	  	  if(!doClustering&&nQueries==1) doClustering = 1; else doClustering = 0;
		  break;
	  case '[':
	  	  TRIANGLE_LOW -= .04;
	  case ']':
	  	  TRIANGLE_LOW += .02;
		  if (TRIANGLE_LOW>1.0f) TRIANGLE_LOW = 1.0f;
		  if (TRIANGLE_LOW<0.0f) TRIANGLE_LOW = 0.0f;
		  break;
	  case ';':
	  	  TRIANGLE_HIGH -= .04;
	  case 39: // '
	  	  TRIANGLE_HIGH += .02;
		  if (TRIANGLE_HIGH>1.0f) TRIANGLE_HIGH = 1.0f;
		  if (TRIANGLE_HIGH<0.0f) TRIANGLE_HIGH = 0.0f;
		  break;
	  case '.':
	  	  if(useT[curQ]) useT[curQ]=0; else useT[curQ]=1;
		  break;
	  case '/':
	  	  triAttrib[curQ]++;
		  if(triAttrib[curQ]>5) triAttrib[curQ]=0;
		  break;
	  case '0': /* save framebuffer */
	          changed = 0;
		  pixels = (unsigned char *) malloc(iwinWidth*iwinHeight*4*sizeof(unsigned char));
		  glReadPixels(0,0,iwinWidth,iwinHeight,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
		  sprintf(o,"pic%.4d.bmp", pic++);
		  vcbImgWriteBMP(o,pixels,4, iwinWidth, iwinHeight);
		  free(pixels);
		  break;
	  case '9': /* enter clustering query */
	  	  jj = (char *)malloc(40);
		  for(i=0;i<nattribs;i++) {
		  	printf("clustering on attribute %d? ", i);
			scanf("%s", jj);
			selC[i] = atoi(jj);
		  }
		  printf("clustering distance? ");
		  scanf("%s", jj);
		  selC[nattribs] = atoi(jj);
		  free(jj);
		  break;
	  case 'o':
	  	  onscreen = 1 - onscreen;
	  default:
		  break;
  }
}

void usage() {
	printf("Usage: mqr inputfile\n");
	exit(1);
}

void initColorBar() {
  int i,j,k;

  float *tmp;

  tlut = (float**)malloc(256 * sizeof(float*));
  for (i = 0; i < 256; i ++) {
	  tlut[i] = (float *) malloc(4 * sizeof(float));
	  tlut[i][3] = 255;
  }

  tmp = (float *)malloc(256*4*sizeof(float));
  vcbCustomizeColorTableFullColor(tmp, 0, 255);
  for(i=0;i<256;i++) tlut[i][0] = tmp[i*4], tlut[i][1] = tmp[i*4+1], tlut[i][2] = tmp[i*4+2];
  free(tmp);

  //for(i = 0; i < 256; i++) tlut[i][0] = 1.f, tlut[i][1] = 0.f, tlut[i][2] = 0.f;
  //vcbBuildColorTblUniform(tlut, 1.f, 0.f, 0.f, 0, 255);

  for(i=0;i<512;i++) for(j=0     ;j<CWIDTH;   j++) {for(k=0;k<3;k++) colorBar[i][j][k] = (GLubyte)(tlut[i/2][k]*255); colorBar[i][j][3] = 0;}
  for(i=0;i<512;i++) for(j=CWIDTH;j<CWIDTH+10;j++) {for(k=0;k<3;k++) colorBar[i][j][k] = (GLubyte)(tlut[i/2][k]*255); colorBar[i][j][3] = 255;}

  buildColorBar();
}

int main(int argc, char ** argv)
{
  int   ntexunits, i;

  load_data("datasets/mqr/fake.qts");

  //if(argc!=2) usage();
  //load_data(argv[1]);

  initColorBar();

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
  vcbEnableOGLlight(0.1f);
  vcbSetOGLlight(GL_LIGHT0, 0.3f, 1.0f, light0.lightpos[0], light0.lightpos[1], light0.lightpos[2], 40.f);

  glutKeyboardFunc(sparsekeys);

  atexit(cleanup);
  glGetIntegerv(GL_MAX_TEXTURE_UNITS, & ntexunits);
  printf("using ogl version %f with %d texture units\n", vcbGetOGLVersion(), ntexunits);

  vcbVolrInitS(voxtransf);

  changed = 1;

  glutMainLoop();

  return 0;
}
