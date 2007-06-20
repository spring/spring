/* See the import.pl script for potential modifications */
/* s_log1pf.c -- Simple version of s_log1p.c.
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
static char rcsid[] = "$NetBSD: s_log1pf.c,v 1.4f 1995/05/10 20:47:48 jtc Exp $";
#endif

#include "SMath.h"
#include "math_private.h"

namespace streflop_libm {
#ifdef __STDC__
static const Simple
#else
static Simple
#endif
ln2_hi =   6.9313812256e-01f,	/* 0x3f317180 */
ln2_lo =   9.0580006145e-06f,	/* 0x3717f7d1 */
two25 =    3.355443200e+07f,	/* 0x4c000000 */
Lp1 = 6.6666668653e-01f,	/* 3F2AAAAB */
Lp2 = 4.0000000596e-01f,	/* 3ECCCCCD */
Lp3 = 2.8571429849e-01f, /* 3E924925 */
Lp4 = 2.2222198546e-01f, /* 3E638E29 */
Lp5 = 1.8183572590e-01f, /* 3E3A3325 */
Lp6 = 1.5313838422e-01f, /* 3E1CD04F */
Lp7 = 1.4798198640e-01f; /* 3E178897 */

#ifdef __STDC__
static const Simple zero = 0.0f;
#else
static Simple zero = 0.0f;
#endif

#ifdef __STDC__
	Simple __log1pf(Simple x)
#else
	Simple __log1pf(x)
	Simple x;
#endif
{
	Simple hfsq,f,c,s,z,R,u;
	int32_t k,hx,hu,ax;

	GET_FLOAT_WORD(hx,x);
	ax = hx&0x7fffffff;

	k = 1;
	if (hx < 0x3ed413d7) {			/* x < 0.41422f  */
	    if(ax>=0x3f800000) {		/* x <= -1.0f */
		if(x==(Simple)-1.0f) return -two25/(x-x); /* log1p(-1)=+inf */
		else return (x-x)/(x-x);	/* log1p(x<-1)=NaN */
	    }
	    if(ax<0x31000000) {			/* |x| < 2**-29 */
		if(two25+x>zero			/* raise inexact */
	            &&ax<0x24800000) 		/* |x| < 2**-54 */
		    return x;
		else
		    return x - x*x*(Simple)0.5f;
	    }
	    if(hx>0||hx<=((int32_t)0xbe95f61f)) {
		k=0;f=x;hu=1;}	/* -0.2929f<x<0.41422f */
	}
	if (hx >= 0x7f800000) return x+x;
	if(k!=0) {
	    if(hx<0x5a000000) {
		u  = (Simple)1.0f+x;
		GET_FLOAT_WORD(hu,u);
	        k  = (hu>>23)-127;
		/* correction term */
	        c  = (k>0)? (Simple)Simple(1.0f)-(u-x):x-(u-(Simple)Simple(1.0f));
		c /= u;
	    } else {
		u  = x;
		GET_FLOAT_WORD(hu,u);
	        k  = (hu>>23)-127;
		c  = 0;
	    }
	    hu &= 0x007fffff;
	    if(hu<0x3504f7) {
	        SET_FLOAT_WORD(u,hu|0x3f800000);/* normalize u */
	    } else {
	        k += 1;
		SET_FLOAT_WORD(u,hu|0x3f000000);	/* normalize u/2 */
	        hu = (0x00800000-hu)>>2;
	    }
	    f = u-(Simple)1.0f;
	}
	hfsq=(Simple)0.5f*f*f;
	if(hu==0) {	/* |f| < 2**-20 */
	    if(f==zero) {
	    	if(k==0) return zero;
		else {c += k*ln2_lo; return k*ln2_hi+c;}
	    }
	    R = hfsq*((Simple)1.0f-(Simple)0.66666666666666666f*f);
	    if(k==0) return f-R; else
	    	     return k*ln2_hi-((R-(k*ln2_lo+c))-f);
	}
 	s = f/((Simple)2.0f+f);
	z = s*s;
	R = z*(Lp1+z*(Lp2+z*(Lp3+z*(Lp4+z*(Lp5+z*(Lp6+z*Lp7))))));
	if(k==0) return f-(hfsq-s*(hfsq+R)); else
		 return k*ln2_hi-((hfsq-(s*(hfsq+R)+(k*ln2_lo+c)))-f);
}
weak_alias (__log1pf, log1pf)
}
