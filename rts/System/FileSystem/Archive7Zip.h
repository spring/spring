/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __ARCHIVE_7ZIP_H
#define __ARCHIVE_7ZIP_H

#include <boost/thread/mutex.hpp>
extern "C" {
#include "lib/7z/7zFile.h"
#include "lib/7z/Archive/7z/7zIn.h"
};

#include "ArchiveBase.h"

class CArchive7Zip : public CArchiveBase
{
public:
	CArchive7Zip(const std::string& name);
	virtual ~CArchive7Zip(void);

	virtual bool IsOpen();
	
	virtual unsigned NumFiles() const;
	virtual bool GetFile(unsigned fid, std::vector<boost::uint8_t>& buffer);
	virtual void FileInfo(unsigned fid, std::string& name, int& size) const;
	virtual bool HasLowReadingCost(unsigned fid) const;
	virtual unsigned GetCrc32(unsigned fid);

private:
	boost::mutex archiveLock;
	UInt32 blockIndex;
	Byte *outBuffer;
	size_t outBufferSize;

	struct FileData
	{
		int fp;
		/**
		 * Real/unpacked size of the file in bytes.
		 * @see #unpackSize
		 */
		int size;
		std::string origName;
		unsigned int crc;
		/**
		 * How many bytes have to be unpacked to get to this file.
		 * This either equal to size, or larger, if there are other files
		 * in the same solid block.
		 * @see #size
		 */
		int unpackSize;
	};
	std::vector<FileData> fileData;

	CFileInStream archiveStream;
	CSzArEx db;
	CLookToRead lookStream;
	ISzAlloc allocImp;
	ISzAlloc allocTempImp;

	bool isOpen;
};

#endif
