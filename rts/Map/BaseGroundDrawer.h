// BFGroundTextures.h
///////////////////////////////////////////////////////////////////////////

#ifndef __BASE_GROUND_DRAWER_H__
#define __BASE_GROUND_DRAWER_H__

#include <vector>
#include "Rendering/Env/BaseTreeDrawer.h"
#include "Rendering/GL/myGL.h"
#include "float3.h"

class CHeightLinePalette;

class CBaseGroundDrawer
{
public:
	CBaseGroundDrawer(void);
	virtual ~CBaseGroundDrawer(void);

	virtual void Draw(bool drawWaterReflection = false, bool drawUnitReflection = false) = 0;
	virtual void DrawShadowPass(void);

	virtual void SetupBaseDrawPass(void) {}
	virtual void SetupReflDrawPass(void) {}
	virtual void SetupRefrDrawPass(void) {}

	virtual void Update() = 0;

	virtual void IncreaseDetail() = 0;
	virtual void DecreaseDetail() = 0;

#ifdef USE_GML
	int multiThreadDrawGround;
	int multiThreadDrawGroundShadow;
#endif

	enum BaseGroundDrawMode {
		drawNormal,
		drawLos,
		drawMetal,
		drawHeight,
		drawPath
	};

protected:
	virtual void SetDrawMode(BaseGroundDrawMode dm) { drawMode = dm; }

public:
	void DrawTrees(bool drawReflection=false) const;

	// Everything that deals with drawing extra textures on top
	void DisableExtraTexture();
	void SetHeightTexture();
	void SetMetalTexture(unsigned char* tex,float* extractMap,unsigned char* pal,bool highRes);
	void SetPathMapTexture();
	void ToggleLosTexture();
	void ToggleRadarAndJammer();
	bool UpdateExtraTexture();
	bool DrawExtraTex() const { return drawMode!=drawNormal; };

	void SetTexGen(float scalex,float scaley, float offsetx, float offsety) const;

	bool updateFov;
	bool drawRadarAndJammer;
	bool drawLineOfSight;
	bool wireframe;

	float LODScaleReflection;
	float LODScaleRefraction;
	float LODScaleUnitReflection;

	GLuint infoTex;

	unsigned char* infoTexMem;
	bool highResInfoTex;
	bool highResInfoTexWanted;

	const unsigned char* extraTex;
	const unsigned char* extraTexPal;
	float* extractDepthMap;

	int updateTextureState;

	BaseGroundDrawMode drawMode;

	float infoTexAlpha;

	int jamColor[3];
	int losColor[3];
	int radarColor[3];
	int alwaysColor[3];
	static const int losColorScale = 10000;

	bool highResLosTex;
// 	bool smoothLosTex;

	CHeightLinePalette* heightLinePal;
};

#endif // __BASE_GROUND_DRAWER__

