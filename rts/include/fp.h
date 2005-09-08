/*
 * fp.h
 * Floating point handling based on platform
 * Copyright (C) 2005 Christopher Han
 */
#ifndef FP_H
#define FP_H

#ifdef _WIN32

#define Control87(f,m) _control87(f,m)
#define Clearfp() _clearfp()

#else

#include <fenv.h>

#define _EM_INVALID FE_INVALID
#define _EM_DENORMAL __FE_DENORM
#define _EM_ZERODIVIDE FE_DIVBYZERO
#define _EM_OVERFLOW FE_OVERFLOW
#define _EM_UNDERFLOW FE_UNDERFLOW
#define _EM_INEXACT FE_INEXACT
#define _MCW_EM FE_ALL_EXCEPT

static inline unsigned int Control87(unsigned int newflags, unsigned int mask)
{
	fenv_t cur;
	fegetenv(&cur);
	if (mask) {
		cur.__control_word = ((cur.__control_word & ~mask)|(newflags & mask));
		fesetenv(&cur);
	}
	return (unsigned int)(cur.__control_word);
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
	return (unsigned int)(cur.__status_word);
}

#endif

#endif
