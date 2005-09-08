/**
This file is here only for fast porting. 
It has to be removed so that no #include <windows.h>
appears in the common code

@author gnibu

*/

#ifndef __WINDOWS_H__
#define __WINDOWS_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <fenv.h>
#include <iostream>
#include <byteswap.h>
#include <boost/cstdint.hpp>
#include <SDL/SDL.h>

/*
 * Bitmap handling
 *
 * The swabbing stuff looks backwards, but the bitmaps
 * are _originally_ little endian (win32 x86).
 * So in this case little endian is the standard while
 * big endian is the exception.
 */
#if __BYTE_ORDER == __BIG_ENDIAN
#define swabword(w)	(bswap_16(w))
#define swabdword(w)	(bswap_32(w))
#else
#define swabword(w)	(w)
#define swabdword(w)	(w)
#endif
struct paletteentry_s {
	Uint8 peRed;
	Uint8 peGreen;
	Uint8 peBlue;
	Uint8 peFlags;
};
typedef struct paletteentry_s PALETTEENTRY;

/*
 * Floating point
 */
#define _EM_INVALID FE_INVALID
#define _EM_DENORMAL __FE_DENORM
#define _EM_ZERODIVIDE FE_DIVBYZERO
#define _EM_OVERFLOW FE_OVERFLOW
#define _EM_UNDERFLOW FE_UNDERFLOW
#define _EM_INEXACT FE_INEXACT
#define _MCW_EM FE_ALL_EXCEPT

static inline unsigned int _control87(unsigned int newflags, unsigned int mask)
{
	fenv_t cur;
	fegetenv(&cur);
	if (mask) {
		cur.__control_word = ((cur.__control_word & ~mask)|(newflags & mask));
		fesetenv(&cur);
	}
	return (unsigned int)(cur.__control_word);
}

static inline unsigned int _clearfp(void)
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

#endif //__WINDOWS_H__
