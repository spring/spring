/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "BufferedArchive.h"
#include "System/GlobalConfig.h"

#include <cassert>

bool CBufferedArchive::GetFile(unsigned int fid, std::vector<std::uint8_t>& buffer)
{
	std::lock_guard<spring::mutex> lck(archiveLock);
	assert(IsFileId(fid));

	if (noCache)
		return GetFileImpl(fid, buffer);
	#ifndef UNITSYNC
	// engine-only
	if (!globalConfig.vfsCacheArchiveFiles)
		return GetFileImpl(fid, buffer);
	#endif

	if (fid >= cache.size())
		cache.resize(std::max(size_t(fid + 1), cache.size() * 2));

	if (!cache[fid].populated) {
		cache[fid].exists = GetFileImpl(fid, cache[fid].data);
		cache[fid].populated = true;
	}

	buffer = cache[fid].data;
	return cache[fid].exists;
}
