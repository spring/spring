/* See the import.pl script for potential modifications */
/* e_acoshf.c -- Simple version of e_acosh.c.
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
static char rcsid[] = "$NetBSD: e_acoshf.c,v 1.5f 1995/05/12 04:57:20 jtc Exp $";
#endif

#include "SMath.h"
#include "math_private.h"

namespace streflop_libm {
#ifdef __STDC__
static const Simple 
#else
static Simple 
#endif
one	= 1.0f,
ln2	= 6.9314718246e-01f;  /* 0x3f317218 */

#ifdef __STDC__
	Simple __ieee754_acoshf(Simple x)
#else
	Simple __ieee754_acoshf(x)
	Simple x;
#endif
{	
	Simple t;
	int32_t hx;
	GET_FLOAT_WORD(hx,x);
	if(hx<0x3f800000) {		/* x < 1 */
	    return (x-x)/(x-x);
	} else if(hx >=0x4d800000) {	/* x > 2**28 */
	    if(hx >=0x7f800000) {	/* x is inf of NaN */
	        return x+x;
	    } else 
		return __ieee754_logf(x)+ln2;	/* acosh(huge)=log(2x) */
	} else if (hx==0x3f800000) {
	    return 0.0f;			/* acosh(1) = 0 */
	} else if (hx > 0x40000000) {	/* 2**28 > x > 2 */
	    t=x*x;
	    return __ieee754_logf((Simple)2.0f*x-one/(x+__ieee754_sqrtf(t-one)));
	} else {			/* 1<x<2 */
	    t = x-one;
	    return __log1pf(t+__sqrtf((Simple)2.0f*t+t*t));
	}
}
}
