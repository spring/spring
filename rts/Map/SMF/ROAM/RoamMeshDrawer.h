/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _ROAM_MESH_DRAWER_H_
#define _ROAM_MESH_DRAWER_H_

#include "Patch.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/SMF/IMeshDrawer.h"
#include "System/EventHandler.h"
#include <vector>


class CSMFReadMap;
class CSMFGroundDrawer;


// How many TriTreeNodes should be allocated?
#define POOL_SIZE (500000)

// How many heightmap pixels a patch consists of
#define PATCH_SIZE 128


/**
 * Map mesh drawer implementation
 */
class CRoamMeshDrawer : public IMeshDrawer, public CEventClient //FIXME merge with landscape class
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
	CRoamMeshDrawer(CSMFReadMap* rm, CSMFGroundDrawer* gd);
	~CRoamMeshDrawer();

	void Update();

	void DrawMesh(const DrawPass::e& drawPass);

	// used by Patch.cpp
	static TriTreeNode* AllocateTri();

private:
	void Reset();
	void Tessellate(const float3& campos, int viewradius);
	int Render(bool shadows);

protected:
	static int  GetNextTriNode() { return m_NextTriNode; }
	static void SetNextTriNode(int nNextNode) { m_NextTriNode = nNextNode; }

	static int m_NextTriNode;                //< Index to next free TriTreeNode
	static TriTreeNode m_TriPool[POOL_SIZE]; //< Pool of TriTree nodes for splitting //FIXME this array is 9.5MB large! better use a std::vector? 

public:
	static bool forceRetessellate;

private:
	CSMFReadMap* smfReadMap;
	CSMFGroundDrawer* smfGroundDrawer;

	float3 lastCamPos;
	int lastViewRadius;

	int numBigTexX;
	int numBigTexY;
	int h, w;

	std::vector<Patch> m_Patches; //[NUM_PATCHES_PER_SIDE][NUM_PATCHES_PER_SIDE];  //< Array of patches
	bool* visibilitygrid;
};

#endif // _ROAM_MESH_DRAWER_H_
