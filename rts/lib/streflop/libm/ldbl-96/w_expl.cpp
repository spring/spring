/* See the import.pl script for potential modifications */
/* w_expl.c -- Extended version of w_exp.c.
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
 * wrapper expl(x)
 */

#include "math.h"
#include "math_private.h"

namespace streflop_libm {
#ifdef __STDC__
static const Extended
#else
static Extended
#endif
o_threshold=  1.135652340629414394949193107797076489134e4l,
  /* 0x400C, 0xB17217F7, 0xD1CF79AC */
u_threshold= -1.140019167866942050398521670162263001513e4l;
  /* 0x400C, 0xB220C447, 0x69C201E8 */

#ifdef __STDC__
	Extended __expl(Extended x)	/* wrapper exp */
#else
	Extended __expl(x)			/* wrapper exp */
	Extended x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_expl(x);
#else
	Extended z;
	z = __ieee754_expl(x);
	if(_LIB_VERSION == _IEEE_) return z;
	if(__finitel(x)) {
	    if(x>o_threshold)
	        return __kernel_standard(x,x,206); /* exp overflow */
	    else if(x<u_threshold)
	        return __kernel_standard(x,x,207); /* exp underflow */
	}
	return z;
#endif
}
weak_alias (__expl, expl)
}
