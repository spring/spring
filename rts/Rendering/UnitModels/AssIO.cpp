#include "StdAfx.h"

#include <string>

#include "AssIO.h"

#include "FileSystem/FileHandler.h"

AssVFSStream::AssVFSStream(const std::string& pFile, const std::string& pMode)
{
	file = new CFileHandler(pFile, pMode);
}

AssVFSStream::~AssVFSStream(void)
{
	delete file;
}

size_t AssVFSStream::Read( void* pvBuffer, size_t pSize, size_t pCount)
{
	// Spring VFS only supports reading chars. Need to convert.
	int length = pSize * pCount;
	return file->Read(pvBuffer, length);
}

size_t AssVFSStream::Write( const void* pvBuffer, size_t pSize, size_t pCount)
{
	// FAKE. Shouldn't need to write back to VFS
	return pSize * pCount;
}

aiReturn AssVFSStream::Seek( size_t pOffset, aiOrigin pOrigin)
{
	int pos;
	switch(pOrigin){
		case aiOrigin_SET: // from start of file
			pos = (int)pOffset; break;
		case aiOrigin_CUR: // from current position in file
			pos = file->GetPos() + (int)pOffset; break;
		case aiOrigin_END: // reverse from end of file
			pos = file->FileSize() + (int)pOffset; break;
	}
	file->Seek( pos );
	return AI_SUCCESS;
}

size_t AssVFSStream::Tell() const
{
	return file->GetPos();
}


size_t AssVFSStream::FileSize() const
{
	return file->FileSize();
}

void AssVFSStream::Flush () // FAKE
{
}


// Check whether a specific file exists
bool AssVFSSystem::Exists( const char* pFile) const
{
	CFileHandler file(pFile);
	return file.FileExists();
}

// Get the path delimiter character we'd like to get
char AssVFSSystem::getOsSeparator() const
{
	return '/';
}

bool AssVFSSystem::ComparePaths (const std::string& one, const std::string& second) const
{
	return one == second; // too naive? probably should convert to absolute paths
}

// open a custom stream
Assimp::IOStream* AssVFSSystem::Open( const char* pFile, const char* pMode)
{
	return new AssVFSStream( pFile, pMode );
}

void AssVFSSystem::Close( Assimp::IOStream* pFile)
{
	delete pFile;
}
