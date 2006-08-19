/* See the import.pl script for potential modifications */
/* s_isnanl.c -- Extended version of s_isnan.c.
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
 * isnanl(x) returns 1 is x is nan, else 0;
 * no branching!
 */

#include "math.h"
#include "math_private.h"

namespace streflop_libm {
#ifdef __STDC__
	int __isnanl(Extended x)
#else
	int __isnanl(x)
	Extended x;
#endif
{
	int32_t se,hx,lx;
	GET_LDOUBLE_WORDS(se,hx,lx,x);
	se = (se & 0x7fff) << 1;
	lx |= hx & 0x7fffffff;
	se |= (u_int32_t)(lx|(-lx))>>31;
	se = 0xfffe - se;
	return (int)(((u_int32_t)(se))>>31);
}
hidden_def (__isnanl)
weak_alias (__isnanl, isnanl)
}
