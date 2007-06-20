/* See the import.pl script for potential modifications */
/* s_ceilf.c -- Simple version of s_ceil.c.
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
static char rcsid[] = "$NetBSD: s_ceilf.c,v 1.4f 1995/05/10 20:46:55 jtc Exp $";
#endif

#include "SMath.h"
#include "math_private.h"

namespace streflop_libm {
#ifdef __STDC__
static const Simple huge = 1.0e30f;
#else
static Simple huge = 1.0e30f;
#endif

#ifdef __STDC__
	Simple __ceilf(Simple x)
#else
	Simple __ceilf(x)
	Simple x;
#endif
{
	int32_t i0,j0;
	u_int32_t i;

	GET_FLOAT_WORD(i0,x);
	j0 = ((i0>>23)&0xff)-0x7f;
	if(j0<23) {
	    if(j0<0) { 	/* raise inexact if x != 0 */
		if(huge+x>(Simple)0.0f) {/* return 0*sign(x) if |x|<1 */
		    if(i0<0) {i0=0x80000000;} 
		    else if(i0!=0) { i0=0x3f800000;}
		}
	    } else {
		i = (0x007fffff)>>j0;
		if((i0&i)==0) return x; /* x is integral */
		if(huge+x>(Simple)0.0f) {	/* raise inexact flag */
		    if(i0>0) i0 += (0x00800000)>>j0;
		    i0 &= (~i);
		}
	    }
	} else {
	    if(j0==0x80) return x+x;	/* inf or NaN */
	    else return x;		/* x is integral */
	}
	SET_FLOAT_WORD(x,i0);
	return x;
}
weak_alias (__ceilf, ceilf)
}
