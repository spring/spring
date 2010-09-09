/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IPathManager.h"
#include "Default/PathManager.h"

IPathManager* pathManager = NULL;

IPathManager* IPathManager::GetInstance() {
	static IPathManager* pm = NULL;

	if (pm == NULL) {
		pm = new CPathManager();
	}

	return pm;
}
