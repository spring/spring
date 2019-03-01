/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "PathFlowMap.hpp"
#include "PathConstants.h"
#include "Map/ReadMap.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/Objects/SolidObject.h"
#include "System/SpringMath.h"

#define FLOW_EPSILON         0.01f
#define FLOW_DECAY_ENABLED   0
#define FLOW_DECAY_FACTOR    0.86f
#define FLOW_COST_MULT      32.00f
#define FLOW_NGB_PROJECTION  0

// not extern'ed, so static
static PathFlowMap gPathFlowMap;


PathFlowMap* PathFlowMap::GetInstance() {
	gPathFlowMap.Init(PATH_FLOWMAP_XSCALE, PATH_FLOWMAP_ZSCALE);
	return &gPathFlowMap;
}

void PathFlowMap::FreeInstance(PathFlowMap* pfm) {
	assert(pfm == &gPathFlowMap);
	pfm->Kill();
}



void PathFlowMap::Init(unsigned int scalex, unsigned int scalez) {
	// const float s = 1.0f / math::sqrt(2.0f);

	fBufferIdx = 0;
	bBufferIdx = 1;

	xscale = Clamp(int(scalex), 1, mapDims.mapx);
	zscale = Clamp(int(scalez), 1, mapDims.mapy);
	xsize  = mapDims.mapx / xscale;
	zsize  = mapDims.mapy / zscale;
	xfact  = SQUARE_SIZE * xscale;
	zfact  = SQUARE_SIZE * zscale;

	maxFlow[fBufferIdx] = 0.0f;
	maxFlow[bBufferIdx] = 0.0f;

#if 0
	buffers[fBufferIdx].resize(xsize * zsize);
	buffers[bBufferIdx].resize(xsize * zsize);
	indices[fBufferIdx].reserve(xsize * zsize);
	indices[bBufferIdx].reserve(xsize * zsize);

	static_assert((PATH_DIRECTIONS << 1) == 16, "");

	pathOptDirs[PATHOPT_LEFT                ] =  RgtVector;
	pathOptDirs[PATHOPT_RIGHT               ] = -RgtVector;
	pathOptDirs[PATHOPT_UP                  ] =  FwdVector;
	pathOptDirs[PATHOPT_DOWN                ] = -FwdVector;
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
#endif
}


void PathFlowMap::Update() {
	return;
#if 0
	std::vector<FlowCell>& fCells = buffers[fBufferIdx];
	std::vector<FlowCell>& bCells = buffers[bBufferIdx];

	spring::unordered_set<unsigned int>& fIndices = indices[fBufferIdx];
	spring::unordered_set<unsigned int>& bIndices = indices[bBufferIdx];

	spring::unordered_set<unsigned int>::iterator it;
	spring::unordered_set<unsigned int>::iterator nit;

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

		// note: if FLOW_DECAY_ENABLED == 1, all cells whose normalized
		// flow-strength is less than FLOW_EPSILON will also be decayed
		// (this can be problematic if the range of unit mass values is
		// wide and there are units at both extremes in-game)
		if (maxFlow[bBufferIdx] > FLOW_EPSILON) {
			bCell.flowVector.y /= maxFlow[bBufferIdx];
		}
	}


	// swap the buffers
	fBufferIdx = (fBufferIdx + 1) & 1;
	bBufferIdx = (bBufferIdx + 1) & 1;

	maxFlow[bBufferIdx] = 0.0f;
#endif
}

void PathFlowMap::AddFlow(const CSolidObject* o) {
	return;
#if 0
	if (!o->HasCollidableStateBit(CSolidObject::CSTATE_BIT_SOLIDOBJECTS))
		return;
	if (!o->pos.IsInBounds())
		return;
	if (!o->moveDef->flowMapping)
		return;

	// prevent self-obstruction if the unit is not moving
	const float3& flowVec = (Square(o->speed.w) >= 1.0f)? float3(o->speed): GetVectorFromHeading(o->heading);
	const unsigned int cellIdx = GetCellIdx(o);

	std::vector<FlowCell>& bCells = buffers[bBufferIdx];
	spring::unordered_set<unsigned int>& bIndices = indices[bBufferIdx];

	FlowCell& bCell = bCells[cellIdx];

	bCell.flowVector.x += (flowVec.x);
	bCell.flowVector.z += (flowVec.z);
	bCell.flowVector.y += (o->mass * o->moveDef->flowMod);
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

		if (ngbs[0] != NULL) {  ngbs[0]->flowVector += float3(flowVec.x, o->mass * o->moveDef->flowMod * 0.666f, flowVec.z);  ngbs[0]->numObjects += 1;  }
		if (ngbs[1] != NULL) {  ngbs[1]->flowVector += float3(flowVec.x, o->mass * o->moveDef->flowMod * 0.666f, flowVec.z);  ngbs[1]->numObjects += 1;  }
		if (ngbs[2] != NULL) {  ngbs[2]->flowVector += float3(flowVec.x, o->mass * o->moveDef->flowMod * 0.333f, flowVec.z);  ngbs[2]->numObjects += 1;  }
	}
	#endif

	maxFlow[bBufferIdx] = std::max(maxFlow[bBufferIdx], bCell.flowVector.y);
#endif
}



unsigned int PathFlowMap::GetCellIdx(const CSolidObject* o) const {
	const unsigned int xcell = o->pos.x / xfact;
	const unsigned int zcell = o->pos.z / zfact;

	return (zcell * xsize + xcell);
}

const float3& PathFlowMap::GetFlowVec(unsigned int hmx, unsigned int hmz) const {
	return ZeroVector;
#if 0
	const std::vector<FlowCell>& fCells = buffers[fBufferIdx];
	const unsigned int fCellIdx = (hmz / zscale) * xsize + (hmx / xscale);

	return (fCells[fCellIdx].flowVector);
#endif
}

float PathFlowMap::GetFlowCost(unsigned int x, unsigned int z, const MoveDef& md, unsigned int pathOpt) const {
	return 0.0f;
#if 0
	const float3& flowVec = GetFlowVec(x, z);
	const float3& pathDir = pathOptDirs[pathOpt];

	const float flowScale = ((flowVec.dot(pathDir) * -1.0f) + 1.0f) * 0.5f;
	const float flowCost = (flowVec.y * FLOW_COST_MULT) * flowScale;

	return flowCost;
#endif
}
