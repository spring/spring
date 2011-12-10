/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SMF_GROUND_DRAWER_H_
#define _SMF_GROUND_DRAWER_H_

#include "Map/BaseGroundDrawer.h"


class CSMFReadMap;
class IMeshDrawer;


namespace Shader {
	struct IProgramObject;
}


enum {
	SMF_MESHDRAWER_LEGACY = 0,
	SMF_MESHDRAWER_ROAM,
	SMF_MESHDRAWER_LAST,
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

	void Draw(const DrawPass::e& drawPass);
	void DrawShadowPass();

	// for non-GLSL clients
	void SetupBaseDrawPass(void) { smfShaderCurrARB = smfShaderBaseARB; }
	void SetupReflDrawPass(void) { smfShaderCurrARB = smfShaderReflARB; }
	void SetupRefrDrawPass(void) { smfShaderCurrARB = smfShaderRefrARB; }

	void Update();
	void UpdateSunDir();

	void IncreaseDetail();
	void DecreaseDetail();
	int GetGroundDetail(const DrawPass::e& drawPass = DrawPass::Normal) const;

	GL::LightHandler* GetLightHandler() { return &lightHandler; }

	void SetupBigSquare(const int bigSquareX, const int bigSquareY);

	void SwitchMeshDrawer(int mode = -1);

private:
	bool LoadMapShaders();
	void CreateWaterPlanes(bool camOufOfMap);
	inline void DrawWaterPlane(bool drawWaterReflection);

	void SetupTextureUnits(bool drawReflection);
	void ResetTextureUnits(bool drawReflection);

protected:
	CSMFReadMap* smfMap;
	IMeshDrawer* meshDrawer;

	int groundDetail;

	GLuint waterPlaneCamOutDispList;
	GLuint waterPlaneCamInDispList;

	Shader::IProgramObject* smfShaderBaseARB;   // default (V+F) SMF ARB shader
	Shader::IProgramObject* smfShaderReflARB;   // shader (V+F) for the DynamicWater reflection pass
	Shader::IProgramObject* smfShaderRefrARB;   // shader (V+F) for the DynamicWater refraction pass
	Shader::IProgramObject* smfShaderCurrARB;   // currently active ARB shader
	Shader::IProgramObject* smfShaderDefGLSL;   // GLSL shader used when shadows are on
	Shader::IProgramObject* smfShaderAdvGLSL;   // GLSL shader used when shadows are off
	Shader::IProgramObject* smfShaderCurGLSL;   // currently active GLSL shader

	GL::LightHandler lightHandler;

	bool useShaders;
	bool waterDrawn;
};

#endif // _SMF_GROUND_DRAWER_H_
