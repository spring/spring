/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_PATHDRAWER_HDR
#define QTPFS_PATHDRAWER_HDR

#include "IPathDrawer.h"

struct MoveData;
class CMoveMath;
class CVertexArray;

namespace QTPFS {
	struct QTNode;
	struct IPath;
};

struct QTPFSPathDrawer: public IPathDrawer {
public:
	QTPFSPathDrawer();

	void DrawAll() const;
	void UpdateExtraTexture(int, int, int, int, unsigned char*) const {}

private:
	void DrawNodeTree(const MoveData* md) const;
	void DrawNodeTreeRec(
		const QTPFS::QTNode* nt,
		const MoveData* md,
		const CMoveMath* mm,
		CVertexArray* va
	) const;
	void DrawPaths(const MoveData* md) const;
	void DrawPath(const QTPFS::IPath* path, CVertexArray* va) const;
};

#endif

