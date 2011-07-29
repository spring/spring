/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SMF_GROUND_DRAWER_H_
#define _SMF_GROUND_DRAWER_H_

#include "Map/BaseGroundDrawer.h"

class CVertexArray;
class CSMFReadMap;

namespace Shader {
	struct IProgramObject;
}

/**
 * Map drawer implementation for the CSMFReadMap map system.
 */
class CSMFGroundDrawer : public CBaseGroundDrawer
{
public:
	CSMFGroundDrawer(CSMFReadMap* rm);
	~CSMFGroundDrawer();

	friend class CSMFReadMap;

	void Draw(bool drawWaterReflection = false, bool drawUnitReflection = false);
	void DrawShadowPass();

	// for non-GLSL clients
	void SetupBaseDrawPass(void) { smfShaderCurrARB = smfShaderBaseARB; }
	void SetupReflDrawPass(void) { smfShaderCurrARB = smfShaderReflARB; }
	void SetupRefrDrawPass(void) { smfShaderCurrARB = smfShaderRefrARB; }

	void Update();
	void UpdateSunDir();

	void IncreaseDetail();
	void DecreaseDetail();

	GL::LightHandler* GetLightHandler() { return &lightHandler; }

private:
#ifdef USE_GML
	static void DoDrawGroundRowMT(void* c, int bty);
	static void DoDrawGroundShadowLODMT(void* c, int nlod);
#endif

	bool LoadMapShaders();
	void CreateWaterPlanes(bool camOufOfMap);
	inline void DrawWaterPlane(bool drawWaterReflection);

	void FindRange(const CCamera* cam, int& xs, int& xe, int y, int lod);
	void DoDrawGroundRow(const CCamera* cam, int bty);
	void DrawVertexAQ(CVertexArray* ma, int x, int y);
	void DrawVertexAQ(CVertexArray* ma, int x, int y, float height);
	void EndStripQ(CVertexArray* ma);
	void DrawGroundVertexArrayQ(CVertexArray*& ma);
	void DoDrawGroundShadowLOD(int nlod);

	inline bool BigTexSquareRowVisible(const CCamera* cam, int) const;
	inline void SetupBigSquare(const int bigSquareX, const int bigSquareY);
	void SetupTextureUnits(bool drawReflection);
	void ResetTextureUnits(bool drawReflection);

	void UpdateCamRestraints(CCamera* camera);


	CSMFReadMap* smfMap;

	int viewRadius;
	int neededLod;

	GLuint waterPlaneCamOutDispList;
	GLuint waterPlaneCamInDispList;

	Shader::IProgramObject* smfShaderBaseARB;   //! default (V+F) SMF ARB shader
	Shader::IProgramObject* smfShaderReflARB;   //! shader (V+F) for the DynamicWater reflection pass
	Shader::IProgramObject* smfShaderRefrARB;   //! shader (V+F) for the DynamicWater refraction pass
	Shader::IProgramObject* smfShaderCurrARB;   //! currently active ARB shader
	Shader::IProgramObject* smfShaderDefGLSL;   //! GLSL shader used when shadows are on
	Shader::IProgramObject* smfShaderAdvGLSL;   //! GLSL shader used when shadows are off
	Shader::IProgramObject* smfShaderCurGLSL;   //! currently active GLSL shader

	GL::LightHandler lightHandler;

	bool useShaders;
	bool waterDrawn;
};

#endif // _SMF_GROUND_DRAWER_H_
