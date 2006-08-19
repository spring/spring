/* See the import.pl script for potential modifications */
/* Compute remainder and a congruent to the quotient.
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


static const Extended zero = 0.0l;


namespace streflop_libm {
Extended
__remquol (Extended x, Extended p, int *quo)
{
  int32_t ex,ep,hx,hp;
  u_int32_t sx,lx,lp;
  int cquo,qs;

  GET_LDOUBLE_WORDS (ex, hx, lx, x);
  GET_LDOUBLE_WORDS (ep, hp, lp, p);
  sx = ex & 0x8000;
  qs = (sx ^ (ep & 0x8000)) >> 15;
  ep &= 0x7fff;
  ex &= 0x7fff;

  /* Purge off exception values.  */
  if ((ep | hp | lp) == 0)
    return (x * p) / (x * p); 			/* p = 0 */
  if ((ex == 0x7fff)				/* x not finite */
      || ((ep == 0x7fff)			/* p is NaN */
	  && ((hp | lp) != 0)))
    return (x * p) / (x * p);

  if (ep <= 0x7ffb)
    x = __ieee754_fmodl (x, 8 * p);		/* now x < 8p */

  if (((ex - ep) | (hx - hp) | (lx - lp)) == 0)
    {
      *quo = qs ? -1 : 1;
      return zero * x;
    }

  x  = fabsl (x);
  p  = fabsl (p);
  cquo = 0;

  if (x >= 4 * p)
    {
      x -= 4 * p;
      cquo += 4;
    }
  if (x >= 2 * p)
    {
      x -= 2 * p;
      cquo += 2;
    }

  if (ep < 0x0002)
    {
      if (x + x > p)
	{
	  x -= p;
	  ++cquo;
	  if (x + x >= p)
	    {
	      x -= p;
	      ++cquo;
	    }
	}
    }
  else
    {
      Extended p_half = 0.5l * p;
      if (x > p_half)
	{
	  x -= p;
	  ++cquo;
	  if (x >= p_half)
	    {
	      x -= p;
	      ++cquo;
	    }
	}
    }

  *quo = qs ? -cquo : cquo;

  if (sx)
    x = -x;
  return x;
}
weak_alias (__remquol, remquol)
}
