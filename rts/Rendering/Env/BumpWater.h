// BumpWater.h: interface for the CBumpWater class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __BUMP_WATER_H__
#define __BUMP_WATER_H__

#include "Rendering/GL/myGL.h"
#include "BaseWater.h"

class CBumpWater : public CBaseWater
{
public:
	void Update();
	void UpdateWater(CGame* game);
	void DrawReflection(CGame* game);
	void DrawRefraction(CGame* game);
	void Draw();
	CBumpWater();
	virtual ~CBumpWater();
	virtual int GetID() const { return 4; }

private:
	void GenerateCoastMap();
	
	// user options
	bool  reflection;
	char  refraction; // 0:=off, 1:=screencopy, 2:=own rendering cycle
	int   reflTexSize;
	bool  shorewaves;
	bool  depthCopy;  // uses a screen depth copy, which allows a nicer interpolation between deep sea and shallow water
	float anisotropy;
	char  depthBits;  // depthBits for reflection/refraction RBO
	bool  blurRefl;

	
	unsigned int target; // for screen copies (color/depth)
	int  screenTextureX;
	int  screenTextureY;
	
	GLuint refractTexture;
	GLuint reflectTexture;
	GLuint reflectRBO;
	GLuint refractRBO;
	GLuint reflectFBO;
	GLuint refractFBO;
	GLuint coastFBO;
	
	GLuint  waveRandTexture;
	GLuint  foamTexture;
	GLuint  normalTexture;
	GLuint  coastTexture;
	GLuint  depthTexture; // screen depth copy
	GLuint* heightTexture;
	std::vector<GLuint> caustTextures;

	GLuint waterFP;
	GLuint waterVP;
	GLuint waterShader;
	
	GLuint blurFP;
	GLuint blurShader;
	GLuint blurDirLoc;

	GLuint frameLoc;
	GLuint midPosLoc;
	GLuint eyePosLoc;

	float3 texcoord1;
	float3 texcoord2;
	float3 texcoord3;
	float3 texcoord4;
};

#endif // __BUMP_WATER_H__

