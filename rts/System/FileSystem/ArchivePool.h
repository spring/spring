#ifndef __ARCHIVE_POOL
#define __ARCHIVE_POOL

#include "ArchiveBuffered.h"
#include <vector>

#ifndef _ZLIB_H
#include "zlib.h"
#endif

class CArchivePool :
	public CArchiveBuffered
{
protected:
	struct FileData {
		std::string hex;
		std::string name;
		unsigned int size;
	};

	bool isOpen;
	std::vector<FileData> files;
	std::map<std::string, FileData> fileMap;

	virtual ABOpenFile_t* GetEntireFile(const std::string& fileName);
public:
	CArchivePool(const std::string& name);
	virtual ~CArchivePool(void);
	virtual bool IsOpen();
	virtual int FindFiles(int cur, std::string* name, int* size);
//	virtual unsigned int GetCrc32 (const std::string& fileName);
};

#endif
