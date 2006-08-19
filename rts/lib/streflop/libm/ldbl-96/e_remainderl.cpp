/* See the import.pl script for potential modifications */
/* e_remainderl.c -- Extended version of e_remainder.c.
 * Conversion to Extended by Ulrich Drepper,
 * Cygnus Support, drepper@cygnus.com.
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
static char rcsid[] = "$NetBSD: $";
#endif

/* __ieee754_remainderl(x,p)
 * Return :
 * 	returns  x REM p  =  x - [x/p]*p as if in infinite
 * 	precise arithmetic, where [x/p] is the (infinite bit)
 *	integer nearest x/p (in half way case choose the even one).
 * Method :
 *	Based on fmod() return x-[x/p]chopped*p exactlp.
 */

#include "math.h"
#include "math_private.h"

namespace streflop_libm {
#ifdef __STDC__
static const Extended zero = 0.0l;
#else
static Extended zero = 0.0l;
#endif


#ifdef __STDC__
	Extended __ieee754_remainderl(Extended x, Extended p)
#else
	Extended __ieee754_remainderl(x,p)
	Extended x,p;
#endif
{
	u_int32_t sx,sex,sep,x0,x1,p0,p1;
	Extended p_half;

	GET_LDOUBLE_WORDS(sex,x0,x1,x);
	GET_LDOUBLE_WORDS(sep,p0,p1,p);
	sx = sex&0x8000;
	sep &= 0x7fff;
	sex &= 0x7fff;

    /* purge off exception values */
	if((sep|p0|p1)==0) return (x*p)/(x*p); 	/* p = 0 */
	if((sex==0x7fff)||			/* x not finite */
	  ((sep==0x7fff)&&			/* p is NaN */
	   ((p0|p1)!=0)))
	    return (x*p)/(x*p);


	if (sep<0x7ffe) x = __ieee754_fmodl(x,p+p);	/* now x < 2p */
	if (((sex-sep)|(x0-p0)|(x1-p1))==0) return zero*x;
	x  = fabsl(x);
	p  = fabsl(p);
	if (sep<0x0002) {
	    if(x+x>p) {
		x-=p;
		if(x+x>=p) x -= p;
	    }
	} else {
	    p_half = 0.5l*p;
	    if(x>p_half) {
		x-=p;
		if(x>=p_half) x -= p;
	    }
	}
	GET_LDOUBLE_EXP(sex,x);
	SET_LDOUBLE_EXP(x,sex^sx);
	return x;
}
}
