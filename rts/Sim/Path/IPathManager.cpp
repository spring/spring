/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IPathManager.h"
#include "Default/PathManager.h"
#include "QTPFS/PathManager.hpp"

IPathManager* pathManager = NULL;

IPathManager* IPathManager::GetInstance() {
	static IPathManager* pm = NULL;

	if (pm == NULL) {
		#ifdef DEFAULT_PFS
		pm = new CPathManager();
		#else
		pm = new QTPFS::PathManager();
		#endif
	}

	return pm;
}
