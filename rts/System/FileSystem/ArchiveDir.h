/* Author: Tobi Vollebregt */

#ifndef ARCHIVEDIR_H
#define ARCHIVEDIR_H

#include "ArchiveBase.h"
#include "FileHandler.h"
#include <map>

/** Archive implementation which falls back to the regular filesystem.
ie. a directory and all it's contents is treated as an archive by this class. */
class CArchiveDir : public CArchiveBase
{
	std::string archiveName;

	int curFileHandle;
	std::map<int, CFileHandler*> fileHandles;

	std::vector<std::string> searchFiles;
	int curSearchHandle;
	std::map<int, std::vector<std::string>::iterator> searchHandles;

	std::map<std::string, std::string> lcNameToOrigName;

	CFileHandler* GetFileHandler(int handle);
	std::vector<std::string>::iterator& GetSearchHandle(int handle);

public:

	CArchiveDir(const std::string& archiveName);
	virtual ~CArchiveDir(void);
	virtual bool IsOpen();
	virtual int OpenFile(const std::string& fileName);
	virtual int ReadFile(int handle, void* buffer, int numBytes);
	virtual void CloseFile(int handle);
	virtual void Seek(int handle, int pos);
	virtual int Peek(int handle);
	virtual bool Eof(int handle);
	virtual int FileSize(int handle);
	virtual int FindFiles(int cur, std::string* name, int* size);
};

#endif
