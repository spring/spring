/* See the import.pl script for potential modifications */
/* e_acoshl.c -- Extended version of e_acosh.c.
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

/* __ieee754_acoshl(x)
 * Method :
 *	Based on
 *		acoshl(x) = logl [ x + sqrtl(x*x-1) ]
 *	we have
 *		acoshl(x) := logl(x)+ln2,	if x is large; else
 *		acoshl(x) := logl(2x-1/(sqrtl(x*x-1)+x)) if x>2; else
 *		acoshl(x) := log1pl(t+sqrtl(2.0l*t+t*t)); where t=x-1.
 *
 * Special cases:
 *	acoshl(x) is NaN with signal if x<1.
 *	acoshl(NaN) is NaN without signal.
 */

#include "math.h"
#include "math_private.h"

namespace streflop_libm {
#ifdef __STDC__
static const Extended
#else
static Extended
#endif
one	= 1.0l,
ln2	= 6.931471805599453094287e-01l; /* 0x3FFE, 0xB17217F7, 0xD1CF79AC */

#ifdef __STDC__
	Extended __ieee754_acoshl(Extended x)
#else
	Extended __ieee754_acoshl(x)
	Extended x;
#endif
{
	Extended t;
	u_int32_t se,i0,i1;
	GET_LDOUBLE_WORDS(se,i0,i1,x);
	if(se<0x3fff || se & 0x8000) {	/* x < 1 */
	    return (x-x)/(x-x);
	} else if(se >=0x401d) {	/* x > 2**30 */
	    if(se >=0x7fff) {		/* x is inf of NaN */
	        return x+x;
	    } else
		return __ieee754_logl(x)+ln2;	/* acoshl(huge)=logl(2x) */
	} else if(((se-0x3fff)|i0|i1)==0) {
	    return 0.0l;			/* acosh(1) = 0 */
	} else if (se > 0x4000) {	/* 2**28 > x > 2 */
	    t=x*x;
	    return __ieee754_logl(2.0l*x-one/(x+__ieee754_sqrtl(t-one)));
	} else {			/* 1<x<2 */
	    t = x-one;
	    return __log1pl(t+__sqrtl(2.0l*t+t*t));
	}
}
}
