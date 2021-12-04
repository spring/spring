/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _BASIC_MESH_DRAWER_H_
#define _BASIC_MESH_DRAWER_H_

#include <array>
#include <vector>

#include "Map/MapDrawPassTypes.h"
#include "Map/SMF/IMeshDrawer.h"
#include "Rendering/GL/VAO.h"
#include "Rendering/GL/VBO.h"
#include "System/EventClient.h"

class CSMFGroundDrawer;
class CCamera;

class CBasicMeshDrawer : public IMeshDrawer, public CEventClient {
public:
	bool WantsEvent(const std::string& eventName) override { return (eventName == "UnsyncedHeightMapUpdate"); }
	bool GetFullRead() const override { return true; }
	int  GetReadAllyTeam() const override { return AllAccessTeam; }

public:
	CBasicMeshDrawer(CSMFGroundDrawer* gd);
	~CBasicMeshDrawer();

	static constexpr int32_t PATCH_SIZE = 128; // must match SMFReadMap::bigSquareSize
	static constexpr int32_t LOD_LEVELS =   8; // log2(PATCH_SIZE) + 1; 129x129 to 2x2

	enum {
		MAP_BORDER_L = 0,
		MAP_BORDER_R = 1,
		MAP_BORDER_T = 2,
		MAP_BORDER_B = 3,
	};

	struct MeshBuffer {
		VAO vao;
		VBO vbo;
	};

	struct MeshPatch {
		std::array<MeshBuffer, LOD_LEVELS> squareVertexBuffers;
		// std::array<MeshBuffer, LOD_LEVELS> squareNormalBuffers;

		std::array<MeshBuffer, LOD_LEVELS> borderVertexBuffers[MAP_BORDER_B + 1];
		// std::array<MeshBuffer, LOD_LEVELS> borderNormalBuffers[MAP_BORDER_B + 1];

		std::array<float3*, LOD_LEVELS> squareVertexPtrs;
		// std::array<float3*, LOD_LEVELS> squareNormalPtrs;

		std::array<uint32_t, 4> visUpdateFrames; // [CAMTYPE_PLAYER, CAMTYPE_ENVMAP]
		std::array<uint32_t, 1> uhmUpdateFrames;
	};

	void Update(const DrawPass::e& drawPass);
	void Update() override {}
	void UnsyncedHeightMapUpdate(const SRectangle& rect) override;

	void DrawMesh(const DrawPass::e& drawPass) override;
	void DrawBorderMesh(const DrawPass::e& drawPass) override;

private:
	void UploadPatchSquareGeometry(uint32_t n, uint32_t px, uint32_t py, const float* chm, const float3* cnm);
	void UploadPatchBorderGeometry(uint32_t n, uint32_t px, uint32_t py, const float* chm, const float3* cnm);
	void UploadPatchIndices(uint32_t n);

	void DrawSquareMeshPatch(const MeshPatch& meshPatch, const CCamera* activeCam) const;
	void DrawBorderMeshPatch(const MeshPatch& meshPatch, const CCamera* activeCam, uint32_t borderSide) const;

	uint32_t CalcDrawPassLOD(const CCamera* cam, const DrawPass::e& drawPass) const;

private:
	uint32_t numPatchesX;
	uint32_t numPatchesY;
	uint32_t drawPassLOD;

	std::vector<MeshPatch> meshPatches;

	std::array<float, LOD_LEVELS> lodDistTable;
	std::array<VBO, LOD_LEVELS> lodSquareIndexBuffers;
	std::array<VBO, LOD_LEVELS> lodBorderIndexBuffers;

	CSMFGroundDrawer* smfGroundDrawer;
};

#endif

