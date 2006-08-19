/* See the import.pl script for potential modifications */
/* s_logbl.c -- Extended version of s_logb.c.
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
 * Extended logbl(x)
 * IEEE 754 logb. Included to pass IEEE test suite. Not recommend.
 * Use ilogb instead.
 */

#include "math.h"
#include "math_private.h"

namespace streflop_libm {
#ifdef __STDC__
	Extended __logbl(Extended x)
#else
	Extended __logbl(x)
	Extended x;
#endif
{
	int32_t es,lx,ix;
	GET_LDOUBLE_WORDS(es,ix,lx,x);
	es &= 0x7fff;				/* exponent */
	if((es|ix|lx)==0) return -1.0l/fabsl(x);
	if(es==0x7fff) return x*x;
	if(es==0) 			/* IEEE 754 logb */
		return -16382.0l;
	else
		return (Extended) (es-0x3fff);
}
weak_alias (__logbl, logbl)
}
