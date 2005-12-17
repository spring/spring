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

#include "lib/hpiutil2/hpifile.h"


HPIFile::HPIFile( char *filename )
{
	infilename = filename;
}

HPIFile::~HPIFile(void) { }

// Returns 0 for failure, 1 for success
int HPIFile::ExtractAll( char *path )
{
	destpath = path;
	if( !(infile = fopen( infilename, "rb" )) )
		return( 1 );

	// Read in header, the only part of the file that's not encrypted
	HPIHEADER head;
	fread( &head, sizeof(HPIHEADER), 1, infile );
	if( head.hpiID != HPI_HAPI ) goto fail;
	if( head.version != HPI_VERSION1 ) goto fail;
	if( head.key )
		key = ~((head.key * 4) | (head.key >> 6));
	else
		key = 0;

	// Read in first directory header, and call ExtractDir() to do all the work (recursivly)
	HPIDIRHEADER dirhead;
	fseek( infile, head.offset, SEEK_SET );
	dc_fread( &dirhead, sizeof(HPIDIRHEADER) );
	printf( "Extracting contents of %s\n", infilename );
	int result = ExtractDir( &dirhead, path );
	fclose( infile );
	return( result );

fail:
	fclose( infile );
	return( 0 );
}

// Returns 0 for failure, 1 for success
int HPIFile::ExtractDir( HPIDIRHEADER *dirhead, char *path )
{
	for( unsigned int i = 0; i < dirhead->numentries; i++ ) {
		HPIENTRY entry;
		fseek( infile, dirhead->offset + i * sizeof(HPIENTRY), SEEK_SET );
		dc_fread( &entry, sizeof(HPIENTRY) );

		// read in the name of the entry, and tack it onto the path
		char name[MAX_PATH];
		fseek( infile, entry.nameoffset, SEEK_SET );
		dc_fread( name, 32 );
		char newname[MAX_PATH];
		strcpy( newname, path );
		strcat( newname, "\\" );
		strcat( newname, name );

		// read in the next directory/file and extract it
		fseek( infile, entry.dataoffset, SEEK_SET );
		if( entry.type == HPI_DIR ) {
			HPIDIRHEADER dirhead;
			dc_fread( &dirhead, sizeof(HPIDIRHEADER) );
			_mkdir( newname );
			if( !ExtractDir( &dirhead, newname ) )
				return( 0 );
		}
		else if( entry.type == HPI_FILE ) {
			HPIFILEHEADER filehead;
			dc_fread( &filehead, sizeof(HPIFILEHEADER) );
			if( !ExtractFile( &filehead, newname ) )
				return( 0 );
		}
		else return( 0 );
	}
	return( 1 );
}

// Returns 0 for failure, 1 for success
int HPIFile::ExtractFile( HPIFILEHEADER *filehead, char* filename )
{
	FILE *outfile;
	if( !(outfile = fopen( filename, "wb" )) )
		return( 1 );

	// read in the sizes of the chunks
	int numchunks = (filehead->filesize / 65536) + (filehead->filesize % 65536 != 0);
	UINT32 *chunksizes = (UINT32 *) malloc( numchunks * sizeof(UINT32) );
	fseek( infile, filehead->dataoffset, SEEK_SET );
	dc_fread( chunksizes, numchunks * sizeof(UINT32) );

	// extract the chunks
	for( int i = 0; i < numchunks; i++ ) {
		HPIFILECHUNK chunk;
		dc_fread( &chunk, sizeof(HPIFILECHUNK) );
		if( !ExtractChunk( &chunk, outfile ) ) {
			free( chunksizes );
			fclose( outfile );
			printf("Failure on %s\n", filename );
			//return( 0 );
		}
	}

	free( chunksizes );
	fclose( outfile );
	return( 1 );
}

// ExtractChunk(): Read, decompress, and commit a chunk of data to disk
//				   Returns number of bytes written to disk
int HPIFile::ExtractChunk( HPIFILECHUNK *chunk, FILE *out )
{
	if( chunk->hpifileID != HPI_SQSH ) return( 0 );

	// read in compressed data
	char *compressed = (char *) malloc( chunk->sizecompressed );
	int cs;
	if( (cs=dc_fread( compressed, chunk->sizecompressed )) != chunk->checksum )
		return( 0 );
	if( chunk->encrypted ) DecryptData( compressed, chunk->sizecompressed );

	// decompress data
	char *decompressed = (char *) malloc( chunk->sizedecompressed );
	int result;
	if( chunk->compression == HPI_LZ77 )
		result = DecompressLZ77( compressed, decompressed, chunk->sizecompressed, chunk->sizedecompressed );
	else if( chunk->compression == HPI_ZLIB )
		result = DecompressZLIB( compressed, decompressed, chunk->sizecompressed, chunk->sizedecompressed );

	// write decompressed data to the file
	if( result ) {
        fwrite( decompressed, chunk->sizedecompressed, 1, out );
		free( compressed );
		free( decompressed );
		return( chunk->sizedecompressed );
	}
	else {
		free( compressed );
		free( decompressed );
		return( 0 );
	}
}

// decompress LZ77, returns bytes decompressed
int HPIFile::DecompressLZ77( char *src, char *dest, int srcsize, int destsize )
{
	int work1 = 1, work2 = 1, work3;
	int inptr = 0, outptr = 0;
	int count;
	char DBuff[4096];
	int DPtr;

	work3 = src[inptr++];
	
	while( 1 ) {
		if( (work2 & work3) == 0 ) {
			dest[outptr++] = src[inptr];
			DBuff[work1] = src[inptr];
			work1 = (work1 + 1) & 0xFFF;
			inptr++;
		}
		else {
			count = *((UINT16 *) (src+inptr));
			inptr += 2;
			DPtr = count >> 4;
			if( DPtr == 0 ) {
				return outptr;
			}
			else {
				count = (count & 0x0f) + 2;
				if( count >= 0 ) {
					for( int x = 0; x < count; x++ ) {
						dest[outptr++] = DBuff[DPtr];
						DBuff[work1] = DBuff[DPtr];
						DPtr = (DPtr + 1) & 0xFFF;
						work1 = (work1 + 1) & 0xFFF;
					}
				}
			}
		}
		work2 *= 2;
		if( work2 & 0x0100 ) {
			work2 = 1;
			work3 = src[inptr++];
		}
	}
	return outptr;
}

// use zlib.lib to do all this work, returns bytes decompressed
int HPIFile::DecompressZLIB( char *src, char *dest, int srcsize, int destsize )
{
	z_stream zs;
	zs.next_in = (Bytef *)src;
	zs.avail_in = srcsize;
	zs.total_in = 0;
	zs.next_out = (Bytef *)dest;
	zs.avail_out = destsize;
	zs.total_out = 0;
	zs.msg = NULL;
	zs.state = NULL;
	zs.zalloc = Z_NULL;
	zs.zfree = Z_NULL;
	zs.opaque = NULL;
	zs.data_type = Z_BINARY;
	zs.adler = 0;
	zs.reserved = 0;

	if( inflateInit(&zs) != Z_OK ) return( 0 );
	if( inflate(&zs, Z_FINISH) != Z_STREAM_END ) return( 0 );
	if( inflateEnd(&zs) != Z_OK ) return( 0 );
	return zs.total_out;
}

// DecryptData(): decrypt the form of encryption found in the chunks
void HPIFile::DecryptData( char *data, int size )
{
	for( int i = 0; i < size; i++ )
		data[i] = data[i] - i ^ i;
}

// dc_fread(): read in from infile and decrypt the type of encryption used in the whole file
int HPIFile::dc_fread( void *buffer, int size )
{
	int filepos = ftell( infile );
	int count = (int) fread( buffer, 1, size, infile );
	int checksum = 0;
	for( int i = 0; i < count; i++ ) {
		if( key ) ((unsigned char *)buffer)[i] = ((filepos + i) ^ key) ^ ~(((unsigned char *)buffer)[i]);
		checksum += ((unsigned char *)buffer)[i];
	}
	return checksum;
}
