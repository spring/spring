#ifndef __ARCHIVE_7ZIP_H
#define __ARCHIVE_7ZIP_H

#include "ArchiveBuffered.h"

extern "C" {
#include "lib/7z/7zFile.h"
#include "lib/7z/Archive/7z/7zIn.h"
};

class CArchive7Zip : public CArchiveBuffered
{
public:
	CArchive7Zip(const std::string& name);
	virtual ~CArchive7Zip(void);
	virtual bool IsOpen();
	virtual int FindFiles(int cur, std::string* name, int* size);
	virtual unsigned int GetCrc32 (const std::string& fileName);

private:
	UInt32 blockIndex;
	Byte *outBuffer;
	size_t outBufferSize;

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
	CSzArEx db;
	CLookToRead lookStream;
	ISzAlloc allocImp;
	ISzAlloc allocTempImp;

	bool isOpen;
	virtual FileBuffer* GetEntireFileImpl(const std::string& fName);
};

#endif
