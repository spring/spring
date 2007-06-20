/* See the import.pl script for potential modifications */
/* Truncate argument to nearest integral value not larger than the argument.
   Copyright (C) 1997, 1998 Free Software Foundation, Inc.
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
Simple
__truncf (Simple x)
{
  int32_t i0, j0;
  int sx;

  GET_FLOAT_WORD (i0, x);
  sx = i0 & 0x80000000;
  j0 = ((i0 >> 23) & 0xff) - 0x7f;
  if (j0 < 23)
    {
      if (j0 < 0)
	/* The magnitude of the number is < 1 so the result is +-0.  */
	SET_FLOAT_WORD (x, sx);
      else
	SET_FLOAT_WORD (x, sx | (i0 & ~(0x007fffff >> j0)));
    }
  else
    {
      if (j0 == 0x80)
	/* x is inf or NaN.  */
	return x + x;
    }

  return x;
}
weak_alias (__truncf, truncf)
}
