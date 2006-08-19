/* See the import.pl script for potential modifications */
/* Compute sine and cosine of argument.
   Copyright (C) 1997, 2001, 2005 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1997.

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

#include "math.h"

#include "math_private.h"


namespace streflop_libm {
void
__sincos (Double x, Double *sinx, Double *cosx)
{
  int32_t ix;

  /* High word of x. */
  GET_HIGH_WORD (ix, x);

  /* |x| ~< pi/4 */
  ix &= 0x7fffffff;
  if (ix>=0x7ff00000)
    {
      /* sin(Inf or NaN) is NaN */
      *sinx = *cosx = x - x;
    }
  else
    {
      *sinx = __sin (x);
      *cosx = __cos (x);
    }
}
weak_alias (__sincos, sincos)
#ifdef NO_LONG_DOUBLE
strong_alias (__sincos, __sincosl)
weak_alias (__sincos, sincosl)
#endif
}
