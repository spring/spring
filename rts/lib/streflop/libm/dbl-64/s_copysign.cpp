/* See the import.pl script for potential modifications */
/* @(#)s_copysign.c 5.1 93/09/24 */
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
static char rcsid[] = "$NetBSD: s_copysign.c,v 1.8 1995/05/10 20:46:57 jtc Exp $";
#endif

/*
 * copysign(Double x, Double y)
 * copysign(x,y) returns a value with the magnitude of x and
 * with the sign bit of y.
 */

#include "math.h"
#include "math_private.h"

namespace streflop_libm {
#ifdef __STDC__
	Double __copysign(Double x, Double y)
#else
	Double __copysign(x,y)
	Double x,y;
#endif
{
	u_int32_t hx,hy;
	GET_HIGH_WORD(hx,x);
	GET_HIGH_WORD(hy,y);
	SET_HIGH_WORD(x,(hx&0x7fffffff)|(hy&0x80000000));
        return x;
}
weak_alias (__copysign, copysign)
#ifdef NO_LONG_DOUBLE
strong_alias (__copysign, __copysignl)
weak_alias (__copysign, copysignl)
#endif
}
