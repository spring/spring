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

#include <math.h>

#ifdef NO_WINSTUFF
#include <windows.h>
#define uint8 unsigned char
#define uint16 unsigned short
#define uint32 unsigned int
#else
#define uint8 unsigned __int8
#define WORD unsigned __int16
#define uint16 unsigned __int16
#define DWORD unsigned __int32
#define uint32 unsigned __int32
#endif

#define DWORD_MAKE(r, g, b, a)   ((DWORD) (((a) << 24) | ((r) << 16) | ((g) << 8) | (b)))
#define WORD_MAKE(r, g, b, a)   ((WORD) (((a) << 12) | ((r) << 8) | ((g) << 4) | (b)))

void Super2xSaI_32( DWORD *srcPtr, DWORD *destPtr, DWORD width, DWORD height, DWORD pitch);
void Super2xSaI_16( WORD *srcPtr, WORD *destPtr, DWORD width, DWORD height, DWORD pitch);

void hq4x_init(void);
void hq4x_16( unsigned char * pIn, unsigned char * pOut, int Xres, int Yres, int SrcPPL, int BpL );
void hq4x_32( unsigned char * pIn, unsigned char * pOut, int Xres, int Yres, int SrcPPL, int BpL );

void hq2x_init(int bits_per_pixel);
void hq2x_16(uint8 *srcPtr, uint32 srcPitch, uint8 *dstPtr, uint32 dstPitch, int width, int height);
void hq2x_32(uint8 *srcPtr, uint32 srcPitch, uint8 *dstPtr, uint32 dstPitch, int width, int height);

#endif
