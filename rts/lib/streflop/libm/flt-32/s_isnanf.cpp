/* See the import.pl script for potential modifications */
/* s_isnanf.c -- Simple version of s_isnan.c.
 * Conversion to Simple by Ian Lance Taylor, Cygnus Support, ian@cygnus.com.
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
static char rcsid[] = "$NetBSD: s_isnanf.c,v 1.4f 1995/05/10 20:47:38 jtc Exp $";
#endif

/*
 * isnanf(x) returns 1 is x is nan, else 0;
 * no branching!
 */

#include "SMath.h"
#include "math_private.h"

namespace streflop_libm {
#ifdef __STDC__
	int __isnanf(Simple x)
#else
	int __isnanf(x)
	Simple x;
#endif
{
	int32_t ix;
	GET_FLOAT_WORD(ix,x);
	ix &= 0x7fffffff;
	ix = 0x7f800000 - ix;
	return (int)(((u_int32_t)(ix))>>31);
}
hidden_def (__isnanf)
weak_alias (__isnanf, isnanf)
}
