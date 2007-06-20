/* See the import.pl script for potential modifications */
/* e_asinf.c -- Simple version of e_asin.c.
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

/*
  Modifications for single precision expansion are 
  Copyright (C) 2001 Stephen L. Moshier <moshier@na-net.ornl.gov>
  and are incorporated herein by permission of the author.  The author 
  reserves the right to distribute this material elsewhere under different
  copying permissions.  These modifications are distributed here under 
  the following terms:

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1f of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA */

#if defined(LIBM_SCCS) && !defined(lint)
static char rcsid[] = "$NetBSD: e_asinf.c,v 1.5f 1995/05/12 04:57:25 jtc Exp $";
#endif

#include "SMath.h"
#include "math_private.h"

namespace streflop_libm {
#ifdef __STDC__
static const Simple
#else
static Simple
#endif
one =  1.0000000000e+00f, /* 0x3F800000 */
huge =  1.000e+30f,

pio2_hi = 1.57079637050628662109375f,
pio2_lo = -4.37113900018624283e-8f,
pio4_hi = 0.785398185253143310546875f,

/* asin x = x + x^3 p(x^2)
   -0.5f <= x <= 0.5f;
   Peak relative error 4.8e-9f */
p0 = 1.666675248e-1f,
p1 = 7.495297643e-2f,
p2 = 4.547037598e-2f,
p3 = 2.417951451e-2f,
p4 = 4.216630880e-2f;

#ifdef __STDC__
	Simple __ieee754_asinf(Simple x)
#else
	Simple __ieee754_asinf(x)
	Simple x;
#endif
{
	Simple t,w,p,q,c,r,s;
	int32_t hx,ix;
	GET_FLOAT_WORD(hx,x);
	ix = hx&0x7fffffff;
	if(ix==0x3f800000) {
		/* asin(1)=+-pi/2 with inexact */
	    return x*pio2_hi+x*pio2_lo;
	} else if(ix> 0x3f800000) {	/* |x|>= 1 */
	    return (x-x)/(x-x);		/* asin(|x|>1) is NaN */
	} else if (ix<0x3f000000) {	/* |x|<0.5f */
	    if(ix<0x32000000) {		/* if |x| < 2**-27 */
		if(huge+x>one) return x;/* return x with inexact if x!=0*/
	    } else {
		t = x*x;
		w = t * (p0 + t * (p1 + t * (p2 + t * (p3 + t * p4))));
		return x+x*w;
	    }
	}
	/* 1> |x|>= 0.5f */
	w = one-fabsf(x);
	t = w*0.5f;
	p = t * (p0 + t * (p1 + t * (p2 + t * (p3 + t * p4))));
	s = __ieee754_sqrtf(t);
	if(ix>=0x3F79999A) { 	/* if |x| > 0.975f */
	    t = pio2_hi-(2.0f*(s+s*p)-pio2_lo);
	} else {
	    int32_t iw;
	    w  = s;
	    GET_FLOAT_WORD(iw,w);
	    SET_FLOAT_WORD(w,iw&0xfffff000);
	    c  = (t-w*w)/(s+w);
	    r  = p;
	    p  = 2.0f*s*r-(pio2_lo-2.0f*c);
	    q  = pio4_hi-2.0f*w;
	    t  = pio4_hi-(p-q);
	}
	if(hx>0) return t; else return -t;
}
}
