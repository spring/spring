/*
---------------------------------------------------------------------------
Open Asset Import Library (ASSIMP)
---------------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team

All rights reserved.

Redistribution and use of this software in source and binary forms, 
with or without modification, are permitted provided that the following 
conditions are met:

* Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

* Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the
  following disclaimer in the documentation and/or other
  materials provided with the distribution.

* Neither the name of the ASSIMP team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the ASSIMP Development Team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
---------------------------------------------------------------------------
*/

/** @file aiFileIO.h
 *  @brief Defines generic routines to access memory-mapped files
 */

#ifndef AI_FILEIO_H_INC
#define AI_FILEIO_H_INC

#include "aiTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

struct aiFileIO;
struct aiFile;

// aiFile callbacks
typedef size_t   (*aiFileWriteProc) (C_STRUCT aiFile*,   const char*, size_t, size_t);
typedef size_t   (*aiFileReadProc)  (C_STRUCT aiFile*,   char*, size_t,size_t);
typedef size_t   (*aiFileTellProc)  (C_STRUCT aiFile*);
typedef void     (*aiFileFlushProc) (C_STRUCT aiFile*);
typedef aiReturn (*aiFileSeek)(aiFile*, size_t, aiOrigin);

// aiFileIO callbackss
typedef aiFile* (*aiFileOpenProc)  (C_STRUCT aiFileIO*, const char*, const char*);
typedef void    (*aiFileCloseProc) (C_STRUCT aiFileIO*, C_STRUCT aiFile*);

// represents user-defined data
typedef char* aiUserData;

// ----------------------------------------------------------------------------------
/** @class aiFileIO
 *  @brief Defines Assimp's way of accessing files.
 *
 *  Provided are functions to open and close files. Supply a custom structure to
 *  the import function. If you don't, a default implementation is used. Use this
 *  to enable reading from other sources, such as ZIPs or memory locations.
*/
struct aiFileIO
{
	//! Function used to open a new file
	aiFileOpenProc OpenProc;

	//! Function used to close an existing file
	aiFileCloseProc CloseProc;

	//! User-defined data
	aiUserData UserData;
};

// ----------------------------------------------------------------------------------
/** @class aiFile
 *  @brief Represents a read/write file
 *
 *  Actually, it is a data structure to wrap a set of fXXXX (e.g fopen) 
 *  replacement functions
 *
 *  The default implementation of the functions utilizes the fXXX functions from 
 *  the CRT. However, you can supply a custom implementation to Assimp by
 *  also supplying a custom aiFileIO. Use this to enable reading from other sources, 
 *  such as ZIPs or memory locations.
 */
struct aiFile
{
	//! Function used to read from a file
	aiFileReadProc ReadProc;

	//! Function used to write to a file
	aiFileWriteProc WriteProc;

	//! Function used to retrieve the current
	//! position of the file cursor (ftell())
	aiFileTellProc TellProc;

	//! Function used to retrieve the size of the file, in bytes
	aiFileTellProc FileSizeProc;

	//! Function used to set the current position
	//! of the file cursor (fseek())
	aiFileSeek SeekProc;

	//! Function used to flush the file contents
	aiFileFlushProc FlushProc;

	//! User-defined data
	aiUserData UserData;
};


#ifdef __cplusplus
}
#endif


#endif // AI_FILEIO_H_INC
