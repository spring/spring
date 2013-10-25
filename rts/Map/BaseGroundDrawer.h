/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _BASE_GROUND_DRAWER_H
#define _BASE_GROUND_DRAWER_H

#include <map>
#include "MapDrawPassTypes.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/PBO.h"
#include "System/float3.h"
#include "System/type2.h"

#define NUM_INFOTEXTURES (1 + 4 + 3)

class CMetalMap;
class CHeightLinePalette;
class CBaseGroundTextures;
class CCamera;

namespace GL {
	struct GeometryBuffer;
	struct LightHandler;
};

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
		drawNormal   = 0,
		drawLos      = 1, // L (';' does not toggle it)
		drawMetal    = 2, // F4
		drawHeight   = 3, // F1
		drawPathTrav = 4, // F2
		drawPathHeat = 5, // not hotkeyed, command-only
		drawPathFlow = 6, // not hotkeyed, command-only
		drawPathCost = 7, // not hotkeyed, command-only
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
	virtual void SetDrawDeferredPass(bool) {}
	virtual bool ToggleMapBorder() { drawMapEdges = !drawMapEdges; return drawMapEdges; }

	virtual const GL::LightHandler* GetLightHandler() const { return NULL; }
	virtual       GL::LightHandler* GetLightHandler()       { return NULL; }
	virtual const GL::GeometryBuffer* GetGeometryBuffer() const { return NULL; }
	virtual       GL::GeometryBuffer* GetGeometryBuffer()       { return NULL; }

	void DrawTrees(bool drawReflection = false) const;

	// Everything that deals with drawing extra textures on top
	void DisableExtraTexture();
	void SetHeightTexture();
	void SetMetalTexture(const CMetalMap*);
	void TogglePathTexture(BaseGroundDrawMode);
	void ToggleLosTexture();
	void ToggleRadarAndJammer();
	bool UpdateExtraTexture(unsigned int texDrawMode);

	bool DrawExtraTex() const { return drawMode != drawNormal; }
	bool DrawDeferred() const { return drawDeferred; }

	bool UseAdvShading() const { return advShading; }
	bool WireFrameMode() const { return wireframe; }

	bool& UseAdvShadingRef() { return advShading; }
	bool& WireFrameModeRef() { return wireframe; }


	BaseGroundDrawMode GetDrawMode() const { return drawMode; }
	CBaseGroundTextures* GetGroundTextures() { return groundTextures; }

	GLuint GetInfoTexture(unsigned int idx) const { return infoTextureIDs[idx]; }
	GLuint GetActiveInfoTexture() const { return infoTextureIDs[drawMode]; }

	int2 GetInfoTexSize() const;

	void UpdateCamRestraints(CCamera* camera);

public:
	bool drawRadarAndJammer;
	bool drawLineOfSight;

	bool highResLosTex;
	bool highResInfoTex;
	bool highResInfoTexWanted;

	float LODScaleReflection;
	float LODScaleRefraction;
	float LODScaleUnitReflection;

	const unsigned char* extraTex;
	const unsigned char* extraTexPal;
	const float* extractDepthMap;

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

protected:
	BaseGroundDrawMode drawMode;

	// note: first texture ID is always 0!
	GLuint infoTextureIDs[NUM_INFOTEXTURES];

	PBO infoTexPBO;

	CHeightLinePalette* heightLinePal;
	CBaseGroundTextures* groundTextures;

	bool drawMapEdges;
	bool drawDeferred;

	bool wireframe;
	bool advShading;
};

#endif // _BASE_GROUND_DRAWER_H
