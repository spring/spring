/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LANDSCAPE_H
#define LANDSCAPE_H

#include "Patch.h"
#include "Map/BaseGroundDrawer.h"
#include "System/EventHandler.h"

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
class Landscape : public CEventClient
{
public:
	//! CEventClient interface
	bool WantsEvent(const std::string& eventName) {
		return (eventName == "UnsyncedHeightMapUpdate");
	}
	bool GetFullRead() const { return true; }
	int  GetReadAllyTeam() const { return AllAccessTeam; }

	void UnsyncedHeightMapUpdate(const SRectangle& rect);

public:
	Landscape();

	void Init(CSMFGroundDrawer* drawer, const float* hMap, int bx, int by);
	void Reset();
	void Tessellate(const float3& campos, int viewradius);
	int Render(bool changed, bool shadows, bool waterdrawn);

	static TriTreeNode* AllocateTri();

protected:
	static int  GetNextTriNode() { return m_NextTriNode; }
	static void SetNextTriNode(int nNextNode) { m_NextTriNode = nNextNode; }

protected:
	static int m_NextTriNode;                //< Index to next free TriTreeNode
	static TriTreeNode m_TriPool[POOL_SIZE]; //< Pool of TriTree nodes for splitting //FIXME this array is 9.5MB large! better use a std::vector? 

	CSMFGroundDrawer* drawer;

public:
	std::vector<Patch> m_Patches; //[NUM_PATCHES_PER_SIDE][NUM_PATCHES_PER_SIDE];  //< Array of patches

	int h, w;
	int updateCount;
};

#endif
