/* See the import.pl script for potential modifications */
/* e_atan2l.c -- Extended version of e_atan2.c.
 * Conversion to Extended by Ulrich Drepper,
 * Cygnus Support, drepper@cygnus.com.
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
static char rcsid[] = "$NetBSD: $";
#endif

/* __ieee754_atan2l(y,x)
 * Method :
 *	1. Reduce y to positive by atan2(y,x)=-atan2(-y,x).
 *	2. Reduce x to positive by (if x and y are unexceptional):
 *		ARG (x+iy) = arctan(y/x)   	   ... if x > 0,
 *		ARG (x+iy) = pi - arctan[y/(-x)]   ... if x < 0,
 *
 * Special cases:
 *
 *	ATAN2((anything), NaN ) is NaN;
 *	ATAN2(NAN , (anything) ) is NaN;
 *	ATAN2(+-0, +(anything but NaN)) is +-0  ;
 *	ATAN2(+-0, -(anything but NaN)) is +-pi ;
 *	ATAN2(+-(anything but 0 and NaN), 0) is +-pi/2;
 *	ATAN2(+-(anything but INF and NaN), +INF) is +-0 ;
 *	ATAN2(+-(anything but INF and NaN), -INF) is +-pi;
 *	ATAN2(+-INF,+INF ) is +-pi/4 ;
 *	ATAN2(+-INF,-INF ) is +-3pi/4;
 *	ATAN2(+-INF, (anything but,0,NaN, and INF)) is +-pi/2;
 *
 * Constants:
 * The hexadecimal values are the intended ones for the following
 * constants. The decimal values may be used, provided that the
 * compiler will convert from decimal to binary accurately enough
 * to produce the hexadecimal values shown.
 */

#include "math.h"
#include "math_private.h"

namespace streflop_libm {
#ifdef __STDC__
static const Extended
#else
static Extended
#endif
tiny  = 1.0e-4900l,
zero  = 0.0l,
pi_o_4  = 7.85398163397448309628202E-01l, /* 0x3FFE, 0xC90FDAA2, 0x2168C235 */
pi_o_2  = 1.5707963267948966192564E+00l,  /* 0x3FFF, 0xC90FDAA2, 0x2168C235 */
pi      = 3.14159265358979323851281E+00l, /* 0x4000, 0xC90FDAA2, 0x2168C235 */
pi_lo   = -5.01655761266833202345176e-20l;/* 0xBFBE, 0xECE675D1, 0xFC8F8CBB */

#ifdef __STDC__
	Extended __ieee754_atan2l(Extended y, Extended x)
#else
	Extended __ieee754_atan2l(y,x)
	Extended  y,x;
#endif
{
	Extended z;
	int32_t k,m,hx,hy,ix,iy;
	u_int32_t sx,sy,lx,ly;

	GET_LDOUBLE_WORDS(sx,hx,lx,x);
	ix = sx&0x7fff;
	lx |= hx & 0x7fffffff;
	GET_LDOUBLE_WORDS(sy,hy,ly,y);
	iy = sy&0x7fff;
	ly |= hy & 0x7fffffff;
	if(((2*ix|((lx|-lx)>>31))>0xfffe)||
	   ((2*iy|((ly|-ly)>>31))>0xfffe))	/* x or y is NaN */
	   return x+y;
	if(((sx-0x3fff)|lx)==0) return __atanl(y);   /* x=1.0l */
	m = ((sy>>15)&1)|((sx>>14)&2);	/* 2*sign(x)+sign(y) */

    /* when y = 0 */
	if((iy|ly)==0) {
	    switch(m) {
		case 0:
		case 1: return y; 	/* atan(+-0,+anything)=+-0 */
		case 2: return  pi+tiny;/* atan(+0,-anything) = pi */
		case 3: return -pi-tiny;/* atan(-0,-anything) =-pi */
	    }
	}
    /* when x = 0 */
	if((ix|lx)==0) return (sy>=0x8000)?  -pi_o_2-tiny: pi_o_2+tiny;

    /* when x is INF */
	if(ix==0x7fff) {
	    if(iy==0x7fff) {
		switch(m) {
		    case 0: return  pi_o_4+tiny;/* atan(+INF,+INF) */
		    case 1: return -pi_o_4-tiny;/* atan(-INF,+INF) */
		    case 2: return  3.0l*pi_o_4+tiny;/*atan(+INF,-INF)*/
		    case 3: return -3.0l*pi_o_4-tiny;/*atan(-INF,-INF)*/
		}
	    } else {
		switch(m) {
		    case 0: return  zero  ;	/* atan(+...,+INF) */
		    case 1: return -zero  ;	/* atan(-...,+INF) */
		    case 2: return  pi+tiny  ;	/* atan(+...,-INF) */
		    case 3: return -pi-tiny  ;	/* atan(-...,-INF) */
		}
	    }
	}
    /* when y is INF */
	if(iy==0x7fff) return (sy>=0x8000)? -pi_o_2-tiny: pi_o_2+tiny;

    /* compute y/x */
	k = sy-sx;
	if(k > 70) z=pi_o_2+0.5l*pi_lo; 	/* |y/x| >  2**70 */
	else if(sx>=0x8000&&k<-70) z=0.0l; 	/* |y|/x < -2**70 */
	else z=__atanl(fabsl(y/x));	/* safe to do y/x */
	switch (m) {
	    case 0: return       z  ;	/* atan(+,+) */
	    case 1: {
	    	      u_int32_t sz;
		      GET_LDOUBLE_EXP(sz,z);
		      SET_LDOUBLE_EXP(z,sz ^ 0x8000);
		    }
		    return       z  ;	/* atan(-,+) */
	    case 2: return  pi-(z-pi_lo);/* atan(+,-) */
	    default: /* case 3 */
	    	    return  (z-pi_lo)-pi;/* atan(-,-) */
	}
}
}
