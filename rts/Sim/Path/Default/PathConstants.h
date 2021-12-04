/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PATH_CONSTANTS_H
#define PATH_CONSTANTS_H

#include <limits>
#include <array>
#include "Sim/Misc/GlobalConstants.h"

static constexpr float PATHCOST_INFINITY = std::numeric_limits<float>::infinity();

// NOTE:
//   PF and PE both use a PathNodeBuffer of size MAX_SEARCHED_NODES,
//   thus MAX_SEARCHED_NODES_{PF, PE} MUST be <= MAX_SEARCHED_NODES
static constexpr unsigned int MAX_SEARCHED_NODES    = 65536U;
static constexpr unsigned int MAX_SEARCHED_NODES_PF = MAX_SEARCHED_NODES;
static constexpr unsigned int MAX_SEARCHED_NODES_PE = MAX_SEARCHED_NODES;

// PathManager distance thresholds (to use PF or PE)
static constexpr float MAXRES_SEARCH_DISTANCE =  50.0f;
static constexpr float MEDRES_SEARCH_DISTANCE = 100.0f;
// path-refinement lookahead distances (MED to MAX and LOW to MED)
static const float MAXRES_SEARCH_DISTANCE_EXT = (MAXRES_SEARCH_DISTANCE * 0.4f) * SQUARE_SIZE;
static const float MEDRES_SEARCH_DISTANCE_EXT = (MEDRES_SEARCH_DISTANCE * 0.4f) * SQUARE_SIZE;

// how many recursive refinement attempts NextWayPoint should make
static constexpr unsigned int MAX_PATH_REFINEMENT_DEPTH = 4;

static constexpr unsigned int PATHESTIMATOR_VERSION = 100;

static constexpr unsigned int MEDRES_PE_BLOCKSIZE = 16;
static constexpr unsigned int LOWRES_PE_BLOCKSIZE = 32;

static constexpr unsigned int SQUARES_TO_UPDATE = 1000;
static constexpr unsigned int MAX_SEARCHED_NODES_ON_REFINE = 2000;

static constexpr unsigned int PATH_HEATMAP_XSCALE =  1; // wrt. mapDims.hmapx
static constexpr unsigned int PATH_HEATMAP_ZSCALE =  1; // wrt. mapDims.hmapy
static constexpr unsigned int PATH_FLOWMAP_XSCALE = 32; // wrt. mapDims.mapx
static constexpr unsigned int PATH_FLOWMAP_ZSCALE = 32; // wrt. mapDims.mapy


// PE-only flags (indices)
static constexpr unsigned int PATHDIR_LEFT       = 0; // +x (LEFT *TO* RIGHT)
static constexpr unsigned int PATHDIR_LEFT_UP    = 1; // +x+z
static constexpr unsigned int PATHDIR_UP         = 2; // +z (UP *TO* DOWN)
static constexpr unsigned int PATHDIR_RIGHT_UP   = 3; // -x+z
static constexpr unsigned int PATHDIR_RIGHT      = 4; // -x (RIGHT *TO* LEFT)
static constexpr unsigned int PATHDIR_RIGHT_DOWN = 5; // -x-z
static constexpr unsigned int PATHDIR_DOWN       = 6; // -z (DOWN *TO* UP)
static constexpr unsigned int PATHDIR_LEFT_DOWN  = 7; // +x-z
static constexpr unsigned int PATH_DIRECTIONS    = 8;


static constexpr unsigned int PATHDIR_CARDINALS[4] = {PATHDIR_LEFT, PATHDIR_RIGHT, PATHDIR_UP, PATHDIR_DOWN};
static constexpr unsigned int PATH_DIRECTION_VERTICES = PATH_DIRECTIONS >> 1;
static constexpr unsigned int PATH_NODE_SPACING = 2;

// note: because the spacing between nodes is 2 (not 1) we
// must also make sure not to skip across any intermediate
// impassable squares (!) but without reducing the spacing
// factor which would drop performance four-fold --> messy
static_assert(PATH_NODE_SPACING == 2, "");


// these give the changes in (x, z) coors
// when moving one step in given direction
//
// NOTE: the choices of +1 for LEFT and UP are *not* arbitrary
// (they are related to GetBlockVertexOffset) and also need to
// be consistent with the PATHOPT_* flags (for PathDir2PathOpt)
static constexpr int2 PE_DIRECTION_VECTORS[] = {
	{+1,  0}, // PATHDIR_LEFT
	{+1, +1}, // PATHDIR_LEFT_UP
	{ 0, +1}, // PATHDIR_UP
	{-1, +1}, // PATHDIR_RIGHT_UP
	{-1,  0}, // PATHDIR_RIGHT
	{-1, -1}, // PATHDIR_RIGHT_DOWN
	{ 0, -1}, // PATHDIR_DOWN
	{+1, -1}, // PATHDIR_LEFT_DOWN
};

//FIXME why not use PATHDIR_* consts and merge code with top one
static constexpr int2 PF_DIRECTION_VECTORS_2D[] = {
	{ 0,                           0                         },
	{+1 * int(PATH_NODE_SPACING),  0 * int(PATH_NODE_SPACING)}, // PATHOPT_LEFT
	{-1 * int(PATH_NODE_SPACING),  0 * int(PATH_NODE_SPACING)}, // PATHOPT_RIGHT
	{ 0,                           0                         }, // PATHOPT_LEFT | PATHOPT_RIGHT
	{ 0 * int(PATH_NODE_SPACING), +1 * int(PATH_NODE_SPACING)}, // PATHOPT_UP
	{+1 * int(PATH_NODE_SPACING), +1 * int(PATH_NODE_SPACING)}, // PATHOPT_LEFT | PATHOPT_UP
	{-1 * int(PATH_NODE_SPACING), +1 * int(PATH_NODE_SPACING)}, // PATHOPT_RIGHT | PATHOPT_UP
	{ 0,                           0                         }, // PATHOPT_LEFT | PATHOPT_RIGHT | PATHOPT_UP
	{ 0 * int(PATH_NODE_SPACING), -1 * int(PATH_NODE_SPACING)}, // PATHOPT_DOWN
	{+1 * int(PATH_NODE_SPACING), -1 * int(PATH_NODE_SPACING)}, // PATHOPT_LEFT | PATHOPT_DOWN
	{-1 * int(PATH_NODE_SPACING), -1 * int(PATH_NODE_SPACING)}, // PATHOPT_RIGHT | PATHOPT_DOWN
	{ 0,                           0                         },
	{ 0,                           0                         },
	{ 0,                           0                         },
	{ 0,                           0                         },
	{ 0,                           0                         },
};


// PF and PE flags (used in nodeMask[])
static constexpr unsigned int PATHOPT_LEFT      =   1; // +x
static constexpr unsigned int PATHOPT_RIGHT     =   2; // -x
static constexpr unsigned int PATHOPT_UP        =   4; // +z
static constexpr unsigned int PATHOPT_DOWN      =   8; // -z
static constexpr unsigned int PATHOPT_OPEN      =  16;
static constexpr unsigned int PATHOPT_CLOSED    =  32;
static constexpr unsigned int PATHOPT_BLOCKED   =  64;
static constexpr unsigned int PATHOPT_OBSOLETE  = 128;
static constexpr unsigned int PATHOPT_SIZE      = 255; // size of PATHOPT bitmask

static constexpr unsigned int PATHOPT_CARDINALS = (PATHOPT_RIGHT | PATHOPT_LEFT | PATHOPT_UP | PATHOPT_DOWN);

static constexpr unsigned int DIR2OPT[] = {
	(PATHOPT_LEFT                ),
	(PATHOPT_LEFT  | PATHOPT_UP  ),
	(                PATHOPT_UP  ),
	(PATHOPT_RIGHT | PATHOPT_UP  ),
	(PATHOPT_RIGHT               ),
	(PATHOPT_RIGHT | PATHOPT_DOWN),
	(PATHOPT_DOWN                ),
	(PATHOPT_LEFT  | PATHOPT_DOWN),
};


static constexpr unsigned int OPT2DIR[] = {
	0,
	PATHDIR_LEFT,       // PATHOPT_LEFT
	PATHDIR_RIGHT,      // PATHOPT_RIGHT
	0,                  // PATHOPT_LEFT  | PATHOPT_RIGHT
	PATHDIR_UP,         // PATHOPT_UP
	PATHDIR_LEFT_UP,    // PATHOPT_LEFT  | PATHOPT_UP
	PATHDIR_RIGHT_UP,   // PATHOPT_RIGHT | PATHOPT_UP
	0,                  // PATHOPT_LEFT  | PATHOPT_RIGHT | PATHOPT_UP
	PATHDIR_DOWN,       // PATHOPT_DOWN
	PATHDIR_LEFT_DOWN,  // PATHOPT_LEFT  | PATHOPT_DOWN
	PATHDIR_RIGHT_DOWN, // PATHOPT_RIGHT | PATHOPT_DOWN
	0,
	0,
	0,
	0,
	0,
};

// converts a PATHDIR* index to a PATHOPT* bitmask and vice versa
static constexpr unsigned int PathDir2PathOpt(unsigned int pathDir)    { return DIR2OPT[pathDir]; }
static constexpr unsigned int PathOpt2PathDir(unsigned int pathOptDir) { return OPT2DIR[pathOptDir]; }


// transition costs between vertices are bi-directional
// (cost(A-->B) == cost(A<--B)) so we only need to store
// (PATH_DIRECTIONS >> 1) values
static inline int GetBlockVertexOffset(unsigned int pathDir, unsigned int numBlocks) {
	int bvo = pathDir;

	switch (pathDir) {
		case PATHDIR_RIGHT:      { bvo = int(PATHDIR_LEFT    ) -                                         PATH_DIRECTION_VERTICES; } break;
		case PATHDIR_RIGHT_DOWN: { bvo = int(PATHDIR_LEFT_UP ) - (numBlocks * PATH_DIRECTION_VERTICES) - PATH_DIRECTION_VERTICES; } break;
		case PATHDIR_DOWN:       { bvo = int(PATHDIR_UP      ) - (numBlocks * PATH_DIRECTION_VERTICES)                          ; } break;
		case PATHDIR_LEFT_DOWN:  { bvo = int(PATHDIR_RIGHT_UP) - (numBlocks * PATH_DIRECTION_VERTICES) + PATH_DIRECTION_VERTICES; } break;
		default: {} break;
	}

	return bvo;
}

enum {
	NODE_COST_F = 0,
	NODE_COST_G = 1,
	NODE_COST_H = 2,
};

#endif
