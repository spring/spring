/* See the import.pl script for potential modifications */
/* k_tanf.c -- Simple version of k_tan.c
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
static char rcsid[] = "$NetBSD: k_tanf.c,v 1.4f 1995/05/10 20:46:39 jtc Exp $";
#endif

#include "SMath.h"
#include "math_private.h"
namespace streflop_libm {
#ifdef __STDC__
static const Simple 
#else
static Simple 
#endif
one   =  1.0000000000e+00f, /* 0x3f800000 */
pio4  =  7.8539812565e-01f, /* 0x3f490fda */
pio4lo=  3.7748947079e-08f, /* 0x33222168 */
T[] =  {
  3.3333334327e-01f, /* 0x3eaaaaab */
  1.3333334029e-01f, /* 0x3e088889 */
  5.3968254477e-02f, /* 0x3d5d0dd1 */
  2.1869488060e-02f, /* 0x3cb327a4 */
  8.8632395491e-03f, /* 0x3c11371f */
  3.5920790397e-03f, /* 0x3b6b6916 */
  1.4562094584e-03f, /* 0x3abede48 */
  5.8804126456e-04f, /* 0x3a1a26c8 */
  2.4646313977e-04f, /* 0x398137b9 */
  7.8179444245e-05f, /* 0x38a3f445 */
  7.1407252108e-05f, /* 0x3895c07a */
 -1.8558637748e-05f, /* 0xb79bae5f */
  2.5907305826e-05f, /* 0x37d95384 */
};

#ifdef __STDC__
	Simple __kernel_tanf(Simple x, Simple y, int iy)
#else
	Simple __kernel_tanf(x, y, iy)
	Simple x,y; int iy;
#endif
{
	Simple z,r,v,w,s;
	int32_t ix,hx;
	GET_FLOAT_WORD(hx,x);
	ix = hx&0x7fffffff;	/* high word of |x| */
	if(ix<0x31800000)			/* x < 2**-28 */
	    {if((int)x==0) {			/* generate inexact */
		if((ix|(iy+1))==0) return one/fabsf(x);
		else return (iy==1)? x: -one/x;
	    }
	    }
	if(ix>=0x3f2ca140) { 			/* |x|>=0.6744f */
	    if(hx<0) {x = -x; y = -y;}
	    z = pio4-x;
	    w = pio4lo-y;
	    x = z+w; y = 0.0f;
	}
	z	=  x*x;
	w 	=  z*z;
    /* Break x^5*(T[1]+x^2*T[2]+...) into
     *	  x^5(T[1]+x^4*T[3]+...+x^20*T[11]) +
     *	  x^5(x^2*(T[2]+x^4*T[4]+...+x^22*[T12]))
     */
	r = T[1]+w*(T[3]+w*(T[5]+w*(T[7]+w*(T[9]+w*T[11]))));
	v = z*(T[2]+w*(T[4]+w*(T[6]+w*(T[8]+w*(T[10]+w*T[12])))));
	s = z*x;
	r = y + z*(s*(r+v)+y);
	r += T[0]*s;
	w = x+r;
	if(ix>=0x3f2ca140) {
	    v = (Simple)iy;
	    return (Simple)(1-((hx>>30)&2))*(v-(Simple)2.0f*(x-(w*w/(w+v)-r)));
	}
	if(iy==1) return w;
	else {		/* if allow error up to 2 ulp, 
			   simply return -1.0f/(x+r) here */
     /*  compute -1.0f/(x+r) accurately */
	    Simple a,t;
	    int32_t i;
	    z  = w;
	    GET_FLOAT_WORD(i,z);
	    SET_FLOAT_WORD(z,i&0xfffff000);
	    v  = r-(z - x); 	/* z+v = r+x */
	    t = a  = -(Simple)1.0f/w;	/* a = -1.0f/w */
	    GET_FLOAT_WORD(i,t);
	    SET_FLOAT_WORD(t,i&0xfffff000);
	    s  = (Simple)1.0f+t*z;
	    return t+a*(s+t*v);
	}
}
}
