/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SMFREADMAP_H
#define SMFREADMAP_H

#include "SMFMapFile.h"
#include "Map/ReadMap.h"
#include "System/EventClient.h"
#include "System/type2.h"


class CSMFGroundDrawer;

class CSMFReadMap : public CReadMap, public CEventClient
{
public:
	// CEventClient interface
	int GetReadAllyTeam() const { return AllAccessTeam; }
	bool WantsEvent(const std::string& eventName) {
		return (eventName == "SunChanged");
	}

	void SunChanged(const float3& sunDir);

public:
	CR_DECLARE(CSMFReadMap)

	CSMFReadMap(std::string mapname);
	~CSMFReadMap();

	void UpdateShadingTexture();
	void UpdateHeightMapUnsynced(const SRectangle&);

	unsigned int GetTexture(unsigned int type, unsigned int num = 0) const {
		unsigned int texID = 0;

		switch (type) {
			case MAP_BASE_GRASS_TEX: { texID = GetGrassShadingTexture(); } break;
			case MAP_BASE_DETAIL_TEX: { texID = GetDetailTexture(); } break;
			case MAP_BASE_MINIMAP_TEX: { texID = GetMiniMapTexture(); } break;
			case MAP_BASE_SHADING_TEX: { texID = GetShadingTexture(); } break;
			case MAP_BASE_NORMALS_TEX: { texID = GetNormalsTexture(); } break;

			case MAP_SSMF_NORMALS_TEX: { texID = GetBlendNormalsTexture(); } break;
			case MAP_SSMF_SPECULAR_TEX: { texID = GetSpecularTexture(); } break;

			case MAP_SSMF_SPLAT_DISTRIB_TEX: { texID = GetSplatDistrTexture(); } break;
			case MAP_SSMF_SPLAT_DETAIL_TEX: { texID = GetSplatDetailTexture(); } break;
			case MAP_SSMF_SPLAT_NORMAL_TEX: { texID = GetSplatNormalTexture(num); } break;

			case MAP_SSMF_SKY_REFLECTION_TEX: { texID = GetSkyReflectModTexture(); } break;
			case MAP_SSMF_LIGHT_EMISSION_TEX: { texID = GetLightEmissionTexture(); } break;
			case MAP_SSMF_PARALLAX_HEIGHT_TEX: { texID = GetParallaxHeightTexture(); } break;

			default: {} break;
		}

		return texID;
	}

	int2 GetTextureSize(unsigned int type) const {
		int2 size;

		switch (type) {
			case MAP_BASE_GRASS_TEX: {
				size = int2(1024, 1024);
			} break;
			case MAP_BASE_MINIMAP_TEX: {
				size = int2(1024, 1024);
			} break;
			case MAP_BASE_SHADING_TEX: {
				size = int2(mapDims.pwr2mapx, mapDims.pwr2mapy);
			} break;
			case MAP_BASE_NORMALS_TEX: {
				size = normalTexSize;
			} break;
		}

		return size;
	}

	void SetTexture(unsigned int texID, unsigned int type, unsigned int num = 0) {
		switch (type) {
			case MAP_BASE_GRASS_TEX: { SetGrassShadingTexture(texID); } break;
			case MAP_BASE_DETAIL_TEX: { SetDetailTexture(texID); } break;
			case MAP_BASE_MINIMAP_TEX: { SetMiniMapTexture(texID); } break;
			case MAP_BASE_SHADING_TEX: { SetShadingTexture(texID); } break;
			case MAP_BASE_NORMALS_TEX: { SetNormalsTexture(texID); } break;

			case MAP_SSMF_NORMALS_TEX: { SetBlendNormalsTexture(texID); } break;
			case MAP_SSMF_SPECULAR_TEX: { SetSpecularTexture(texID); } break;

			case MAP_SSMF_SPLAT_DISTRIB_TEX: { SetSplatDistrTexture(texID); } break;
			case MAP_SSMF_SPLAT_DETAIL_TEX: { SetSplatDetailTexture(texID); } break;
			case MAP_SSMF_SPLAT_NORMAL_TEX: { SetSplatNormalTexture(texID, num); } break;

			case MAP_SSMF_SKY_REFLECTION_TEX: { SetSkyReflectModTexture(texID); } break;
			case MAP_SSMF_LIGHT_EMISSION_TEX: { SetLightEmissionTexture(texID); } break;
			case MAP_SSMF_PARALLAX_HEIGHT_TEX: { SetParallaxHeightTexture(texID); } break;

			default: {} break;
		}
	}

	unsigned int GetShadingTexture() const { return shadingTex.texID; }
	unsigned int GetNormalsTexture() const { return normalsTex.texID; }

	unsigned int GetBlendNormalsTexture() const { return blendNormalsTex.texID; }
	unsigned int GetSpecularTexture() const { return specularTex.texID; }

	unsigned int GetSplatDistrTexture() const { return splatDistrTex.texID; }
	unsigned int GetSplatDetailTexture() const { return splatDetailTex.texID; }
	unsigned int GetSplatNormalTexture(int i) const { return splatNormalTextures[i].texID; }
 
	unsigned int GetSkyReflectModTexture() const { return skyReflectModTex.texID; }
	unsigned int GetLightEmissionTexture() const { return lightEmissionTex.texID; }
	unsigned int GetParallaxHeightTexture() const { return parallaxHeightTex.texID; }

	unsigned int GetGrassShadingTexture() const { return grassShadingTex.texID; }
	unsigned int GetMiniMapTexture() const { return minimapTex.texID; }
	unsigned int GetDetailTexture() const { return detailTex.texID; }

	void SetDetailTexture(unsigned int texID) {
		detailTex.SetLuaTexture(texID);
	}
	void SetMiniMapTexture(unsigned int texID) {
		minimapTex.SetLuaTexture(texID);
	}
	void SetShadingTexture(unsigned int texID) {
		shadingTex.SetLuaTexture(texID);
	}
	void SetNormalsTexture(unsigned int texID) {
		normalsTex.SetLuaTexture(texID);
	}
	void SetSpecularTexture(unsigned int texID) {
		specularTex.SetLuaTexture(texID);
	}
	void SetGrassShadingTexture(unsigned int texID) {
		grassShadingTex.SetLuaTexture(texID);
	}
	void SetSplatDetailTexture(unsigned int texID) {
		splatDetailTex.SetLuaTexture(texID);
	}
	void SetSplatNormalTexture(unsigned int texID, unsigned int i) {
		splatNormalTextures[i].SetLuaTexture(texID);
	}
	void SetSplatDistrTexture(unsigned int texID) {
		splatDistrTex.SetLuaTexture(texID);
	}
	void SetSkyReflectModTexture(unsigned int texID) {
		skyReflectModTex.SetLuaTexture(texID);
	}
	void SetBlendNormalsTexture(unsigned int texID) {
		blendNormalsTex.SetLuaTexture(texID);
	}
	void SetLightEmissionTexture(unsigned int texID) {
		lightEmissionTex.SetLuaTexture(texID);
	}
	void SetParallaxHeightTexture(unsigned int texID) {
		parallaxHeightTex.SetLuaTexture(texID);
	}


	void DrawMinimap() const;
	void GridVisibility(CCamera* cam, IQuadDrawer* cb, float maxDist, int quadSize, int extraSize = 0) override;

	void NewGroundDrawer();

	int GetNumFeatureTypes();
	int GetNumFeatures();
	void GetFeatureInfo(MapFeatureInfo* f); // returns all feature info in MapFeatureInfo[NumFeatures]
	const char* GetFeatureTypeName(int typeID);

	unsigned char* GetInfoMap(const std::string& name, MapBitmapInfo* bm);
	void FreeInfoMap(const std::string& name, unsigned char* data);

	// NOTE: do not use, just here for backward compatibility with SMFGroundTextures.cpp
	CSMFMapFile& GetFile() { return file; }


	void ConfigureAnisotropy();
	float GetAnisotropy() const { return anisotropy; }

	bool HaveSpecularTexture() const { return haveSpecularTexture; }
	bool HaveSplatDetailDistribTexture() const { return haveSplatDetailDistribTexture; }
	bool HaveSplatNormalDistribTexture() const { return haveSplatNormalDistribTexture; }
	bool HaveDetailNormalDiffuseAlpha() const { return haveDetailNormalDiffuseAlpha; }

private:
	void ParseHeader();
	void LoadHeightMap();
	void LoadMinimap();
	void InitializeWaterHeightColors();
	void CreateSpecularTex();
	void CreateSplatDetailTextures();
	void CreateGrassTex();
	void CreateDetailTex();
	void CreateShadingTex();
	void CreateNormalTex();

	void UpdateVertexNormalsUnsynced(const SRectangle& update);
	void UpdateFaceNormalsUnsynced(const SRectangle& update);
	void UpdateNormalTexture(const SRectangle& update);
	void UpdateShadingTexture(const SRectangle& update);

	inline void UpdateShadingTexPart(int idx1, int idx2, unsigned char* dst) const;
	inline CBaseGroundDrawer* GetGroundDrawer();

	inline const float GetCenterHeightUnsynced(const int x, const int y) const;

	inline float DiffuseSunCoeff(const int x, const int y) const;
	inline float3 GetLightValue(const int x, const int y) const;
	void ParseSMD(std::string filename);

public:
	// constants
	static const int tileScale     = 4;
	static const int bigSquareSize = 32 * tileScale;
	static const int NUM_SPLAT_DETAIL_NORMALS = 4;

	// globals for SMFGround{Drawer, Textures}
	int numBigTexX;
	int numBigTexY;
	int bigTexSize;
	int tileMapSizeX;
	int tileMapSizeY;
	int tileCount;
	int mapSizeX;
	int mapSizeZ;
	int maxHeightMapIdx;
	int heightMapSizeX;

	int2 normalTexSize;

private:
	CSMFMapFile file;
	CSMFGroundDrawer* groundDrawer;

private:
	std::vector<float> cornerHeightMapSynced;
	std::vector<float> cornerHeightMapUnsynced;

	std::vector<unsigned char> shadingTexBuffer;
	std::vector<unsigned char> waterHeightColors;

private:
	MapTexture detailTex;             // supplied by the map
	MapTexture specularTex;           // supplied by the map, moderates specular contribution
	MapTexture shadingTex;            // holds precomputed dot(lightDir, vertexNormal) values
	MapTexture normalsTex;            // holds vertex normals in RGBA32F internal format (GL_RGBA + GL_FLOAT)
	MapTexture minimapTex;            // supplied by the map

	MapTexture splatDistrTex;         // specifies the per-channel distribution of splatDetailTex (map-wide, overrides detailTex)
	MapTexture splatDetailTex;        // contains per-channel separate greyscale detail-textures (overrides detailTex)

	// contains RGBA texture with RGB channels containing normals;
	// alpha contains greyscale diffuse for splat detail normals
	// if haveDetailNormalDiffuseAlpha (overrides detailTex)
	MapTexture splatNormalTextures[NUM_SPLAT_DETAIL_NORMALS];

	MapTexture grassShadingTex;       // specifies grass-blade modulation color (defaults to minimapTex)
	MapTexture skyReflectModTex;      // modulates sky-reflection RGB intensities (must be the same size as specularTex)
	MapTexture blendNormalsTex;       // tangent-space offset normals
	MapTexture lightEmissionTex;
	MapTexture parallaxHeightTex;

private:
	int shadingTexUpdateProgress;

	float anisotropy;

	bool haveSpecularTexture;
	bool haveSplatDetailDistribTexture; // true if we have both splatDetailTex and splatDistrTex
	bool haveSplatNormalDistribTexture; // true if we have splatDistrTex and at least one splat[Detail]NormalTex
	bool haveDetailNormalDiffuseAlpha;

	// Did original textures exist
	bool haveSpecularTextureOrig; 

	bool minimapOverride;
	bool shadingTexUpdateNeeded;
};

#endif
