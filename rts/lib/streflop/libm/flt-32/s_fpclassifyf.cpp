/* See the import.pl script for potential modifications */
/* Return classification value corresponding to argument.
   Copyright (C) 1997, 2000, 2002 Free Software Foundation, Inc.
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
int
__fpclassifyf (Simple x)
{
  u_int32_t wx;
  int retval = FP_NORMAL;

  GET_FLOAT_WORD (wx, x);
  wx &= 0x7fffffff;
  if (wx == 0)
    retval = FP_ZERO;
  else if (wx < 0x800000)
    retval = FP_SUBNORMAL;
  else if (wx >= 0x7f800000)
    retval = wx > 0x7f800000 ? FP_NAN : FP_INFINITE;

  return retval;
}
libm_hidden_def (__fpclassifyf)
}
