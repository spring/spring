/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IPathManager.h"
#include "Default/PathManager.h"
#include "QTPFS/PathManager.hpp"
#include "System/Log/ILog.h"

IPathManager* pathManager = nullptr;

IPathManager* IPathManager::GetInstance(unsigned int type) {
	if (pathManager == nullptr) {
		const char* fmtStr = "[IPathManager::%s] using %sPFS";
		const char* typeStr = "";

		switch (type) {
			case NOPFS_TYPE: { typeStr = "NO"; pathManager = new       IPathManager(); } break;
			case HAPFS_TYPE: { typeStr = "HA"; pathManager = new       CPathManager(); } break;
			case QTPFS_TYPE: { typeStr = "QT"; pathManager = new QTPFS::PathManager(); } break;
		}

		LOG(fmtStr, __func__, typeStr);
	}

	return pathManager;
}

void IPathManager::FreeInstance(IPathManager* pm) {
	assert(pm == pathManager);
	delete pm;
	pathManager = nullptr;
}

