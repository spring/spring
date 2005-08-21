#ifndef __ARCHIVE_7ZIP_DLL_H
#define __ARCHIVE_7ZIP_DLL_H

#include "ArchiveBuffered.h"

//#include <initguid.h>
#include "7zip/Common/StringConvert.h"
#include "7zip/7zip/Common/FileStreams.h"
#include "7zip/7zip/Archive/IArchive.h"

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
	CMyComPtr<IInArchive> archive;
	int curSearchHandle;
	map<int, map<string, FileData>::iterator> searchHandles;
	virtual ABOpenFile_t* GetEntireFile(const string& fileName);
public:
	CArchive7ZipDll(const string& name);
	virtual ~CArchive7ZipDll(void);
	virtual bool IsOpen();
	virtual int FindFiles(int cur, string* name, int* size);
};

#endif
