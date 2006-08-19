/* See the import.pl script for potential modifications */
/* Truncate argument to nearest integral value not larger than the argument.
   Copyright (C) 1997, 1999 Free Software Foundation, Inc.
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
Extended
__truncl (Extended x)
{
  int32_t i0, j0;
  u_int32_t se, i1;
  int sx;

  GET_LDOUBLE_WORDS (se, i0, i1, x);
  sx = se & 0x8000;
  j0 = (se & 0x7fff) - 0x3fff;
  if (j0 < 31)
    {
      if (j0 < 0)
	/* The magnitude of the number is < 1 so the result is +-0.  */
	SET_LDOUBLE_WORDS (x, sx, 0, 0);
      else
	SET_LDOUBLE_WORDS (x, se, i0 & ~(0x7fffffff >> j0), 0);
    }
  else if (j0 > 63)
    {
      if (j0 == 0x4000)
	/* x is inf or NaN.  */
	return x + x;
    }
  else
    {
      SET_LDOUBLE_WORDS (x, se, i0, i1 & ~(0xffffffffu >> (j0 - 31)));
    }

  return x;
}
weak_alias (__truncl, truncl)
}
