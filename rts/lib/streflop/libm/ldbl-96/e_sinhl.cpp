/* See the import.pl script for potential modifications */
/* e_asinhl.c -- Extended version of e_asinh.c.
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

/* __ieee754_sinhl(x)
 * Method :
 * mathematically sinh(x) if defined to be (exp(x)-exp(-x))/2
 *	1. Replace x by |x| (sinhl(-x) = -sinhl(x)).
 *	2.
 *		                                     E + E/(E+1)
 *	    0        <= x <= 25     :  sinhl(x) := --------------, E=expm1l(x)
 *			       			         2
 *
 *	    25       <= x <= lnovft :  sinhl(x) := expl(x)/2
 *	    lnovft   <= x <= ln2ovft:  sinhl(x) := expl(x/2)/2 * expl(x/2)
 *	    ln2ovft  <  x	    :  sinhl(x) := x*shuge (overflow)
 *
 * Special cases:
 *	sinhl(x) is |x| if x is +INF, -INF, or NaN.
 *	only sinhl(0)=0 is exact for finite x.
 */

#include "math.h"
#include "math_private.h"

namespace streflop_libm {
#ifdef __STDC__
static const Extended one = 1.0l, shuge = 1.0e4931l;
#else
static Extended one = 1.0l, shuge = 1.0e4931l;
#endif

#ifdef __STDC__
	Extended __ieee754_sinhl(Extended x)
#else
	Extended __ieee754_sinhl(x)
	Extended x;
#endif
{
	Extended t,w,h;
	u_int32_t jx,ix,i0,i1;

    /* Words of |x|. */
	GET_LDOUBLE_WORDS(jx,i0,i1,x);
	ix = jx&0x7fff;

    /* x is INF or NaN */
	if(ix==0x7fff) return x+x;

	h = 0.5l;
	if (jx & 0x8000) h = -h;
    /* |x| in [0,25], return sign(x)*0.5l*(E+E/(E+1))) */
	if (ix < 0x4003 || (ix == 0x4003 && i0 <= 0xc8000000)) { /* |x|<25 */
	    if (ix<0x3fdf) 		/* |x|<2**-32 */
		if(shuge+x>one) return x;/* sinh(tiny) = tiny with inexact */
	    t = __expm1l(fabsl(x));
	    if(ix<0x3fff) return h*(2.0l*t-t*t/(t+one));
	    return h*(t+t/(t+one));
	}

    /* |x| in [25, log(maxdouble)] return 0.5l*exp(|x|) */
	if (ix < 0x400c || (ix == 0x400c && i0 < 0xb17217f7))
		return h*__ieee754_expl(fabsl(x));

    /* |x| in [log(maxdouble), overflowthreshold] */
	if (ix<0x400c || (ix == 0x400c && (i0 < 0xb174ddc0
					   || (i0 == 0xb174ddc0
					       && i1 <= 0x31aec0ea)))) {
	    w = __ieee754_expl(0.5l*fabsl(x));
	    t = h*w;
	    return t*w;
	}

    /* |x| > overflowthreshold, sinhl(x) overflow */
	return x*shuge;
}
}
