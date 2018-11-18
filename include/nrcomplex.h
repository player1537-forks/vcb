/****************************************************************************
 *   Copyright (C) 2004  by Jian Huang										*
 *	 seelab@cs.utk.edu														*
 *																			*
 *   This library is free software; you can redistribute it and/or			*
 *   modify it under the terms of the GNU Lesser General Public				*
 *   License as published by the Free Software Foundation; either			*
 *   version 2.1 of the License, or (at your option) any later version.		*
 *																			*
 *   This library is distributed in the hope that it will be useful,		*
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of			*
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU		*
 *   Lesser General Public License for more details.						*
 *																			*
 *   You should have received a copy of the GNU Lesser General Public		*
 *   License along with this library; if not, write to the					*
 *   Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,			*
 *   Boston, MA  02110-1301  USA											*
 ****************************************************************************/

#ifndef _NR_COMPLEX_H_
#define _NR_COMPLEX_H_

typedef struct FCOMPLEX {float r,i;} fcomplex;

fcomplex Cadd(fcomplex a, fcomplex b);

fcomplex Csub(fcomplex a, fcomplex b);

fcomplex Cmul(fcomplex a, fcomplex b);

fcomplex Complex(float re, float im);

fcomplex Conjg(fcomplex z);

fcomplex Cdiv(fcomplex a, fcomplex b);

float Cabs(fcomplex z);

fcomplex Csqrt(fcomplex z);

fcomplex RCmul(float x, fcomplex a);

#endif
