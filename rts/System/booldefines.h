/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Public Domain (PD) 2009 Robin Vobruba <hoijui.quaero@gmail.com> */

/*
 * This header defines the bool type for C.
 * It is mainly usefull under Visual Studio.
 */

#ifndef _BOOL_DEFINES_H_
#define _BOOL_DEFINES_H_

// If C++, do nothing since bool is built in
#if !defined(__cplusplus)

#if !defined(__bool_true_false_are_defined) && !defined(__BOOL_DEFINED)
#define __bool_true_false_are_defined 1
#define __BOOL_DEFINED

// If this is C99 or later, use built-in bool
#if __STDC_VERSION__ >= 199901L
#define bool _Bool
#else
// Choose an unsigned type that can be used as a bitfield
#define bool unsigned char
//#define bool int
#endif

#define true 1
#define false 0
#endif // !defined(__bool_true_false_are_defined) && !defined(__BOOL_DEFINED)

#endif // !defined(__cplusplus)

#endif // _BOOL_DEFINES_H_
