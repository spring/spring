// BFGroundTextures.h
///////////////////////////////////////////////////////////////////////////

#ifndef __BASE_GROUND_DRAWER_H__
#define __BASE_GROUND_DRAWER_H__

#include <vector>
#include "Rendering/Env/BaseTreeDrawer.h"
#include "float3.h"

class CBaseGroundDrawer
{
public:
	CBaseGroundDrawer(void);
	virtual ~CBaseGroundDrawer(void);

	virtual void Draw(bool drawWaterReflection=false,bool drawUnitReflection=false,unsigned int overrideVP=0)=0;
	virtual void DrawShadowPass(void);
	virtual void UpdateCamRestraints() = 0;
	virtual void Update()=0;

	enum DrawMode
	{
		drawNormal,
		drawLos,
		drawMetal,
		drawHeight,
		drawPath
	};

protected:
	virtual void SetDrawMode(DrawMode dm);
public:
	// Everything that deals with drawing extra textures on top
	void DisableExtraTexture();
	void SetHeightTexture();
	void SetMetalTexture(unsigned char* tex,float* extractMap,unsigned char* pal,bool highRes);
	void SetPathMapTexture();
	void ToggleLosTexture();
	bool UpdateExtraTexture();
	bool DrawExtraTex() { return drawMode!=drawNormal; }

	void SetTexGen(float scalex,float scaley, float offsetx, float offsety);

	bool updateFov;
	bool drawRadarAndJammer;

	int striptype;
	unsigned int infoTex;

	unsigned char* infoTexMem;
	bool highResInfoTex;
	bool highResInfoTexWanted;

	unsigned char* extraTex;
	unsigned char* extraTexPal;
	float* extractDepthMap;

	int updateTextureState;

	float infoTexAlpha;
	int viewRadius;

	DrawMode drawMode;
};

#endif // __BASE_GROUND_DRAWER__

