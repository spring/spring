/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_PATHDRAWER_HDR
#define QTPFS_PATHDRAWER_HDR

#include "IPathDrawer.h"

struct QTPFSPathDrawer: public IPathDrawer {
public:
	QTPFSPathDrawer() {}

	void Draw() const {}
	void UpdateExtraTexture(int, int, int, int, unsigned char*) const {}
};

#endif
