/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SMF_GROUND_DRAWER_H_
#define _SMF_GROUND_DRAWER_H_

#include "Map/BaseGroundDrawer.h"
#include "Rendering/GL/GeometryBuffer.h"
#include "Rendering/GL/LightHandler.h"
#include "Rendering/GL/RenderDataBuffer.hpp"


class CSMFReadMap;
class IMeshDrawer;
struct ISMFRenderState;

enum {
	SMF_MESHDRAWER_LEGACY = 0,
	SMF_MESHDRAWER_BASIC  = 1,
	SMF_MESHDRAWER_ROAM   = 2,
	SMF_MESHDRAWER_LAST   = 3,
};


/**
 * Map drawer implementation for the CSMFReadMap map system.
 */
class CSMFGroundDrawer : public CBaseGroundDrawer
{
public:
	CSMFGroundDrawer(CSMFReadMap* rm);
	~CSMFGroundDrawer();

	friend class CSMFReadMap;

	void Draw(const DrawPass::e& drawPass) override;
	void DrawDeferredPass(const DrawPass::e& drawPass, bool alphaTest);
	void DrawForwardPass(const DrawPass::e& drawPass, bool alphaTest);
	void DrawShadowPass() override;

	void Update() override;
	void UpdateRenderState() override;
	void SunChanged();

	void SetLuaShader(const LuaMapShaderData*) override;
	void SetDrawDeferredPass(bool b) override {
		if ((drawDeferred = b)) {
			drawDeferred &= UpdateGeometryBuffer(false);
		}
	}

	void SetupBigSquare(const int bigSquareX, const int bigSquareY);


	void IncreaseDetail() override { SetDetail(groundDetail + 1); }
	void DecreaseDetail() override { SetDetail(groundDetail - 1); }
	void SetDetail(int newGroundDetail) override;
	int GetGroundDetail(const DrawPass::e& drawPass = DrawPass::Normal) const override;

	const CSMFReadMap* GetReadMap() const { return smfMap; }
	      CSMFReadMap* GetReadMap()       { return smfMap; }
	const GL::LightHandler* GetLightHandler() const override { return &lightHandler; }
	      GL::LightHandler* GetLightHandler()       override { return &lightHandler; }

	const GL::GeometryBuffer* GetGeometryBuffer() const override { return &geomBuffer; }
	      GL::GeometryBuffer* GetGeometryBuffer()       override { return &geomBuffer; }

	IMeshDrawer* GetMeshDrawer() { return meshDrawer; }
	IMeshDrawer* SwitchMeshDrawer(int wantedMode = -1);

private:
	ISMFRenderState* SelectRenderState(const DrawPass::e& drawPass);

	void CreateWaterPlanes(bool camOufOfMap);
	void CreateBorderShader();

	void DrawWaterPlane(bool drawWaterReflection);
	void DrawBorder(const DrawPass::e drawPass);

	bool HaveLuaRenderState() const;
	bool UpdateGeometryBuffer(bool init);

protected:
	CSMFReadMap* smfMap;
	IMeshDrawer* meshDrawer;

	int drawerMode;
	int groundDetail;

	// [0] := no-op rendering path
	// [1] := default shader-driven rendering path
	// [2] := custom shader-driven rendering path (via Lua)
	// [3] := currently selected state (shared by deferred pass)
	std::vector<ISMFRenderState*> smfRenderStates;

	GL::LightHandler lightHandler;
	GL::GeometryBuffer geomBuffer;
	GL::RenderDataBuffer waterPlaneBuffers[2];
	Shader::GLSLProgramObject borderShader;
};

#endif // _SMF_GROUND_DRAWER_H_
