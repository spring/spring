/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _BUFFERED_ARCHIVE_H
#define _BUFFERED_ARCHIVE_H

#include "IArchive.h"
#include "System/Threading/SpringThreading.h"

/**
 * Provides a helper implementation for archive types that can only uncompress
 * one file to memory at a time.
 */
class CBufferedArchive : public IArchive
{
public:
	CBufferedArchive(const std::string& name, bool cached = true): IArchive(name) {
		noCache = !cached;
	}

	virtual ~CBufferedArchive();

	virtual int GetType() const override { return ARCHIVE_TYPE_BUF; }

	bool GetFile(unsigned int fid, std::vector<std::uint8_t>& buffer) override;

protected:
	virtual int GetFileImpl(unsigned int fid, std::vector<std::uint8_t>& buffer) = 0;

	struct FileBuffer {
		FileBuffer() = default;
		FileBuffer(const FileBuffer& fb) = delete;
		FileBuffer(FileBuffer&& fb) { *this = std::move(fb); }

		FileBuffer& operator = (const FileBuffer& fb) = delete;
		FileBuffer& operator = (FileBuffer&& fb) = default;

		bool populated = false; // files may be empty (0 bytes)
		bool exists = false;

		std::vector<std::uint8_t> data;
	};

	// indexed by file-id
	std::vector<FileBuffer> cache;
	// neither 7zip (.sd7) nor minizip (.sdz) are threadsafe
	// zlib (used to extract pool archive .gz entries) should
	// not need this, but currently each buffered GetFileImpl
	// call is protected
	spring::mutex archiveLock;

private:
	uint32_t cacheSize = 0;
	uint32_t fileCount = 0;

	bool noCache = false;
};

#endif // _BUFFERED_ARCHIVE_H
