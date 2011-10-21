/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LANDSCAPE_H
#define LANDSCAPE_H

#include "Patch.h"
#include "Map/BaseGroundDrawer.h"

#include <vector>

class CSMFGroundDrawer;

// ---------------------------------------------------------------------
// Various Pre-Defined map sizes & their #define counterparts:



// How many TriTreeNodes should be allocated?
#define POOL_SIZE (500000)

// How many heightmap pixels a patch consists of
#define PATCH_SIZE 128

//useful stuff:
#define SQR(x) ((x) * (x))
#define MAX(a,b) ((a < b) ? (b) : (a))
#define MIN(a,b) ((a > b) ? (b) : (a))
#define DEG2RAD(a) (((a) * M_PI) / 180.0f)
#define M_PI (3.14159265358979323846f)



//
// Landscape Class
// Holds all the information to render an entire landscape.
//
class Landscape
{
public:
	void Init(CSMFGroundDrawer* drawer, const float* hMap, int bx, int by);
	void Reset();
	void Tessellate(const float3& campos, int viewradius);
	int Render(bool changed, bool shadows, bool waterdrawn);
	void Explosion(float x, float y, float z, float radius);

protected:
	static int GetNextTriNode() { return m_NextTriNode; }
	static void SetNextTriNode( int nNextNode ) { m_NextTriNode = nNextNode; }

protected:
	const float* m_HeightMap;                //< HeightMap of the Landscape

	static int m_NextTriNode;                //< Index to next free TriTreeNode
	static TriTreeNode m_TriPool[POOL_SIZE]; //< Pool of TriTree nodes for splitting

	CSMFGroundDrawer* drawer;

public:
	std::vector<Patch> m_Patches; //[NUM_PATCHES_PER_SIDE][NUM_PATCHES_PER_SIDE];  //< Array of patches
	std::vector<float> minhpatch; //< min and maximum heights for each patch for view testing.
	std::vector<float> maxhpatch;

	static TriTreeNode* AllocateTri();
	const float* heightData;
	int h, w;

	int updateCount;
};

#endif
