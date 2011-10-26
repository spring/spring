/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_PATHDRAWER_HDR
#define QTPFS_PATHDRAWER_HDR

#include <list>
#include "IPathDrawer.h"

struct MoveData;
class CMoveMath;
class CVertexArray;

namespace QTPFS {
	struct QTNode;
	struct IPath;
	struct PathSearch;

	namespace PathSearchTrace {
		struct Execution;
	};
};

struct QTPFSPathDrawer: public IPathDrawer {
public:
	QTPFSPathDrawer();

	void DrawAll() const;
	void UpdateExtraTexture(int, int, int, int, unsigned char*) const {}

private:
	enum {
		COLOR_BIT_R = 1,
		COLOR_BIT_G = 2,
		COLOR_BIT_B = 4,
	};

	void DrawNodeTree(const MoveData* md) const;
	void DrawNodeTreeRec(
		const QTPFS::QTNode* nt,
		const MoveData* md,
		const CMoveMath* mm,
		CVertexArray* va
	) const;

	void GetVisibleNodes(const QTPFS::QTNode* nt, std::list<const QTPFS::QTNode*>& nodes) const;

	void DrawPaths(const MoveData* md) const;
	void DrawPath(const QTPFS::IPath* path, CVertexArray* va) const;
	void DrawSearchExecution(unsigned int pathType, const QTPFS::PathSearchTrace::Execution* searchExec) const;
	void DrawSearchIteration(unsigned int pathType, const std::list<unsigned int>& nodeIndices, CVertexArray* va) const;
	void DrawNode(
		const QTPFS::QTNode* node,
		const MoveData* md,
		const CMoveMath* mm,
		CVertexArray* va,
		bool fillQuad,
		bool showCost,
		bool batchDraw
	) const;
	void DrawNodeLink(const QTPFS::QTNode* pushedNode, const QTPFS::QTNode* poppedNode, CVertexArray* va) const;
};

#endif

