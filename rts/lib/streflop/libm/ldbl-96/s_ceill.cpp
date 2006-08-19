/* See the import.pl script for potential modifications */
/* s_ceill.c -- Extended version of s_ceil.c.
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

/*
 * ceill(x)
 * Return x rounded toward -inf to integral value
 * Method:
 *	Bit twiddling.
 * Exception:
 *	Inexact flag raised if x not equal to ceil(x).
 */

#include "math.h"
#include "math_private.h"

namespace streflop_libm {
#ifdef __STDC__
static const Extended huge = 1.0e4930l;
#else
static Extended huge = 1.0e4930l;
#endif

#ifdef __STDC__
	Extended __ceill(Extended x)
#else
	Extended __ceill(x)
	Extended x;
#endif
{
	int32_t i1,j0;
	u_int32_t i,j,se,i0,sx;
	GET_LDOUBLE_WORDS(se,i0,i1,x);
	sx = (se>>15)&1;
	j0 = (se&0x7fff)-0x3fff;
	if(j0<31) {
	    if(j0<0) { 	/* raise inexact if x != 0 */
		if(huge+x>0.0l) {/* return 0*sign(x) if |x|<1 */
		    if(sx) {se=0x8000;i0=0;i1=0;}
		    else if((i0|i1)!=0) { se=0x3fff;i0=0;i1=0;}
		}
	    } else {
		i = (0x7fffffff)>>j0;
		if(((i0&i)|i1)==0) return x; /* x is integral */
		if(huge+x>0.0l) {	/* raise inexact flag */
		    if(sx==0) {
			if (j0>0 && (i0+(0x80000000>>j0))>i0)
			  i0+=0x80000000>>j0;
			else
			  {
			    i = 0x7fffffff;
			    ++se;
			  }
		    }
		    i0 &= (~i); i1=0;
		}
	    }
	} else if (j0>62) {
	    if(j0==0x4000) return x+x;	/* inf or NaN */
	    else return x;		/* x is integral */
	} else {
	    i = ((u_int32_t)(0xffffffff))>>(j0-31);
	    if((i1&i)==0) return x;	/* x is integral */
	    if(huge+x>0.0l) { 		/* raise inexact flag */
		if(sx==0) {
		    if(j0==31) i0+=1;
		    else {
			j = i1 + (1<<(63-j0));
			if(j<i1) i0+=1;	/* got a carry */
			i1 = j;
		    }
		}
		i1 &= (~i);
	    }
	}
	SET_LDOUBLE_WORDS(x,se,i0,i1);
	return x;
}
weak_alias (__ceill, ceill)
}
