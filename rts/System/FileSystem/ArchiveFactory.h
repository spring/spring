/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _ARCHIVE_FACTORY_H
#define _ARCHIVE_FACTORY_H

#include <string>
#include "ArchiveBase.h"

class CArchiveFactory
{
public:
	/// Returns true if the indicated file is in fact an archive
	static bool IsScanArchive(const std::string& fileName);

	/// Returns a pointer to a newly created suitable subclass of CArchiveBase
	static CArchiveBase* OpenArchive(const std::string& fileName,
	                                 const std::string& type = "");
};

#endif // _ARCHIVE_FACTORY_H
