/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_PATHDRAWER_HDR
#define QTPFS_PATHDRAWER_HDR

#include <vector>

#include "IPathDrawer.h"
#include "Rendering/GL/RenderBuffersFwd.h"

struct MoveDef;

namespace QTPFS {
	class PathManager;

	struct QTNode;
	struct IPath;
	struct NodeLayer;
	struct PathSearch;

	namespace PathSearchTrace {
		struct Execution;
	}
}

struct QTPFSPathDrawer: public IPathDrawer {
public:
	QTPFSPathDrawer();

	void DrawAll() const override;
	void UpdateExtraTexture(int, int, int, int, unsigned char*) const override;

private:
	void DrawNodes(TypedRenderBuffer<VA_TYPE_C>& rb, const std::vector<const QTPFS::QTNode*>& nodes) const;
	void DrawCosts(const std::vector<const QTPFS::QTNode*>& nodes) const;

	void GetVisibleNodes(const QTPFS::QTNode* nt, const QTPFS::NodeLayer& nl, std::vector<const QTPFS::QTNode*>& nodes) const;

	void DrawPaths(const MoveDef* md, TypedRenderBuffer<VA_TYPE_C>& rd) const;
	void DrawPath(const QTPFS::IPath* path, TypedRenderBuffer<VA_TYPE_C>& wla) const;
	void DrawSearchExecution(unsigned int pathType, const QTPFS::PathSearchTrace::Execution* se, TypedRenderBuffer<VA_TYPE_C>& rd) const;
	void DrawSearchIteration(unsigned int pathType, const std::vector<unsigned int>& nodeIndices, TypedRenderBuffer<VA_TYPE_C>& rd) const;
	void DrawNode(const QTPFS::QTNode* node, TypedRenderBuffer<VA_TYPE_C>& rd, const unsigned char* color) const;
	void DrawNodeW(const QTPFS::QTNode* node, TypedRenderBuffer<VA_TYPE_C>& rb, const unsigned char* color) const;
	void DrawNodeLink(const QTPFS::QTNode* pushedNode, const QTPFS::QTNode* poppedNode, TypedRenderBuffer<VA_TYPE_C>& rb) const;

private:
	QTPFS::PathManager* pm;
};

#endif

