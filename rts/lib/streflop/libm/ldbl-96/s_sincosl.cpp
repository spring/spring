/* See the import.pl script for potential modifications */
/* Compute sine and cosine of argument.
   Copyright (C) 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1997.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1l of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#include "math.h"

#include "math_private.h"


namespace streflop_libm {
void
__sincosl (Extended x, Extended *sinx, Extended *cosx)
{
  int32_t se, i0, i1;

  /* High word of x. */
  GET_LDOUBLE_WORDS (se, i0, i1, x);

  /* |x| ~< pi/4 */
  se &= 0x7fff;
  if (se < 0x3ffe || (se == 0x3ffe && i0 <= 0xc90fdaa2))
    {
      *sinx = __kernel_sinl (x, 0.0l, 0);
      *cosx = __kernel_cosl (x, 0.0l);
    }
  else if (se == 0x7fff)
    {
      /* sin(Inf or NaN) is NaN */
      *sinx = *cosx = x - x;
    }
  else
    {
      /* Argument reduction needed.  */
      Extended y[2];
      int n;

      n = __ieee754_rem_pio2l (x, y);
      switch (n & 3)
	{
	case 0:
	  *sinx = __kernel_sinl (y[0], y[1], 1);
	  *cosx = __kernel_cosl (y[0], y[1]);
	  break;
	case 1:
	  *sinx = __kernel_cosl (y[0], y[1]);
	  *cosx = -__kernel_sinl (y[0], y[1], 1);
	  break;
	case 2:
	  *sinx = -__kernel_sinl (y[0], y[1], 1);
	  *cosx = -__kernel_cosl (y[0], y[1]);
	  break;
	default:
	  *sinx = -__kernel_cosl (y[0], y[1]);
	  *cosx = __kernel_sinl (y[0], y[1], 1);
	  break;
	}
    }
}
weak_alias (__sincosl, sincosl)
}
