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






/**
 * Map mesh drawer implementation
 */
class CRoamMeshDrawer : public IMeshDrawer, public CEventClient
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

private:
	void Reset();
	void Tessellate(const float3& campos, int viewradius);
	int Render(bool shadows);
	
public:
	static bool forceRetessellate;

private:
	CSMFReadMap* smfReadMap;
	CSMFGroundDrawer* smfGroundDrawer;

	float3 lastCamPos;
	int lastGroundDetail;

	int numPatchesX;
	int numPatchesY;

	std::vector<Patch> m_Patches; //[NUM_PATCHES_PER_SIDE][NUM_PATCHES_PER_SIDE];  //< Array of patches
	bool* visibilitygrid;
};

#endif // _ROAM_MESH_DRAWER_H_
