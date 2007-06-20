/* See the import.pl script for potential modifications */
/* Round argument to nearest integral value according to current rounding
   direction.
   Copyright (C) 1997 Free Software Foundation, Inc.
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

static const Simple two23[2] =
{
  8.3886080000e+06f, /* 0x4B000000 */
 -8.3886080000e+06f, /* 0xCB000000 */
};


long int
__lrintf (Simple x)
{
  int32_t j0;
  u_int32_t i0;
  Simple w;
  Simple t;
  long int result;
  int sx;

  GET_FLOAT_WORD (i0, x);

  sx = i0 >> 31;
  j0 = ((i0 >> 23) & 0xff) - 0x7f;
  i0 &= 0x7fffff;
  i0 |= 0x800000;

  if (j0 < (int32_t) (sizeof (long int) * 8) - 1)
    {
      if (j0 < -1)
	return 0;
      else if (j0 >= 23)
	result = (long int) i0 << (j0 - 23);
      else
	{
	  w = two23[sx] + x;
	  t = w - two23[sx];
	  GET_FLOAT_WORD (i0, t);
	  j0 = ((i0 >> 23) & 0xff) - 0x7f;
	  i0 &= 0x7fffff;
	  i0 |= 0x800000;

	  result = i0 >> (23 - j0);
	}
    }
  else
    {
      /* The number is too large.  It is left implementation defined
	 what happens.  */
      return (long int) x;
    }

  return sx ? -result : result;
}

weak_alias (__lrintf, lrintf)
