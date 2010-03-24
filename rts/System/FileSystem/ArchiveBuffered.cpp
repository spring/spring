/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"

#include "ArchiveBuffered.h"


CArchiveBuffered::CArchiveBuffered(const std::string& name) : CArchiveBase(name)
{
}

CArchiveBuffered::~CArchiveBuffered(void)
{
}

bool CArchiveBuffered::GetFile(unsigned fid, std::vector<boost::uint8_t>& buffer)
{
	boost::mutex::scoped_lock lck(archiveLock);
	assert(fid >= 0 && fid < NumFiles());

	if (fid >= cache.size())
		cache.resize(fid+1);
	
	if (!cache[fid].populated)
	{
		cache[fid].exists = GetFileImpl(fid, cache[fid].data);
		cache[fid].populated = true;
	}

	buffer = cache[fid].data;
	return cache[fid].exists;
}
