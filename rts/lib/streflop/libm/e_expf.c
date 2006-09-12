/* e_expf.c -- float version of e_exp.c.
 * Conversion to float by Ian Lance Taylor, Cygnus Support, ian@cygnus.com.
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
static char rcsid[] = "$NetBSD: e_expf.c,v 1.6 1996/04/08 15:43:43 phil Exp $";
#endif

#include "math.h"
#include "math_private.h"

static const float huge = 1.0e+30f;

#ifdef __STDC__
static const float
#else
static float
#endif
one	= 1.0f,
halF[2]	= {0.5f,-0.5f,},
twom100 = 7.8886090522e-31f,      /* 2**-100=0x0d800000 */
o_threshold=  8.8721679688e+01f,  /* 0x42b17180 */
u_threshold= -1.0397208405e+02f,  /* 0xc2cff1b5 */
ln2HI[2]   ={ 6.9313812256e-01f,		/* 0x3f317180 */
	     -6.9313812256e-01f,},	/* 0xbf317180 */
ln2LO[2]   ={ 9.0580006145e-06f,  	/* 0x3717f7d1 */
	     -9.0580006145e-06f,},	/* 0xb717f7d1 */
invln2 =  1.4426950216e+00f, 		/* 0x3fb8aa3b */
P1   =  1.6666667163e-01f, /* 0x3e2aaaab */
P2   = -2.7777778450e-03f, /* 0xbb360b61 */
P3   =  6.6137559770e-05f, /* 0x388ab355 */
P4   = -1.6533901999e-06f, /* 0xb5ddea0e */
P5   =  4.1381369442e-08f; /* 0x3331bb4c */

#ifdef __STDC__
	float __ieee754_expf(float x)	/* default IEEE double exp */
#else
	float __ieee754_expf(x)	/* default IEEE double exp */
	float x;
#endif
{
	float y,hi,lo,c,t;
	int32_t k,xsb;
	u_int32_t hx;

	GET_FLOAT_WORD(hx,x);
	xsb = (hx>>31)&1;		/* sign bit of x */
	hx &= 0x7fffffff;		/* high word of |x| */

    /* filter out non-finite argument */
	if(hx >= 0x42b17218) {			/* if |x|>=88.721... */
	    if(hx>0x7f800000)
		 return x+x;	 		/* NaN */
            if(hx==0x7f800000)
		return (xsb==0)? x:0.0f;		/* exp(+-inf)={inf,0} */
	    if(x > o_threshold) return huge*huge; /* overflow */
	    if(x < u_threshold) return twom100*twom100; /* underflow */
	}

    /* argument reduction */
	if(hx > 0x3eb17218) {		/* if  |x| > 0.5 ln2 */
	    if(hx < 0x3F851592) {	/* and |x| < 1.5 ln2 */
		hi = x-ln2HI[xsb]; lo=ln2LO[xsb]; k = 1-xsb-xsb;
	    } else {
		k  = invln2*x+halF[xsb];
		t  = k;
		hi = x - t*ln2HI[0];	/* t*ln2HI is exact here */
		lo = t*ln2LO[0];
	    }
	    x  = hi - lo;
	}
	else if(hx < 0x31800000)  {	/* when |x|<2**-28 */
	    if(huge+x>one) return one+x;/* trigger inexact */
	}
	else k = 0;

    /* x is now in primary range */
	t  = x*x;
	c  = x - t*(P1+t*(P2+t*(P3+t*(P4+t*P5))));
	if(k==0) 	return one-((x*c)/(c-(float)2.0f)-x);
	else 		y = one-((lo-(x*c)/((float)2.0f-c))-hi);
	if(k >= -125) {
	    u_int32_t hy;
	    GET_FLOAT_WORD(hy,y);
	    SET_FLOAT_WORD(y,hy+(k<<23));	/* add k to y's exponent */
	    return y;
	} else {
	    u_int32_t hy;
	    GET_FLOAT_WORD(hy,y);
	    SET_FLOAT_WORD(y,hy+((k+100)<<23));	/* add k to y's exponent */
	    return y*twom100;
	}
}
