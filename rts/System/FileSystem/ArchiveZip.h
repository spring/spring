/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

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

class CArchiveZip : public CArchiveBuffered
{
public:
	CArchiveZip(const std::string& name);
	virtual ~CArchiveZip(void);

	virtual bool IsOpen();

	virtual unsigned NumFiles() const;
	virtual void FileInfo(unsigned fid, std::string& name, int& size) const;
	virtual unsigned GetCrc32(unsigned fid);

protected:
	unzFile zip;

	struct FileData {
		unz_file_pos fp;
		int size;
		std::string origName;
		unsigned crc;
	};
	std::vector<FileData> fileData;
	
	virtual bool GetFileImpl(unsigned fid, std::vector<boost::uint8_t>& buffer);
};

#endif
