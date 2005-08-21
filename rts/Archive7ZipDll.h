#ifndef __ARCHIVE_7ZIP_DLL_H
#define __ARCHIVE_7ZIP_DLL_H

#include "ArchiveBuffered.h"

extern "C" {
#include "7zip/7zExtract.h"
}

struct _CFileInStream
{
	ISzInStream InStream;
	FILE *File;
};

class CArchive7ZipDll :
	public CArchiveBuffered
{
protected:
	struct FileData {
		int index;
		int size;
		string origName;
	};
	map<string, FileData> fileData;		
	int curSearchHandle;
	map<int, map<string, FileData>::iterator> searchHandles;
	virtual ABOpenFile_t* GetEntireFile(const string& fileName);
public:
	CArchive7ZipDll(const string& name);
	virtual ~CArchive7ZipDll(void);
	virtual bool IsOpen();
	virtual int FindFiles(int cur, string* name, int* size);
	struct _CFileInStream archiveStream;
	CArchiveDatabaseEx db;
	ISzAlloc allocImp;
	ISzAlloc allocTempImp;
};

#endif
