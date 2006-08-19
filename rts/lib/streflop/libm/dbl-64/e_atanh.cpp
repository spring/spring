/* See the import.pl script for potential modifications */
/* @(#)e_atanh.c 5.1 93/09/24 */
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
static char rcsid[] = "$NetBSD: e_atanh.c,v 1.8 1995/05/10 20:44:55 jtc Exp $";
#endif

/* __ieee754_atanh(x)
 * Method :
 *    1.Reduced x to positive by atanh(-x) = -atanh(x)
 *    2.For x>=0.5
 *                  1              2x                          x
 *	atanh(x) = --- * log(1 + -------) = 0.5 * log1p(2 * --------)
 *                  2             1 - x                      1 - x
 *	
 * 	For x<0.5
 *	atanh(x) = 0.5*log1p(2x+2x*x/(1-x))
 *
 * Special cases:
 *	atanh(x) is NaN if |x| > 1 with signal;
 *	atanh(NaN) is that NaN with no signal;
 *	atanh(+-1) is +-INF with signal.
 *
 */

#include "math.h"
#include "math_private.h"

namespace streflop_libm {
#ifdef __STDC__
static const Double one = 1.0, huge = 1e300;
#else
static Double one = 1.0, huge = 1e300;
#endif

#ifdef __STDC__
static const Double zero = 0.0;
#else
static Double zero = 0.0;
#endif

#ifdef __STDC__
	Double __ieee754_atanh(Double x)
#else
	Double __ieee754_atanh(x)
	Double x;
#endif
{
	Double t;
	int32_t hx,ix;
	u_int32_t lx;
	EXTRACT_WORDS(hx,lx,x);
	ix = hx&0x7fffffff;
	if ((ix|((lx|(-lx))>>31))>0x3ff00000) /* |x|>1 */
	    return (x-x)/(x-x);
	if(ix==0x3ff00000) 
	    return x/zero;
	if(ix<0x3e300000&&(huge+x)>zero) return x;	/* x<2**-28 */
	SET_HIGH_WORD(x,ix);
	if(ix<0x3fe00000) {		/* x < 0.5 */
	    t = x+x;
	    t = 0.5*__log1p(t+t*x/(one-x));
	} else 
	    t = 0.5*__log1p((x+x)/(one-x));
	if(hx>=0) return t; else return -t;
}
}
