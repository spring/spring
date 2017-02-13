/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PATH_FLOWMAP_HDR
#define PATH_FLOWMAP_HDR

#include <vector>

#include "System/type2.h"
#include "System/float3.h"
#include "System/UnorderedSet.hpp"

struct MoveDef;
class CSolidObject;
class PathFlowMap {
public:
	struct FlowCell {
		FlowCell(): numObjects(0) {
		}

		float3 flowVector;
		float3 cellCenter; // WS

		unsigned int numObjects;
	};

	static PathFlowMap* GetInstance();
	static void FreeInstance(PathFlowMap*);

	PathFlowMap(unsigned int scalex, unsigned int scalez);
	~PathFlowMap();

	void Update();
	void AddFlow(const CSolidObject*);

	const float3& GetFlowVec(unsigned int hmx, unsigned int hmz) const;
	float GetFlowCost(unsigned int x, unsigned int z, const MoveDef&, unsigned int opt) const;
	float GetMaxFlow() const { return maxFlow[fBufferIdx]; }

	unsigned int GetFrontBufferIdx() const { return fBufferIdx; }
	unsigned int GetBackBufferIdx() const { return bBufferIdx; }

	const std::vector<FlowCell>& GetFrontBuffer() { return buffers[fBufferIdx]; }
	const std::vector<FlowCell>& GetBackBuffer() { return buffers[bBufferIdx]; }

private:
	unsigned int GetCellIdx(const CSolidObject*) const;

	std::vector<FlowCell> buffers[2];
	spring::unordered_set<unsigned int> indices[2];

	std::vector<float3> pathOptDirs;

	unsigned int fBufferIdx;
	unsigned int bBufferIdx;
	unsigned int xscale, xsize, xfact;
	unsigned int zscale, zsize, zfact;

	float maxFlow[2];
};

#endif
