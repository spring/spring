/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __BUMP_WATER_H__
#define __BUMP_WATER_H__

#include "Rendering/GL/FBO.h"
#include "Rendering/GL/myGL.h"
#include "BaseWater.h"

#include <bitset>

namespace Shader {
	struct IProgramObject;
}

class CBumpWater : public CBaseWater
{
public:
	CBumpWater();
	~CBumpWater();

	void Update();
	void UpdateWater(CGame* game);
	void OcclusionQuery();
	void HeightmapChanged(const int x1, const int y1, const int x2, const int y2);
	void DrawReflection(CGame* game);
	void DrawRefraction(CGame* game);
	void Draw();
	int GetID() const { return WATER_RENDERER_BUMPMAPPED; }
	const char* GetName() const { return "bumpmapped"; }

private:
	void SetUniforms(); //! see useUniforms
	void SetupUniforms( std::string& definitions );
	void GetUniformLocations();

	//! user options
	char  reflection;   //! 0:=off, 1:=don't render the terrain, 2:=render everything+terrain
	char  refraction;   //! 0:=off, 1:=screencopy, 2:=own rendering cycle
	int   reflTexSize;
	bool  depthCopy;    //! uses a screen depth copy, which allows a nicer interpolation between deep sea and shallow water
	float anisotropy;
	char  depthBits;    //! depthBits for reflection/refraction RBO
	bool  blurRefl;
	bool  shoreWaves;
	bool  endlessOcean; //! render the water around the whole map
	bool  dynWaves;     //! only usable if bumpmap/normal texture is a TileSet
	bool  useUniforms;  //! use Uniforms instead of #define'd const. Warning: this is much slower, but has the advantage that you can change the params on runtime.

	unsigned char* tileOffsets; //! used to randomize the wave/bumpmap/normal texture
	int  normalTextureX; //! needed for dynamic waves
	int  normalTextureY;

	GLuint target; //! for screen copies (color/depth), can be GL_TEXTURE_RECTANGLE (nvidia) or GL_TEXTURE_2D (others)
	int  screenTextureX;
	int  screenTextureY;

	FBO reflectFBO;
	FBO refractFBO;
	FBO coastFBO;
	FBO dynWavesFBO;

	GLuint displayList;

	//! coastmap
	struct CoastUpdateRect {
		CoastUpdateRect(int x1_,int z1_,int x2_,int z2_) : x1(x1_),z1(z1_),x2(x2_),z2(z2_) {};
		int x1,z1;
		int x2,z2;
	};
	std::vector<CoastUpdateRect> coastmapUpdates;
	struct CoastAtlasRect {
		CoastAtlasRect(CoastUpdateRect& rect);
		bool isCoastline; //! if false, then the whole rect is either above water or below water (no coastline -> no need to calc/render distfield)
		int ix1,iy1;
		int ix2,iy2;
		int xsize,ysize;
		float x1,y1;
		float x2,y2;
		float tx1,ty1;
		float tx2,ty2;
	};
	std::vector<CoastAtlasRect> coastmapAtlasRects;

	std::bitset<4> GetEdgesInRect(CoastUpdateRect& rect1, CoastUpdateRect& rect2);
	std::bitset<4> GetSharedEdges(CoastUpdateRect& rect1, CoastUpdateRect& rect2);
	void HandleOverlapping(size_t i, size_t& j);

	void UploadCoastline(const bool forceFull = false);
	void UpdateCoastmap();
	void UpdateDynWaves(const bool initialize = false);

	int atlasX,atlasY;

	GLuint refractTexture;
	GLuint reflectTexture;
	GLuint depthTexture;   //! screen depth copy
	GLuint waveRandTexture;
	GLuint foamTexture;
	GLuint normalTexture;  //! final used
	GLuint normalTexture2; //! updates normalTexture with dynamic waves turned on
	GLuint coastTexture;
	GLuint coastUpdateTexture;
	std::vector<GLuint> caustTextures;

	Shader::IProgramObject* waterShader;
	Shader::IProgramObject* blurShader;

	GLuint uniforms[20]; //! see useUniforms

	bool wasLastFrameVisible;
	GLuint occlusionQuery;
	GLuint occlusionQueryResult;

	float3 windVec;
	float3 windndir;
	float  windStrength;
};

#endif // __BUMP_WATER_H__

