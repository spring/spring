/* See the import.pl script for potential modifications */
/* s_asinhl.c -- Extended version of s_asinh.c.
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

/* asinhl(x)
 * Method :
 *	Based on
 *		asinhl(x) = signl(x) * logl [ |x| + sqrtl(x*x+1) ]
 *	we have
 *	asinhl(x) := x  if  1+x*x=1,
 *		  := signl(x)*(logl(x)+ln2)) for large |x|, else
 *		  := signl(x)*logl(2|x|+1/(|x|+sqrtl(x*x+1))) if|x|>2, else
 *		  := signl(x)*log1pl(|x| + x^2/(1 + sqrtl(1+x^2)))
 */

#include "math.h"
#include "math_private.h"

namespace streflop_libm {
#ifdef __STDC__
static const Extended
#else
static Extended
#endif
one =  1.000000000000000000000e+00l, /* 0x3FFF, 0x00000000, 0x00000000 */
ln2 =  6.931471805599453094287e-01l, /* 0x3FFE, 0xB17217F7, 0xD1CF79AC */
huge=  1.000000000000000000e+4900l;

#ifdef __STDC__
	Extended __asinhl(Extended x)
#else
	Extended __asinhl(x)
	Extended x;
#endif
{
	Extended t,w;
	int32_t hx,ix;
	GET_LDOUBLE_EXP(hx,x);
	ix = hx&0x7fff;
	if(ix==0x7fff) return x+x;	/* x is inf or NaN */
	if(ix< 0x3fde) {	/* |x|<2**-34 */
	    if(huge+x>one) return x;	/* return x inexact except 0 */
	}
	if(ix>0x4020) {		/* |x| > 2**34 */
	    w = __ieee754_logl(fabsl(x))+ln2;
	} else if (ix>0x4000) {	/* 2**34 > |x| > 2.0l */
	    t = fabsl(x);
	    w = __ieee754_logl(2.0l*t+one/(__ieee754_sqrtl(x*x+one)+t));
	} else {		/* 2.0l > |x| > 2**-28 */
	    t = x*x;
	    w =__log1pl(fabsl(x)+t/(one+__ieee754_sqrtl(one+t)));
	}
	if(hx&0x8000) return -w; else return w;
}
weak_alias (__asinhl, asinhl)
}
