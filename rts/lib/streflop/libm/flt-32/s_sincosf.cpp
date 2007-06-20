/* See the import.pl script for potential modifications */
/* Compute sine and cosine of argument.
   Copyright (C) 1997, 2000 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1997.

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

#include "SMath.h"

#include "math_private.h"


namespace streflop_libm {
void
__sincosf (Simple x, Simple *sinx, Simple *cosx)
{
  int32_t ix;

  /* High word of x. */
  GET_FLOAT_WORD (ix, x);

  /* |x| ~< pi/4 */
  ix &= 0x7fffffff;
  if (ix <= 0x3f490fd8)
    {
      *sinx = __kernel_sinf (x, 0.0f, 0);
      *cosx = __kernel_cosf (x, 0.0f);
    }
  else if (ix>=0x7f800000)
    {
      /* sin(Inf or NaN) is NaN */
      *sinx = *cosx = x - x;
    }
  else
    {
      /* Argument reduction needed.  */
      Simple y[2];
      int n;

      n = __ieee754_rem_pio2f (x, y);
      switch (n & 3)
	{
	case 0:
	  *sinx = __kernel_sinf (y[0], y[1], 1);
	  *cosx = __kernel_cosf (y[0], y[1]);
	  break;
	case 1:
	  *sinx = __kernel_cosf (y[0], y[1]);
	  *cosx = -__kernel_sinf (y[0], y[1], 1);
	  break;
	case 2:
	  *sinx = -__kernel_sinf (y[0], y[1], 1);
	  *cosx = -__kernel_cosf (y[0], y[1]);
	  break;
	default:
	  *sinx = -__kernel_cosf (y[0], y[1]);
	  *cosx = __kernel_sinf (y[0], y[1], 1);
	  break;
	}
    }
}
weak_alias (__sincosf, sincosf)
}
