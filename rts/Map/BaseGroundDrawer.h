/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _BASE_GROUND_DRAWER_H
#define _BASE_GROUND_DRAWER_H

#include <map>
#include "MapDrawPassTypes.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/LightHandler.h"
#include "Rendering/GL/PBO.h"
#include "System/float3.h"
#include "System/type2.h"

class CMetalMap;
class CHeightLinePalette;
class CBaseGroundTextures;
class CCamera;

class CBaseGroundDrawer
{
public:
	enum {
		COLOR_R = 2,
		COLOR_G = 1,
		COLOR_B = 0,
		COLOR_A = 3,
	};
	enum BaseGroundDrawMode {
		drawNormal,
		drawLos,
		drawMetal,
		drawHeight,
		drawPathTraversability,
		drawPathHeat,
		drawPathFlow,
		drawPathCost,
	};
	enum {
		GBUFFER_ATTACHMENT_NORMTEX = 0, // shading (not geometric) normals
		GBUFFER_ATTACHMENT_DIFFTEX = 1, // diffuse texture fragments
		GBUFFER_ATTACHMENT_SPECTEX = 2, // specular texture fragments
		GBUFFER_ATTACHMENT_ZVALTEX = 3,
		GBUFFER_ATTACHMENT_COUNT   = 4,
	};

	CBaseGroundDrawer();
	virtual ~CBaseGroundDrawer();

	virtual void Draw(const DrawPass::e& drawPass) = 0;
	virtual void DrawShadowPass();

	virtual void SetupBaseDrawPass() {}
	virtual void SetupReflDrawPass() {}
	virtual void SetupRefrDrawPass() {}

	virtual void Update() = 0;
	virtual void UpdateSunDir() = 0;

	virtual void IncreaseDetail() = 0;
	virtual void DecreaseDetail() = 0;
	virtual int GetGroundDetail(const DrawPass::e& drawPass = DrawPass::Normal) const = 0;

	virtual void SetDrawMode(BaseGroundDrawMode dm) { drawMode = dm; }
	virtual void SetDeferredDrawMode(bool) {}
	virtual bool ToggleMapBorder() { drawMapEdges = !drawMapEdges; return drawMapEdges; }

	virtual GL::LightHandler* GetLightHandler() { return NULL; }
	virtual GLuint GetGeomBufferTexture(unsigned int idx) { return 0; }

	void DrawTrees(bool drawReflection = false) const;

	// Everything that deals with drawing extra textures on top
	void DisableExtraTexture();
	void SetHeightTexture();
	void SetMetalTexture(const CMetalMap*);
	void TogglePathTexture(BaseGroundDrawMode);
	void ToggleLosTexture();
	void ToggleRadarAndJammer();
	bool UpdateExtraTexture();

	bool DrawExtraTex() const { return drawMode != drawNormal; }
	bool DrawDeferred() const { return drawDeferred; }

	BaseGroundDrawMode GetDrawMode() const { return drawMode; }
	CBaseGroundTextures* GetGroundTextures() { return groundTextures; }

	int2 GetGeomBufferSize(bool allowed) const;
	int2 GetInfoTexSize() const;

	void UpdateCamRestraints(CCamera* camera);

public:
	bool wireframe;
	bool advShading;

	bool drawRadarAndJammer;
	bool drawLineOfSight;

	bool highResLosTex;
	bool highResInfoTex;
	bool highResInfoTexWanted;

	float LODScaleReflection;
	float LODScaleRefraction;
	float LODScaleUnitReflection;

	GLuint infoTex;
	PBO extraTexPBO;

	const unsigned char* extraTex;
	const unsigned char* extraTexPal;
	const float* extractDepthMap;

	float infoTexAlpha;

	int jamColor[3];
	int losColor[3];
	int radarColor[3];
	int alwaysColor[3];

	static const int losColorScale = 10000;

	int updateTextureState;
	int extraTextureUpdateRate;

#ifdef USE_GML
	bool multiThreadDrawGround;
	bool multiThreadDrawGroundShadow;
#endif

	CHeightLinePalette* heightLinePal;
	CBaseGroundTextures* groundTextures;

protected:
	BaseGroundDrawMode drawMode;

	bool drawMapEdges;
	bool drawDeferred;
};

#endif // _BASE_GROUND_DRAWER_H
