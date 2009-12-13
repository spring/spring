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
	inline GLuint GetShadingTexture() const { return shadowTex; }
	inline GLuint GetGrassShadingTexture() const { return minimapTex; }
	void DrawMinimap() const;
	void GridVisibility(CCamera *cam, int quadSize, float maxdist, IQuadDrawer *cb, int extraSize);
	inline CBaseGroundDrawer* GetGroundDrawer() { return (CBaseGroundDrawer*)groundDrawer; }
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

	int GetNumFeatureTypes ();
	int GetNumFeatures ();
	void GetFeatureInfo (MapFeatureInfo* f); // returns all feature info in MapFeatureInfo[NumFeatures]
	const char *GetFeatureTypeName (int typeID);

	unsigned char *GetInfoMap (const std::string& name, MapBitmapInfo* bm);
	void FreeInfoMap(const std::string& name, unsigned char *data);

	// todo: do not use, just here for backward compatibility with BFGroundTextures.cpp
	CSmfMapFile& GetFile() { return file; }

	bool usePBO;
	float anisotropy;

protected:
	void ConfigureAnisotropy();

	CSmfMapFile file;

	std::string detailTexName;
	GLuint detailTex;

	unsigned char waterHeightColors[1024*4];

	GLuint shadowTex;
	GLuint minimapTex;

	float* heightmap;

	CBFGroundDrawer* groundDrawer;


	float DiffuseSunCoeff(const int& x, const int& y) const;
	float3 GetLightValue(const int& x, const int& y) const;
	void ParseSMD(std::string filename);

	friend class CBFGroundDrawer;
};

#endif
