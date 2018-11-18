/*
 * base64.h -- base64 encoding and decoding routines.
 *
 * Written by Huadong Liu, www.cs.utk.edu/~hliu/
 * @(#) $Id: base64.h,v 1.1 2005/09/20 20:33:44 hliu Exp $
 */

#ifndef BASE64_H_INCLUDED
#define BASE64_H_INCLUDED


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

void 
encode(unsigned char *in, int inlength, char *out);

void 
decode(char *in, unsigned char *out);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ifndef BASE64_H_INCLUDED */

