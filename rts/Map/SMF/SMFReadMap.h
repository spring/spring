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

	unsigned int GetDetailTexture() const { return detailTex; }
	unsigned int GetMiniMapTexture() const { return minimapTex; }
	int2 GetMiniMapTextureSize() const { return int2(1024, 1024); }
	unsigned int GetShadingTexture() const { return shadingTex; }
	unsigned int GetNormalsTexture() const { return normalsTex; }
	unsigned int GetSpecularTexture() const { return specularTex; }
	unsigned int GetGrassShadingTexture() const { return grassShadingTex; }
	unsigned int GetSplatDetailTexture() const { return splatDetailTex; }
	unsigned int GetSplatDistrTexture() const { return splatDistrTex; }
	unsigned int GetSkyReflectModTexture() const { return skyReflectModTex; }
	unsigned int GetDetailNormalTexture() const { return detailNormalTex; }
	unsigned int GetLightEmissionTexture() const { return lightEmissionTex; }
	unsigned int GetParallaxHeightTexture() const { return parallaxHeightTex; }

	void DrawMinimap() const;
	void GridVisibility(CCamera* cam, int quadSize, float maxdist, IQuadDrawer* cb, int extraSize);

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
	bool HaveSplatTexture() const { return haveSplatTexture; }

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
	static const int tileScale     =   4;
	static const int bigSquareSize = 128; // 32 * tileScale

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

protected:
	CSMFMapFile file;

	unsigned int detailTex;         // supplied by the map
	unsigned int specularTex;       // supplied by the map, moderates specular contribution
	unsigned int shadingTex;        // holds precomputed dot(lightDir, vertexNormal) values
	unsigned int normalsTex;        // holds vertex normals in RGBA32F internal format (GL_RGBA + GL_FLOAT)
	unsigned int minimapTex;        // supplied by the map
	unsigned int splatDetailTex;    // contains per-channel separate greyscale detail-textures (overrides detailTex)
	unsigned int splatDistrTex;     // specifies the per-channel distribution of splatDetailTex (map-wide, overrides detailTex)
	unsigned int grassShadingTex;   // specifies grass-blade modulation color (defaults to minimapTex)
	unsigned int skyReflectModTex;  // modulates sky-reflection RGB intensities (must be the same size as specularTex)
	unsigned int detailNormalTex;   // tangent-space offset normals
	unsigned int lightEmissionTex;
	unsigned int parallaxHeightTex;

	bool haveSpecularTexture;
	bool haveSplatTexture;
	bool minimapOverride;

	unsigned char waterHeightColors[1024 * 4];

	CSMFGroundDrawer* groundDrawer;

private:
	std::vector<float> cornerHeightMapSynced;
	std::vector<float> cornerHeightMapUnsynced;

	std::vector<unsigned char> shadingTexBuffer;
	bool shadingTexUpdateNeeded;
	int shadingTexUpdateProgress;

	float anisotropy;
};

#endif
