/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __ARCHIVE_FACTORY_H
#define __ARCHIVE_FACTORY_H

#include <string>
#include "ArchiveBase.h"

class CArchiveFactory
{
public:
	static bool IsScanArchive(const std::string& fileName);
	static CArchiveBase* OpenArchive(const std::string& fileName,
	                                 const std::string& type = "");
};

#endif
