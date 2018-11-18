/*
 * scene_ascii2val.c -- functions for processing the scene setting buffer.
 *
 * Written by Huadong Liu, http://www.cs.utk.edu/~hliu/
 */

#include <string.h>
#include "rcs.h"
#include "base64.h"

RCS_ID("@(#) $Id: scene_ascii2val.c,v 1.2 2005/09/21 15:42:49 hliu Exp $")

#define TRIPLE_SZ 12
#define MCUBE_SZ  4
#define VIEWING_FIXED_LEN 156

void *scene_setting_buffer=0;

/**
 * Output overwrites input. Since base64 encoding increases length of the
 * string by 4/3, length of the decoded buffer will not exceed length of
 * the input buffer.
 */ 
void
scene_ascii2val_init(char *input)
{
	if (scene_setting_buffer) return;
	
	decode(input, input);
	scene_setting_buffer = (void *)input;
	return;
}

int
scene_get_num_lights()
{
	int num_lights;
	
	if (!scene_setting_buffer) return -1;

	memcpy(&num_lights, scene_setting_buffer, sizeof(int));
	return num_lights;
}

float *
scene_get_lights()
{
	if (!scene_setting_buffer) return 0;

	return (float *)(scene_setting_buffer+sizeof(int));
}

float *
scene_get_halves()
{
	int num_lights;
	int offset;
	
	if (!scene_setting_buffer) return 0;

	num_lights = scene_get_num_lights();
	offset = sizeof(int) + TRIPLE_SZ*num_lights;
	return (float *)(scene_setting_buffer+offset);
}

float *
scene_get_lightvecs()
{
	int num_lights;
	int offset;
	
	if (!scene_setting_buffer) return 0;

	num_lights = scene_get_num_lights();
	offset = sizeof(int) + 2*TRIPLE_SZ*num_lights;
	return (float *)(scene_setting_buffer+offset);
}

float *
scene_get_ambient()
{
	int num_lights;
	int offset;
	
	if (!scene_setting_buffer) return 0;

	num_lights = scene_get_num_lights();
	offset = sizeof(int) + 3*TRIPLE_SZ*num_lights;
	return (float *)(scene_setting_buffer+offset);
}

float *
scene_get_diffuse()
{
	int num_lights;
	int offset;

	if (!scene_setting_buffer) return 0;

	num_lights = scene_get_num_lights();
	offset = sizeof(int) + (3*TRIPLE_SZ+sizeof(float))*num_lights;
	return (float *)(scene_setting_buffer+offset);
}

float *
scene_get_specular()
{
	int num_lights;
	int offset;
	
	if (!scene_setting_buffer) return 0;

	num_lights = scene_get_num_lights();
	offset = sizeof(int) + (3*TRIPLE_SZ+2*sizeof(float))*num_lights;
	return (float *)(scene_setting_buffer+offset);
}

float *
scene_get_model_view_matrix()
{
	int num_lights;
	int offset;
	
	if (!scene_setting_buffer) return 0;

	num_lights = scene_get_num_lights();
	offset = sizeof(int) + 3*(TRIPLE_SZ+sizeof(float))*num_lights;
	return (float *)(scene_setting_buffer+offset);
}

float *
scene_get_viewing_trans()
{
	int num_lights;
	int offset;
	
	if (!scene_setting_buffer) return 0;

	num_lights = scene_get_num_lights();
	offset = sizeof(int) + 3*(TRIPLE_SZ+sizeof(float))*num_lights +
		sizeof(float)*4*4;
	return (float *)(scene_setting_buffer+offset);
}

void *
scene_get_proj_trans()
{
	int num_lights;
	int offset;
	
	if (!scene_setting_buffer) return 0;

	num_lights = scene_get_num_lights();
	offset = sizeof(int) + 3*(TRIPLE_SZ+sizeof(float))*num_lights +
		sizeof(float)*(4*4 + 3*3);
	return scene_setting_buffer+offset;
}

float *
scene_get_perspective_proj()
{
	int num_lights;
	int offset;
	
	if (!scene_setting_buffer) return 0;

	num_lights = scene_get_num_lights();
	offset = 2*sizeof(int) + 3*(TRIPLE_SZ+sizeof(float))*num_lights +
		sizeof(float)*(4*4 + 3*3 + 2);
	return scene_setting_buffer+offset;
}

float *
scene_get_orthographic_proj()
{
	int num_lights;
	int offset;
	
	if (!scene_setting_buffer) return 0;

	num_lights = scene_get_num_lights();
	offset = 2*sizeof(int) + 3*(TRIPLE_SZ+sizeof(float))*num_lights +
		sizeof(float)*(4*4 + 3*3 + 2 + 4);
	return scene_setting_buffer+offset;
}

int *
scene_get_viewport_trans()
{
	int num_lights;
	int offset;
	
	if (!scene_setting_buffer) return 0;

	num_lights = scene_get_num_lights();
	offset = 2*sizeof(int) + 3*(TRIPLE_SZ+sizeof(float))*num_lights +
		sizeof(float)*(4*4 + 3*3 + 2 + 4 + 4);
	return scene_setting_buffer+offset;
}

/**
 * The user need to interpret the returned buffer for mcube setting.
 */ 
void *
scene_get_mcube()
{
	int num_lights;
	int offset;
	int *ip;
	
	if (!scene_setting_buffer) return 0;

	num_lights = scene_get_num_lights();
	offset = 3*(TRIPLE_SZ+sizeof(float))*num_lights + 
		VIEWING_FIXED_LEN;
	ip = (int *)(scene_setting_buffer + offset);
	if (*ip)
		return (void *)(ip+1);
	else
		return 0;
}

/**
 * The user need to interpret the returned buffer for raycasting setting.
 */ 
void *
scene_get_raycasting()
{
	int num_lights;
	int offset;
	int *ip;

	if (!scene_setting_buffer) return 0;

	num_lights = scene_get_num_lights();
	offset = 3*(TRIPLE_SZ+sizeof(float))*num_lights + 
		VIEWING_FIXED_LEN;
	ip = (int *)(scene_setting_buffer + offset);
	offset += sizeof(int);
	if (*ip)
		offset += MCUBE_SZ;
	
	ip = (int *)(scene_setting_buffer + offset);
	if (*ip)
		return (void *)(ip+1);
	else
		return 0;
}

/**
 * The user need to interpret the returned buffer for streamline setting.
 */ 
void *
scene_get_streamline()
{
	int num_lights;
	int num_iso_range;
	int offset;
	int *ip;

	if (!scene_setting_buffer) return 0;
	
	num_lights = scene_get_num_lights();
	offset = 3*(TRIPLE_SZ+sizeof(float))*num_lights + 
		VIEWING_FIXED_LEN;
	/* check mcube. */
	ip = (int *)(scene_setting_buffer + offset);
	offset += sizeof(int);
	if (*ip)
		offset += MCUBE_SZ;

	/* check raycasting. */
	ip = (int *)(scene_setting_buffer + offset);
	offset += sizeof(int);
	if (*ip) {
		offset += TRIPLE_SZ;
		offset += sizeof(int);
		ip = (int *)(scene_setting_buffer + offset);
		offset += sizeof(int);
		num_iso_range = *ip;
		offset += num_iso_range * sizeof(float) * 3;
	}
	
	ip = (int *)(scene_setting_buffer + offset);
	if (*ip)
		return (void *)(ip+1);
	else
		return 0;
}
