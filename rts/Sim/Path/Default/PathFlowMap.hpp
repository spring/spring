/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PATH_FLOWMAP_HDR
#define PATH_FLOWMAP_HDR

#include <array>
#include <vector>

#include "System/type2.h"
#include "System/float3.h"
#include "System/UnorderedSet.hpp"

struct MoveDef;
class CSolidObject;
class PathFlowMap {
public:
	struct FlowCell {
		float3 flowVector;
		float3 cellCenter; // WS

		unsigned int numObjects = 0;
	};

	static PathFlowMap* GetInstance();
	static void FreeInstance(PathFlowMap*);

	void Init(unsigned int scalex, unsigned int scalez);
	void Kill() {
		buffers[fBufferIdx].clear();
		buffers[bBufferIdx].clear();
		indices[fBufferIdx].clear();
		indices[bBufferIdx].clear();
	}

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

	std::array<float3, 16> pathOptDirs;

	unsigned int fBufferIdx = 0;
	unsigned int bBufferIdx = 0;
	unsigned int xscale = 0, xsize = 0, xfact = 0;
	unsigned int zscale = 0, zsize = 0, zfact = 0;

	float maxFlow[2] = {0.0f, 0.0f};
};

#endif
