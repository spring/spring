#ifndef __ARCHIVE_BUFFERED_H
#define __ARCHIVE_BUFFERED_H

#include <map>
#include <boost/thread/mutex.hpp>

#include "ArchiveBase.h"

struct ABOpenFile_t {
	int size;
	int pos;
	char* data;
};

// Provides a helper implementation for archive types that can only uncompress one file to
// memory at a time
class CArchiveBuffered :
	public CArchiveBase
{
protected:
	boost::mutex archiveLock; // neither 7zip nor zlib are threadsafe
	int curFileHandle;
	std::map<int, ABOpenFile_t*> fileHandles;
	ABOpenFile_t* GetEntireFile(const std::string& fileName)
	{
		boost::mutex::scoped_lock lock(archiveLock);
		return GetEntireFileImpl(fileName);
	};
	virtual ABOpenFile_t* GetEntireFileImpl(const std::string& fileName) = 0;
	ABOpenFile_t* GetOpenFile(int handle);

public:
	CArchiveBuffered(const std::string& name);
	virtual ~CArchiveBuffered(void);
	virtual int OpenFile(const std::string& fileName);
	virtual int ReadFile(int handle, void* buffer, int numBytes);
	virtual void CloseFile(int handle);
	virtual void Seek(int handle, int pos);
	virtual int Peek(int handle);
	virtual bool Eof(int handle);
	virtual int FileSize(int handle);
};

#endif
