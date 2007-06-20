/* See the import.pl script for potential modifications */
/* Round Simple to integer away from zero.
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


static const Simple huge = 1.0e30f;


namespace streflop_libm {
Simple
__roundf (Simple x)
{
  int32_t i0, j0;

  GET_FLOAT_WORD (i0, x);
  j0 = ((i0 >> 23) & 0xff) - 0x7f;
  if (j0 < 23)
    {
      if (j0 < 0)
	{
	  if (huge + x > 0.0f)
	    {
	      i0 &= 0x80000000;
	      if (j0 == -1)
		i0 |= 0x3f800000;
	    }
	}
      else
	{
	  u_int32_t i = 0x007fffff >> j0;
	  if ((i0 & i) == 0)
	    /* X is integral.  */
	    return x;
	  if (huge + x > 0.0f)
	    {
	      /* Raise inexact if x != 0.  */
	      i0 += 0x00400000 >> j0;
	      i0 &= ~i;
	    }
	}
    }
  else
    {
      if (j0 == 0x80)
	/* Inf or NaN.  */
	return x + x;
      else
	return x;
    }

  SET_FLOAT_WORD (x, i0);
  return x;
}
weak_alias (__roundf, roundf)
}
