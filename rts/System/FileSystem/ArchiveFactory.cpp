#include "StdAfx.h"
#include <algorithm>
#include "mmgr.h"

#include "ArchiveFactory.h"
#include "ArchivePool.h"
#include "ArchiveDir.h"
#include "ArchiveHPI.h"
#include "ArchiveZip.h"
#include "Archive7Zip.h"
#include "FileSystem.h"

#include "Util.h"

// Returns true if the indicated file is in fact an archive
bool CArchiveFactory::IsScanArchive(const std::string& fileName)
{
	std::string ext = StringToLower(filesystem.GetExtension(fileName));

	return  (ext == "sd7") || (ext == "sdz") || (ext == "sdd") ||
	        (ext == "ccx") || (ext == "hpi") || (ext == "ufo") ||
	        (ext == "gp3") || (ext == "gp4") || (ext == "swx") ||
		(ext == "sdp");
}


// Returns a pointer to a newly created suitable subclass of CArchiveBase
CArchiveBase* CArchiveFactory::OpenArchive(const std::string& fileName,
                                           const std::string& type)
{
	std::string ext = type;
	if (type.empty()) {
		ext = StringToLower(filesystem.GetExtension(fileName));
	}

	     if (ext == "sd7") { ext = "7z";  }
	else if (ext == "sdz") { ext = "zip"; }
	else if (ext == "sdd") { ext = "dir"; }
	else if (ext == "sdp") { ext = "pool"; }
	else if ((ext == "ccx") || (ext == "hpi") || (ext == "ufo") ||
	         (ext == "gp3") || (ext == "gp4") || (ext == "swx")) {
		ext = "hpi";
	}
	

	std::string fn = filesystem.LocateFile(fileName);

	CArchiveBase* ret = NULL;

	if (ext == "7z") {
		ret = new CArchive7Zip(fn);
	} else if (ext == "zip") {
		ret = new CArchiveZip(fn);
	} else if (ext == "dir") {
		ret = new CArchiveDir(fn);
	} else if (ext == "pool") {
		ret = new CArchivePool(fn);

	} else if (ext == "hpi") {
		ret = new CArchiveHPI(fn);
	}

	if (ret && ret->IsOpen()) {
		return ret;
	}

	delete ret;
	return NULL;
}


CArchiveBase::~CArchiveBase() {
}
