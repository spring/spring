/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _BUFFERED_ARCHIVE_H
#define _BUFFERED_ARCHIVE_H

#include <map>
#include <boost/thread/mutex.hpp>

#include "IArchive.h"

/**
 * Provides a helper implementation for archive types that can only uncompress
 * one file to memory at a time.
 */
class CBufferedArchive : public IArchive
{
public:
	CBufferedArchive(const std::string& name, bool cache = true);
	virtual ~CBufferedArchive();

	virtual bool GetFile(unsigned int fid, std::vector<std::uint8_t>& buffer);

protected:
	virtual bool GetFileImpl(unsigned int fid, std::vector<std::uint8_t>& buffer) = 0;

	boost::mutex archiveLock; // neither 7zip nor zlib are threadsafe
	struct FileBuffer
	{
		FileBuffer() : populated(false), exists(false) {};
		bool populated; // cause a file may be 0 bytes big
		bool exists;
		std::vector<std::uint8_t> data;
	};
	std::vector<FileBuffer> cache; // cache[fileId]
private:
	bool caching;
};

#endif // _BUFFERED_ARCHIVE_H
