/* zip.h -- IO for compress .zip files using zlib 
   Version 0.18 beta, Feb 26th, 2002

   Copyright (C) 1998-2002 Gilles Vollant

   This unzip package allow creates .ZIP file, compatible with PKZip 2.04g
     WinZip, InfoZip tools and compatible.
   Encryption and multi volume ZipFile (span) are not supported.
   Old compressions used by old PKZip 1.x are not supported

  For uncompress .zip file, look at unzip.h

   THIS IS AN ALPHA VERSION. AT THIS STAGE OF DEVELOPPEMENT, SOMES API OR STRUCTURE
   CAN CHANGE IN FUTURE VERSION !!
   I WAIT FEEDBACK at mail info@winimage.com
   Visit also http://www.winimage.com/zLibDll/unzip.html for evolution

   Condition of use and distribution are the same than zlib :

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.


*/

/* for more info about .ZIP format, see 
      http://www.info-zip.org/pub/infozip/doc/appnote-981119-iz.zip
      http://www.info-zip.org/pub/infozip/doc/
   PkWare has also a specification at :
      ftp://ftp.pkware.com/probdesc.zip
*/

#ifndef _zip_H
#define _zip_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ZLIB_H
#include "zlib.h"
#endif

#ifndef _ZLIBIOAPI_H
#include "ioapi.h"
#endif

#if defined(STRICTZIP) || defined(STRICTZIPUNZIP)
/* like the STRICT of WIN32, we define a pointer that cannot be converted
    from (void*) without cast */
typedef struct TagzipFile__ { int unused; } zipFile__; 
typedef zipFile__ *zipFile;
#else
typedef voidp zipFile;
#endif

#define ZIP_OK                                  (0)
#define ZIP_ERRNO               (Z_ERRNO)
#define ZIP_PARAMERROR                  (-102)
#define ZIP_INTERNALERROR               (-104)

/* tm_zip contain date/time info */
typedef struct tm_zip_s 
{
	uInt tm_sec;            /* seconds after the minute - [0,59] */
	uInt tm_min;            /* minutes after the hour - [0,59] */
	uInt tm_hour;           /* hours since midnight - [0,23] */
	uInt tm_mday;           /* day of the month - [1,31] */
	uInt tm_mon;            /* months since January - [0,11] */
	uInt tm_year;           /* years - [1980..2044] */
} tm_zip;

typedef struct
{
	tm_zip      tmz_date;       /* date in understandable format           */
    uLong       dosDate;       /* if dos_date == 0, tmu_date is used      */
/*    uLong       flag;        */   /* general purpose bit flag        2 bytes */

    uLong       internal_fa;    /* internal file attributes        2 bytes */
    uLong       external_fa;    /* external file attributes        4 bytes */
} zip_fileinfo;

typedef const char* zipcharpc;


extern zipFile ZEXPORT zipOpen OF((const char *pathname, int append));
/*
  Create a zipfile.
	 pathname contain on Windows XP a filename like "c:\\zlib\\zlib113.zip" or on
	   an Unix computer "zlib/zlib113.zip".
	 if the file pathname exist and append=1, the zip will be created at the end
	   of the file. (useful if the file contain a self extractor code)
	 If the zipfile cannot be opened, the return value is NULL.
     Else, the return value is a zipFile Handle, usable with other function
	   of this zip package.
*/

extern zipFile ZEXPORT zipOpen2 OF((const char *pathname, 
                                   int append,
                                   zipcharpc* globalcomment,
                                   zlib_filefunc_def* pzlib_filefunc_def));

extern int ZEXPORT zipOpenNewFileInZip OF((zipFile file,
					   const char* filename,
					   const zip_fileinfo* zipfi,
					   const void* extrafield_local,
					   uInt size_extrafield_local,
					   const void* extrafield_global,
					   uInt size_extrafield_global,
					   const char* comment,
					   int method,
					   int level));
/*
  Open a file in the ZIP for writing.
  filename : the filename in zip (if NULL, '-' without quote will be used
  *zipfi contain supplemental information
  if extrafield_local!=NULL and size_extrafield_local>0, extrafield_local
    contains the extrafield data the the local header
  if extrafield_global!=NULL and size_extrafield_global>0, extrafield_global
    contains the extrafield data the the local header
  if comment != NULL, comment contain the comment string
  method contain the compression method (0 for store, Z_DEFLATED for deflate)
  level contain the level of compression (can be Z_DEFAULT_COMPRESSION)
*/


extern int ZEXPORT zipOpenNewFileInZip2 OF((zipFile file,
					   const char* filename,
					   const zip_fileinfo* zipfi,
					   const void* extrafield_local,
					   uInt size_extrafield_local,
					   const void* extrafield_global,
					   uInt size_extrafield_global,
					   const char* comment,
					   int method,
					   int level,
                       int raw));

/*
  Same than zipOpenNewFileInZip, except if raw=1, we write raw file
 */

extern int ZEXPORT zipWriteInFileInZip OF((zipFile file,
					   const voidp buf,
					   unsigned len));
/*
  Write data in the zipfile
*/

extern int ZEXPORT zipCloseFileInZip OF((zipFile file));
/*
  Close the current file in the zipfile
*/


extern int ZEXPORT zipCloseFileInZipRaw OF((zipFile file,
                                            uLong uncompressed_size,
                                            uLong crc32));
/*
  Close the current file in the zipfile, for fiel opened with 
    parameter raw=1 in zipOpenNewFileInZip2
  uncompressed_size and crc32 are value for the uncompressed size
*/

extern int ZEXPORT zipClose OF((zipFile file,
				const char* global_comment));
/*
  Close the zipfile
*/

#ifdef __cplusplus
}
#endif

#endif /* _zip_H */
