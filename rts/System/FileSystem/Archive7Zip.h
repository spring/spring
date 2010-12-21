/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _ARCHIVE_7ZIP_H
#define _ARCHIVE_7ZIP_H

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
	virtual ~CArchive7Zip();

	virtual bool IsOpen();
	
	virtual unsigned NumFiles() const;
	virtual bool GetFile(unsigned fid, std::vector<boost::uint8_t>& buffer);
	virtual void FileInfo(unsigned fid, std::string& name, int& size) const;
	virtual bool HasLowReadingCost(unsigned fid) const;
	virtual unsigned GetCrc32(unsigned fid);

private:
	boost::mutex archiveLock;
	UInt32 blockIndex;
	Byte* outBuffer;
	size_t outBufferSize;

	/**
	 * How much more unpacked data may be allowed in a solid block,
	 * besides a meta-file.
	 * @see FileData#size
	 * @see FileData#unpackedSize
	 */
	static const size_t COST_LIMIT_UNPACK_OVERSIZE;
	/**
	 * Maximum allowed packed data allowed in a solid block
	 * that contains a meta-file.
	 * This is only checked for if the unpack-oversize limit was not met.
	 * @see FileData#size
	 * @see FileData#unpackedSize
	 */
	static const size_t COST_LIMIT_DISC_READ;

	struct FileData
	{
		int fp;
		/**
		 * Real/unpacked size of the file in bytes.
		 * @see #unpackedSize
		 * @see #packSize
		 */
		int size;
		std::string origName;
		unsigned int crc;
		/**
		 * How many bytes of files have to be unpacked to get to this file.
		 * This either equal to size, or is larger, if there are other files
		 * in the same solid block.
		 * @see #size
		 * @see #packedSize
		 */
		int unpackedSize;
		/**
		 * How many bytes of the archive have to be read
		 * from disc to get to this file.
		 * This may be smaller or larger then size,
		 * and is smaller then or equal to unpackedSize.
		 * @see #size
		 * @see #unpackedSize
		 */
		int packedSize;
	};
	std::vector<FileData> fileData;

	CFileInStream archiveStream;
	CSzArEx db;
	CLookToRead lookStream;
	ISzAlloc allocImp;
	ISzAlloc allocTempImp;

	bool isOpen;
};

#endif // _ARCHIVE_7ZIP_H
