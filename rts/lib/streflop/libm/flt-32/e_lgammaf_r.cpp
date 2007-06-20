/* See the import.pl script for potential modifications */
/* e_lgammaf_r.c -- Simple version of e_lgamma_r.c.
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
static char rcsid[] = "$NetBSD: e_lgammaf_r.c,v 1.3f 1995/05/10 20:45:47 jtc Exp $";
#endif

#include "SMath.h"
#include "math_private.h"

namespace streflop_libm {
#ifdef __STDC__
static const Simple
#else
static Simple
#endif
two23=  8.3886080000e+06f, /* 0x4b000000 */
half=  5.0000000000e-01f, /* 0x3f000000 */
one =  1.0000000000e+00f, /* 0x3f800000 */
pi  =  3.1415927410e+00f, /* 0x40490fdb */
a0  =  7.7215664089e-02f, /* 0x3d9e233f */
a1  =  3.2246702909e-01f, /* 0x3ea51a66 */
a2  =  6.7352302372e-02f, /* 0x3d89f001 */
a3  =  2.0580807701e-02f, /* 0x3ca89915 */
a4  =  7.3855509982e-03f, /* 0x3bf2027e */
a5  =  2.8905137442e-03f, /* 0x3b3d6ec6 */
a6  =  1.1927076848e-03f, /* 0x3a9c54a1 */
a7  =  5.1006977446e-04f, /* 0x3a05b634 */
a8  =  2.2086278477e-04f, /* 0x39679767 */
a9  =  1.0801156895e-04f, /* 0x38e28445 */
a10 =  2.5214456400e-05f, /* 0x37d383a2 */
a11 =  4.4864096708e-05f, /* 0x383c2c75 */
tc  =  1.4616321325e+00f, /* 0x3fbb16c3 */
tf  = -1.2148628384e-01f, /* 0xbdf8cdcd */
/* tt = -(tail of tf) */
tt  =  6.6971006518e-09f, /* 0x31e61c52 */
t0  =  4.8383611441e-01f, /* 0x3ef7b95e */
t1  = -1.4758771658e-01f, /* 0xbe17213c */
t2  =  6.4624942839e-02f, /* 0x3d845a15 */
t3  = -3.2788541168e-02f, /* 0xbd064d47 */
t4  =  1.7970675603e-02f, /* 0x3c93373d */
t5  = -1.0314224288e-02f, /* 0xbc28fcfe */
t6  =  6.1005386524e-03f, /* 0x3bc7e707 */
t7  = -3.6845202558e-03f, /* 0xbb7177fe */
t8  =  2.2596477065e-03f, /* 0x3b141699 */
t9  = -1.4034647029e-03f, /* 0xbab7f476 */
t10 =  8.8108185446e-04f, /* 0x3a66f867 */
t11 = -5.3859531181e-04f, /* 0xba0d3085 */
t12 =  3.1563205994e-04f, /* 0x39a57b6b */
t13 = -3.1275415677e-04f, /* 0xb9a3f927 */
t14 =  3.3552918467e-04f, /* 0x39afe9f7 */
u0  = -7.7215664089e-02f, /* 0xbd9e233f */
u1  =  6.3282704353e-01f, /* 0x3f2200f4 */
u2  =  1.4549225569e+00f, /* 0x3fba3ae7 */
u3  =  9.7771751881e-01f, /* 0x3f7a4bb2 */
u4  =  2.2896373272e-01f, /* 0x3e6a7578 */
u5  =  1.3381091878e-02f, /* 0x3c5b3c5e */
v1  =  2.4559779167e+00f, /* 0x401d2ebe */
v2  =  2.1284897327e+00f, /* 0x4008392d */
v3  =  7.6928514242e-01f, /* 0x3f44efdf */
v4  =  1.0422264785e-01f, /* 0x3dd572af */
v5  =  3.2170924824e-03f, /* 0x3b52d5db */
s0  = -7.7215664089e-02f, /* 0xbd9e233f */
s1  =  2.1498242021e-01f, /* 0x3e5c245a */
s2  =  3.2577878237e-01f, /* 0x3ea6cc7a */
s3  =  1.4635047317e-01f, /* 0x3e15dce6 */
s4  =  2.6642270386e-02f, /* 0x3cda40e4 */
s5  =  1.8402845599e-03f, /* 0x3af135b4 */
s6  =  3.1947532989e-05f, /* 0x3805ff67 */
r1  =  1.3920053244e+00f, /* 0x3fb22d3b */
r2  =  7.2193557024e-01f, /* 0x3f38d0c5 */
r3  =  1.7193385959e-01f, /* 0x3e300f6e */
r4  =  1.8645919859e-02f, /* 0x3c98bf54 */
r5  =  7.7794247773e-04f, /* 0x3a4beed6 */
r6  =  7.3266842264e-06f, /* 0x36f5d7bd */
w0  =  4.1893854737e-01f, /* 0x3ed67f1d */
w1  =  8.3333335817e-02f, /* 0x3daaaaab */
w2  = -2.7777778450e-03f, /* 0xbb360b61 */
w3  =  7.9365057172e-04f, /* 0x3a500cfd */
w4  = -5.9518753551e-04f, /* 0xba1c065c */
w5  =  8.3633989561e-04f, /* 0x3a5b3dd2 */
w6  = -1.6309292987e-03f; /* 0xbad5c4e8 */

#ifdef __STDC__
static const Simple zero=  0.0000000000e+00f;
#else
static Simple zero=  0.0000000000e+00f;
#endif

#ifdef __STDC__
	static Simple sin_pif(Simple x)
#else
	static Simple sin_pif(x)
	Simple x;
#endif
{
	Simple y,z;
	int n,ix;

	GET_FLOAT_WORD(ix,x);
	ix &= 0x7fffffff;

	if(ix<0x3e800000) return __kernel_sinf(pi*x,zero,0);
	y = -x;		/* x is assume negative */

    /*
     * argument reduction, make sure inexact flag not raised if input
     * is an integer
     */
	z = __floorf(y);
	if(z!=y) {				/* inexact anyway */
	    y  *= (Simple)0.5f;
	    y   = (Simple)2.0f*(y - __floorf(y));	/* y = |x| mod 2.0f */
	    n   = (int) (y*(Simple)4.0f);
	} else {
            if(ix>=0x4b800000) {
                y = zero; n = 0;                 /* y must be even */
            } else {
                if(ix<0x4b000000) z = y+two23;	/* exact */
		GET_FLOAT_WORD(n,z);
		n &= 1;
                y  = n;
                n<<= 2;
            }
        }
	switch (n) {
	    case 0:   y =  __kernel_sinf(pi*y,zero,0); break;
	    case 1:
	    case 2:   y =  __kernel_cosf(pi*((Simple)0.5f-y),zero); break;
	    case 3:
	    case 4:   y =  __kernel_sinf(pi*(one-y),zero,0); break;
	    case 5:
	    case 6:   y = -__kernel_cosf(pi*(y-(Simple)1.5f),zero); break;
	    default:  y =  __kernel_sinf(pi*(y-(Simple)2.0f),zero,0); break;
	    }
	return -y;
}


#ifdef __STDC__
	Simple __ieee754_lgammaf_r(Simple x, int *signgamp)
#else
	Simple __ieee754_lgammaf_r(x,signgamp)
	Simple x; int *signgamp;
#endif
{
	Simple t,y,z,nadj,p,p1,p2,p3,q,r,w;
	int i,hx,ix;

	GET_FLOAT_WORD(hx,x);

    /* purge off +-inf, NaN, +-0, and negative arguments */
	*signgamp = 1;
	ix = hx&0x7fffffff;
	if(ix>=0x7f800000) return x*x;
	if(ix==0) return one/fabsf(x);
	if(ix<0x1c800000) {	/* |x|<2**-70, return -log(|x|) */
	    if(hx<0) {
	        *signgamp = -1;
	        return -__ieee754_logf(-x);
	    } else return -__ieee754_logf(x);
	}
	if(hx<0) {
	    if(ix>=0x4b000000) 	/* |x|>=2**23, must be -integer */
		return x/zero;
	    t = sin_pif(x);
	    if(t==zero) return one/fabsf(t); /* -integer */
	    nadj = __ieee754_logf(pi/fabsf(t*x));
	    if(t<zero) *signgamp = -1;
	    x = -x;
	}

    /* purge off 1 and 2 */
	if (ix==0x3f800000||ix==0x40000000) r = 0;
    /* for x < 2.0f */
	else if(ix<0x40000000) {
	    if(ix<=0x3f666666) { 	/* lgamma(x) = lgamma(x+1)-log(x) */
		r = -__ieee754_logf(x);
		if(ix>=0x3f3b4a20) {y = one-x; i= 0;}
		else if(ix>=0x3e6d3308) {y= x-(tc-one); i=1;}
	  	else {y = x; i=2;}
	    } else {
	  	r = zero;
	        if(ix>=0x3fdda618) {y=(Simple)2.0f-x;i=0;} /* [1.7316f,2] */
	        else if(ix>=0x3F9da620) {y=x-tc;i=1;} /* [1.23f,1.73f] */
		else {y=x-one;i=2;}
	    }
	    switch(i) {
	      case 0:
		z = y*y;
		p1 = a0+z*(a2+z*(a4+z*(a6+z*(a8+z*a10))));
		p2 = z*(a1+z*(a3+z*(a5+z*(a7+z*(a9+z*a11)))));
		p  = y*p1+p2;
		r  += (p-(Simple)0.5f*y); break;
	      case 1:
		z = y*y;
		w = z*y;
		p1 = t0+w*(t3+w*(t6+w*(t9 +w*t12)));	/* parallel comp */
		p2 = t1+w*(t4+w*(t7+w*(t10+w*t13)));
		p3 = t2+w*(t5+w*(t8+w*(t11+w*t14)));
		p  = z*p1-(tt-w*(p2+y*p3));
		r += (tf + p); break;
	      case 2:
		p1 = y*(u0+y*(u1+y*(u2+y*(u3+y*(u4+y*u5)))));
		p2 = one+y*(v1+y*(v2+y*(v3+y*(v4+y*v5))));
		r += (-(Simple)0.5f*y + p1/p2);
	    }
	}
	else if(ix<0x41000000) { 			/* x < 8.0f */
	    i = (int)x;
	    t = zero;
	    y = x-(Simple)i;
	    p = y*(s0+y*(s1+y*(s2+y*(s3+y*(s4+y*(s5+y*s6))))));
	    q = one+y*(r1+y*(r2+y*(r3+y*(r4+y*(r5+y*r6)))));
	    r = half*y+p/q;
	    z = one;	/* lgamma(1+s) = log(s) + lgamma(s) */
	    switch(i) {
	    case 7: z *= (y+(Simple)6.0f);	/* FALLTHRU */
	    case 6: z *= (y+(Simple)5.0f);	/* FALLTHRU */
	    case 5: z *= (y+(Simple)4.0f);	/* FALLTHRU */
	    case 4: z *= (y+(Simple)3.0f);	/* FALLTHRU */
	    case 3: z *= (y+(Simple)2.0f);	/* FALLTHRU */
		    r += __ieee754_logf(z); break;
	    }
    /* 8.0f <= x < 2**58 */
	} else if (ix < 0x5c800000) {
	    t = __ieee754_logf(x);
	    z = one/x;
	    y = z*z;
	    w = w0+z*(w1+y*(w2+y*(w3+y*(w4+y*(w5+y*w6)))));
	    r = (x-half)*(t-one)+w;
	} else
    /* 2**58 <= x <= inf */
	    r =  x*(__ieee754_logf(x)-one);
	if(hx<0) r = nadj - r;
	return r;
}
}
