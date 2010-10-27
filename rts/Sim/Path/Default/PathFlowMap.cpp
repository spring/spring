/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "PathFlowMap.hpp"
#include "PathConstants.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/MoveTypes/MoveInfo.h"
#include "Sim/Objects/SolidObject.h"

#define FLOW_EPSILON         0.01f
#define FLOW_DECAY_ENABLED   0
#define FLOW_DECAY_FACTOR    0.86f
#define FLOW_COST_MULT      32.00f
#define FLOW_NGB_PROJECTION  0

PathFlowMap* PathFlowMap::GetInstance() {
	static PathFlowMap* pfm = NULL;

	if (pfm == NULL) {
		pfm = new PathFlowMap(PATH_FLOWMAP_XSCALE, PATH_FLOWMAP_ZSCALE);
	}

	return pfm;
}

void PathFlowMap::FreeInstance(PathFlowMap* pfm) {
	delete pfm;
}



PathFlowMap::PathFlowMap(unsigned int scalex, unsigned int scalez) {
	const float s = 1.0f / math::sqrt(2.0f);

	fBufferIdx = 0;
	bBufferIdx = 1;

	xscale = std::max(1, std::min(gs->mapx, int(scalex)));
	zscale = std::max(1, std::min(gs->mapy, int(scalez)));
	xsize  = gs->mapx / xscale;
	zsize  = gs->mapy / zscale;
	xfact  = SQUARE_SIZE * xscale;
	zfact  = SQUARE_SIZE * zscale;

	maxFlow[fBufferIdx] = 0.0f;
	maxFlow[bBufferIdx] = 0.0f;

	buffers[fBufferIdx].resize(xsize * zsize, FlowCell());
	buffers[bBufferIdx].resize(xsize * zsize, FlowCell());

	pathOptDirs.resize(PATH_DIRECTIONS << 1);

	pathOptDirs[PATHOPT_LEFT                ] = float3(+1.0f, 0.0f,  0.0f);
	pathOptDirs[PATHOPT_RIGHT               ] = float3(-1.0f, 0.0f,  0.0f);
	pathOptDirs[PATHOPT_UP                  ] = float3( 0.0f, 0.0f, +1.0f);
	pathOptDirs[PATHOPT_DOWN                ] = float3( 0.0f, 0.0f, -1.0f);
	pathOptDirs[PATHOPT_LEFT  | PATHOPT_UP  ] = (pathOptDirs[PATHOPT_LEFT ] + pathOptDirs[PATHOPT_UP  ]) * s;
	pathOptDirs[PATHOPT_RIGHT | PATHOPT_UP  ] = (pathOptDirs[PATHOPT_RIGHT] + pathOptDirs[PATHOPT_UP  ]) * s;
	pathOptDirs[PATHOPT_RIGHT | PATHOPT_DOWN] = (pathOptDirs[PATHOPT_RIGHT] + pathOptDirs[PATHOPT_DOWN]) * s;
	pathOptDirs[PATHOPT_LEFT  | PATHOPT_DOWN] = (pathOptDirs[PATHOPT_LEFT ] + pathOptDirs[PATHOPT_DOWN]) * s;

	for (unsigned int n = 0; n < xsize * zsize; n++) {
		const unsigned int x = n % xsize;
		const unsigned int z = n / xsize;
		const float3 p = float3((x * xfact) + (xfact >> 1), 0.0f, (z * zfact) + (zfact >> 1));

		buffers[fBufferIdx][n].cellCenter = p;
		buffers[bBufferIdx][n].cellCenter = p;
	}
}

PathFlowMap::~PathFlowMap() {
	buffers[fBufferIdx].clear();
	buffers[bBufferIdx].clear();
	indices[fBufferIdx].clear();
	indices[bBufferIdx].clear();

	pathOptDirs.clear();
}

void PathFlowMap::Update() {
	std::vector<FlowCell>& fCells = buffers[fBufferIdx];
	std::vector<FlowCell>& bCells = buffers[bBufferIdx];
	std::set<unsigned int>& fIndices = indices[fBufferIdx];
	std::set<unsigned int>& bIndices = indices[bBufferIdx];

	std::set<unsigned int>::iterator it;
	std::set<unsigned int>::iterator nit;

	#if (FLOW_DECAY_ENABLED == 0)
		for (it = fIndices.begin(); it != fIndices.end(); ++it) {
			FlowCell& fCell = fCells[*it];

			fCell.flowVector = ZeroVector;
			fCell.numObjects = 0;
		}

		fIndices.clear();
	#else
		for (it = fIndices.begin(); it != fIndices.end(); ) {
			nit = it; ++nit;

			FlowCell& fCell = fCells[*it];
			float3& fCellFlow = fCell.flowVector;

			bool fCellReset = false;

			if (bIndices.find(*it) == bIndices.end()) {
				// if the cell at index <*it> was NOT written to
				// during any AddFlow call last frame (meaning no
				// units were projected into it), start decaying
				// its flow-strength contribution
				fCellFlow.y *= FLOW_DECAY_FACTOR;
				fCellReset = (fCellFlow.y < FLOW_EPSILON);
			} else {
				// otherwise, force a cell reset
				fCellReset = true;
			}

			if (fCellReset) {
				fCell.flowVector = ZeroVector;
				fCell.numObjects = 0;

				fIndices.erase(it);
			}

			it = nit;
		}
	#endif

	for (it = bIndices.begin(); it != bIndices.end(); ++it) {
		FlowCell& bCell = bCells[*it];

		if (bCell.flowVector.SqLength2D() > FLOW_EPSILON) {
			const float flowLen = bCell.flowVector.Length2D();

			bCell.flowVector.x /= flowLen;
			bCell.flowVector.z /= flowLen;
		}

		// note: if FLOW_DECAY_ENABLED == 1, cells
		// whose normalized flow-strength is less
		// than FLOW_EPSILON will also be decayed
		// (this can be a problem if the range of
		// unit mass values is very wide and there
		// are units at both extremes in-game)
		if (maxFlow[bBufferIdx] > FLOW_EPSILON) {
			bCell.flowVector.y /= maxFlow[bBufferIdx];
		}
	}


	// swap the buffers
	fBufferIdx = (fBufferIdx + 1) & 1;
	bBufferIdx = (bBufferIdx + 1) & 1;

	maxFlow[bBufferIdx] = 0.0f;
}

void PathFlowMap::AddFlow(const CSolidObject* o) {
	if (!o->blocking) {
		return;
	}
	if (!o->pos.IsInBounds()) {
		return;
	}
	if (!o->mobility->flowMapping) {
		return;
	}

	const unsigned int cellIdx = GetCellIdx(o);

	std::vector<FlowCell>& bCells = buffers[bBufferIdx];
	std::set<unsigned int>& bIndices = indices[bBufferIdx];

	FlowCell& bCell = bCells[cellIdx];

	bCell.flowVector.x += (o->speed.x);
	bCell.flowVector.z += (o->speed.z);
	bCell.flowVector.y += (o->mass * o->mobility->flowMod);
	bCell.numObjects   += 1;

	bIndices.insert(cellIdx);

	#if (FLOW_NGB_PROJECTION == 1)
	{
		const unsigned int x = cellIdx % xsize;
		const unsigned int z = cellIdx / xsize;
		      unsigned int i = -1U;

		const bool halfSpaces[4] = {
			(o->pos.x <  bCell.cellCenter.x && x >         0),
			(o->pos.x >= bCell.cellCenter.x && x < xsize - 1),
			(o->pos.z <  bCell.cellCenter.z && z >         0),
			(o->pos.z >= bCell.cellCenter.z && z < zsize - 1),
		};

		FlowCell* ngbs[3] = {NULL, NULL, NULL};

		if (halfSpaces[0]) {  i = ((z    ) * xsize + (x - 1));  bIndices.insert(i);  ngbs[0] = &bCells[i];  }
		if (halfSpaces[1]) {  i = ((z    ) * xsize + (x + 1));  bIndices.insert(i);  ngbs[0] = &bCells[i];  }
		if (halfSpaces[2]) {  i = ((z - 1) * xsize + (x    ));  bIndices.insert(i);  ngbs[1] = &bCells[i];  }
		if (halfSpaces[3]) {  i = ((z + 1) * xsize + (x    ));  bIndices.insert(i);  ngbs[1] = &bCells[i];  }

		     if (halfSpaces[0] && halfSpaces[2]) {  i = ((z - 1) * xsize + (x - 1));  bIndices.insert(i);  ngbs[2] = &bCells[i];  }
		else if (halfSpaces[0] && halfSpaces[3]) {  i = ((z + 1) * xsize + (x - 1));  bIndices.insert(i);  ngbs[2] = &bCells[i];  }
		else if (halfSpaces[1] && halfSpaces[2]) {  i = ((z - 1) * xsize + (x + 1));  bIndices.insert(i);  ngbs[2] = &bCells[i];  }
		else if (halfSpaces[1] && halfSpaces[3]) {  i = ((z + 1) * xsize + (x + 1));  bIndices.insert(i);  ngbs[2] = &bCells[i];  }

		if (ngbs[0] != NULL) {  ngbs[0]->flowVector += float3(o->speed.x, o->mass * o->mobility->flowMod * 0.666f, o->speed.z);  ngbs[0]->numObjects += 1;  }
		if (ngbs[1] != NULL) {  ngbs[1]->flowVector += float3(o->speed.x, o->mass * o->mobility->flowMod * 0.666f, o->speed.z);  ngbs[1]->numObjects += 1;  }
		if (ngbs[2] != NULL) {  ngbs[2]->flowVector += float3(o->speed.x, o->mass * o->mobility->flowMod * 0.333f, o->speed.z);  ngbs[2]->numObjects += 1;  }
	}
	#endif

	maxFlow[bBufferIdx] = std::max(maxFlow[bBufferIdx], bCell.flowVector.y);
}



unsigned int PathFlowMap::GetCellIdx(const CSolidObject* o) const {
	const unsigned int xcell = o->pos.x / xfact;
	const unsigned int zcell = o->pos.z / zfact;

	return (zcell * xsize + xcell);
}

const float3& PathFlowMap::GetFlowVec(unsigned int hmx, unsigned int hmz) const {
	const std::vector<FlowCell>& fCells = buffers[fBufferIdx];
	const unsigned int fCellIdx = (hmz / zscale) * xsize + (hmx / xscale);

	return (fCells[fCellIdx].flowVector);
}

float PathFlowMap::GetFlowCost(unsigned int x, unsigned int z, const MoveData& md, unsigned int pathOpt) const {
	const float3& flowVec = GetFlowVec(x, z);
	const float3& pathDir = pathOptDirs[pathOpt];

	const float flowScale = ((flowVec.dot(pathDir) * -1.0f) + 1.0f) * 0.5f;
	const float flowCost = (flowVec.y * FLOW_COST_MULT) * flowScale;

	return flowCost;
}
