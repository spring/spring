/* See the import.pl script for potential modifications */
/* Round Double to integer away from zero.
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


static const Double huge = 1.0e300;


namespace streflop_libm {
Double
__round (Double x)
{
  int32_t i0, j0;
  u_int32_t i1;

  EXTRACT_WORDS (i0, i1, x);
  j0 = ((i0 >> 20) & 0x7ff) - 0x3ff;
  if (j0 < 20)
    {
      if (j0 < 0)
	{
	  if (huge + x > 0.0)
	    {
	      i0 &= 0x80000000;
	      if (j0 == -1)
		i0 |= 0x3ff00000;
	      i1 = 0;
	    }
	}
      else
	{
	  u_int32_t i = 0x000fffff >> j0;
	  if (((i0 & i) | i1) == 0)
	    /* X is integral.  */
	    return x;
	  if (huge + x > 0.0)
	    {
	      /* Raise inexact if x != 0.  */
	      i0 += 0x00080000 >> j0;
	      i0 &= ~i;
	      i1 = 0;
	    }
	}
    }
  else if (j0 > 51)
    {
      if (j0 == 0x400)
	/* Inf or NaN.  */
	return x + x;
      else
	return x;
    }
  else
    {
      u_int32_t i = 0xffffffff >> (j0 - 20);
      if ((i1 & i) == 0)
	/* X is integral.  */
	return x;

      if (huge + x > 0.0)
	{
	  /* Raise inexact if x != 0.  */
	  u_int32_t j = i1 + (1 << (51 - j0));
	  if (j < i1)
	    i0 += 1;
	  i1 = j;
	}
      i1 &= ~i;
    }

  INSERT_WORDS (x, i0, i1);
  return x;
}
weak_alias (__round, round)
#ifdef NO_LONG_DOUBLE
strong_alias (__round, __roundl)
weak_alias (__round, roundl)
#endif
}
