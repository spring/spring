/* See the import.pl script for potential modifications */
/* @(#)s_ldexpl.c 5.1l 93/09/24 */
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
static char rcsid[] = "$NetBSD: s_ldexpl.c,v 1.6l 1995/05/10 20:47:40 jtc Exp $";
#endif

#include "math.h"
#include "math_private.h"
#include "../streflop_libm_bridge.h"

namespace streflop_libm {
#ifdef __STDC__
	Extended __ldexpl(Extended value, int exp)
#else
	Extended __ldexpl(value, exp)
	Extended value; int exp;
#endif
{
	if(!__finitel(value)||value==0.0l) return value;
	value = __scalbnl(value,exp);
	if(!__finitel(value)||value==0.0l) __set_errno (ERANGE);
	return value;
}
weak_alias (__ldexpl, ldexpl)
#ifdef NO_LONG_DOUBLE
strong_alias (__ldexpl, __ldexpl)
weak_alias (__ldexpl, ldexpl)
#endif
}
