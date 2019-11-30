/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _ROAM_MESH_DRAWER_H_
#define _ROAM_MESH_DRAWER_H_

#include <vector>

#include "Patch.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/SMF/IMeshDrawer.h"
#include "System/EventHandler.h"



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
	enum {
		MESH_NORMAL = 0,
		MESH_SHADOW = 1,
		MESH_COUNT  = 2,
	};

	CRoamMeshDrawer(CSMFGroundDrawer* gd);
	~CRoamMeshDrawer();

	void Update();

	void DrawMesh(const DrawPass::e& drawPass);
	void DrawBorderMesh(const DrawPass::e& drawPass);

	static void ForceNextTesselation(bool normal, bool shadow) {
		forceNextTesselation[MESH_NORMAL] = normal;
		forceNextTesselation[MESH_SHADOW] = shadow;
	}
	static void UseThreadTesselation(bool normal, bool shadow) {
		useThreadTesselation[MESH_NORMAL] = normal;
		useThreadTesselation[MESH_SHADOW] = shadow;
	}

private:
	void Reset(bool shadowPass);
	void Tessellate(std::vector<Patch>& patches, const CCamera* cam, int viewRadius, bool useThreads, bool shadowPass) {
		forceNextTesselation[shadowPass] = tesselateFuncs[useThreads](patches, cam, viewRadius, shadowPass);
	}

private:
	CSMFGroundDrawer* smfGroundDrawer;

	int numPatchesX = 0;
	int numPatchesY = 0;
	int lastGroundDetail[MESH_COUNT] = {0, 0};

	float3 lastCamPos[MESH_COUNT];

	std::function<bool(std::vector<Patch>&, const CCamera*, int, bool)> tesselateFuncs[2];

	// [1] is used for the shadow pass, [0] is used for all other passes
	std::vector< Patch > patchMeshGrid[MESH_COUNT];
	std::vector< Patch*> borderPatches[MESH_COUNT];

	// char instead of bool, accessors to different elements must be thread-safe
	std::vector<uint8_t> patchVisFlags[MESH_COUNT];

	// whether tessellation should be forcibly performed next frame
	static bool forceNextTesselation[MESH_COUNT];
	// whether tessellation should be performed with threads
	static bool useThreadTesselation[MESH_COUNT];
};

#endif // _ROAM_MESH_DRAWER_H_
