/* See the import.pl script for potential modifications */
/* s_scalbnl.c -- Extended version of s_scalbn.c.
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
 * scalbnl (Extended x, int n)
 * scalbnl(x,n) returns x* 2**n  computed by  exponent
 * manipulation rather than by actually performing an
 * exponentiation or a multiplication.
 */

#include "math.h"
#include "math_private.h"

namespace streflop_libm {
#ifdef __STDC__
static const Extended
#else
static Extended
#endif
two64   =  1.8446744073709551616e19l,
twom64  =  5.421010862427522170037e-20l,
huge   = 1.0e+4900l,
tiny   = 1.0e-4900l;

#ifdef __STDC__
	Extended __scalbnl (Extended x, int n)
#else
	Extended __scalbnl (x,n)
	Extended x; int n;
#endif
{
	int32_t k,es,hx,lx;
	GET_LDOUBLE_WORDS(es,hx,lx,x);
        k = es&0x7fff;				/* extract exponent */
        if (k==0) {				/* 0 or subnormal x */
            if ((lx|(hx&0x7fffffff))==0) return x; /* +-0 */
	    x *= two64;
	    GET_LDOUBLE_EXP(hx,x);
	    k = (hx&0x7fff) - 64;
	    }
        if (k==0x7fff) return x+x;		/* NaN or Inf */
        k = k+n;
        if (n> 50000 || k > 0x7ffe)
	  return huge*__copysignl(huge,x); /* overflow  */
	if (n< -50000)
	  return tiny*__copysignl(tiny,x);
        if (k > 0) 				/* normal result */
	    {SET_LDOUBLE_EXP(x,(es&0x8000)|k); return x;}
        if (k <= -64)
	    return tiny*__copysignl(tiny,x); 	/*underflow*/
        k += 64;				/* subnormal result */
	SET_LDOUBLE_EXP(x,(es&0x8000)|k);
        return x*twom64;
}
weak_alias (__scalbnl, scalbnl)
}
