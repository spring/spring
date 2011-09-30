/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SMFREADMAP_H
#define SMFREADMAP_H

#include "SMFMapFile.h"
#include "Map/ReadMap.h"
#include "System/Vec2.h"

class CSMFGroundDrawer;

class CSMFReadMap : public CReadMap
{
public:
	CR_DECLARE(CSMFReadMap)

	CSMFReadMap(std::string mapname);
	~CSMFReadMap();

	void UpdateShadingTexture();
	void UpdateHeightMapUnsynced(const HeightMapUpdate&);

	inline unsigned int GetDetailTexture() const { return detailTex; }
	inline unsigned int GetShadingTexture() const { return shadingTex; }
	inline unsigned int GetNormalsTexture() const { return normalsTex; }
	inline unsigned int GetSpecularTexture() const { return specularTex; }
	inline unsigned int GetGrassShadingTexture() const { return grassShadingTex; }
	inline unsigned int GetSplatDetailTexture() const { return splatDetailTex; }
	inline unsigned int GetSplatDistrTexture() const { return splatDistrTex; }
	inline unsigned int GetSkyReflectModTexture() const { return skyReflectModTex; }
	inline unsigned int GetDetailNormalTexture() const { return detailNormalTex; }
	inline unsigned int GetLightEmissionTexture() const { return lightEmissionTex; }

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

	const float* GetCornerHeightMapSynced()   const { return &cornerHeightMapSynced[0]; }
#ifdef USE_UNSYNCED_HEIGHTMAP
	const float* GetCornerHeightMapUnsynced() const { return &cornerHeightMapUnsynced[0]; }
#endif

	void ConfigureAnisotropy();
	float GetAnisotropy() const { return anisotropy; }

	bool HaveSpecularLighting() const { return haveSpecularLighting; }
	bool HaveSplatTexture() const { return haveSplatTexture; }

private:
	inline void UpdateShadingTexPart(int y, int x1, int y1, int xsize, unsigned char* pixelRow);
	inline CBaseGroundDrawer* GetGroundDrawer();

	inline const float GetCenterHeightUnsynced(const int& x, const int& y) const;

	inline float DiffuseSunCoeff(const int& x, const int& y) const;
	inline float3 GetLightValue(const int& x, const int& y) const;
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

	unsigned int detailTex;        // supplied by the map
	unsigned int specularTex;      // supplied by the map, moderates specular contribution
	unsigned int shadingTex;       // holds precomputed dot(lightDir, vertexNormal) values
	unsigned int normalsTex;       // holds vertex normals in RGBA32F internal format (GL_RGBA + GL_FLOAT)
	unsigned int minimapTex;       // supplied by the map
	unsigned int splatDetailTex;   // contains per-channel separate greyscale detail-textures (overrides detailTex)
	unsigned int splatDistrTex;    // specifies the per-channel distribution of splatDetailTex (map-wide, overrides detailTex)
	unsigned int grassShadingTex;  // specifies grass-blade modulation color (defaults to minimapTex)
	unsigned int skyReflectModTex; // modulates sky-reflection RGB intensities (must be the same size as specularTex)
	unsigned int detailNormalTex;  // tangent-space offset normals
	unsigned int lightEmissionTex;

	bool haveSpecularLighting;
	bool haveSplatTexture;

	unsigned char waterHeightColors[1024 * 4];

	CSMFGroundDrawer* groundDrawer;

private:
	std::vector<float> cornerHeightMapSynced;
#ifdef USE_UNSYNCED_HEIGHTMAP
	std::vector<float> cornerHeightMapUnsynced;
#endif

	std::vector<unsigned char> shadingTexPixelRow;
	unsigned int shadingTexUpdateIter;
	unsigned int shadingTexUpdateRate;

	float anisotropy;
};

#endif
