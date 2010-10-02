/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "PathFlowMap.hpp"
#include "PathConstants.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/MoveTypes/MoveInfo.h"
#include "Sim/Objects/SolidObject.h"

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
	fBufferIdx = 0;
	bBufferIdx = 1;

	xscale = std::max(1, std::min(gs->mapx, int(scalex)));
	zscale = std::max(1, std::min(gs->mapy, int(scalez)));
	xsize  = gs->mapx / xscale;
	zsize  = gs->mapy / zscale;

	buffers[fBufferIdx].resize(xsize * zsize, FlowCell());
	buffers[bBufferIdx].resize(xsize * zsize, FlowCell());
}

PathFlowMap::~PathFlowMap() {
	buffers[fBufferIdx].clear();
	buffers[bBufferIdx].clear();
}

void PathFlowMap::Update() {
	std::vector<FlowCell>& fCells = buffers[fBufferIdx];
	std::set<unsigned int>& fIndices = indices[fBufferIdx];

	for (std::set<unsigned int>::iterator it = fIndices.begin(); it != fIndices.end(); ++it) {
		FlowCell& fCell = fCells[*it];

		// TODO: decay the flow
		fCell.flowVector = ZeroVector;
		fCell.numObjects = 0;
	}

	fIndices.clear();

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

	const unsigned int xcell = o->pos.x / (SQUARE_SIZE * xscale);
	const unsigned int zcell = o->pos.z / (SQUARE_SIZE * zscale);
	const unsigned int icell = zcell * xsize + xcell;

	std::vector<FlowCell>& bCells = buffers[bBufferIdx];
	std::set<unsigned int>& bIndices = indices[bBufferIdx];

	FlowCell& cell = bCells[icell];

	cell.flowVector.x += (o->speed.x);
	cell.flowVector.z += (o->speed.z);
	cell.flowVector.y += (o->mass * o->mobility->flowMod);
	cell.numObjects   += 1;

	bIndices.insert(icell);
}



const float3& PathFlowMap::GetFlowVec(unsigned int hmx, unsigned int hmz) const {
	// read from the front-buffer
	const std::vector<FlowCell>& buf = buffers[fBufferIdx];
	const unsigned int idx = (hmz / zscale) * xsize + (hmx / xscale);

	return buf[idx].flowVector;
}

float PathFlowMap::GetFlowCost(unsigned int x, unsigned int z, const MoveData& md, const int2& dir) const {
	const float3& flowVec = GetFlowVec(x, z);
	const float3  pathDir = float3(dir.x, 0.0f, dir.y);

	const float flowScale = ((flowVec.dot(pathDir) * -1.0f) + 1.0f) * 0.5f;
	const float flowCost = flowVec.y * flowScale;

	return flowCost;
}
