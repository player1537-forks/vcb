#include "vcbpbuffer.h"
#include <GL/glut.h>
#include "vcbimage.h"
#include <stdlib.h>
#include <stdio.h>

int main (int argc, char ** argv)
{
	VCBpbuffer vcbp;
	unsigned char * pixels;
	GLenum err;

	int w = 512, h = 512;

#ifdef _WIN32
	/* no need to use glut to initialize pbuffer on linux */
	int winid;
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE| GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowSize(w,h);
	winid = glutCreateWindow(argv[0]);
#endif

	printf("initially \n");

#ifdef _WIN32
    err = glewInit();
    if (GLEW_OK != err) {
		fprintf(stderr,"glew error, initialization failed\n");
        fprintf(stderr,"Glew Error: %s\n", glewGetErrorString(err)); 
    }
#endif

	vcbp =  vcbNewPbuffer(w,h,GLUT_RGBA|GLUT_DOUBLE|GLUT_DEPTH);

	if (vcbp == NULL) {
		fprintf(stderr, "failed to create pbuffer, quitting \n");
		return 0;
	}

	vcbPbufferMakeCurrent(vcbp);

	/* a simple ogl text code to generate a uniform color image */
	glClearColor(0.0f, 0.0f, 1.0f, 1.0f); 
	glClearDepth(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	pixels = (unsigned char *) malloc(w*h*4*sizeof(unsigned char));
	glReadPixels(0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
	
	vcbImgWriteTGA("pbufres.tga",pixels,4, w, h);
	fprintf( stderr, "Wrote pbuffer to pbufres.tga\n" );

	free(pixels);
	vcbFreePbuffer(vcbp);

	return 1;
}
