/* See the import.pl script for potential modifications */
/* s_atanf.c -- Simple version of s_atan.c.
 * Conversion to Simple by Ian Lance Taylor, Cygnus Support, ian@cygnus.com.
 */

/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice 
 * is preserved.
 * ====================================================
 */

#if defined(LIBM_SCCS) && !defined(lint)
static char rcsid[] = "$NetBSD: s_atanf.c,v 1.4f 1995/05/10 20:46:47 jtc Exp $";
#endif

#include "SMath.h"
#include "math_private.h"

namespace streflop_libm {
#ifdef __STDC__
static const Simple atanhi[] = {
#else
static Simple atanhi[] = {
#endif
  4.6364760399e-01f, /* atan(0.5f)hi 0x3eed6338 */
  7.8539812565e-01f, /* atan(1.0f)hi 0x3f490fda */
  9.8279368877e-01f, /* atan(1.5f)hi 0x3f7b985e */
  1.5707962513e+00f, /* atan(inf)hi 0x3fc90fda */
};

#ifdef __STDC__
static const Simple atanlo[] = {
#else
static Simple atanlo[] = {
#endif
  5.0121582440e-09f, /* atan(0.5f)lo 0x31ac3769 */
  3.7748947079e-08f, /* atan(1.0f)lo 0x33222168 */
  3.4473217170e-08f, /* atan(1.5f)lo 0x33140fb4 */
  7.5497894159e-08f, /* atan(inf)lo 0x33a22168 */
};

#ifdef __STDC__
static const Simple aT[] = {
#else
static Simple aT[] = {
#endif
  3.3333334327e-01f, /* 0x3eaaaaaa */
 -2.0000000298e-01f, /* 0xbe4ccccd */
  1.4285714924e-01f, /* 0x3e124925 */
 -1.1111110449e-01f, /* 0xbde38e38 */
  9.0908870101e-02f, /* 0x3dba2e6e */
 -7.6918758452e-02f, /* 0xbd9d8795 */
  6.6610731184e-02f, /* 0x3d886b35 */
 -5.8335702866e-02f, /* 0xbd6ef16b */
  4.9768779427e-02f, /* 0x3d4bda59 */
 -3.6531571299e-02f, /* 0xbd15a221 */
  1.6285819933e-02f, /* 0x3c8569d7 */
};

#ifdef __STDC__
	static const Simple 
#else
	static Simple 
#endif
one   = 1.0f,
huge   = 1.0e30f;

#ifdef __STDC__
	Simple __atanf(Simple x)
#else
	Simple __atanf(x)
	Simple x;
#endif
{
	Simple w,s1,s2,z;
	int32_t ix,hx,id;

	GET_FLOAT_WORD(hx,x);
	ix = hx&0x7fffffff;
	if(ix>=0x50800000) {	/* if |x| >= 2^34 */
	    if(ix>0x7f800000)
		return x+x;		/* NaN */
	    if(hx>0) return  atanhi[3]+atanlo[3];
	    else     return -atanhi[3]-atanlo[3];
	} if (ix < 0x3ee00000) {	/* |x| < 0.4375f */
	    if (ix < 0x31000000) {	/* |x| < 2^-29 */
		if(huge+x>one) return x;	/* raise inexact */
	    }
	    id = -1;
	} else {
	x = fabsf(x);
	if (ix < 0x3f980000) {		/* |x| < 1.1875f */
	    if (ix < 0x3f300000) {	/* 7/16 <=|x|<11/16 */
		id = 0; x = ((Simple)2.0f*x-one)/((Simple)2.0f+x); 
	    } else {			/* 11/16<=|x|< 19/16 */
		id = 1; x  = (x-one)/(x+one); 
	    }
	} else {
	    if (ix < 0x401c0000) {	/* |x| < 2.4375f */
		id = 2; x  = (x-(Simple)1.5f)/(one+(Simple)1.5f*x);
	    } else {			/* 2.4375f <= |x| < 2^66 */
		id = 3; x  = -(Simple)1.0f/x;
	    }
	}}
    /* end of argument reduction */
	z = x*x;
	w = z*z;
    /* break sum from i=0 to 10 aT[i]z**(i+1) into odd and even poly */
	s1 = z*(aT[0]+w*(aT[2]+w*(aT[4]+w*(aT[6]+w*(aT[8]+w*aT[10])))));
	s2 = w*(aT[1]+w*(aT[3]+w*(aT[5]+w*(aT[7]+w*aT[9]))));
	if (id<0) return x - x*(s1+s2);
	else {
	    z = atanhi[id] - ((x*(s1+s2) - atanlo[id]) - x);
	    return (hx<0)? -z:z;
	}
}
weak_alias (__atanf, atanf)
}
