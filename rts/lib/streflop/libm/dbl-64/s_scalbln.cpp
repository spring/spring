/* See the import.pl script for potential modifications */
/* @(#)s_scalbn.c 5.1 93/09/24 */
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
static char rcsid[] = "$NetBSD: s_scalbn.c,v 1.8 1995/05/10 20:48:08 jtc Exp $";
#endif

/*
 * scalbn (Double x, int n)
 * scalbn(x,n) returns x* 2**n  computed by  exponent
 * manipulation rather than by actually performing an
 * exponentiation or a multiplication.
 */

#include "math.h"
#include "math_private.h"

namespace streflop_libm {
#ifdef __STDC__
static const Double
#else
static Double
#endif
two54   =  1.80143985094819840000e+16, /* 0x43500000, 0x00000000 */
twom54  =  5.55111512312578270212e-17, /* 0x3C900000, 0x00000000 */
huge   = 1.0e+300,
tiny   = 1.0e-300;

#ifdef __STDC__
	Double __scalbln (Double x, long int n)
#else
	Double __scalbln (x,n)
	Double x; long int n;
#endif
{
	int32_t k,hx,lx;
	EXTRACT_WORDS(hx,lx,x);
        k = (hx&0x7ff00000)>>20;		/* extract exponent */
        if (k==0) {				/* 0 or subnormal x */
            if ((lx|(hx&0x7fffffff))==0) return x; /* +-0 */
	    x *= two54;
	    GET_HIGH_WORD(hx,x);
	    k = ((hx&0x7ff00000)>>20) - 54;
	    }
        if (k==0x7ff) return x+x;		/* NaN or Inf */
        k = k+n;
        if (n> 50000 || k >  0x7fe)
	  return huge*__copysign(huge,x); /* overflow  */
	if (n< -50000) return tiny*__copysign(tiny,x); /*underflow*/
        if (k > 0) 				/* normal result */
	    {SET_HIGH_WORD(x,(hx&0x800fffff)|(k<<20)); return x;}
        if (k <= -54)
	  return tiny*__copysign(tiny,x); 	/*underflow*/
        k += 54;				/* subnormal result */
	SET_HIGH_WORD(x,(hx&0x800fffff)|(k<<20));
        return x*twom54;
}
weak_alias (__scalbln, scalbln)
#ifdef NO_LONG_DOUBLE
strong_alias (__scalbln, __scalblnl)
weak_alias (__scalbln, scalblnl)
#endif
}
