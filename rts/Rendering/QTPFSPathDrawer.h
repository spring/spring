/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_PATHDRAWER_HDR
#define QTPFS_PATHDRAWER_HDR

#include "IPathDrawer.h"

class CVertexArray;

namespace QTPFS {
	class QTNode;
};

struct QTPFSPathDrawer: public IPathDrawer {
public:
	QTPFSPathDrawer();

	void DrawAll() const;
	void UpdateExtraTexture(int, int, int, int, unsigned char*) const {}

private:
	void Draw() const;
	void DrawNodeTree(const QTPFS::QTNode*, CVertexArray*) const;
};

#endif

