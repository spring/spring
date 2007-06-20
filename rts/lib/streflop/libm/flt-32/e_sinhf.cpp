/* See the import.pl script for potential modifications */
/* e_sinhf.c -- Simple version of e_sinh.c.
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
static char rcsid[] = "$NetBSD: e_sinhf.c,v 1.4f 1995/05/10 20:46:15 jtc Exp $";
#endif

#include "SMath.h"
#include "math_private.h"

namespace streflop_libm {
#ifdef __STDC__
static const Simple one = 1.0f, shuge = 1.0e37f;
#else
static Simple one = 1.0f, shuge = 1.0e37f;
#endif

#ifdef __STDC__
	Simple __ieee754_sinhf(Simple x)
#else
	Simple __ieee754_sinhf(x)
	Simple x;
#endif
{	
	Simple t,w,h;
	int32_t ix,jx;

	GET_FLOAT_WORD(jx,x);
	ix = jx&0x7fffffff;

    /* x is INF or NaN */
	if(ix>=0x7f800000) return x+x;	

	h = 0.5f;
	if (jx<0) h = -h;
    /* |x| in [0,22], return sign(x)*0.5f*(E+E/(E+1))) */
	if (ix < 0x41b00000) {		/* |x|<22 */
	    if (ix<0x31800000) 		/* |x|<2**-28 */
		if(shuge+x>one) return x;/* sinh(tiny) = tiny with inexact */
	    t = __expm1f(fabsf(x));
	    if(ix<0x3f800000) return h*((Simple)2.0f*t-t*t/(t+one));
	    return h*(t+t/(t+one));
	}

    /* |x| in [22, log(maxdouble)] return 0.5f*exp(|x|) */
	if (ix < 0x42b17180)  return h*__ieee754_expf(fabsf(x));

    /* |x| in [log(maxdouble), overflowthresold] */
	if (ix<=0x42b2d4fc) {
	    w = __ieee754_expf((Simple)0.5f*fabsf(x));
	    t = h*w;
	    return t*w;
	}

    /* |x| > overflowthresold, sinh(x) overflow */
	return x*shuge;
}
}
