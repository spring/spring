/* See the import.pl script for potential modifications */
/* Single-precision floating point 2^x.
   Copyright (C) 1997,1998,2000,2001,2005,2006 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Geoffrey Keating <geoffk@ozemail.com.au>

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1f of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

/* The basic design here is from
   Shmuel Gal and Boris Bachelis, "An Accurate Elementary Mathematical
   Library for the IEEE Floating Point Standard", ACM Trans. Math. Soft.,
   17 (1), March 1991, pp. 26-45.
   It has been slightly modified to compute 2^x instead of e^x, and for
   single-precision.
   */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif
#include "../streflop_libm_bridge.h"
#include "../streflop_libm_bridge.h"
#include "ieee754.h"
#include "SMath.h"
#include "../streflop_libm_bridge.h"
#include "../streflop_libm_bridge.h"
#include "math_private.h"

#include "t_exp2f.h"

static const Simple TWOM100 = 7.88860905e-31f;
static const Simple TWO127 = 1.7014118346e+38f;

namespace streflop_libm {
Simple
__ieee754_exp2f (Simple x)
{
  static const Simple himark = (Simple) FLT_MAX_EXP;
  static const Simple lomark = (Simple) (FLT_MIN_EXP - FLT_MANT_DIG - 1);

  /* Check for usual case.  */
  if (isless (x, himark) && isgreaterequal (x, lomark))
    {
      static const Simple THREEp14 = 49152.0f;
      int tval, unsafe;
      Simple rx, x22, result;
      union ieee754_float ex2_u, scale_u;
      fpenv_t oldenv;

      feholdexcept (&oldenv);
#ifdef FE_TONEAREST
      /* If we don't have this, it's too bad.  */
      fesetround (FE_TONEAREST);
#endif

      /* 1. Argument reduction.
	 Choose integers ex, -128 <= t < 128, and some real
	 -1/512 <= x1 <= 1/512 so that
	 x = ex + t/512 + x1.

	 First, calculate rx = ex + t/256.  */
      rx = x + THREEp14;
      rx -= THREEp14;
      x -= rx;  /* Compute x=x1. */
      /* Compute tval = (ex*256 + t)+128.
	 Now, t = (tval mod 256)-128 and ex=tval/256  [that's mod, NOT %; and
	 /-round-to-nearest not the usual c integer /].  */
      tval = (int) (rx * 256.0f + 128.0f);

      /* 2. Adjust for accurate table entry.
	 Find e so that
	 x = ex + t/256 + e + x2
	 where -7e-4 < e < 7e-4, and
	 (Simple)(2^(t/256+e))
	 is accurate to one part in 2^-64.  */

      /* 'tval & 255' is the same as 'tval%256' except that it's always
	 positive.
	 Compute x = x2.  */
      x -= __exp2f_deltatable[tval & 255];

      /* 3. Compute ex2 = 2^(t/255+e+ex).  */
      ex2_u.f() = __exp2f_atable[tval & 255];
      tval >>= 8;
      unsafe = abs(tval) >= -FLT_MIN_EXP - 1;
      ex2_u.ieee.exponent += tval >> unsafe;
      scale_u.f() = 1.0f;
      scale_u.ieee.exponent += tval - (tval >> unsafe);

      /* 4. Approximate 2^x2 - 1, using a second-degree polynomial,
	 with maximum error in [-2^-9 - 2^-14, 2^-9 + 2^-14]
	 less than 1.3e-10f.  */

      x22 = (.24022656679f * x + .69314736128f) * ex2_u.f();

      /* 5. Return (2^x2-1) * 2^(t/512+e+ex) + 2^(t/512+e+ex).  */
      fesetenv (&oldenv);

      result = x22 * x + ex2_u.f();

      if (!unsafe)
	return result;
      else
	return result * scale_u.f();
    }
  /* Exceptional cases:  */
  else if (isless (x, himark))
    {
      if (__isinff (x))
	/* e^-inf == 0, with no error.  */
	return 0;
      else
	/* Underflow */
	return TWOM100 * TWOM100;
    }
  else
    /* Return x, if x is a NaN or Inf; or overflow, otherwise.  */
    return TWO127*x;
}
}
