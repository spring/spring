/*
Copyright (C) 2003 Rice1964

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/


#ifndef __FILTERS_H__
#define __FILTERS_H__

#ifdef _WIN32
#include <windows.h>
#endif
#include <math.h>
#include "SDL_types.h"

typedef Uint32 uint32;
typedef Uint16 uint16;
typedef Uint8 uint8;

#define abs(x) ((x^(x>>(sizeof(int)*8-1)))-(x>>(sizeof(int)*8-1)))

#define DWORD_MAKE(r, g, b, a)   ((Uint32) (((a) << 24) | ((r) << 16) | ((g) << 8) | (b)))
#define WORD_MAKE(r, g, b, a)   ((Uint16) (((a) << 12) | ((r) << 8) | ((g) << 4) | (b)))

void Super2xSaI_32( Uint32 *srcPtr, Uint32 *destPtr, Uint32 width, Uint32 height, Uint32 pitch);
void Super2xSaI_16( Uint16 *srcPtr, Uint16 *destPtr, Uint32 width, Uint32 height, Uint32 pitch);

void hq4x_init(void);
void hq4x_16( unsigned char * pIn, unsigned char * pOut, int Xres, int Yres, int SrcPPL, int BpL );
void hq4x_32( unsigned char * pIn, unsigned char * pOut, int Xres, int Yres, int SrcPPL, int BpL );

void hq2x_init(int bits_per_pixel);
void hq2x_16(Uint8 *srcPtr, Uint32 srcPitch, Uint8 *dstPtr, Uint32 dstPitch, int width, int height);
void hq2x_32(Uint8 *srcPtr, Uint32 srcPitch, Uint8 *dstPtr, Uint32 dstPitch, int width, int height);

#endif
