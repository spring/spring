#include "StdAfx.h"
#include "ArchiveFactory.h"
#include "ArchiveHPI.h"
#include "ArchiveZip.h"
#ifndef NO_7ZIP
#include "Archive7ZipDll.h"
#endif
#include <algorithm>
//#include "mmgr.h"

// Returns true if the indicated file is in fact an archive
bool CArchiveFactory::IsArchive(const string& fileName)
{
	return true;
}

// Returns a pointer to a newly created suitable subclass of CArchiveBase
CArchiveBase* CArchiveFactory::OpenArchive(const string& fileName)
{
	string ext = fileName.substr(fileName.find_last_of('.') + 1);
	transform(ext.begin(), ext.end(), ext.begin(), (int (*)(int))tolower);

	CArchiveBase* ret = NULL;

#ifndef NO_7ZIP
	if (ext == "sd7")
		ret = new CArchive7ZipDll(fileName);
	else
#endif
	if (ext == "sdz")
		ret = new CArchiveZip(fileName);
	else if ((ext == "ccx") || (ext == "hpi") || (ext == "ufo") || (ext == "gp3") || (ext == "gp4") || (ext == "swx"))
		ret = new CArchiveHPI(fileName);

	if (!ret)
		return NULL;

	if (!ret->IsOpen()) {
		delete ret;
		return NULL;
	}

	return ret;
}
