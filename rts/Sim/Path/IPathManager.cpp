/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IPathManager.h"
#include "Default/PathManager.h"
#include "QTPFS/PathManager.hpp"

IPathManager* pathManager = NULL;

IPathManager* IPathManager::GetInstance(unsigned int type) {
	static IPathManager* pm = NULL;

	if (pm == NULL) {
		switch (type) {
			case PM_TYPE_DEFAULT: { pm = new       CPathManager(); } break;
			case PM_TYPE_QTPFS:   { pm = new QTPFS::PathManager(); } break;
		}
	}

	return pm;
}
