/***[ OpenRTS ]***
 *
 *	HPIFile.cpp
 *    
 *  Copyright (C) 2005  Wesley Hill
 *
 *
 *  This file is part of OpenRTS.
 *
 *  OpenRTS is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  OpenRTS is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with OpenRTS; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 *  HPI Reading code is based on HPIUtil by Joe D, opened freely in 1998
 *
 */

#ifndef HEADER_OPENRTS_HPIFILE_H
#define HEADER_OPENRTS_HPIFILE_H


/***[ INCLUDES ]***/

#include "ImportTA.h"
#include "zlib.h"
#include "direct.h"


/***[ MACROS ]***/

#define HPI_FILE	0
#define HPI_DIR		1

#define HPI_NONE	0
#define HPI_LZ77	1
#define HPI_ZLIB	2

#define HPI_HAPI	((UINT32)0x49504148)
#define HPI_SQSH	((UINT32)0x48535153)

#define HPI_VERSION1	0x00010000


/***[ STRUCTURES ]***/

#pragma pack( 1 )

typedef struct HPIHEADER_TAG
{
	UINT32 hpiID;		// 'HAPI'
	UINT32 version;		// 0x00010000 = TA/TA:CC, 0x00020000 = TA:K (not supported), 'BANK' = savegame (not supported)
	UINT32 size;
	UINT32 key;			// decrytion key
	UINT32 offset;		// file offset to the root HPIDIRHEADER
} HPIHEADER;

typedef struct HPIDIRHEADER_TAG
{
	UINT32 numentries;	// number of entries in this directory
	UINT32 offset;		// file offset to array of HPIENTRY's
} HPIDIRHEADER;

typedef struct HPIENTRY_TAG
{
	UINT32 nameoffset;	// file offset to name of entry (null-terminated)
	UINT32 dataoffset;	// file offset to HPIDIRHEADER or HPIFILEHEADER, depending on type
	UCHAR type;			// 0 for file, 1 for directory (use HPI_FILE, HPI_DIR macros)
} HPIENTRY;

typedef struct HPIFILEHEADER_TAG
{
	UINT32 dataoffset;	// file offset to first HPIFILECHUNK, or the raw data if uncompressed
	UINT32 filesize;	// the uncompressed size of the file
	UCHAR compression;	// 0 for none, 1 for LZ77, 2 for zlib (use HPI_NONE, HPI,LZ77, HPI_ZLIB macros)
} HPIFILEHEADER;

typedef struct HPIFILECHUNK_TAG
{
	UINT32 hpifileID;		// 'SQSH'
	UCHAR gap;				// 0x02, maybe a version?
	UCHAR compression;		// 1 for LZ77, 2 for zlib (use HPI,LZ77, HPI_ZLIB macros)
	UCHAR encrypted;		// flag for further decryption
	UINT32 sizecompressed;	// compressed size of this chunk (can be larger than sizedecompressed)
	UINT32 sizedecompressed;// decompressed size of this chunk
	UINT32 checksum;		// sum of decompressed bytes
} HPIFILECHUNK;

#pragma pack()


/***[ CLASSES ]***/

class HPIFile
{
public:
	HPIFile( char *filename );		// Open a HPI formatted file
	~HPIFile( void );
	int ExtractAll( char *path );	// Extract the contents

private:
	FILE *infile;
	char key;
	char *infilename;
	char *destpath;
	
	int ExtractDir( HPIDIRHEADER *dirhead, char *path );
	int ExtractFile( HPIFILEHEADER *filehead, char *filename );
	int ExtractChunk( HPIFILECHUNK *chunk, FILE *out );
	int DecompressLZ77( char *src, char *dest, int srcsize, int destsize );
	int DecompressZLIB( char *src, char *dest, int srcsize, int destsize );
	void DecryptData( char *data, int size );				// perform the 2nd type of decryption
	int dc_fread( void *buffer, int size );					// calls fread, and decrypts
};

#endif // #ifndef HEADER_OPENRTS_HPIFILE_H
