/* See the import.pl script for potential modifications */
/* s_finitel.c -- Extended version of s_finite.c.
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
 * finitel(x) returns 1 is x is finite, else 0;
 * no branching!
 */

#include "math.h"
#include "math_private.h"

namespace streflop_libm {
#ifdef __STDC__
	int __finitel(Extended x)
#else
	int __finitel(x)
	Extended x;
#endif
{
	int32_t exp;
	GET_LDOUBLE_EXP(exp,x);
	return (int)((u_int32_t)((exp&0x7fff)-0x7fff)>>31);
}
hidden_def (__finitel)
weak_alias (__finitel, finitel)
}
