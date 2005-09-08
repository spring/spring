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

#endif //__WINDOWS_H__
