/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef ASS_IO_H
#define ASS_IO_H

#include "lib/assimp/include/assimp/IOStream.hpp"
#include "lib/assimp/include/assimp/IOSystem.hpp"
class CFileHandler;

// Custom implementation of Assimp IOStream to support Spring's VFS
// Required because Assimp models often need to load textures from other files

class AssVFSStream : public Assimp::IOStream
{
	friend class AssVFSSystem;
	CFileHandler* file;

protected:
	// Constructor protected for private usage by AssVFSSystem
	AssVFSStream(const std::string& pFile, const std::string& pMode);

public:
	~AssVFSStream();
	size_t Read( void* pvBuffer, size_t pSize, size_t pCount);
	size_t Write( const void* pvBuffer, size_t pSize, size_t pCount);
	aiReturn Seek( size_t pOffset, aiOrigin pOrigin);
	size_t Tell() const;
	size_t FileSize() const;
	void Flush ();
};


// Spring VFS Filesystem Wrapper for Assimp

class AssVFSSystem : public Assimp::IOSystem
{
public:
	AssVFSSystem() { }
	~AssVFSSystem() { }

	// Check whether a specific file exists
	bool Exists( const char* pFile) const override;

	// Get the path delimiter character we'd like to get
	char getOsSeparator() const override;

	// open a custom stream
	Assimp::IOStream* Open( const char* pFile, const char* pMode  = "rb" ) override;

	void Close( Assimp::IOStream* pFile) override;
};

#endif // ASS_IO_H
