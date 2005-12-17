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
		string origName;
	};
	map<string, FileData> fileData;

	int curSearchHandle;
	map<int, map<string, FileData>::iterator> searchHandles;

	CFileInStream archiveStream;
	CArchiveDatabaseEx db;
	ISzAlloc allocImp;
	ISzAlloc allocTempImp;

	bool isOpen;
	virtual ABOpenFile_t* GetEntireFile(const string& fName);
	void SetSlashesForwardToBack(string& name);
public:
	CArchive7Zip(const string& name);
	virtual ~CArchive7Zip(void);
	virtual bool IsOpen();
	virtual int FindFiles(int cur, string* name, int* size);
};

#endif
