/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _ROAM_MESH_DRAWER_H_
#define _ROAM_MESH_DRAWER_H_

#include <vector>

#include "Patch.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/SMF/IMeshDrawer.h"
#include "System/EventHandler.h"



class CSMFReadMap;
class CSMFGroundDrawer;
class CCamera;



// Visualize visible patches in Minimap for debugging?
// #define DRAW_DEBUG_IN_MINIMAP


/**
 * Map mesh drawer implementation; based on the Tread Marks engine
 * by Longbow Digital Arts (www.LongbowDigitalArts.com, circa 2000)
 */
class CRoamMeshDrawer : public IMeshDrawer, public CEventClient
{
public:
	// CEventClient interface
	bool WantsEvent(const std::string& eventName) {
		return (eventName == "UnsyncedHeightMapUpdate") || (eventName == "DrawInMiniMap");
	}
	bool GetFullRead() const { return true; }
	int  GetReadAllyTeam() const { return AllAccessTeam; }

	void UnsyncedHeightMapUpdate(const SRectangle& rect);
	void DrawInMiniMap();

public:
	CRoamMeshDrawer(CSMFReadMap* rm, CSMFGroundDrawer* gd);
	~CRoamMeshDrawer();

	void Update();

	void DrawMesh(const DrawPass::e& drawPass);
	void DrawBorderMesh(const DrawPass::e& drawPass);

	static void ForceTesselation() {
		forceTessellate[0] = true;
		forceTessellate[1] = true;
	}

private:
	void Reset(std::vector<Patch>& patches);
	bool Tessellate(std::vector<Patch>& patches, const CCamera* cam, int viewRadius);

private:
	CSMFReadMap* smfReadMap;
	CSMFGroundDrawer* smfGroundDrawer;

	int numPatchesX;
	int numPatchesY;
	int lastGroundDetail[2];

	float3 lastCamPos[2];

	// [1] is used for the shadow pass, [0] is used for all other passes
	std::vector< Patch > patchMeshGrid[2];
	std::vector< Patch*> borderPatches[2];

	//< char instead of bool, accessors to different elements must be thread-safe
	std::vector<uint8_t> patchVisFlags[2];

	//< whether tessellation should be forcibly performed next frame
	static bool forceTessellate[2];
};

#endif // _ROAM_MESH_DRAWER_H_
