/* See the import.pl script for potential modifications */
/* Round Simple value to long long int.
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


long long int
__llroundf (Simple x)
{
  int32_t j0;
  u_int32_t i;
  long long int result;
  int sign;

  GET_FLOAT_WORD (i, x);
  j0 = ((i >> 23) & 0xff) - 0x7f;
  sign = (i & 0x80000000) != 0 ? -1 : 1;
  i &= 0x7fffff;
  i |= 0x800000;

  if (j0 < (int32_t) (8 * sizeof (long long int)) - 1)
    {
      if (j0 < 0)
	return j0 < -1 ? 0 : sign;
      else if (j0 >= 23)
	result = (long long int) i << (j0 - 23);
      else
	{
	  i += 0x400000 >> j0;

	  result = i >> (23 - j0);
	}
    }
  else
    {
      /* The number is too large.  It is left implementation defined
	 what happens.  */
      return (long long int) x;
    }

  return sign * result;
}

weak_alias (__llroundf, llroundf)
