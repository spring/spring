/* See the import.pl script for potential modifications */
/* Double-precision floating point 2^x.
   Copyright (C) 1997,1998,2000,2001,2005,2006 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Geoffrey Keating <geoffk@ozemail.com.au>

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

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
   It has been slightly modified to compute 2^x instead of e^x.
   */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "../streflop_libm_bridge.h"
#include "../streflop_libm_bridge.h"
#include "ieee754.h"
#include "math.h"
#include "../streflop_libm_bridge.h"
#include "../streflop_libm_bridge.h"
#include "math_private.h"

#include "t_exp2.h"

/* XXX I know the assembler generates a warning about incorrect section
   attributes. But without the attribute here the compiler places the
   constants in the .data section.  Ideally the constant is placed in
   .rodata.cst8 so that it can be merged, but gcc sucks, it ICEs when
   we try to force this section on it.  --drepper  */
static const Double TWO1023 = 8.988465674311579539e+307;
static const Double TWOM1000 = 9.3326361850321887899e-302;

namespace streflop_libm {
Double
__ieee754_exp2 (Double x)
{
  static const Double himark = (Double) DBL_MAX_EXP;
  static const Double lomark = (Double) (DBL_MIN_EXP - DBL_MANT_DIG - 1);

  /* Check for usual case.  */
  if (isless (x, himark) && isgreaterequal (x, lomark))
    {
      static const Double THREEp42 = 13194139533312.0;
      int tval, unsafe;
      Double rx, x22, result;
      union ieee754_double ex2_u, scale_u;
      fenv_t oldenv;

      feholdexcept (&oldenv);
#ifdef FE_TONEAREST
      /* If we don't have this, it's too bad.  */
      fesetround (FE_TONEAREST);
#endif

      /* 1. Argument reduction.
	 Choose integers ex, -256 <= t < 256, and some real
	 -1/1024 <= x1 <= 1024 so that
	 x = ex + t/512 + x1.

	 First, calculate rx = ex + t/512.  */
      rx = x + THREEp42;
      rx -= THREEp42;
      x -= rx;  /* Compute x=x1. */
      /* Compute tval = (ex*512 + t)+256.
	 Now, t = (tval mod 512)-256 and ex=tval/512  [that's mod, NOT %; and
	 /-round-to-nearest not the usual c integer /].  */
      tval = (int) (rx * 512.0 + 256.0);

      /* 2. Adjust for accurate table entry.
	 Find e so that
	 x = ex + t/512 + e + x2
	 where -1e6 < e < 1e6, and
	 (Double)(2^(t/512+e))
	 is accurate to one part in 2^-64.  */

      /* 'tval & 511' is the same as 'tval%512' except that it's always
	 positive.
	 Compute x = x2.  */
      x -= exp2_deltatable[tval & 511];

      /* 3. Compute ex2 = 2^(t/512+e+ex).  */
      ex2_u.d() = exp2_accuratetable[tval & 511];
      tval >>= 9;
      unsafe = abs(tval) >= -DBL_MIN_EXP - 1;
      ex2_u.ieee.exponent += tval >> unsafe;
      scale_u.d() = 1.0;
      scale_u.ieee.exponent += tval - (tval >> unsafe);

      /* 4. Approximate 2^x2 - 1, using a fourth-degree polynomial,
	 with maximum error in [-2^-10-2^-30,2^-10+2^-30]
	 less than 10^-19.  */

      x22 = (((.0096181293647031180
	       * x + .055504110254308625)
	      * x + .240226506959100583)
	     * x + .69314718055994495) * ex2_u.d();

      /* 5. Return (2^x2-1) * 2^(t/512+e+ex) + 2^(t/512+e+ex).  */
      fesetenv (&oldenv);

      result = x22 * x + ex2_u.d();

      if (!unsafe)
	return result;
      else
	return result * scale_u.d();
    }
  /* Exceptional cases:  */
  else if (isless (x, himark))
    {
      if (__isinf (x))
	/* e^-inf == 0, with no error.  */
	return 0;
      else
	/* Underflow */
	return TWOM1000 * TWOM1000;
    }
  else
    /* Return x, if x is a NaN or Inf; or overflow, otherwise.  */
    return TWO1023*x;
}
}
