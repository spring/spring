#include "StdAfx.h"
#include "ArchiveFactory.h"
#include "ArchiveDir.h"
#include "ArchiveHPI.h"
#include "ArchiveZip.h"
#include "Archive7Zip.h"
#include "Platform/FileSystem.h"
#include <algorithm>
#include "mmgr.h"

// Returns true if the indicated file is in fact an archive
bool CArchiveFactory::IsArchive(const std::string& fileName)
{
	std::string ext = StringToLower(filesystem.GetExtension(fileName));

	return  (ext == "sd7") || (ext == "sdz") || (ext == "sdd") ||
			(ext == "ccx") || (ext == "hpi") || (ext == "ufo") || (ext == "gp3") || (ext == "gp4") || (ext == "swx");
}

// Returns a pointer to a newly created suitable subclass of CArchiveBase
CArchiveBase* CArchiveFactory::OpenArchive(const std::string& fileName)
{
	std::string ext = StringToLower(filesystem.GetExtension(fileName));
	std::string fn = filesystem.LocateFile(fileName);

	CArchiveBase* ret = NULL;

	if (ext == "sd7")
		ret = SAFE_NEW CArchive7Zip(fn);
	else if (ext == "sdz")
		ret = SAFE_NEW CArchiveZip(fn);
	else if (ext == "sdd")
		ret = SAFE_NEW CArchiveDir(fn);
	else if ((ext == "ccx") || (ext == "hpi") || (ext == "ufo") || (ext == "gp3") || (ext == "gp4") || (ext == "swx"))
		ret = SAFE_NEW CArchiveHPI(fn);

	if (ret && ret->IsOpen())
		return ret;

	delete ret;
	return NULL;
}

CArchiveBase::~CArchiveBase() {
}
