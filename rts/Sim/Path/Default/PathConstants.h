/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PATH_CONSTANTS_HDR
#define PATH_CONSTANTS_HDR

#include <limits>
#include <array>
#include "Sim/Misc/GlobalConstants.h"

static const float PATHCOST_INFINITY = std::numeric_limits<float>::infinity();

// NOTE:
//   PF and PE both use a PathNodeBuffer of size MAX_SEARCHED_NODES,
//   thus MAX_SEARCHED_NODES_{PF, PE} MUST be <= MAX_SEARCHED_NODES
static const unsigned int MAX_SEARCHED_NODES    = 65536U;
static const unsigned int MAX_SEARCHED_NODES_PF = MAX_SEARCHED_NODES;
static const unsigned int MAX_SEARCHED_NODES_PE = MAX_SEARCHED_NODES;

// PathManager distance thresholds (to use PF or PE)
static const float MAXRES_SEARCH_DISTANCE =  50.0f;
static const float MEDRES_SEARCH_DISTANCE = 100.0f;
// path-refinement lookahead distances (MED to MAX and LOW to MED)
static const float MAXRES_SEARCH_DISTANCE_EXT = (MAXRES_SEARCH_DISTANCE * 0.4f) * SQUARE_SIZE;
static const float MEDRES_SEARCH_DISTANCE_EXT = (MEDRES_SEARCH_DISTANCE * 0.4f) * SQUARE_SIZE;

// how many recursive refinement attempts NextWayPoint should make
static const unsigned int MAX_PATH_REFINEMENT_DEPTH = 4;

static const unsigned int PATHESTIMATOR_VERSION = 78;

static const unsigned int MEDRES_PE_BLOCKSIZE =  8;
static const unsigned int LOWRES_PE_BLOCKSIZE = 32;

static const unsigned int SQUARES_TO_UPDATE = 1000;
static const unsigned int MAX_SEARCHED_NODES_ON_REFINE = 2000;

static const unsigned int PATH_HEATMAP_XSCALE =  1; // wrt. mapDims.hmapx
static const unsigned int PATH_HEATMAP_ZSCALE =  1; // wrt. mapDims.hmapy
static const unsigned int PATH_FLOWMAP_XSCALE = 32; // wrt. mapDims.mapx
static const unsigned int PATH_FLOWMAP_ZSCALE = 32; // wrt. mapDims.mapy


// PE-only flags (indices)
enum {
	PATHDIR_LEFT       = 0, // +x
	PATHDIR_LEFT_UP    = 1, // +x+z
	PATHDIR_UP         = 2, // +z
	PATHDIR_RIGHT_UP   = 3, // -x+z
	PATHDIR_RIGHT      = 4, // -x
	PATHDIR_RIGHT_DOWN = 5, // -x-z
	PATHDIR_DOWN       = 6, // -z
	PATHDIR_LEFT_DOWN  = 7, // +x-z
	PATH_DIRECTIONS    = 8,
};
static const unsigned int PATHDIR_CARDINALS[4] = {PATHDIR_LEFT, PATHDIR_RIGHT, PATHDIR_UP, PATHDIR_DOWN};
static const unsigned int PATH_DIRECTION_VERTICES = PATH_DIRECTIONS >> 1;
static const unsigned int PATH_NODE_SPACING = 2;

// note: because the spacing between nodes is 2 (not 1) we
// must also make sure not to skip across any intermediate
// impassable squares (!) but without reducing the spacing
// factor which would drop performance four-fold --> messy
static_assert(PATH_NODE_SPACING == 2, "");

// PF and PE flags (used in nodeMask[])
enum {
	PATHOPT_LEFT      =   1, // +x
	PATHOPT_RIGHT     =   2, // -x
	PATHOPT_UP        =   4, // +z
	PATHOPT_DOWN      =   8, // -z
	PATHOPT_OPEN      =  16,
	PATHOPT_CLOSED    =  32,
	PATHOPT_BLOCKED   =  64,
	PATHOPT_OBSOLETE  = 128,

	PATHOPT_SIZE      = 255, // size of PATHOPT bitmask
};
static const unsigned int PATHOPT_CARDINALS = (PATHOPT_RIGHT | PATHOPT_LEFT | PATHOPT_UP | PATHOPT_DOWN);


static inline std::array<unsigned int, PATH_DIRECTIONS> GetPathDir2PathOpt()
{
	std::array<unsigned int, PATH_DIRECTIONS> a;
	a[PATHDIR_LEFT]       = PATHOPT_LEFT;
	a[PATHDIR_RIGHT]      = PATHOPT_RIGHT;
	a[PATHDIR_UP]         = PATHOPT_UP;
	a[PATHDIR_DOWN]       = PATHOPT_DOWN;
	a[PATHDIR_LEFT_UP]    = (PATHOPT_LEFT  | PATHOPT_UP);
	a[PATHDIR_RIGHT_UP]   = (PATHOPT_RIGHT | PATHOPT_UP);
	a[PATHDIR_RIGHT_DOWN] = (PATHOPT_RIGHT | PATHOPT_DOWN);
	a[PATHDIR_LEFT_DOWN]  = (PATHOPT_LEFT  | PATHOPT_DOWN);
	return a;
}

static inline std::array<unsigned int, 15> GetPathOpt2PathDir()
{
	std::array<unsigned int, 15> a;
	a.fill(0);
	a[PATHOPT_LEFT]       = PATHDIR_LEFT;
	a[PATHOPT_RIGHT]      = PATHDIR_RIGHT;
	a[PATHOPT_UP]         = PATHDIR_UP;
	a[PATHOPT_DOWN]       = PATHDIR_DOWN;
	a[(PATHOPT_LEFT  | PATHOPT_UP)]   = PATHDIR_LEFT_UP;
	a[(PATHOPT_RIGHT | PATHOPT_UP)]   = PATHDIR_RIGHT_UP;
	a[(PATHOPT_RIGHT | PATHOPT_DOWN)] = PATHDIR_RIGHT_DOWN;
	a[(PATHOPT_LEFT  | PATHOPT_DOWN)] = PATHDIR_LEFT_DOWN;
	return a;
}
static const std::array<unsigned int, PATH_DIRECTIONS> DIR2OPT = GetPathDir2PathOpt();
static const std::array<unsigned int, 15>              OPT2DIR = GetPathOpt2PathDir();

// converts a PATHDIR* index to a PATHOPT* bitmask and vice versa
static inline unsigned int PathDir2PathOpt(unsigned int pathDir)    { return DIR2OPT[pathDir]; }
static inline unsigned int PathOpt2PathDir(unsigned int pathOptDir) { return OPT2DIR[pathOptDir]; }


// transition costs between vertices are bi-directional
// (cost(A-->B) == cost(A<--B)) so we only need to store
// (PATH_DIRECTIONS >> 1) values
static inline int GetBlockVertexOffset(unsigned int pathDir, unsigned int numBlocks) {
	int bvo = 0;

	switch (pathDir) {
		case PATHDIR_LEFT:       { bvo = PATHDIR_LEFT;      } break;
		case PATHDIR_LEFT_UP:    { bvo = PATHDIR_LEFT_UP;   } break;
		case PATHDIR_UP:         { bvo = PATHDIR_UP;        } break;
		case PATHDIR_RIGHT_UP:   { bvo = PATHDIR_RIGHT_UP;  } break;

		case PATHDIR_RIGHT:      { bvo = int(PATHDIR_LEFT    ) -                                         PATH_DIRECTION_VERTICES; } break;
		case PATHDIR_RIGHT_DOWN: { bvo = int(PATHDIR_LEFT_UP ) - (numBlocks * PATH_DIRECTION_VERTICES) - PATH_DIRECTION_VERTICES; } break;
		case PATHDIR_DOWN:       { bvo = int(PATHDIR_UP      ) - (numBlocks * PATH_DIRECTION_VERTICES);                           } break;
		case PATHDIR_LEFT_DOWN:  { bvo = int(PATHDIR_RIGHT_UP) - (numBlocks * PATH_DIRECTION_VERTICES) + PATH_DIRECTION_VERTICES; } break;
	}

	return bvo;
}

enum {
	NODE_COST_F = 0,
	NODE_COST_G = 1,
	NODE_COST_H = 2,
};

#endif
