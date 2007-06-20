/* See the import.pl script for potential modifications */
/* e_coshf.c -- Simple version of e_cosh.c.
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
static char rcsid[] = "$NetBSD: e_coshf.c,v 1.6f 1996/04/08 15:43:41 phil Exp $";
#endif

#include "SMath.h"
#include "math_private.h"

namespace streflop_libm {
#ifdef __STDC__
static const Simple huge = 1.0e30f;
static const Simple one = 1.0f, half=0.5f;
#else
static Simple one = 1.0f, half=0.5f, huge = 1.0e30f;
#endif

#ifdef __STDC__
	Simple __ieee754_coshf(Simple x)
#else
	Simple __ieee754_coshf(x)
	Simple x;
#endif
{
	Simple t,w;
	int32_t ix;

	GET_FLOAT_WORD(ix,x);
	ix &= 0x7fffffff;

    /* x is INF or NaN */
	if(ix>=0x7f800000) return x*x;

    /* |x| in [0,0.5f*ln2], return 1+expm1(|x|)^2/(2*exp(|x|)) */
	if(ix<0x3eb17218) {
	    t = __expm1f(fabsf(x));
	    w = one+t;
	    if (ix<0x24000000) return w;	/* cosh(tiny) = 1 */
	    return one+(t*t)/(w+w);
	}

    /* |x| in [0.5f*ln2,22], return (exp(|x|)+1/exp(|x|)/2; */
	if (ix < 0x41b00000) {
		t = __ieee754_expf(fabsf(x));
		return half*t+half/t;
	}

    /* |x| in [22, log(maxdouble)] return half*exp(|x|) */
	if (ix < 0x42b17180)  return half*__ieee754_expf(fabsf(x));

    /* |x| in [log(maxdouble), overflowthresold] */
	if (ix<=0x42b2d4fc) {
	    w = __ieee754_expf(half*fabsf(x));
	    t = half*w;
	    return t*w;
	}

    /* |x| > overflowthresold, cosh(x) overflow */
	return huge*huge;
}
}
