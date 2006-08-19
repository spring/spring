/* See the import.pl script for potential modifications */
/* @(#)w_exp.c 5.1 93/09/24 */
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
static char rcsid[] = "$NetBSD: w_exp.c,v 1.6 1995/05/10 20:48:51 jtc Exp $";
#endif

/*
 * wrapper exp(x)
 */

#include "math.h"
#include "math_private.h"

namespace streflop_libm {
#ifdef __STDC__
static const Double
#else
static Double
#endif
o_threshold=  7.09782712893383973096e+02,  /* 0x40862E42, 0xFEFA39EF */
u_threshold= -7.45133219101941108420e+02;  /* 0xc0874910, 0xD52D3051 */

#ifdef __STDC__
	Double __exp(Double x)		/* wrapper exp */
#else
	Double __exp(x)			/* wrapper exp */
	Double x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_exp(x);
#else
	Double z;
	z = __ieee754_exp(x);
	if(_LIB_VERSION == _IEEE_) return z;
	if(__finite(x)) {
	    if(x>o_threshold)
	        return __kernel_standard(x,x,6); /* exp overflow */
	    else if(x<u_threshold)
	        return __kernel_standard(x,x,7); /* exp underflow */
	}
	return z;
#endif
}
weak_alias (__exp, exp)
#ifdef NO_LONG_DOUBLE
strong_alias (__exp, __expl)
weak_alias (__exp, expl)
#endif
}
