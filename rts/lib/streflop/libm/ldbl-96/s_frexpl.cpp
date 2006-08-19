/* See the import.pl script for potential modifications */
/* s_frexpl.c -- Extended version of s_frexp.c.
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

/*
 * for non-zero x
 *	x = frexpl(arg,&exp);
 * return a Extended fp quantity x such that 0.5l <= |x| <1.0l
 * and the corresponding binary exponent "exp". That is
 *	arg = x*2^exp.
 * If arg is inf, 0.0l, or NaN, then frexpl(arg,&exp) returns arg
 * with *exp=0.
 */

#include "../streflop_libm_bridge.h"
#include "math.h"
#include "math_private.h"

namespace streflop_libm {
#ifdef __STDC__
static const Extended
#else
static Extended
#endif
#if LDBL_MANT_DIG == 64
two65 =  3.68934881474191032320e+19l; /* 0x4040, 0x80000000, 0x00000000 */
#else
# error "Cannot handle this MANT_DIG"
#endif


#ifdef __STDC__
	Extended __frexpl(Extended x, int *eptr)
#else
	Extended __frexpl(x, eptr)
	Extended x; int *eptr;
#endif
{
	u_int32_t se, hx, ix, lx;
	GET_LDOUBLE_WORDS(se,hx,lx,x);
	ix = 0x7fff&se;
	*eptr = 0;
	if(ix==0x7fff||((ix|hx|lx)==0)) return x;	/* 0,inf,nan */
	if (ix==0x0000) {		/* subnormal */
	    x *= two65;
	    GET_LDOUBLE_EXP(se,x);
	    ix = se&0x7fff;
	    *eptr = -65;
	}
	*eptr += ix-16382;
	se = (se & 0x8000) | 0x3ffe;
	SET_LDOUBLE_EXP(x,se);
	return x;
}
weak_alias (__frexpl, frexpl)
}
