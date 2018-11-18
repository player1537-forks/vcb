/*
 * base64.c - base64 encoding and decoding
 *
 * Written by Felix Opatz <felix@zotteljedi.de>
 * Adapted by Huadong Liu, http://www.cs.utk.edu/~hliu/
 */

#include "rcs.h"

RCS_ID("@(#) $Id: base64.c,v 1.1.1.1 2005/09/19 03:36:37 hliu Exp $")
  
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define DECODED_CHUNK_SIZE	3
#define ENCODED_CHUNK_SIZE	4

unsigned char base64_table[] = \
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/** 
 * encodes a buffer into base64. 
 */
void encode(unsigned char *in, int inlength, char *out)
{
	int i = 0, j = 0;
	int loop=0, index=0;
	unsigned char chunk[DECODED_CHUNK_SIZE], echunk[ENCODED_CHUNK_SIZE];

	loop=0;
	while (loop < inlength)
	{
		chunk[i++] = in[loop++];
		if (i == DECODED_CHUNK_SIZE)
		{
			/* transforms 3 * 8 to 4 * 6 bit */
			echunk[0] = (chunk[0] & 0xfc) >> 2;
			echunk[1] = ((chunk[0] & 0x03) << 4) + ((chunk[1] & 0xf0) >> 4);
			echunk[2] = ((chunk[1] & 0x0f) << 2) + ((chunk[2] & 0xc0) >> 6);
			echunk[3] = chunk[2] & 0x3f;

			/* translates into base64 chars */
			for(i = 0; i < ENCODED_CHUNK_SIZE; i++)
				out[index++]=base64_table[echunk[i]];
			i = 0;
		}
	}

	/* if the number of characters were not a multiple of 3 */
	if (i)
	{
		/* fill the rest up with zeros */
		for(j = i; j < 3; j++)
			chunk[j] = '\0';

		/* same as above */
		echunk[0] = (chunk[0] & 0xfc) >> 2;
		echunk[1] = ((chunk[0] & 0x03) << 4) + ((chunk[1] & 0xf0) >> 4);
		echunk[2] = ((chunk[1] & 0x0f) << 2) + ((chunk[2] & 0xc0) >> 6);
		echunk[3] = chunk[2] & 0x3f;

		/* dito*/
		for (j = 0; j < i + 1; j++)
			out[index++]=base64_table[echunk[j]];

		/* pad to 4 chars with = */
		while(i++ < 3)
			out[index++]='=';
	}
}

/** 
 * translates a char from base64. 
 */
unsigned char decode_char(unsigned char c)
{
	unsigned char *p = base64_table, i = 0;

	while(*(p + i) != c)
		i++;

	return i;
}

/** 
 * checks whether the character is from the base64 alphabet. 
 */
int is_base64(unsigned char c)
{
	if (isalnum(c))
		return 1;

	if (c == '+')
		return 1;

	if (c == '/')
		return 1;

	return 0;
}

/** 
 * decodes from a base64 buffer. 
 */
void decode(char *in, unsigned char *out)
{
	int c, i = 0, j = 0;
	unsigned char chunk[ENCODED_CHUNK_SIZE], dchunk[DECODED_CHUNK_SIZE];
	int loop=0;
	int outlength=0;
	while ((c = in[loop++]) && (c != '=') && (is_base64(c) || isspace(c)))
	{
		/* ignore whitespaces */
		if (isspace(c))
			continue;

		chunk[i++] = c;
		if (i == ENCODED_CHUNK_SIZE)
		{
			/* translates from base64 into 6 bit chars */
			for (i = 0; i < ENCODED_CHUNK_SIZE; i++)
				chunk[i] = decode_char(chunk[i]);

			/* makes 3 * 8 from 4 * 6 bits */
			dchunk[0] = (chunk[0] << 2) + ((chunk[1] & 0x30) >> 4);
			dchunk[1] = ((chunk[1] & 0xf) << 4) + ((chunk[2] & 0x3c) >> 2);
			dchunk[2] = ((chunk[2] & 0x3) << 6) + chunk[3];

			/* write them to stdout */
			for (i = 0; i < DECODED_CHUNK_SIZE; i++){
				out[outlength++]=dchunk[i];
			}
			i = 0;
		}
	}

	/* if the number of characters were not a multiple of ENCODED_CHUNKSIZE */
	if (i)
	{
		/* fills rest up with zeros */
		for (j = i; j < ENCODED_CHUNK_SIZE; j++)
			chunk[j] = 0;

		/* same as above */
		for (j = 0; j < ENCODED_CHUNK_SIZE; j++)
			chunk[j] = decode_char(chunk[j]);

		/* dito */
		dchunk[0] = (chunk[0] << 2) + ((chunk[1] & 0x30) >> 4);
		dchunk[1] = ((chunk[1] & 0xf) << 4) + ((chunk[2] & 0x3c) >> 2);
		dchunk[2] = ((chunk[2] & 0x3) << 6) + chunk[3];

		/* puts it out */
		for (j = 0; j < i - 1; j++)
			out[outlength++]=dchunk[j];
	}
}
