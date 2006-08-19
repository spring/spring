/* See the import.pl script for potential modifications */
/* s_ilogbl.c -- Extended version of s_ilogb.c.
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

/* ilogbl(Extended x)
 * return the binary exponent of non-zero x
 * ilogbl(0) = FP_ILOGB0
 * ilogbl(NaN) = FP_ILOGBNAN (no signal is raised)
 * ilogbl(+-Inf) = INT_MAX (no signal is raised)
 */

#include "../streflop_libm_bridge.h"
#include "math.h"
#include "math_private.h"

namespace streflop_libm {
#ifdef __STDC__
	int __ilogbl(Extended x)
#else
	int __ilogbl(x)
	Extended x;
#endif
{
	int32_t es,hx,lx,ix;

	GET_LDOUBLE_EXP(es,x);
	es &= 0x7fff;
	if(es==0) {
	    GET_LDOUBLE_WORDS(es,hx,lx,x);
	    if((hx|lx)==0)
		return FP_ILOGB0;	/* ilogbl(0) = FP_ILOGB0 */
	    else			/* subnormal x */
		if(hx==0) {
		    for (ix = -16415; lx>0; lx<<=1) ix -=1;
		} else {
		    for (ix = -16383; hx>0; hx<<=1) ix -=1;
		}
	    return ix;
	}
	else if (es<0x7fff) return es-0x3fff;
	else if (FP_ILOGBNAN != INT_MAX)
	{
	    GET_LDOUBLE_WORDS(es,hx,lx,x);
	    if (((hx & 0x7fffffff)|lx) == 0)
	      /* ISO C99 requires ilogbl(+-Inf) == INT_MAX.  */
	      return INT_MAX;
	}
	return FP_ILOGBNAN;
}
weak_alias (__ilogbl, ilogbl)
}
