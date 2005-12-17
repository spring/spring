#ifndef __ARCHIVE_ZIP
#define __ARCHIVE_ZIP

#include "ArchiveBuffered.h"
#include "lib/minizip/unzip.h"

#ifdef _WIN32
//#define ZLIB_WINAPI this is specified in the build config, because minizip needs to have it defined as well
#define USEWIN32IOAPI
#include "System/Platform/Win/win32.h"
#include "lib/minizip/iowin32.h"
#endif

class CArchiveZip :
	public CArchiveBuffered
{
protected:
	struct FileData {
		unz_file_pos fp;
		int size;
		string origName;
	};
	unzFile zip;
	map<string, FileData> fileData;		// using unzLocateFile is quite slow
	int curSearchHandle;
	map<int, map<string, FileData>::iterator> searchHandles;
	virtual ABOpenFile_t* GetEntireFile(const string& fileName);
	void SetSlashesForwardToBack(string& name);
	void SetSlashesBackToForward(string& name);
public:
	CArchiveZip(const string& name);
	virtual ~CArchiveZip(void);
	virtual bool IsOpen();
	virtual int FindFiles(int cur, string* name, int* size);
};

#endif
