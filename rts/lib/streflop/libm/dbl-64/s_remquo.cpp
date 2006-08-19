/* See the import.pl script for potential modifications */
/* Compute remainder and a congruent to the quotient.
   Copyright (C) 1997 Free Software Foundation, Inc.
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


static const Double zero = 0.0;


namespace streflop_libm {
Double
__remquo (Double x, Double y, int *quo)
{
  int32_t hx,hy;
  u_int32_t sx,lx,ly;
  int cquo, qs;

  EXTRACT_WORDS (hx, lx, x);
  EXTRACT_WORDS (hy, ly, y);
  sx = hx & 0x80000000;
  qs = sx ^ (hy & 0x80000000);
  hy &= 0x7fffffff;
  hx &= 0x7fffffff;

  /* Purge off exception values.  */
  if ((hy | ly) == 0)
    return (x * y) / (x * y); 			/* y = 0 */
  if ((hx >= 0x7ff00000)			/* x not finite */
      || ((hy >= 0x7ff00000)			/* p is NaN */
	  && (((hy - 0x7ff00000) | ly) != 0)))
    return (x * y) / (x * y);

  if (hy <= 0x7fbfffff)
    x = __ieee754_fmod (x, 8 * y);		/* now x < 8y */

  if (((hx - hy) | (lx - ly)) == 0)
    {
      *quo = qs ? -1 : 1;
      return zero * x;
    }

  x  = fabs (x);
  y  = fabs (y);
  cquo = 0;

  if (x >= 4 * y)
    {
      x -= 4 * y;
      cquo += 4;
    }
  if (x >= 2 * y)
    {
      x -= 2 * y;
      cquo += 2;
    }

  if (hy < 0x00200000)
    {
      if (x + x > y)
	{
	  x -= y;
	  ++cquo;
	  if (x + x >= y)
	    {
	      x -= y;
	      ++cquo;
	    }
	}
    }
  else
    {
      Double y_half = 0.5 * y;
      if (x > y_half)
	{
	  x -= y;
	  ++cquo;
	  if (x >= y_half)
	    {
	      x -= y;
	      ++cquo;
	    }
	}
    }

  *quo = qs ? -cquo : cquo;

  if (sx)
    x = -x;
  return x;
}
weak_alias (__remquo, remquo)
#ifdef NO_LONG_DOUBLE
strong_alias (__remquo, __remquol)
weak_alias (__remquo, remquol)
#endif
}
