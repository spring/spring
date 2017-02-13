/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_ENUMS_HDR
#define QTPFS_ENUMS_HDR

namespace QTPFS {
	enum {
		REL_RECT_INTERIOR_NODE = 0, // rect is in interior of node
		REL_RECT_EXTERIOR_NODE = 1, // rect is in exterior (~interior) of node
		REL_NODE_INTERIOR_RECT = 2, // node is in interior of rect
		REL_NODE_OVERLAPS_RECT = 3, // rect and node overlap partially
	};
	enum {
		REL_NGB_EDGE_T = 1, // top-edge neighbor
		REL_NGB_EDGE_R = 2, // right-edge neighbor
		REL_NGB_EDGE_B = 4, // bottom-edge neighbor
		REL_NGB_EDGE_L = 8, // left-edge neighbor
	};
	enum {
		NODE_STATE_OPEN   = 0,
		NODE_STATE_CLOSED = 1,
		NODE_STATE_OFFSET = 2,
	};
	enum {
		NODE_DIST_EUCLIDEAN = 0,
		NODE_DIST_MANHATTAN = 1,
	};
	enum {
		NODE_IDX_TL = 0,
		NODE_IDX_TR = 1,
		NODE_IDX_BR = 2,
		NODE_IDX_BL = 3,
	};
	enum {
		NODE_PATH_COST_F = 0,
		NODE_PATH_COST_G = 1,
		NODE_PATH_COST_H = 2,
	};
	enum {
		PATH_SEARCH_ASTAR    = 0,
		PATH_SEARCH_DIJKSTRA = 1,
	};
	enum {
		PATH_TYPE_TEMP = 0,
		PATH_TYPE_LIVE = 1,
		PATH_TYPE_DEAD = 2,
	};
}

#endif

