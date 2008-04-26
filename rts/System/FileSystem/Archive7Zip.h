#ifndef __ARCHIVE_7ZIP_H
#define __ARCHIVE_7ZIP_H

#include "ArchiveBuffered.h"

extern "C" {
#include "lib/7zip/7zCrc.h"
#include "lib/7zip/7zIn.h"
#include "lib/7zip/7zExtract.h"
};

typedef struct _CFileInStream
{
	ISzInStream InStream;
	FILE *File;
} CFileInStream;

class CArchive7Zip :
	public CArchiveBuffered
{
protected:
	struct FileData {
		int fp;
		int size;
		std::string origName;
		unsigned int crc;
	};
	std::map<std::string, FileData> fileData;

	int curSearchHandle;
	std::map<int, std::map<std::string, FileData>::iterator> searchHandles;

	CFileInStream archiveStream;
	CArchiveDatabaseEx db;
	ISzAlloc allocImp;
	ISzAlloc allocTempImp;

	bool isOpen;
	virtual ABOpenFile_t* GetEntireFile(const std::string& fName);
	void SetSlashesForwardToBack(std::string& name);
public:
	CArchive7Zip(const std::string& name);
	virtual ~CArchive7Zip(void);
	virtual bool IsOpen();
	virtual int FindFiles(int cur, std::string* name, int* size);
	virtual unsigned int GetCrc32 (const std::string& fileName);
};

#endif
