/* See the import.pl script for potential modifications */
/* s_expm1f.c -- Simple version of s_expm1.c.
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
static char rcsid[] = "$NetBSD: s_expm1f.c,v 1.5f 1995/05/10 20:47:11 jtc Exp $";
#endif

#include "SMath.h"
#include "math_private.h"

static const Simple huge = 1.0e+30f;
static const Simple tiny = 1.0e-30f;

namespace streflop_libm {
#ifdef __STDC__
static const Simple
#else
static Simple
#endif
one		= 1.0f,
o_threshold	= 8.8721679688e+01f,/* 0x42b17180 */
ln2_hi		= 6.9313812256e-01f,/* 0x3f317180 */
ln2_lo		= 9.0580006145e-06f,/* 0x3717f7d1 */
invln2		= 1.4426950216e+00f,/* 0x3fb8aa3b */
	/* scaled coefficients related to expm1 */
Q1  =  -3.3333335072e-02f, /* 0xbd088889 */
Q2  =   1.5873016091e-03f, /* 0x3ad00d01 */
Q3  =  -7.9365076090e-05f, /* 0xb8a670cd */
Q4  =   4.0082177293e-06f, /* 0x36867e54 */
Q5  =  -2.0109921195e-07f; /* 0xb457edbb */

#ifdef __STDC__
	Simple __expm1f(Simple x)
#else
	Simple __expm1f(x)
	Simple x;
#endif
{
	Simple y,hi,lo,c,t,e,hxs,hfx,r1;
	int32_t k,xsb;
	u_int32_t hx;

	GET_FLOAT_WORD(hx,x);
	xsb = hx&0x80000000;		/* sign bit of x */
	if(xsb==0) y=x; else y= -x;	/* y = |x| */
	hx &= 0x7fffffff;		/* high word of |x| */

    /* filter out huge and non-finite argument */
	if(hx >= 0x4195b844) {			/* if |x|>=27*ln2 */
	    if(hx >= 0x42b17218) {		/* if |x|>=88.721f... */
                if(hx>0x7f800000)
		    return x+x; 	 /* NaN */
		if(hx==0x7f800000)
		    return (xsb==0)? x:-Simple(1.0f);/* exp(+-inf)={inf,-1} */
	        if(x > o_threshold) return huge*huge; /* overflow */
	    }
	    if(xsb!=0) { /* x < -27*ln2, return -1.0f with inexact */
		if(x+tiny<(Simple)0.0f)	/* raise inexact */
		return tiny-one;	/* return -1 */
	    }
	}

    /* argument reduction */
	if(hx > 0x3eb17218) {		/* if  |x| > 0.5f ln2 */
	    if(hx < 0x3F851592) {	/* and |x| < 1.5f ln2 */
		if(xsb==0)
		    {hi = x - ln2_hi; lo =  ln2_lo;  k =  1;}
		else
		    {hi = x + ln2_hi; lo = -ln2_lo;  k = -1;}
	    } else {
		k  = invln2*x+((xsb==0)?(Simple)0.5f:(Simple)-0.5f);
		t  = k;
		hi = x - t*ln2_hi;	/* t*ln2_hi is exact here */
		lo = t*ln2_lo;
	    }
	    x  = hi - lo;
	    c  = (hi-x)-lo;
	}
	else if(hx < 0x33000000) {  	/* when |x|<2**-25, return x */
	    t = huge+x;	/* return x with inexact flags when x!=0 */
	    return x - (t-(huge+x));
	}
	else k = 0;

    /* x is now in primary range */
	hfx = (Simple)0.5f*x;
	hxs = x*hfx;
	r1 = one+hxs*(Q1+hxs*(Q2+hxs*(Q3+hxs*(Q4+hxs*Q5))));
	t  = (Simple)3.0f-r1*hfx;
	e  = hxs*((r1-t)/((Simple)6.0f - x*t));
	if(k==0) return x - (x*e-hxs);		/* c is 0 */
	else {
	    e  = (x*(e-c)-c);
	    e -= hxs;
	    if(k== -1) return (Simple)0.5f*(x-e)-(Simple)0.5f;
	    if(k==1) {
	       	if(x < (Simple)-0.25f) return -(Simple)2.0f*(e-(x+(Simple)0.5f));
	       	else 	      return  one+(Simple)2.0f*(x-e);
	    }
	    if (k <= -2 || k>56) {   /* suffice to return exp(x)-1 */
	        int32_t i;
	        y = one-(e-x);
		GET_FLOAT_WORD(i,y);
		SET_FLOAT_WORD(y,i+(k<<23));	/* add k to y's exponent */
	        return y-one;
	    }
	    t = one;
	    if(k<23) {
	        int32_t i;
	        SET_FLOAT_WORD(t,0x3f800000 - (0x1000000>>k)); /* t=1-2^-k */
	       	y = t-(e-x);
		GET_FLOAT_WORD(i,y);
		SET_FLOAT_WORD(y,i+(k<<23));	/* add k to y's exponent */
	   } else {
	        int32_t i;
		SET_FLOAT_WORD(t,((0x7f-k)<<23));	/* 2^-k */
	       	y = x-(e+t);
	       	y += one;
		GET_FLOAT_WORD(i,y);
		SET_FLOAT_WORD(y,i+(k<<23));	/* add k to y's exponent */
	    }
	}
	return y;
}
weak_alias (__expm1f, expm1f)
}
