/* See the import.pl script for potential modifications */
/* k_cosf.c -- Simple version of k_cos.c
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
static char rcsid[] = "$NetBSD: k_cosf.c,v 1.4f 1995/05/10 20:46:23 jtc Exp $";
#endif

#include "SMath.h"
#include "math_private.h"

namespace streflop_libm {
#ifdef __STDC__
static const Simple 
#else
static Simple 
#endif
one =  1.0000000000e+00f, /* 0x3f800000 */
C1  =  4.1666667908e-02f, /* 0x3d2aaaab */
C2  = -1.3888889225e-03f, /* 0xbab60b61 */
C3  =  2.4801587642e-05f, /* 0x37d00d01 */
C4  = -2.7557314297e-07f, /* 0xb493f27c */
C5  =  2.0875723372e-09f, /* 0x310f74f6 */
C6  = -1.1359647598e-11f; /* 0xad47d74e */

#ifdef __STDC__
	Simple __kernel_cosf(Simple x, Simple y)
#else
	Simple __kernel_cosf(x, y)
	Simple x,y;
#endif
{
	Simple a,hz,z,r,qx;
	int32_t ix;
	GET_FLOAT_WORD(ix,x);
	ix &= 0x7fffffff;			/* ix = |x|'s high word*/
	if(ix<0x32000000) {			/* if x < 2**27 */
	    if(((int)x)==0) return one;		/* generate inexact */
	}
	z  = x*x;
	r  = z*(C1+z*(C2+z*(C3+z*(C4+z*(C5+z*C6)))));
	if(ix < 0x3e99999a) 			/* if |x| < 0.3f */ 
	    return one - ((Simple)0.5f*z - (z*r - x*y));
	else {
	    if(ix > 0x3f480000) {		/* x > 0.78125f */
		qx = (Simple)0.28125f;
	    } else {
	        SET_FLOAT_WORD(qx,ix-0x01000000);	/* x/4 */
	    }
	    hz = (Simple)0.5f*z-qx;
	    a  = one-qx;
	    return a - (hz - (z*r-x*y));
	}
}
}
