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
	void UpdateWater(CGame* game);
	void DrawReflection(CGame* game);
	void DrawRefraction(CGame* game);
	void Draw();
	CBumpWater();
	virtual ~CBumpWater();
	virtual int GetID() const { return 4; }

private:
	// user options
	bool reflection;
	char refraction; //0:=off, 1:=screencopy, 2:=own rendering cycle
	int  reflTexSize;
	bool waves;

	// map options
	float3 surfaceColor;
	float3 specularColor;

	int  refrSizeX;
	int  refrSizeY;

	unsigned int target; // of the refract texture
	GLuint refractTexture;
	GLuint reflectTexture;
	GLuint rbo;
	GLuint fbo;

	GLuint foamTexture;
	GLuint normalTexture;
	GLuint caustTextures[32];
	GLuint* heightTexture;

	GLuint waterFP;
	GLuint waterVP;
	GLuint waterShader;
	
	GLuint midPosLoc;
	GLuint eyePosLoc;
	GLuint lightDirLoc;
	GLuint fresnelMinLoc;
	GLuint fresnelMaxLoc;
	GLuint fresnelPowerLoc;
	GLuint frameLoc;
	GLuint screenInverseLoc;
	GLuint viewPosLoc;
	GLuint normalmapLoc;
	GLuint heightmapLoc;
	GLuint causticLoc;
	GLuint foamLoc;
	GLuint reflectionLoc;
	GLuint refractionLoc;
};

#endif // __BUMP_WATER_H__

