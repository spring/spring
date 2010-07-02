/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __ARCHIVE_BUFFERED_H
#define __ARCHIVE_BUFFERED_H

#include <map>
#include <boost/thread/mutex.hpp>

#include "ArchiveBase.h"

// Provides a helper implementation for archive types that can only uncompress one file to
// memory at a time
class CArchiveBuffered : public CArchiveBase
{
public:
	CArchiveBuffered(const std::string& name);
	virtual ~CArchiveBuffered(void);

	virtual bool GetFile(unsigned fid, std::vector<boost::uint8_t>& buffer);

protected:
	virtual bool GetFileImpl(unsigned fid, std::vector<boost::uint8_t>& buffer) = 0;

	boost::mutex archiveLock; // neither 7zip nor zlib are threadsafe
	struct FileBuffer
	{
		FileBuffer() : populated(false), exists(false) {};
		bool populated; // cause a file may be 0 bytes big
		bool exists;
		std::vector<boost::uint8_t> data;
	};
	std::vector<FileBuffer> cache; // cache[fileId]
};

#endif
