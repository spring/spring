/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/StdAfx.h"

#include "BufferedArchive.h"


CBufferedArchive::CBufferedArchive(const std::string& name)
	: IArchive(name)
{
}

CBufferedArchive::~CBufferedArchive()
{
}

bool CBufferedArchive::GetFile(unsigned int fid, std::vector<boost::uint8_t>& buffer)
{
	boost::mutex::scoped_lock lck(archiveLock);
	assert(IsFileId(fid));

	if (fid >= cache.size()) {
		cache.resize(fid + 1);
	}
	
	if (!cache[fid].populated) {
		cache[fid].exists = GetFileImpl(fid, cache[fid].data);
		cache[fid].populated = true;
	}

	buffer = cache[fid].data;
	return cache[fid].exists;
}
