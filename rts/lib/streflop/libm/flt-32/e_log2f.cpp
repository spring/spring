/* See the import.pl script for potential modifications */
/* e_logf.c -- Simple version of e_log.c.
 * Conversion to Simple by Ian Lance Taylor, Cygnus Support, ian@cygnus.com.
 * adapted for log2 by Ulrich Drepper <drepper@cygnus.com>
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


#include "SMath.h"
#include "math_private.h"

namespace streflop_libm {
#ifdef __STDC__
static const Simple
#else
static Simple
#endif
ln2 = 0.69314718055994530942f,
two25 =    3.355443200e+07f,	/* 0x4c000000 */
Lg1 = 6.6666668653e-01f,	/* 3F2AAAAB */
Lg2 = 4.0000000596e-01f,	/* 3ECCCCCD */
Lg3 = 2.8571429849e-01f, /* 3E924925 */
Lg4 = 2.2222198546e-01f, /* 3E638E29 */
Lg5 = 1.8183572590e-01f, /* 3E3A3325 */
Lg6 = 1.5313838422e-01f, /* 3E1CD04F */
Lg7 = 1.4798198640e-01f; /* 3E178897 */

#ifdef __STDC__
static const Simple zero   =  0.0f;
#else
static Simple zero   =  0.0f;
#endif

#ifdef __STDC__
	Simple __ieee754_log2f(Simple x)
#else
	Simple __ieee754_log2f(x)
	Simple x;
#endif
{
	Simple hfsq,f,s,z,R,w,t1,t2,dk;
	int32_t k,ix,i,j;

	GET_FLOAT_WORD(ix,x);

	k=0;
	if (ix < 0x00800000) {			/* x < 2**-126  */
	    if ((ix&0x7fffffff)==0)
		return -two25/(x-x);		/* log(+-0)=-inf */
	    if (ix<0) return (x-x)/(x-x);	/* log(-#) = NaN */
	    k -= 25; x *= two25; /* subnormal number, scale up x */
	    GET_FLOAT_WORD(ix,x);
	}
	if (ix >= 0x7f800000) return x+x;
	k += (ix>>23)-127;
	ix &= 0x007fffff;
	i = (ix+(0x95f64<<3))&0x800000;
	SET_FLOAT_WORD(x,ix|(i^0x3f800000));	/* normalize x or x/2 */
	k += (i>>23);
	dk = (Simple)k;
	f = x-(Simple)1.0f;
	if((0x007fffff&(15+ix))<16) {	/* |f| < 2**-20 */
	    if(f==zero) return dk;
	    R = f*f*((Simple)0.5f-(Simple)0.33333333333333333f*f);
	    return dk-(R-f)/ln2;
	}
 	s = f/((Simple)2.0f+f);
	z = s*s;
	i = ix-(0x6147a<<3);
	w = z*z;
	j = (0x6b851<<3)-ix;
	t1= w*(Lg2+w*(Lg4+w*Lg6));
	t2= z*(Lg1+w*(Lg3+w*(Lg5+w*Lg7)));
	i |= j;
	R = t2+t1;
	if(i>0) {
	    hfsq=(Simple)0.5f*f*f;
	    return dk-((hfsq-(s*(hfsq+R)))-f)/ln2;
	} else {
	    return dk-((s*(f-R))-f)/ln2;
	}
}
}
