/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PATH_CONSTANTS_HDR
#define PATH_CONSTANTS_HDR

#include <limits>
#include "Sim/Misc/GlobalConstants.h"

static const float PATHCOST_INFINITY = std::numeric_limits<float>::infinity();

// NOTE:
//     PF and PE both use a PathNodeBuffer of size MAX_SEARCHED_NODES,
//     thus MAX_SEARCHED_NODES_{PF, PE} MUST be <= MAX_SEARCHED_NODES
static const unsigned int MAX_SEARCHED_NODES    = 65536U;
static const unsigned int MAX_SEARCHED_NODES_PF = MAX_SEARCHED_NODES;
static const unsigned int MAX_SEARCHED_NODES_PE = MAX_SEARCHED_NODES;

// PathManager distance thresholds (to use PF or PE)
static const float     MAXRES_SEARCH_DISTANCE = 25.0f;
static const float     MEDRES_SEARCH_DISTANCE = 55.0f;
static const float MIN_MEDRES_SEARCH_DISTANCE = 40.0f;
static const float MIN_MAXRES_SEARCH_DISTANCE = 12.0f;

// how many recursive refinement attempts NextWayPoint should make
static const unsigned int MAX_PATH_REFINEMENT_DEPTH = 4;

static const unsigned int PATHESTIMATOR_VERSION = 55;

static const unsigned int MEDRES_PE_BLOCKSIZE =  8;
static const unsigned int LOWRES_PE_BLOCKSIZE = 32;

static const unsigned int SQUARES_TO_UPDATE = 1000;
static const unsigned int MAX_SEARCHED_NODES_ON_REFINE = 2000;

static const unsigned int PATH_HEATMAP_XSCALE =  1; // wrt. gs->hmapx
static const unsigned int PATH_HEATMAP_ZSCALE =  1; // wrt. gs->hmapy
static const unsigned int PATH_FLOWMAP_XSCALE = 32; // wrt. gs->mapx
static const unsigned int PATH_FLOWMAP_ZSCALE = 32; // wrt. gs->mapy

static const unsigned int PATH_DIRECTIONS = 8;
static const unsigned int PATH_DIRECTION_VERTICES = PATH_DIRECTIONS >> 1;
static const unsigned int PATH_NODE_SPACING = 2;

// PE-only flags (indices)
static const unsigned int PATHDIR_LEFT       = 0; // +x
static const unsigned int PATHDIR_LEFT_UP    = 1; // +x+z
static const unsigned int PATHDIR_UP         = 2; // +z
static const unsigned int PATHDIR_RIGHT_UP   = 3; // -x+z
static const unsigned int PATHDIR_RIGHT      = 4; // -x
static const unsigned int PATHDIR_RIGHT_DOWN = 5; // -x-z
static const unsigned int PATHDIR_DOWN       = 6; // -z
static const unsigned int PATHDIR_LEFT_DOWN  = 7; // +x-z
static const unsigned int PATHDIR_CARDINALS[4] = {PATHDIR_LEFT, PATHDIR_RIGHT, PATHDIR_UP, PATHDIR_DOWN};
static const unsigned int PATHDIR_DIAGONALS[4] = {PATHDIR_LEFT_UP, PATHDIR_LEFT_DOWN, PATHDIR_RIGHT_UP, PATHDIR_RIGHT_DOWN};

// PF-only flags (bitmasks)
static const unsigned int PATHOPT_LEFT      =   1; // +x
static const unsigned int PATHOPT_RIGHT     =   2; // -x
static const unsigned int PATHOPT_UP        =   4; // +z
static const unsigned int PATHOPT_DOWN      =   8; // -z
static const unsigned int PATHOPT_CARDINALS = (PATHOPT_RIGHT | PATHOPT_LEFT | PATHOPT_UP | PATHOPT_DOWN);

// PF and PE flags
static const unsigned int PATHOPT_START     =  16;
static const unsigned int PATHOPT_OPEN      =  32;
static const unsigned int PATHOPT_CLOSED    =  64;
static const unsigned int PATHOPT_FORBIDDEN = 128;
static const unsigned int PATHOPT_BLOCKED   = 256;
static const unsigned int PATHOPT_OBSOLETE  = 512;



// converts a PATHDIR* index to a PATHOPT* bitmask
static inline unsigned int PathDir2PathOpt(unsigned int pathDir) {
	unsigned int pathOpt = 0;

	switch (pathDir) {
		case PATHDIR_LEFT:       { pathOpt |= PATHOPT_LEFT;                   } break;
		case PATHDIR_RIGHT:      { pathOpt |= PATHOPT_RIGHT;                  } break;
		case PATHDIR_UP:         { pathOpt |= PATHOPT_UP;                     } break;
		case PATHDIR_DOWN:       { pathOpt |= PATHOPT_DOWN;                   } break;
		case PATHDIR_LEFT_UP:    { pathOpt |= (PATHOPT_LEFT  | PATHOPT_UP);   } break;
		case PATHDIR_RIGHT_UP:   { pathOpt |= (PATHOPT_RIGHT | PATHOPT_UP);   } break;
		case PATHDIR_RIGHT_DOWN: { pathOpt |= (PATHOPT_RIGHT | PATHOPT_DOWN); } break;
		case PATHDIR_LEFT_DOWN:  { pathOpt |= (PATHOPT_LEFT  | PATHOPT_DOWN); } break;
	}

	return pathOpt;
}

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
