// BumpWater.h: interface for the CBumpWater class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __BUMP_WATER_H__
#define __BUMP_WATER_H__

#include "Rendering/GL/FBO.h"
#include "Rendering/GL/myGL.h"
#include "BaseWater.h"

class CBumpWater : public CBaseWater
{
public:
	void Update();
	void UpdateWater(CGame* game);
	void HeightmapChanged(const int x1, const int y1, const int x2, const int y2);
	void DrawReflection(CGame* game);
	void DrawRefraction(CGame* game);
	void Draw();
	CBumpWater();
	~CBumpWater();
	int GetID() const { return 4; }

private:
	void UploadCoastline(const int x1, const int y1, const int x2, const int y2);
	void UpdateCoastmap(const int x1, const int y1, const int x2, const int y2);
	void UpdateDynWaves(const bool initialize = false);

	void SetUniforms(); //! see useUniforms
	void SetupUniforms( std::string& definitions );
	void GetUniformLocations( GLuint& program );

	//! user options
	bool  reflection;
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
	bool coastmapNeedUpload;
	bool coastmapNeedUpdate;
	int  coastUploadX1,coastUploadX2,coastUploadZ1,coastUploadZ2;
	int  coastUpdateX1,coastUpdateX2,coastUpdateZ1,coastUpdateZ2;
	GLuint pboID;

	GLuint refractTexture;
	GLuint reflectTexture;
	GLuint depthTexture;   //! screen depth copy
	GLuint waveRandTexture;
	GLuint foamTexture;
	GLuint normalTexture;  //! final used
	GLuint normalTexture2; //! updates normalTexture with dynamic waves turned on
	GLuint coastTexture[2];
	std::vector<GLuint> caustTextures;

	GLuint waterFP;
	GLuint waterVP;
	GLuint waterShader;

	GLuint blurFP;
	GLuint blurShader;
	GLuint blurDirLoc;
	GLuint blurTexLoc;

	GLuint frameLoc;
	GLuint midPosLoc;
	GLuint eyePosLoc;

	GLuint uniforms[20]; //! see useUniforms

	float3 texcoord1;
};

#endif // __BUMP_WATER_H__

