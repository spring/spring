/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "BufferedArchive.h"
#include "System/GlobalConfig.h"
#include "System/Log/ILog.h"

#include <cassert>

CBufferedArchive::~CBufferedArchive()
{
	// filter archives for which only {map,mod}info.lua was accessed
	if (cacheSize <= 1 || fileCount <= 1)
		return;

	LOG_L(L_INFO, "[%s][name=%s] %u bytes cached in %u files", __func__, archiveFile.c_str(), cacheSize, fileCount);
}

bool CBufferedArchive::GetFile(unsigned int fid, std::vector<std::uint8_t>& buffer)
{
	std::lock_guard<spring::mutex> lck(archiveLock);
	assert(IsFileId(fid));

	if (noCache)
		return GetFileImpl(fid, buffer);
	// engine-only
	if (!globalConfig.vfsCacheArchiveFiles)
		return GetFileImpl(fid, buffer);

	// NumFiles is virtual, can't do this in ctor
	if (cache.empty())
		cache.resize(NumFiles());

	FileBuffer& fb = cache.at(fid);

	if (!fb.populated) {
		fb.exists = GetFileImpl(fid, fb.data);
		fb.populated = true;

		cacheSize += fb.data.size();
		fileCount += fb.exists;
	}

	// TODO: zero-copy access
	buffer = fb.data;
	return fb.exists;
}
