/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IPathManager.h"
#include "Default/PathManager.h"
#include "QTPFS/PathManager.hpp"
#include "System/Log/ILog.h"

IPathManager* pathManager = NULL;

IPathManager* IPathManager::GetInstance(unsigned int type) {
	static IPathManager* pm = NULL;

	if (pm == NULL) {
		const char* fmtStr = "[IPathManager::GetInstance] using %s path-manager";
		const char* typeStr = "";
type = PFS_TYPE_QTPFS;
		switch (type) {
			case PFS_TYPE_DEFAULT: { typeStr = "DEFAULT"; pm = new       CPathManager(); } break;
			case PFS_TYPE_QTPFS:   { typeStr = "QTPFS";   pm = new QTPFS::PathManager(); } break;
		}

		LOG(fmtStr, typeStr);
	}

	return pm;
}
