#ifndef __ARCHIVE_BASE_H
#define __ARCHIVE_BASE_H

// A general class for handling of file archives (such as hpi and zip files)

#include <string>

class CArchiveBase
{
public:
	CArchiveBase(const std::string& archiveName) {};
	virtual ~CArchiveBase();
	virtual bool IsOpen() = 0;
	virtual int OpenFile(const std::string& fileName) = 0;
	virtual int ReadFile(int handle, void* buffer, int numBytes) = 0;
	virtual void CloseFile(int handle) = 0;
	virtual void Seek(int handle, int pos) = 0;
	virtual int Peek(int handle) = 0;
	virtual bool Eof(int handle) = 0;
	virtual int FileSize(int handle) = 0;
	virtual int FindFiles(int cur, std::string* name, int* size) = 0;
	virtual unsigned int GetCrc32 (const std::string& fileName);
};

#endif
