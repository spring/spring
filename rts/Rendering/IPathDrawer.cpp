/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include "IPathDrawer.h"
#include "DefaultPathDrawer.h"
#include "QTPFSPathDrawer.h"
#include "Sim/Path/IPathManager.h"
#include "Sim/Path/Default/PathManager.h"
#include "Sim/Path/QTPFS/PathManager.hpp"

IPathDrawer* pathDrawer = NULL;

IPathDrawer* IPathDrawer::GetInstance() {
	static IPathDrawer* pd = NULL;

	if (pd == NULL) {
		if (dynamic_cast<QTPFS::PathManager*>(pathManager) != NULL) {
			return (pd = new QTPFSPathDrawer());
		}
		if (dynamic_cast<CPathManager*>(pathManager) != NULL) {
			return (pd = new DefaultPathDrawer());
		}

		pd = new IPathDrawer();
	}

	return pd;
}

void IPathDrawer::FreeInstance(IPathDrawer* pd) {
	delete pd;
}
