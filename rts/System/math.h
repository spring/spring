/* 
 * math.h
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is a part of the mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within the package.
 *
 * Mathematical functions.
 *
 * This file is just a wrapper to the streflop math functions.
 */

#ifndef MATH_H_INCLUDED
#define MATH_H_INCLUDED

#ifndef __cplusplus
#error streflop does not support C programs
#endif

#ifdef __GNUC__
#warning math.h inclusion detected, include streflop.h instead!
#endif

#include "streflop.h"
using namespace streflop;

#endif	/* Not MATH_H_INCLUDED */
