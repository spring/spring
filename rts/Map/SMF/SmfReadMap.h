/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SMFREADMAP_H
#define SMFREADMAP_H

#include "SmfMapFile.h"

class CBFGroundDrawer;

class CSmfReadMap : public CReadMap
{
public:
	CR_DECLARE(CSmfReadMap)

	CSmfReadMap(std::string mapname);
	~CSmfReadMap();

	void UpdateHeightmapUnsynced(int x1, int y1, int x2, int y2);
	inline void UpdateShadingTexPart(int y, int x1, int y1, int xsize, unsigned char *pixels);
	virtual void UpdateShadingTexture();

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
	inline CBaseGroundDrawer* GetGroundDrawer();

	const float* GetHeightmap() const { return heightmap; }

	inline void SetHeight(const int& idx, const float& h) {
		heightmap[idx] = h;
		currMinHeight = std::min(h, currMinHeight);
		currMaxHeight = std::max(h, currMaxHeight);
	}
	inline void AddHeight(const int& idx, const float& a) {
		heightmap[idx] += a;
		currMinHeight = std::min(heightmap[idx], currMinHeight);
		currMaxHeight = std::max(heightmap[idx], currMaxHeight);
	}

	int GetNumFeatureTypes();
	int GetNumFeatures();
	void GetFeatureInfo(MapFeatureInfo* f); // returns all feature info in MapFeatureInfo[NumFeatures]
	const char* GetFeatureTypeName(int typeID);

	unsigned char* GetInfoMap(const std::string& name, MapBitmapInfo* bm);
	void FreeInfoMap(const std::string& name, unsigned char* data);

	// NOTE: do not use, just here for backward compatibility with BFGroundTextures.cpp
	CSmfMapFile& GetFile() { return file; }

	float anisotropy;

protected:
	void ConfigureAnisotropy();

	CSmfMapFile file;

	unsigned int detailTex;        // supplied by the map
	unsigned int specularTex;      // supplied by the map, moderates specular contribution
	unsigned int shadingTex;       // holds precomputed dot(lightDir, faceNormal) values
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
	float* heightmap;

	float DiffuseSunCoeff(const int& x, const int& y) const;
	float3 GetLightValue(const int& x, const int& y) const;
	void ParseSMD(std::string filename);

	friend class CBFGroundDrawer;
	CBFGroundDrawer* groundDrawer;

private:
	std::vector<unsigned char> shadingTexPixelRow;
	unsigned int shadingTexUpdateIter;
	unsigned int shadingTexUpdateRate;
};

#endif
