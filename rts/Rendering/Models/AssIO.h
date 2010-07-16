#ifndef ASSIO_H
#define ASSIO_H

#include "IOStream.h"
#include "IOSystem.h"
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
	~AssVFSStream(void);
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
	bool Exists( const char* pFile) const;

	// Get the path delimiter character we'd like to get
	char getOsSeparator() const;
	bool ComparePaths (const std::string& one, const std::string& second) const;

	// open a custom stream
	Assimp::IOStream* Open( const char* pFile, const char* pMode  = "rb" );

	void Close( Assimp::IOStream* pFile);
};

#endif
