/* See the import.pl script for potential modifications */
/* s_copysignl.c -- Extended version of s_copysign.c.
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
 * copysignl(Extended x, Extended y)
 * copysignl(x,y) returns a value with the magnitude of x and
 * with the sign bit of y.
 */

#include "math.h"
#include "math_private.h"

namespace streflop_libm {
#ifdef __STDC__
	Extended __copysignl(Extended x, Extended y)
#else
	Extended __copysignl(x,y)
	Extended x,y;
#endif
{
	u_int32_t es1,es2;
	GET_LDOUBLE_EXP(es1,x);
	GET_LDOUBLE_EXP(es2,y);
	SET_LDOUBLE_EXP(x,(es1&0x7fff)|(es2&0x8000));
        return x;
}
weak_alias (__copysignl, copysignl)
}
