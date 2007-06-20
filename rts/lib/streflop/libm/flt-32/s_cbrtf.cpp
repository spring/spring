/* See the import.pl script for potential modifications */
/* Compute cubic root of Simple value.
   Copyright (C) 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Dirk Alboth <dirka@uni-paderborn.de> and
   Ulrich Drepper <drepper@cygnus.com>, 1997.

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


#define CBRT2 1.2599210498948731648f		/* 2^(1/3) */
#define SQR_CBRT2 1.5874010519681994748f		/* 2^(2/3) */

static const Simple factor[5] =
{
  1.0f / SQR_CBRT2,
  1.0f / CBRT2,
  1.0f,
  CBRT2,
  SQR_CBRT2
};


namespace streflop_libm {
Simple
__cbrtf (Simple x)
{
  Simple xm, ym, u, t2;
  int xe;

  /* Reduce X.  XM now is an range 1.0f to 0.5f.  */
  xm = __frexpf (fabsf (x), &xe);

  /* If X is not finite or is null return it (with raising exceptions
     if necessary.
     Note: *Our* version of `frexp' sets XE to zero if the argument is
     Inf or NaN.  This is not portable but faster.  */
  if (xe == 0 && fpclassify (x) <= FP_ZERO)
    return x + x;

  u = (0.492659620528969547f + (0.697570460207922770f
			       - 0.191502161678719066f * xm) * xm);

  t2 = u * u * u;

  ym = u * (t2 + 2.0f * xm) / (2.0f * t2 + xm) * factor[2 + xe % 3];

  return __ldexpf (x > 0.0f ? ym : -ym, xe / 3);
}
weak_alias (__cbrtf, cbrtf)
}
