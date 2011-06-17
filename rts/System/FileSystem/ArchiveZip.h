/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _ARCHIVE_ZIP
#define _ARCHIVE_ZIP

#include "ArchiveBuffered.h"
#include "lib/minizip/unzip.h"

#ifdef _WIN32
// This is specified in the build config,
// because minizip needs to have it defined as well.
//#define ZLIB_WINAPI
#define USEWIN32IOAPI
#include "Platform/Win/win32.h"
#include "lib/minizip/iowin32.h"
#endif

#include <string>
#include <vector>

class CArchiveZip : public CArchiveBuffered
{
public:
	CArchiveZip(const std::string& archiveName);
	virtual ~CArchiveZip();

	virtual bool IsOpen();

	virtual unsigned int NumFiles() const;
	virtual void FileInfo(unsigned int fid, std::string& name, int& size) const;
	virtual unsigned int GetCrc32(unsigned int fid);

protected:
	unzFile zip;

	struct FileData {
		unz_file_pos fp;
		int size;
		std::string origName;
		unsigned int crc;
	};
	std::vector<FileData> fileData;
	
	virtual bool GetFileImpl(unsigned int fid, std::vector<boost::uint8_t>& buffer);
};

#endif // _ARCHIVE_ZIP
