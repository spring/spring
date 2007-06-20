/* See the import.pl script for potential modifications */
/* s_fabsf.c -- Simple version of s_fabs.c.
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
static char rcsid[] = "$NetBSD: s_fabsf.c,v 1.4f 1995/05/10 20:47:15 jtc Exp $";
#endif

/*
 * fabsf(x) returns the absolute value of x.
 */

#include "SMath.h"
#include "math_private.h"

namespace streflop_libm {
#ifdef __STDC__
	Simple __fabsf(Simple x)
#else
	Simple __fabsf(x)
	Simple x;
#endif
{
	u_int32_t ix;
	GET_FLOAT_WORD(ix,x);
	SET_FLOAT_WORD(x,ix&0x7fffffff);
        return x;
}
weak_alias (__fabsf, fabsf)
}
