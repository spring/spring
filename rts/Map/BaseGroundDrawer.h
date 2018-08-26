/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _BASE_GROUND_DRAWER_H
#define _BASE_GROUND_DRAWER_H

#include "MapDrawPassTypes.h"


class CBaseGroundTextures;
class CCamera;

namespace GL {
	struct GeometryBuffer;
	struct LightHandler;
}

struct LuaMapShaderData {
	// [0] := standard program from gl.CreateShader
	// [1] := deferred program from gl.CreateShader
	unsigned int shaderIDs[2];
	// Should Spring set the default map shader uniforms?
	bool setDefaultUniforms;
};

class CBaseGroundDrawer
{
public:
	CBaseGroundDrawer();
	virtual ~CBaseGroundDrawer() {}
	CBaseGroundDrawer(const CBaseGroundDrawer&) = delete; // no-copy

	virtual void Draw(const DrawPass::e& drawPass) = 0;
	virtual void DrawShadowPass() {}

	virtual void Update() = 0;
	virtual void UpdateRenderState() = 0;

	virtual void IncreaseDetail() = 0;
	virtual void DecreaseDetail() = 0;
	virtual void SetDetail(int newGroundDetail) = 0;
	virtual int GetGroundDetail(const DrawPass::e& drawPass = DrawPass::Normal) const = 0;

	virtual void SetLuaShader(const LuaMapShaderData*) {}
	virtual void SetDrawForwardPass(bool b) { drawForward = b; }
	virtual void SetDrawDeferredPass(bool) {}

	virtual bool ToggleMapBorder() { drawMapEdges = !drawMapEdges; return drawMapEdges; }

	virtual const GL::LightHandler* GetLightHandler() const { return nullptr; }
	virtual       GL::LightHandler* GetLightHandler()       { return nullptr; }
	virtual const GL::GeometryBuffer* GetGeometryBuffer() const { return nullptr; }
	virtual       GL::GeometryBuffer* GetGeometryBuffer()       { return nullptr; }

	bool DrawForward() const { return drawForward; }
	bool DrawDeferred() const { return drawDeferred; }

	bool UseAdvShading() const { return advShading; }
	bool WireFrameMode() const { return wireframe; }

	bool& UseAdvShadingRef() { return advShading; }
	bool& WireFrameModeRef() { return wireframe; }

	CBaseGroundTextures* GetGroundTextures() { return groundTextures; }

public:
	float LODScaleReflection;
	float LODScaleRefraction;
	float LODScaleTerrainReflection;

	float spPolygonOffsetScale = 10.0f;
	float spPolygonOffsetUnits = 10000.0f;

	int jamColor[3];
	int losColor[3];
	int radarColor[3];
	int alwaysColor[3];
	int radarColor2[3]; // Color of inner radar edge.

	static const int losColorScale = 10000;

protected:
	CBaseGroundTextures* groundTextures;

	bool drawForward;
	bool drawDeferred;
	bool drawMapEdges;

	bool wireframe;
	bool advShading;
};

#endif // _BASE_GROUND_DRAWER_H
