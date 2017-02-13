/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IPathManager.h"
#include "Default/PathManager.h"
#include "QTPFS/PathManager.hpp"
#include "System/Log/ILog.h"

IPathManager* pathManager = nullptr;

IPathManager* IPathManager::GetInstance(unsigned int type) {
	if (pathManager == nullptr) {
		const char* fmtStr = "[IPathManager::GetInstance] using %s path-manager";
		const char* typeStr = "";

		switch (type) {
			case PFS_TYPE_DEFAULT: { typeStr = "DEFAULT"; pathManager = new       CPathManager(); } break;
			case PFS_TYPE_QTPFS:   { typeStr = "QTPFS";   pathManager = new QTPFS::PathManager(); } break;
		}

		LOG(fmtStr, typeStr);
	}

	return pathManager;
}

void IPathManager::FreeInstance(IPathManager* pm) {
	assert(pm == pathManager);
	delete pm;
	pathManager = nullptr;
}

