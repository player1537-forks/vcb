/*
 * scene_xml2ascii.h -- structures for scene settting.
 *
 * Written by Huadong Liu, www.cs.utk.edu/~hliu/
 *
 * @(#) $Id: scene_xml2ascii.h,v 1.1 2005/09/21 15:42:49 hliu Exp $
 */

#ifndef SCENE_XML2ASCII_H_INCLUDED
#define SCENE_XML2ASCII_H_INCLUDED


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


#define MODEL_VIEW_MATRIX_SZ	4

typedef struct _triple {
	float x;
	float y;
	float z;
} tripleValue;

/**
 * Scene settings of viewing parameters. 
 */
typedef struct _viewing {
	int numLights;
	tripleValue *lights;
	tripleValue *halves;
	tripleValue *lightVec;
	float *ambient;
	float *diffuse;
	float *specular;
	float modelViewMatrix[MODEL_VIEW_MATRIX_SZ * MODEL_VIEW_MATRIX_SZ];
	/* viewing transformation. */
	tripleValue eye;
	tripleValue center;
	tripleValue up;
	/* projection transformation. */
	int projMode;
	float near;
	float far;
	/* perspective projection. */
	float fov;
	float aspect;
	float hither;
	float yon;
	/* orthographic projection. */
	float left;
	float right;
	float bottom;
	float top;
	/* viewport transoformation. */
	int width;
	int height;
} sceneViewing;

/**
 * Scene settings for marching cubes. 
 */
typedef struct _mcube {
	float isoValue;
} sceneMcube;

/**
 * Scene settings for raycasting. 
 */
typedef struct _raycast {
	tripleValue scale;
	int colorScheme;
	int numIsoRange;
	float *posIsoRange;
	float *sizeIsoRange;
	float *opaIsoRange;
} sceneRaycast;

/**
 * Scene settings for streamline. 
 */
typedef struct _streamline {
	int numSeeds;
	tripleValue *seeds;
	float stepSize;
	int numVerts;
} sceneStreamline;


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ifndef SCENE_XML2ASCII_H_INCLUDED */

