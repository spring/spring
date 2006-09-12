/* See the import.pl script for potential modifications */
/* e_jnf.c -- Simple version of e_jn.c.
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
static char rcsid[] = "$NetBSD: e_jnf.c,v 1.5f 1995/05/10 20:45:37 jtc Exp $";
#endif

#include "math.h"
#include "math_private.h"

namespace streflop_libm {
#ifdef __STDC__
static const Simple
#else
static Simple
#endif
two   =  2.0000000000e+00f, /* 0x40000000 */
one   =  1.0000000000e+00f; /* 0x3F800000 */

#ifdef __STDC__
static const Simple zero  =  0.0000000000e+00f;
#else
static Simple zero  =  0.0000000000e+00f;
#endif

#ifdef __STDC__
	Simple __ieee754_jnf(int n, Simple x)
#else
	Simple __ieee754_jnf(n,x)
	int n; Simple x;
#endif
{
	int32_t i,hx,ix, sgn;
	Simple a, b, temp, di;
	Simple z, w;

    /* J(-n,x) = (-1)^n * J(n, x), J(n, -x) = (-1)^n * J(n, x)
     * Thus, J(-n,x) = J(n,-x)
     */
	GET_FLOAT_WORD(hx,x);
	ix = 0x7fffffff&hx;
    /* if J(n,NaN) is NaN */
	if(ix>0x7f800000) return x+x;
	if(n<0){
		n = -n;
		x = -x;
		hx ^= 0x80000000;
	}
	if(n==0) return(__ieee754_j0f(x));
	if(n==1) return(__ieee754_j1f(x));
	sgn = (n&1)&(hx>>31);	/* even n -- 0, odd n -- sign(x) */
	x = fabsf(x);
	if(ix==0||ix>=0x7f800000)	/* if x is 0 or inf */
	    b = zero;
	else if((Simple)n<=x) {
		/* Safe to use J(n+1,x)=2n/x *J(n,x)-J(n-1,x) */
	    a = __ieee754_j0f(x);
	    b = __ieee754_j1f(x);
	    for(i=1;i<n;i++){
		temp = b;
		b = b*((Simple)(i+i)/x) - a; /* avoid underflow */
		a = temp;
	    }
	} else {
	    if(ix<0x30800000) {	/* x < 2**-29 */
    /* x is tiny, return the first Taylor expansion of J(n,x)
     * J(n,x) = 1/n!*(x/2)^n  - ...
     */
		if(n>33)	/* underflow */
		    b = zero;
		else {
		    temp = x*(Simple)0.5f; b = temp;
		    for (a=one,i=2;i<=n;i++) {
			a *= (Simple)i;		/* a = n! */
			b *= temp;		/* b = (x/2)^n */
		    }
		    b = b/a;
		}
	    } else {
		/* use backward recurrence */
		/*			x      x^2      x^2
		 *  J(n,x)/J(n-1,x) =  ----   ------   ------   .....
		 *			2n  - 2(n+1) - 2(n+2)
		 *
		 *			1      1        1
		 *  (for large x)   =  ----  ------   ------   .....
		 *			2n   2(n+1)   2(n+2)
		 *			-- - ------ - ------ -
		 *			 x     x         x
		 *
		 * Let w = 2n/x and h=2/x, then the above quotient
		 * is equal to the continued fraction:
		 *		    1
		 *	= -----------------------
		 *		       1
		 *	   w - -----------------
		 *			  1
		 *	        w+h - ---------
		 *		       w+2h - ...
		 *
		 * To determine how many terms needed, let
		 * Q(0) = w, Q(1) = w(w+h) - 1,
		 * Q(k) = (w+k*h)*Q(k-1) - Q(k-2),
		 * When Q(k) > 1e4	good for single
		 * When Q(k) > 1e9	good for Double
		 * When Q(k) > 1e17	good for quadruple
		 */
	    /* determine k */
		Simple t,v;
		Simple q0,q1,h,tmp; int32_t k,m;
		w  = (n+n)/(Simple)x; h = (Simple)2.0f/(Simple)x;
		q0 = w;  z = w+h; q1 = w*z - (Simple)1.0f; k=1;
		while(q1<(Simple)1.0e9f) {
			k += 1; z += h;
			tmp = z*q1 - q0;
			q0 = q1;
			q1 = tmp;
		}
		m = n+n;
		for(t=zero, i = 2*(n+k); i>=m; i -= 2) t = one/(i/x-t);
		a = t;
		b = one;
		/*  estimate log((2/x)^n*n!) = n*log(2/x)+n*ln(n)
		 *  Hence, if n*(log(2n/x)) > ...
		 *  single 8.8722839355e+01f
		 *  Double 7.09782712893383973096e+02f
		 *  Extended 1.1356523406294143949491931077970765006170e+04f
		 *  then recurrent value may overflow and the result is
		 *  likely underflow to zero
		 */
		tmp = n;
		v = two/x;
		tmp = tmp*__ieee754_logf(fabsf(v*tmp));
		if(tmp<(Simple)8.8721679688e+01f) {
		    for(i=n-1,di=(Simple)(i+i);i>0;i--){
		        temp = b;
			b *= di;
			b  = b/x - a;
		        a = temp;
			di -= two;
		    }
		} else {
		    for(i=n-1,di=(Simple)(i+i);i>0;i--){
		        temp = b;
			b *= di;
			b  = b/x - a;
		        a = temp;
			di -= two;
		    /* scale b to avoid spurious overflow */
			if(b>(Simple)1e10f) {
			    a /= b;
			    t /= b;
			    b  = one;
			}
		    }
		}
		b = (t*__ieee754_j0f(x)/b);
	    }
	}
	if(sgn==1) return -b; else return b;
}

#ifdef __STDC__
	Simple __ieee754_ynf(int n, Simple x)
#else
	Simple __ieee754_ynf(n,x)
	int n; Simple x;
#endif
{
	int32_t i,hx,ix;
	u_int32_t ib;
	int32_t sign;
	Simple a, b, temp;

	GET_FLOAT_WORD(hx,x);
	ix = 0x7fffffff&hx;
    /* if Y(n,NaN) is NaN */
	if(ix>0x7f800000) return x+x;
	if(ix==0) return -HUGE_VALF+x;  /* -inf and overflow exception.  */
	if(hx<0) return zero/(zero*x);
	sign = 1;
	if(n<0){
		n = -n;
		sign = 1 - ((n&1)<<1);
	}
	if(n==0) return(__ieee754_y0f(x));
	if(n==1) return(sign*__ieee754_y1f(x));
	if(ix==0x7f800000) return zero;

	a = __ieee754_y0f(x);
	b = __ieee754_y1f(x);
	/* quit if b is -inf */
	GET_FLOAT_WORD(ib,b);
	for(i=1;i<n&&ib!=0xff800000;i++){
	    temp = b;
	    b = ((Simple)(i+i)/x)*b - a;
	    GET_FLOAT_WORD(ib,b);
	    a = temp;
	}
	if(sign>0) return b; else return -b;
}
}
