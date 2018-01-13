/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_PATHDRAWER_HDR
#define QTPFS_PATHDRAWER_HDR

#include <vector>

#include "IPathDrawer.h"
#include "Rendering/GL/RenderDataBufferFwd.hpp"

struct MoveDef;

namespace QTPFS {
	class PathManager;

	struct QTNode;
	struct IPath;
	struct PathSearch;

	namespace PathSearchTrace {
		struct Execution;
	}
}

struct QTPFSPathDrawer: public IPathDrawer {
public:
	QTPFSPathDrawer();

	void DrawAll() const;
	void UpdateExtraTexture(int, int, int, int, unsigned char*) const;

private:
	void DrawNodes(GL::RenderDataBufferC* rdb, const std::vector<const QTPFS::QTNode*>& nodes) const;
	void DrawCosts(const std::vector<const QTPFS::QTNode*>& nodes) const;

	void GetVisibleNodes(const QTPFS::QTNode* nt, std::vector<const QTPFS::QTNode*>& nodes) const;

	void DrawPaths(const MoveDef* md, GL::RenderDataBufferC* rdb) const;
	void DrawPath(const QTPFS::IPath* path, GL::RenderDataBufferC* rdb) const;
	void DrawSearchExecution(unsigned int pathType, const QTPFS::PathSearchTrace::Execution* se, GL::RenderDataBufferC* rdb) const;
	void DrawSearchIteration(unsigned int pathType, const std::vector<unsigned int>& nodeIndices, GL::RenderDataBufferC* rdb) const;
	void DrawNode(const QTPFS::QTNode* node, GL::RenderDataBufferC* rdb, const unsigned char* color) const;
	void DrawNodeLink(const QTPFS::QTNode* pushedNode, const QTPFS::QTNode* poppedNode, GL::RenderDataBufferC* rdb) const;

private:
	QTPFS::PathManager* pm;
};

#endif

