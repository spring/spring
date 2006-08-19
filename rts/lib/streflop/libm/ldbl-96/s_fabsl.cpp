/* See the import.pl script for potential modifications */
/* s_fabsl.c -- Extended version of s_fabs.c.
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
 * fabsl(x) returns the absolute value of x.
 */

#include "math.h"
#include "math_private.h"

namespace streflop_libm {
#ifdef __STDC__
	Extended __fabsl(Extended x)
#else
	Extended __fabsl(x)
	Extended x;
#endif
{
	u_int32_t exp;
	GET_LDOUBLE_EXP(exp,x);
	SET_LDOUBLE_EXP(x,exp&0x7fff);
        return x;
}
weak_alias (__fabsl, fabsl)
}
