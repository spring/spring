/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "BufferedArchive.h"
#include "System/GlobalConfig.h"
#include "System/MainDefines.h"
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

	int ret = 0;

	if (noCache) {
		if ((ret = GetFileImpl(fid, buffer)) != 1)
			LOG_L(L_WARNING, "[BufferedArchive::%s(fid=%u)][noCache] name=%s ret=%d size=" _STPF_, __func__, fid, archiveFile.c_str(), ret, buffer.size());

		return (ret == 1);
	}

	// engine-only
	if (!globalConfig.vfsCacheArchiveFiles) {
		if ((ret = GetFileImpl(fid, buffer)) != 1)
			LOG_L(L_WARNING, "[BufferedArchive::%s(fid=%u)][!vfsCache] name=%s ret=%d size=" _STPF_, __func__, fid, archiveFile.c_str(), ret, buffer.size());

		return (ret == 1);
	}

	// NumFiles is virtual, can't do this in ctor
	if (cache.empty())
		cache.resize(NumFiles());

	FileBuffer& fb = cache.at(fid);

	if (!fb.populated) {
		fb.exists = ((ret = GetFileImpl(fid, fb.data)) == 1);
		fb.populated = true;

		cacheSize += fb.data.size();
		fileCount += fb.exists;
	}

	if (!fb.exists) {
		LOG_L(L_WARNING, "[BufferedArchive::%s(fid=%u)][!fb.exists] name=%s ret=%d size=" _STPF_, __func__, fid, archiveFile.c_str(), ret, fb.data.size());
		return false;
	}

	if (buffer.size() != fb.data.size())
		buffer.resize(fb.data.size());

	// TODO: zero-copy access
	std::copy(fb.data.begin(), fb.data.end(), buffer.begin());
	return true;
}
