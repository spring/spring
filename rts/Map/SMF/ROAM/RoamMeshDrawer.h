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
	bool WantsEvent(const std::string& eventName) override {
		return (eventName == "UnsyncedHeightMapUpdate") || (eventName == "DrawInMiniMap");
	}
	bool GetFullRead() const override { return true; }
	int  GetReadAllyTeam() const override { return AllAccessTeam; }

	void UnsyncedHeightMapUpdate(const SRectangle& rect) override;
	void DrawInMiniMap() override;

public:
	enum {
		MESH_NORMAL = 0,
		MESH_SHADOW = 1,
		MESH_COUNT  = 2,
	};

	CRoamMeshDrawer(CSMFGroundDrawer* gd);
	~CRoamMeshDrawer();

	void Update() override;

	void DrawMesh(const DrawPass::e& drawPass) override;
	void DrawBorderMesh(const DrawPass::e& drawPass) override;

	static void ForceNextTesselation(bool normal, bool shadow) {
		forceNextTesselation[MESH_NORMAL] = normal;
		forceNextTesselation[MESH_SHADOW] = shadow;
	}

private:
	void Reset(bool shadowPass);
	void Tessellate(std::vector<Patch>& patches, const CCamera* cam, int viewRadius, bool shadowPass);

private:
	CSMFGroundDrawer* smfGroundDrawer;

	int numPatchesX = 0;
	int numPatchesY = 0;
	std::array<int, MESH_COUNT> lastGroundDetail = {};

	bool heightMapChanged = false;

	std::array<float3, MESH_COUNT> lastCamPos;
	std::array<float3, MESH_COUNT> lastCamDir;

	std::array<int, MESH_COUNT> numPatchesLeftVisibility = {};
	std::array<int, MESH_COUNT> tesselationsSinceLastReset = {};
	std::function<bool(std::vector<Patch>&, const CCamera*, int, bool)> tesselateFuncs[2];

	// [1] is used for the shadow pass, [0] is used for all other passes
	std::vector< Patch > patchMeshGrid[MESH_COUNT];
	std::vector< Patch*> borderPatches[MESH_COUNT];

	// char instead of bool, accessors to different elements must be thread-safe
	std::vector<uint8_t> patchVisFlags[MESH_COUNT];

	// whether tessellation should be forcibly performed next frame
	static bool forceNextTesselation[MESH_COUNT];

#ifdef DRAW_DEBUG_IN_MINIMAP
	std::vector<float3> debugColors;
#endif
};

#endif // _ROAM_MESH_DRAWER_H_
