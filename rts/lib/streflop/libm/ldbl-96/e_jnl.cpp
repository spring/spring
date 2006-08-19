/* See the import.pl script for potential modifications */
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

/* Modifications for Extended are
  Copyright (C) 2001 Stephen L. Moshier <moshier@na-net.ornl.gov>
  and are incorporated herein by permission of the author.  The author 
  reserves the right to distribute this material elsewhere under different
  copying permissions.  These modifications are distributed here under 
  the following terms:

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1l of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA */

/*
 * __ieee754_jn(n, x), __ieee754_yn(n, x)
 * floating point Bessel's function of the 1st and 2nd kind
 * of order n
 *
 * Special cases:
 *	y0(0)=y1(0)=yn(n,0) = -inf with overflow signal;
 *	y0(-ve)=y1(-ve)=yn(n,-ve) are NaN with invalid signal.
 * Note 2. About jn(n,x), yn(n,x)
 *	For n=0, j0(x) is called,
 *	for n=1, j1(x) is called,
 *	for n<x, forward recursion us used starting
 *	from values of j0(x) and j1(x).
 *	for n>x, a continued fraction approximation to
 *	j(n,x)/j(n-1,x) is evaluated and then backward
 *	recursion is used starting from a supposed value
 *	for j(n,x). The resulting value of j(0,x) is
 *	compared with the actual value to correct the
 *	supposed value of j(n,x).
 *
 *	yn(n,x) is similar in all respects, except
 *	that forward recursion is used for all
 *	values of n>1.
 *
 */

#include "math.h"
#include "math_private.h"

namespace streflop_libm {
#ifdef __STDC__
static const Extended
#else
static Extended
#endif
  invsqrtpi = 5.64189583547756286948079e-1l, two = 2.0e0l, one = 1.0e0l;

#ifdef __STDC__
static const Extended zero = 0.0l;
#else
static Extended zero = 0.0l;
#endif

#ifdef __STDC__
Extended
__ieee754_jnl (int n, Extended x)
#else
Extended
__ieee754_jnl (n, x)
     int n;
     Extended x;
#endif
{
  u_int32_t se, i0, i1;
  int32_t i, ix, sgn;
  Extended a, b, temp, di;
  Extended z, w;

  /* J(-n,x) = (-1)^n * J(n, x), J(n, -x) = (-1)^n * J(n, x)
   * Thus, J(-n,x) = J(n,-x)
   */

  GET_LDOUBLE_WORDS (se, i0, i1, x);
  ix = se & 0x7fff;

  /* if J(n,NaN) is NaN */
  if ((ix == 0x7fff) && ((i0 & 0x7fffffff) != 0))
    return x + x;
  if (n < 0)
    {
      n = -n;
      x = -x;
      se ^= 0x8000;
    }
  if (n == 0)
    return (__ieee754_j0l (x));
  if (n == 1)
    return (__ieee754_j1l (x));
  sgn = (n & 1) & (se >> 15);	/* even n -- 0, odd n -- sign(x) */
  x = fabsl (x);
  if ((ix | i0 | i1) == 0 || ix >= 0x7fff)	/* if x is 0 or inf */
    b = zero;
  else if ((Extended) n <= x)
    {
      /* Safe to use J(n+1,x)=2n/x *J(n,x)-J(n-1,x) */
      if (ix >= 0x412D)
	{			/* x > 2**302 */

	  /* ??? This might be a futile gesture.
	     If x exceeds X_TLOSS anyway, the wrapper function
	     will set the result to zero. */

	  /* (x >> n**2)
	   *      Jn(x) = cos(x-(2n+1)*pi/4)*sqrt(2/x*pi)
	   *      Yn(x) = sin(x-(2n+1)*pi/4)*sqrt(2/x*pi)
	   *      Let s=sin(x), c=cos(x),
	   *          xn=x-(2n+1)*pi/4, sqt2 = sqrt(2),then
	   *
	   *             n    sin(xn)*sqt2    cos(xn)*sqt2
	   *          ----------------------------------
	   *             0     s-c             c+s
	   *             1    -s-c            -c+s
	   *             2    -s+c            -c-s
	   *             3     s+c             c-s
	   */
	  Extended s;
	  Extended c;
	  __sincosl (x, &s, &c);
	  switch (n & 3)
	    {
	    case 0:
	      temp = c + s;
	      break;
	    case 1:
	      temp = -c + s;
	      break;
	    case 2:
	      temp = -c - s;
	      break;
	    case 3:
	      temp = c - s;
	      break;
	    }
	  b = invsqrtpi * temp / __ieee754_sqrtl (x);
	}
      else
	{
	  a = __ieee754_j0l (x);
	  b = __ieee754_j1l (x);
	  for (i = 1; i < n; i++)
	    {
	      temp = b;
	      b = b * ((Extended) (i + i) / x) - a;	/* avoid underflow */
	      a = temp;
	    }
	}
    }
  else
    {
      if (ix < 0x3fde)
	{			/* x < 2**-33 */
	  /* x is tiny, return the first Taylor expansion of J(n,x)
	   * J(n,x) = 1/n!*(x/2)^n  - ...
	   */
	  if (n >= 400)		/* underflow, result < 10^-4952 */
	    b = zero;
	  else
	    {
	      temp = x * 0.5l;
	      b = temp;
	      for (a = one, i = 2; i <= n; i++)
		{
		  a *= (Extended) i;	/* a = n! */
		  b *= temp;	/* b = (x/2)^n */
		}
	      b = b / a;
	    }
	}
      else
	{
	  /* use backward recurrence */
	  /*                      x      x^2      x^2
	   *  J(n,x)/J(n-1,x) =  ----   ------   ------   .....
	   *                      2n  - 2(n+1) - 2(n+2)
	   *
	   *                      1      1        1
	   *  (for large x)   =  ----  ------   ------   .....
	   *                      2n   2(n+1)   2(n+2)
	   *                      -- - ------ - ------ -
	   *                       x     x         x
	   *
	   * Let w = 2n/x and h=2/x, then the above quotient
	   * is equal to the continued fraction:
	   *                  1
	   *      = -----------------------
	   *                     1
	   *         w - -----------------
	   *                        1
	   *              w+h - ---------
	   *                     w+2h - ...
	   *
	   * To determine how many terms needed, let
	   * Q(0) = w, Q(1) = w(w+h) - 1,
	   * Q(k) = (w+k*h)*Q(k-1) - Q(k-2),
	   * When Q(k) > 1e4      good for single
	   * When Q(k) > 1e9      good for Double
	   * When Q(k) > 1e17     good for quadruple
	   */
	  /* determine k */
	  Extended t, v;
	  Extended q0, q1, h, tmp;
	  int32_t k, m;
	  w = (n + n) / (Extended) x;
	  h = 2.0l / (Extended) x;
	  q0 = w;
	  z = w + h;
	  q1 = w * z - 1.0l;
	  k = 1;
	  while (q1 < 1.0e11l)
	    {
	      k += 1;
	      z += h;
	      tmp = z * q1 - q0;
	      q0 = q1;
	      q1 = tmp;
	    }
	  m = n + n;
	  for (t = zero, i = 2 * (n + k); i >= m; i -= 2)
	    t = one / (i / x - t);
	  a = t;
	  b = one;
	  /*  estimate log((2/x)^n*n!) = n*log(2/x)+n*ln(n)
	   *  Hence, if n*(log(2n/x)) > ...
	   *  single 8.8722839355e+01l
	   *  Double 7.09782712893383973096e+02l
	   *  Extended 1.1356523406294143949491931077970765006170e+04l
	   *  then recurrent value may overflow and the result is
	   *  likely underflow to zero
	   */
	  tmp = n;
	  v = two / x;
	  tmp = tmp * __ieee754_logl (fabsl (v * tmp));

	  if (tmp < 1.1356523406294143949491931077970765006170e+04l)
	    {
	      for (i = n - 1, di = (Extended) (i + i); i > 0; i--)
		{
		  temp = b;
		  b *= di;
		  b = b / x - a;
		  a = temp;
		  di -= two;
		}
	    }
	  else
	    {
	      for (i = n - 1, di = (Extended) (i + i); i > 0; i--)
		{
		  temp = b;
		  b *= di;
		  b = b / x - a;
		  a = temp;
		  di -= two;
		  /* scale b to avoid spurious overflow */
		  if (b > 1e100L)
		    {
		      a /= b;
		      t /= b;
		      b = one;
		    }
		}
	    }
	  b = (t * __ieee754_j0l (x) / b);
	}
    }
  if (sgn == 1)
    return -b;
  else
    return b;
}

#ifdef __STDC__
Extended
__ieee754_ynl (int n, Extended x)
#else
Extended
__ieee754_ynl (n, x)
     int n;
     Extended x;
#endif
{
  u_int32_t se, i0, i1;
  int32_t i, ix;
  int32_t sign;
  Extended a, b, temp;


  GET_LDOUBLE_WORDS (se, i0, i1, x);
  ix = se & 0x7fff;
  /* if Y(n,NaN) is NaN */
  if ((ix == 0x7fff) && ((i0 & 0x7fffffff) != 0))
    return x + x;
  if ((ix | i0 | i1) == 0)
    return -HUGE_VALL + x;  /* -inf and overflow exception.  */
  if (se & 0x8000)
    return zero / (zero * x);
  sign = 1;
  if (n < 0)
    {
      n = -n;
      sign = 1 - ((n & 1) << 1);
    }
  if (n == 0)
    return (__ieee754_y0l (x));
  if (n == 1)
    return (sign * __ieee754_y1l (x));
  if (ix == 0x7fff)
    return zero;
  if (ix >= 0x412D)
    {				/* x > 2**302 */

      /* ??? See comment above on the possible futility of this.  */

      /* (x >> n**2)
       *      Jn(x) = cos(x-(2n+1)*pi/4)*sqrt(2/x*pi)
       *      Yn(x) = sin(x-(2n+1)*pi/4)*sqrt(2/x*pi)
       *      Let s=sin(x), c=cos(x),
       *          xn=x-(2n+1)*pi/4, sqt2 = sqrt(2),then
       *
       *             n    sin(xn)*sqt2    cos(xn)*sqt2
       *          ----------------------------------
       *             0     s-c             c+s
       *             1    -s-c            -c+s
       *             2    -s+c            -c-s
       *             3     s+c             c-s
       */
      Extended s;
      Extended c;
      __sincosl (x, &s, &c);
      switch (n & 3)
	{
	case 0:
	  temp = s - c;
	  break;
	case 1:
	  temp = -s - c;
	  break;
	case 2:
	  temp = -s + c;
	  break;
	case 3:
	  temp = s + c;
	  break;
	}
      b = invsqrtpi * temp / __ieee754_sqrtl (x);
    }
  else
    {
      a = __ieee754_y0l (x);
      b = __ieee754_y1l (x);
      /* quit if b is -inf */
      GET_LDOUBLE_WORDS (se, i0, i1, b);
      for (i = 1; i < n && se != 0xffff; i++)
	{
	  temp = b;
	  b = ((Extended) (i + i) / x) * b - a;
	  GET_LDOUBLE_WORDS (se, i0, i1, b);
	  a = temp;
	}
    }
  if (sign > 0)
    return b;
  else
    return -b;
}
}
