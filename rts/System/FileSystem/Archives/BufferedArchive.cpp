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

	if (fid >= cache.size())
		cache.resize(std::max(size_t(fid + 1), cache.size() * 2));

	if (!cache[fid].populated) {
		cache[fid].exists = GetFileImpl(fid, cache[fid].data);
		cache[fid].populated = true;

		cacheSize += cache[fid].data.size();
		fileCount += cache[fid].exists;
	}

	buffer = cache[fid].data;
	return cache[fid].exists;
}
