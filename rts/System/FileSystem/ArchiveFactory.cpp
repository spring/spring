/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "ArchiveFactory.h"

#include "ArchivePool.h"
#include "ArchiveDir.h"
#include "ArchiveZip.h"
#include "Archive7Zip.h"
#include "FileSystem.h"

#include "Util.h"

/// Returns true if the indicated file is in fact an archive
bool CArchiveFactory::IsScanArchive(const std::string& fileName)
{
	std::string ext = filesystem.GetExtension(fileName);

	return  (ext == "sd7") || (ext == "sdz") || (ext == "sdd") || (ext == "sdp");
}


/// Returns a pointer to a newly created suitable subclass of CArchiveBase
CArchiveBase* CArchiveFactory::OpenArchive(const std::string& fileName, const std::string& type)
{
	std::string ext = type;
	if (type.empty()) {
		ext = filesystem.GetExtension(fileName);
	}

	std::string fn = filesystem.LocateFile(fileName);

	CArchiveBase* ret = NULL;

	if (ext == "sd7") {
		ret = new CArchive7Zip(fn);
	} else if (ext == "sdz") {
		ret = new CArchiveZip(fn);
	} else if (ext == "sdd") {
		ret = new CArchiveDir(fn);
	} else if (ext == "sdp") {
		ret = new CArchivePool(fn);
	}

	if (ret && ret->IsOpen()) {
		return ret;
	}

	delete ret;
	return NULL;
}
