/*
Open Asset Import Library (ASSIMP)
----------------------------------------------------------------------

Copyright (c) 2006-2010, ASSIMP Development Team
All rights reserved.

Redistribution and use of this software in source and binary forms, 
with or without modification, are permitted provided that the 
following conditions are met:

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

----------------------------------------------------------------------
*/

/** @file Default file I/O using fXXX()-family of functions */
#ifndef AI_DEFAULTIOSTREAM_H_INC
#define AI_DEFAULTIOSTREAM_H_INC

#include <stdio.h>
#include "../include/IOStream.h"

namespace Assimp	{

// ----------------------------------------------------------------------------------
//!	@class	DefaultIOStream
//!	@brief	Default IO implementation, use standard IO operations
//! @note   An instance of this class can exist without a valid file handle
//!         attached to it. All calls fail, but the instance can nevertheless be
//!         used with no restrictions.
class DefaultIOStream : public IOStream
{
	friend class DefaultIOSystem;

protected:
	DefaultIOStream ();
	DefaultIOStream (FILE* pFile, const std::string &strFilename);

public:
	/** Destructor public to allow simple deletion to close the file. */
	~DefaultIOStream ();

	// -------------------------------------------------------------------
	// Read from stream
    size_t Read(void* pvBuffer, 
		size_t pSize, 
		size_t pCount);


	// -------------------------------------------------------------------
	// Write to stream
    size_t Write(const void* pvBuffer, 
		size_t pSize,
		size_t pCount);

	// -------------------------------------------------------------------
	// Seek specific position
	aiReturn Seek(size_t pOffset,
		aiOrigin pOrigin);

	// -------------------------------------------------------------------
	// Get current seek position
    size_t Tell() const;

	// -------------------------------------------------------------------
	// Get size of file
	size_t FileSize() const;

	// -------------------------------------------------------------------
	// Flush file contents
	void Flush();

private:
	//!	File datastructure, using clib
	FILE* mFile;
	//!	Filename
	std::string	mFilename;

	//! Cached file size
	mutable size_t cachedSize;
};


// ----------------------------------------------------------------------------------
inline DefaultIOStream::DefaultIOStream () : 
	mFile		(NULL), 
	mFilename	(""),
	cachedSize	(0xffffffff)
{
	// empty
}


// ----------------------------------------------------------------------------------
inline DefaultIOStream::DefaultIOStream (FILE* pFile, 
		const std::string &strFilename) :
	mFile(pFile), 
	mFilename(strFilename),
	cachedSize	(0xffffffff)
{
	// empty
}
// ----------------------------------------------------------------------------------

} // ns assimp

#endif //!!AI_DEFAULTIOSTREAM_H_INC

