/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _7ZIP_ARCHIVE_H
#define _7ZIP_ARCHIVE_H

extern "C" {
#include "lib/7z/7zFile.h"
#include "lib/7z/7z.h"
}

#include "IArchiveFactory.h"
#include "BufferedArchive.h"
#include <vector>
#include <string>
#include "IArchive.h"

/**
 * Creates LZMA/7zip compressed, single-file archives.
 * @see CSevenZipArchive
 */
class CSevenZipArchiveFactory : public IArchiveFactory {
public:
	CSevenZipArchiveFactory(): IArchiveFactory("sd7") {}
	bool CheckForSolid() const { return true; }
private:
	IArchive* DoCreateArchive(const std::string& filePath) const override;
};


/**
 * An LZMA/7zip compressed, single-file archive.
 */
class CSevenZipArchive : public CBufferedArchive
{
public:
	CSevenZipArchive(const std::string& name);
	virtual ~CSevenZipArchive();

	int GetType() const override { return ARCHIVE_TYPE_SD7; }

	bool IsOpen() override { return isOpen; }
	bool HasLowReadingCost(unsigned int fid) const override;

	unsigned int NumFiles() const override { return (fileEntries.size()); }
	int GetFileImpl(unsigned int fid, std::vector<std::uint8_t>& buffer) override;
	void FileInfo(unsigned int fid, std::string& name, int& size) const override;

	#if 0
	unsigned GetCrc32(unsigned int fid) {
		assert(IsFileId(fid));
		return fileEntries[fid].crc;
	}
	#endif

private:
	int GetFileName(const CSzArEx* db, int i);

private:
	/**
	 * How much more unpacked data may be allowed in a solid block,
	 * besides a meta-file.
	 * @see FileEntry#size
	 * @see FileEntry#unpackedSize
	 */
	static constexpr size_t COST_LIMIT_UNPACK_OVERSIZE = 32 * 1024;
	/**
	 * Maximum allowed packed data allowed in a solid block
	 * that contains a meta-file.
	 * This is only checked for if the unpack-oversize limit was not met.
	 * @see FileEntry#size
	 * @see FileEntry#unpackedSize
	 */
	static constexpr size_t COST_LIMIT_DISK_READ = 32 * 1024;

	// actual data is in BufferedArchive
	struct FileEntry {
		int fp;
		/**
		 * Real/unpacked size of the file in bytes.
		 * @see #unpackedSize
		 * @see #packedSize
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

	std::vector<FileEntry> fileEntries;

	UInt32 blockIndex = 0xFFFFFFFF;
	size_t outBufferSize = 0;

	Byte* outBuffer = nullptr;
	// used for file names
	UInt16 tempBuffer[2048];

	CFileInStream archiveStream;
	CSzArEx db;
	CLookToRead lookStream;
	ISzAlloc allocImp;
	ISzAlloc allocTempImp;

	bool isOpen = false;
};

#endif // _7ZIP_ARCHIVE_H
