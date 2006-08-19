/* See the import.pl script for potential modifications */
/* s_tanhl.c -- Extended version of s_tanh.c.
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

/* tanhl(x)
 * Return the Hyperbolic Tangent of x
 *
 * Method :
 *				        x    -x
 *				       e  - e
 *	0. tanhl(x) is defined to be -----------
 *				        x    -x
 *				       e  + e
 *	1. reduce x to non-negative by tanhl(-x) = -tanhl(x).
 *	2.  0      <= x <= 2**-55 : tanhl(x) := x*(one+x)
 *					         -t
 *	    2**-55 <  x <=  1     : tanhl(x) := -----; t = expm1l(-2x)
 *					        t + 2
 *						      2
 *	    1      <= x <=  23.0l  : tanhl(x) := 1-  ----- ; t=expm1l(2x)
 *						    t + 2
 *	    23.0l   <  x <= INF    : tanhl(x) := 1.
 *
 * Special cases:
 *	tanhl(NaN) is NaN;
 *	only tanhl(0)=0 is exact for finite argument.
 */

#include "math.h"
#include "math_private.h"

namespace streflop_libm {
#ifdef __STDC__
static const Extended one=1.0l, two=2.0l, tiny = 1.0e-4900l;
#else
static Extended one=1.0l, two=2.0l, tiny = 1.0e-4900l;
#endif

#ifdef __STDC__
	Extended __tanhl(Extended x)
#else
	Extended __tanhl(x)
	Extended x;
#endif
{
	Extended t,z;
	int32_t se;
	u_int32_t j0,j1,ix;

    /* High word of |x|. */
	GET_LDOUBLE_WORDS(se,j0,j1,x);
	ix = se&0x7fff;

    /* x is INF or NaN */
	if(ix==0x7fff) {
	    /* for NaN it's not important which branch: tanhl(NaN) = NaN */
	    if (se&0x8000) return one/x-one;	/* tanhl(-inf)= -1; */
	    else           return one/x+one;	/* tanhl(+inf)=+1 */
	}

    /* |x| < 23 */
	if (ix < 0x4003 || (ix == 0x4003 && j0 < 0xb8000000u)) {/* |x|<23 */
	    if ((ix|j0|j1) == 0)
		return x;		/* x == +- 0 */
	    if (ix<0x3fc8)		/* |x|<2**-55 */
		return x*(one+tiny);	/* tanh(small) = small */
	    if (ix>=0x3fff) {	/* |x|>=1  */
		t = __expm1l(two*fabsl(x));
		z = one - two/(t+two);
	    } else {
	        t = __expm1l(-two*fabsl(x));
	        z= -t/(t+two);
	    }
    /* |x| > 23, return +-1 */
	} else {
	    z = one - tiny;		/* raised inexact flag */
	}
	return (se&0x8000)? -z: z;
}
weak_alias (__tanhl, tanhl)
}
