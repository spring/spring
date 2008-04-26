#ifndef __ARCHIVE_ZIP
#define __ARCHIVE_ZIP

#include "ArchiveBuffered.h"
#include "lib/minizip/unzip.h"

#ifdef _WIN32
//#define ZLIB_WINAPI this is specified in the build config, because minizip needs to have it defined as well
#define USEWIN32IOAPI
#include "Platform/Win/win32.h"
#include "lib/minizip/iowin32.h"
#endif

class CArchiveZip :
	public CArchiveBuffered
{
protected:
	struct FileData {
		unz_file_pos fp;
		int size;
		std::string origName;
		unsigned int crc;
	};
	unzFile zip;
	std::map<std::string, FileData> fileData;		// using unzLocateFile is quite slow
	int curSearchHandle;
	std::map<int, std::map<std::string, FileData>::iterator> searchHandles;
	virtual ABOpenFile_t* GetEntireFile(const std::string& fileName);
	void SetSlashesForwardToBack(std::string& name);
	void SetSlashesBackToForward(std::string& name);
public:
	CArchiveZip(const std::string& name);
	virtual ~CArchiveZip(void);
	virtual bool IsOpen();
	virtual int FindFiles(int cur, std::string* name, int* size);
	virtual unsigned int GetCrc32 (const std::string& fileName);
};

#endif
