// BFGroundDrawer.h
///////////////////////////////////////////////////////////////////////////

#ifndef __BF_GROUND_DRAWER_H__
#define __BF_GROUND_DRAWER_H__

#include "BaseGroundDrawer.h"

class CVertexArray;

class CBFGroundDrawer :
	public CBaseGroundDrawer
{
public:
	CBFGroundDrawer(void);
	~CBFGroundDrawer(void);
	void Draw(bool drawWaterReflection=false);
	void SetExtraTexture(unsigned char* tex,unsigned char* pal,bool highRes);
	void SetMetalTexture(unsigned char* tex,float* extractMap,unsigned char* pal,bool highRes);

	bool UpdateTextures();
	void ToggleLosTexture();

protected:
	int numBigTexX;
	int numBigTexY;

	unsigned char* infoTexMem;
	bool highResInfoTex;
	bool highResInfoTexWanted;

	unsigned char* extraTex;
	unsigned char* extraTexPal;
	float* extractDepthMap;

	float infoTexSizeX;
	float infoTexSizeY;
	
	float* heightData;
	int heightDataX;

	int updateTextureState;

	CVertexArray* va;

	unsigned int groundVP;
	unsigned int groundShadowVP;
	unsigned int groundFPShadow;

	inline void DrawVertexA(int x,int y);
	inline void DrawVertexA(int x,int y,float height);
	inline void EndStrip();
	void DrawGroundVertexArray();
	void SetupTextureUnits(bool drawReflection);
	void ResetTextureUnits(bool drawReflection);
	void SetTexGen(float scalex,float scaley, float offsetx, float offsety);
public:
	void DrawShadowPass(void);
};

#endif // __BF_GROUND_DRAWER_H__
