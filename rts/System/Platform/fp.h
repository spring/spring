/**
 * @file fp.h
 * @brief floating point
 * @author Christopher Han <xiphux@gmail.com>
 *
 * Floating point handling based on platform
 * Copyright (C) 2005.  Licensed under the terms of the
 * GNU GPL, v2 or later
 */
#ifndef FP_H
#define FP_H

#ifdef _WIN32

#define Control87(f,m) _control87(f,m)
#define Clearfp() _clearfp()

#else /* emulate WIN32 */

#include <fenv.h>

#define _EM_INVALID FE_INVALID
#define _EM_DENORMAL __FE_DENORM
#define _EM_ZERODIVIDE FE_DIVBYZERO
#define _EM_OVERFLOW FE_OVERFLOW
#define _EM_UNDERFLOW FE_UNDERFLOW
#define _EM_INEXACT FE_INEXACT
#define _MCW_EM FE_ALL_EXCEPT

#ifdef __i386

#if defined(__linux__)
#define fenv_control(xfenv_t) (xfenv_t.__control_word)
#define fenv_status(xfenv_t) (xfenv_t.__status_word)
#elif defined(__FreeBSD__) || defined(__APPLE_CC__)
#define fenv_control(xfenv_t) (xfenv_t.__control)
#define fenv_status(xfenv_t) (xfenv_t.__status)
#else
Add support for your platform here and below!
#endif		

static inline unsigned int Control87(unsigned int newflags, unsigned int mask)
{
	fenv_t cur;
	fegetenv(&cur);
	if (mask) {
		fenv_control(cur) = ((fenv_control(cur) & ~mask)|(newflags & mask));
		fesetenv(&cur);
	}
	return (unsigned int)(fenv_control(cur));
}

static inline unsigned int Clearfp(void)
{
	fenv_t cur;
	fegetenv(&cur);
#if 0	/* Windows control word default */
	cur.__control_word &= ~FE_ALL_EXCEPT;
	fesetenv(cur);
#else	/* Posix control word default */
	fesetenv(FE_DFL_ENV);
#endif
	return (unsigned int)(fenv_status(cur));
}

#else /* not __i386 */

static inline unsigned int Control87(unsigned int newflags, unsigned int mask)
{
	fexcept_t cur;
	fegetexceptflag(&cur, FE_ALL_EXCEPT);
	if (mask) {
		cur = (cur & ~mask)|(newflags & mask);
		fesetexceptflag(&cur, FE_ALL_EXCEPT);
	}
	return (unsigned int)cur;
}

static inline unsigned int Clearfp(void)
{
	int ret = fetestexcept(FE_ALL_EXCEPT);
	feclearexcept(FE_ALL_EXCEPT);
	return ret;
}


#endif /* not __i386 */

#endif /* emulate WIN32 */

#endif /* ifndef FP_H */
