/* See the import.pl script for potential modifications */
/* @(#)s_frexp.c 5.1 93/09/24 */
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
static char rcsid[] = "$NetBSD: s_frexp.c,v 1.9 1995/05/10 20:47:24 jtc Exp $";
#endif

/*
 * for non-zero x
 *	x = frexp(arg,&exp);
 * return a Double fp quantity x such that 0.5 <= |x| <1.0
 * and the corresponding binary exponent "exp". That is
 *	arg = x*2^exp.
 * If arg is inf, 0.0, or NaN, then frexp(arg,&exp) returns arg
 * with *exp=0.
 */

#include "math.h"
#include "math_private.h"

namespace streflop_libm {
#ifdef __STDC__
static const Double
#else
static Double
#endif
two54 =  1.80143985094819840000e+16; /* 0x43500000, 0x00000000 */

#ifdef __STDC__
	Double __frexp(Double x, int *eptr)
#else
	Double __frexp(x, eptr)
	Double x; int *eptr;
#endif
{
	int32_t hx, ix, lx;
	EXTRACT_WORDS(hx,lx,x);
	ix = 0x7fffffff&hx;
	*eptr = 0;
	if(ix>=0x7ff00000||((ix|lx)==0)) return x;	/* 0,inf,nan */
	if (ix<0x00100000) {		/* subnormal */
	    x *= two54;
	    GET_HIGH_WORD(hx,x);
	    ix = hx&0x7fffffff;
	    *eptr = -54;
	}
	*eptr += (ix>>20)-1022;
	hx = (hx&0x800fffff)|0x3fe00000;
	SET_HIGH_WORD(x,hx);
	return x;
}
weak_alias (__frexp, frexp)
#ifdef NO_LONG_DOUBLE
strong_alias (__frexp, __frexpl)
weak_alias (__frexp, frexpl)
#endif
}
