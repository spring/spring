/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PATH_CONSTANTS_HDR
#define PATH_CONSTANTS_HDR

#include <limits>
#include "Sim/Misc/GlobalConstants.h"

const float PATHCOST_INFINITY = std::numeric_limits<float>::infinity();

// NOTE:
//     PF and PE both use a PathNodeBuffer of size MAX_SEARCHED_NODES,
//     thus MAX_SEARCHED_NODES_{PF, PE} MUST be <= MAX_SEARCHED_NODES
const unsigned int MAX_SEARCHED_NODES    = 10000U;
const unsigned int MAX_SEARCHED_NODES_PF = MAX_SEARCHED_NODES;
const unsigned int MAX_SEARCHED_NODES_PE = MAX_SEARCHED_NODES;

// PathManager distance thresholds (to use PF or PE)
const float DETAILED_DISTANCE     = 25;
const float ESTIMATE_DISTANCE     = 55;
const float MIN_ESTIMATE_DISTANCE = 64;
const float MIN_DETAILED_DISTANCE = 32;

const unsigned int PATHESTIMATOR_VERSION = 45;
const unsigned int MEDRES_PE_BLOCKSIZE =  8;
const unsigned int LOWRES_PE_BLOCKSIZE = 32;
const unsigned int SQUARES_TO_UPDATE = 600;
const unsigned int MAX_SEARCHED_NODES_ON_REFINE = 2000;

const unsigned int PATH_HEATMAP_XSCALE = 1; // wrt. gs->hmapx
const unsigned int PATH_HEATMAP_ZSCALE = 1; // wrt. gs->hmapy
const unsigned int PATH_FLOWMAP_XSCALE = 8; // wrt. gs->mapx
const unsigned int PATH_FLOWMAP_ZSCALE = 8; // wrt. gs->mapy

const unsigned int PATH_DIRECTIONS = 8;
const unsigned int PATH_DIRECTION_VERTICES = PATH_DIRECTIONS >> 1;

// PE-only flags
const unsigned int PATHDIR_LEFT       = 0; // +x
const unsigned int PATHDIR_LEFT_UP    = 1; // +x+z
const unsigned int PATHDIR_UP         = 2; // +z
const unsigned int PATHDIR_RIGHT_UP   = 3; // -x+z
const unsigned int PATHDIR_RIGHT      = 4; // -x
const unsigned int PATHDIR_RIGHT_DOWN = 5; // -x-z
const unsigned int PATHDIR_DOWN       = 6; // -z
const unsigned int PATHDIR_LEFT_DOWN  = 7; // +x-z

// PF-only flags
const unsigned int PATHOPT_LEFT      =   1; //-x
const unsigned int PATHOPT_RIGHT     =   2; //+x
const unsigned int PATHOPT_UP        =   4; //-z
const unsigned int PATHOPT_DOWN      =   8; //+z
const unsigned int PATHOPT_AXIS_DIRS = (PATHOPT_RIGHT | PATHOPT_LEFT | PATHOPT_UP | PATHOPT_DOWN);

inline unsigned int PathDir2PathOpt(unsigned int pathDir) {
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
inline int GetBlockVertexOffset(unsigned int pathDir, unsigned int numBlocks) {
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

// PF and PE flags
const unsigned int PATHOPT_START     =  16;
const unsigned int PATHOPT_OPEN      =  32;
const unsigned int PATHOPT_CLOSED    =  64;
const unsigned int PATHOPT_FORBIDDEN = 128;
const unsigned int PATHOPT_BLOCKED   = 256;
const unsigned int PATHOPT_OBSOLETE  = 512;

#endif
