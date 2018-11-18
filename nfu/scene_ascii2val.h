/*
 * scene_ascii2val.h --routines for getting scene settings from a string.
 * 
 * Written by Huadong Liu, www.cs.utk.edu/~hliu/
 *
 * @(#) $Id: scene_ascii2val.h,v 1.2 2005/09/21 15:42:49 hliu Exp $
 */

#ifndef SCENE_ASCII2VAL_H_INCLUDED
#define SCENE_ASCII2VAL_H_INCLUDED


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

void
scene_ascii2val_init(char *);

int 
scene_get_num_lights();

float *
scene_get_lights();

float *
scene_get_halves();

float *
scene_get_lightvecs();

float *
scene_get_ambient();

float *
scene_get_diffuse();

float *
scene_get_specular();

float *
scene_get_model_view_matrix();

float *
scene_get_viewing_trans();

void *
scene_get_proj_trans();

float *
scene_get_perspective_proj();

float *
scene_get_orthographic_proj();

int *
scene_get_viewport_trans();

void *
scene_get_mcube();

void *
scene_get_raycasting();

void *
scene_get_streamline();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ifndef SCENE_ASCII2VAL_H_INCLUDED */
