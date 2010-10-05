/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "PathFlowMap.hpp"
#include "PathConstants.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/MoveTypes/MoveInfo.h"
#include "Sim/Objects/SolidObject.h"

#define FLOW_DECAY_ENABLED 0
#define FLOW_DECAY_FACTOR  0.86f

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

	buffers[fBufferIdx].resize(xsize * zsize, FlowCell());
	buffers[bBufferIdx].resize(xsize * zsize, FlowCell());

	pathOptDirs.resize(PATH_DIRECTIONS << 1);

	pathOptDirs[PATHOPT_LEFT                ] = float3(-1.0f, 0.0f,  0.0f);
	pathOptDirs[PATHOPT_RIGHT               ] = float3( 1.0f, 0.0f,  0.0f);
	pathOptDirs[PATHOPT_UP                  ] = float3( 0.0f, 0.0f, -1.0f);
	pathOptDirs[PATHOPT_DOWN                ] = float3( 0.0f, 0.0f,  1.0f);
	pathOptDirs[PATHOPT_LEFT  | PATHOPT_UP  ] = float3(-1.0f, 0.0f, -1.0f) * s;
	pathOptDirs[PATHOPT_RIGHT | PATHOPT_UP  ] = float3( 1.0f, 0.0f, -1.0f) * s;
	pathOptDirs[PATHOPT_RIGHT | PATHOPT_DOWN] = float3( 1.0f, 0.0f,  1.0f) * s;
	pathOptDirs[PATHOPT_LEFT  | PATHOPT_DOWN] = float3(-1.0f, 0.0f,  1.0f) * s;
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
		/*
		// FIXME:
		//     if a unit is not moving, it will be added
		//     to the same cell every frame and cause the
		//     y-component to increase faster than it can
		//     decay
		//
		//     if each cell maintains a set of unitID's,
		//     then a non-moving unit would instead cause
		//     the flow to decay entirely (since the cell
		//     would not have its flow refreshed)
		for (it = fIndices.begin(); it != fIndices.end(); ) {
			nit = it; ++nit;

			FlowCell& fCell = fCells[*it];

			if (fCell.flowVector.y > 0.01f) {
				fCell.flowVector.y *= FLOW_DECAY_FACTOR;
			} else {
				fCell.flowVector = ZeroVector;
				fCell.numObjects = 0;

				fIndices.erase(it);
			}

			it = nit;
		}
		*/
	#endif

	for (it = bIndices.begin(); it != bIndices.end(); ++it) {
		FlowCell& bCell = bCells[*it];

		if (bCell.flowVector.SqLength2D() > 0.01f) {
			const float flowLen = bCell.flowVector.Length2D();

			bCell.flowVector.x /= flowLen;
			bCell.flowVector.z /= flowLen;
		}
	}

	// swap the buffers
	fBufferIdx = (fBufferIdx + 1) & 1;
	bBufferIdx = (bBufferIdx + 1) & 1;
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
}



unsigned int PathFlowMap::GetCellIdx(const CSolidObject* o) const {
	const unsigned int xcell = o->pos.x / (SQUARE_SIZE * xscale);
	const unsigned int zcell = o->pos.z / (SQUARE_SIZE * zscale);

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
	const float flowCost = flowVec.y * flowScale;

	return flowCost;
}
