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
class CCamera;



// Visualize visible patches in Minimap for debugging?
//#define DRAW_DEBUG_IN_MINIMAP


/**
 * Map mesh drawer implementation
 */
class CRoamMeshDrawer : public IMeshDrawer, public CEventClient
{
public:
	// CEventClient interface
	bool WantsEvent(const std::string& eventName) {
		return (eventName == "UnsyncedHeightMapUpdate")
#ifdef DRAW_DEBUG_IN_MINIMAP
			|| (eventName == "DrawInMiniMap")
#endif
		;
	}
	bool GetFullRead() const { return true; }
	int  GetReadAllyTeam() const { return AllAccessTeam; }

	void UnsyncedHeightMapUpdate(const SRectangle& rect);
#ifdef DRAW_DEBUG_IN_MINIMAP
	void DrawInMiniMap();
#endif

public:
	CRoamMeshDrawer(CSMFReadMap* rm, CSMFGroundDrawer* gd);
	~CRoamMeshDrawer();

	void Update();

	void DrawMesh(const DrawPass::e& drawPass);
	void DrawBorderMesh(const DrawPass::e& drawPass);

	static void ForceTesselation() { forceRetessellate = true; }

private:
	void Reset();
	bool Tessellate(const CCamera* cam, int viewradius);
	bool IsInteriorPatch(int px, int py) const;
	int Render(bool shadows);

private:
	CSMFReadMap* smfReadMap;
	CSMFGroundDrawer* smfGroundDrawer;

	float3 lastCamPos;
	int lastGroundDetail;

	int numPatchesX;
	int numPatchesY;

	std::vector<Patch> roamPatches; //< array of patches, size [NUM_PATCHES_PER_SIDE][NUM_PATCHES_PER_SIDE]
	std::vector<char> patchVisGrid; //< char instead of bool, accessors to different elements must be thread-safe

	static bool forceRetessellate;
};

#endif // _ROAM_MESH_DRAWER_H_
