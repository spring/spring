/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PATH_FLOWMAP_HDR
#define PATH_FLOWMAP_HDR

#include <set>
#include <vector>

#include "System/Vec2.h"
#include "System/float3.h"

struct MoveData;
class CSolidObject;
class PathFlowMap {
public:
	struct FlowCell {
		FlowCell(): numObjects(0) {
		}

		float3 flowVector;

		unsigned int numObjects;
	};

	static PathFlowMap* GetInstance();
	static void FreeInstance(PathFlowMap*);

	PathFlowMap(unsigned int scalex, unsigned int scalez);
	~PathFlowMap();

	void Update();
	void AddFlow(const CSolidObject*);

	const float3& GetFlowVec(unsigned int hmx, unsigned int hmz) const;
	float GetFlowCost(unsigned int x, unsigned int z, const MoveData&, const int2& dir) const;

	unsigned int GetFrontBufferIdx() const { return fBufferIdx; }
	unsigned int GetBackBufferIdx() const { return bBufferIdx; }

	const std::vector<FlowCell>& GetFBuffer() { return buffers[fBufferIdx]; }
	const std::vector<FlowCell>& GetBBuffer() { return buffers[bBufferIdx]; }

private:
	std::vector<FlowCell> buffers[2];
	std::set<unsigned int> indices[2];

	unsigned int fBufferIdx;
	unsigned int bBufferIdx;
	unsigned int xscale, xsize;
	unsigned int zscale, zsize;
};

#endif
