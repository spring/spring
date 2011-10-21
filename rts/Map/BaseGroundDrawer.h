/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __BASE_GROUND_DRAWER_H__
#define __BASE_GROUND_DRAWER_H__

#include <map>
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/LightHandler.h"
#include "Rendering/GL/PBO.h"
#include "System/float3.h"

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

	CBaseGroundDrawer(void);
	virtual ~CBaseGroundDrawer(void);

	virtual void Draw(bool drawWaterReflection = false, bool drawUnitReflection = false) = 0;
	virtual void DrawShadowPass(void);

	virtual void SetupBaseDrawPass(void) {}
	virtual void SetupReflDrawPass(void) {}
	virtual void SetupRefrDrawPass(void) {}

	virtual void Update() = 0;
	virtual void UpdateSunDir() = 0;

	virtual void IncreaseDetail() = 0;
	virtual void DecreaseDetail() = 0;

	virtual void SetDrawMode(BaseGroundDrawMode dm) { drawMode = dm; }
	virtual GL::LightHandler* GetLightHandler() { return NULL; }

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

	CBaseGroundTextures* GetGroundTextures() { return groundTextures; }

	void UpdateCamRestraints(CCamera* camera);

public:
	bool wireframe;
	bool advShading;

	float LODScaleReflection;
	float LODScaleRefraction;
	float LODScaleUnitReflection;

	BaseGroundDrawMode drawMode;

	bool drawRadarAndJammer;
	bool drawLineOfSight;

	int updateTextureState;

	GLuint infoTex;
	PBO extraTexPBO;
	bool highResInfoTex;
	bool highResInfoTexWanted;

	const unsigned char* extraTex;
	const unsigned char* extraTexPal;
	const float* extractDepthMap;

	float infoTexAlpha;

	int jamColor[3];
	int losColor[3];
	int radarColor[3];
	int alwaysColor[3];
	static const int losColorScale = 10000;

	bool highResLosTex;
	int extraTextureUpdateRate;

#ifdef USE_GML
	bool multiThreadDrawGround;
	bool multiThreadDrawGroundShadow;
#endif

	CHeightLinePalette* heightLinePal;
	CBaseGroundTextures* groundTextures;
};

#endif // __BASE_GROUND_DRAWER__
