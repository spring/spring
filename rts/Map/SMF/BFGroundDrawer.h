/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _BF_GROUND_DRAWER_H_
#define _BF_GROUND_DRAWER_H_

#include "Map/BaseGroundDrawer.h"

class CVertexArray;
class CSmfReadMap;
class CBFGroundTextures;

namespace Shader {
	struct IProgramObject;
}

/**
Map drawer implementation for the CSmfReadMap map system.
*/
class CBFGroundDrawer : public CBaseGroundDrawer
{
public:
	CBFGroundDrawer(CSmfReadMap*);
	~CBFGroundDrawer(void);

	friend class CSmfReadMap;

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
	struct fline {
		float base;
		float dir;
	};

#ifdef USE_GML
	static void DoDrawGroundRowMT(void* c, int bty) { ((CBFGroundDrawer*) c)->DoDrawGroundRow(bty); }
	static void DoDrawGroundShadowLODMT(void* c, int nlod) { ((CBFGroundDrawer*) c)->DoDrawGroundShadowLOD(nlod); }
#endif

	bool LoadMapShaders();
	void CreateWaterPlanes(bool camOufOfMap);
	inline void DrawWaterPlane(bool drawWaterReflection);

	void FindRange(int &xs, int &xe, const std::vector<fline>& left, const std::vector<fline>& right, int y, int lod);
	void DoDrawGroundRow(int bty);
	void DrawVertexAQ(CVertexArray* ma, int x, int y);
	void DrawVertexAQ(CVertexArray* ma, int x, int y, float height);
	void EndStripQ(CVertexArray* ma);
	void DrawGroundVertexArrayQ(CVertexArray* &ma);
	void DoDrawGroundShadowLOD(int nlod);

	inline bool BigTexSquareRowVisible(int);
	inline void SetupBigSquare(const int bigSquareX, const int bigSquareY);
	void SetupTextureUnits(bool drawReflection);
	void ResetTextureUnits(bool drawReflection);

	void AddFrustumRestraint(const float3& side);
	void UpdateCamRestraints();


	CSmfReadMap* map;
	CBFGroundTextures* textures;

	const int bigSquareSize;
	const int numBigTexX;
	const int numBigTexY;
	const int maxIdx;

	int mapWidth;
	int mapHeight;
	int bigTexH;

	int viewRadius;
	int neededLod;

	GLuint waterPlaneCamOutDispList;
	GLuint waterPlaneCamInDispList;

	float* heightData;
	const int heightDataX;

	Shader::IProgramObject* smfShaderBaseARB;   //! default (V+F) SMF ARB shader
	Shader::IProgramObject* smfShaderReflARB;   //! shader (V+F) for the DynamicWater reflection pass
	Shader::IProgramObject* smfShaderRefrARB;   //! shader (V+F) for the DynamicWater refraction pass
	Shader::IProgramObject* smfShaderCurrARB;   //! currently active ARB shader
	Shader::IProgramObject* smfShaderDefGLSL;   //! GLSL shader used when shadows are on
	Shader::IProgramObject* smfShaderAdvGLSL;   //! GLSL shader used when shadows are off
	Shader::IProgramObject* smfShaderCurGLSL;   //! currently active GLSL shader

	std::vector<fline> right, left;

	GL::LightHandler lightHandler;

	bool useShaders;
	bool waterDrawn;
};

#endif // _BF_GROUND_DRAWER_H_
