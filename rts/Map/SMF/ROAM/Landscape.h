/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LANDSCAPE_H
#define LANDSCAPE_H

#include "Patch.h"
#include "Map/BaseGroundDrawer.h"
#include "System/EventHandler.h"

#include <vector>

class CSMFGroundDrawer;

// How many TriTreeNodes should be allocated?
#define POOL_SIZE (500000)

// How many heightmap pixels a patch consists of
#define PATCH_SIZE 128

//
// Landscape Class
// Holds all the information to render an entire landscape.
//
class Landscape : public CEventClient
{
public:
	// CEventClient interface
	bool WantsEvent(const std::string& eventName) {
		return (eventName == "UnsyncedHeightMapUpdate");
	}
	bool GetFullRead() const { return true; }
	int  GetReadAllyTeam() const { return AllAccessTeam; }

	void UnsyncedHeightMapUpdate(const SRectangle& rect);

public:
	Landscape(CSMFGroundDrawer* drawer, const float* hMap, int bx, int by);

	void Reset();
	void Tessellate(const float3& campos, int viewradius);
	int Render(bool shadows);

	static TriTreeNode* AllocateTri();

protected:
	static int  GetNextTriNode() { return m_NextTriNode; }
	static void SetNextTriNode(int nNextNode) { m_NextTriNode = nNextNode; }

protected:
	static int m_NextTriNode;                //< Index to next free TriTreeNode
	static TriTreeNode m_TriPool[POOL_SIZE]; //< Pool of TriTree nodes for splitting //FIXME this array is 9.5MB large! better use a std::vector? 

	CSMFGroundDrawer* drawer;

	int h, w;

public:
	std::vector<Patch> m_Patches; //[NUM_PATCHES_PER_SIDE][NUM_PATCHES_PER_SIDE];  //< Array of patches
};

#endif
