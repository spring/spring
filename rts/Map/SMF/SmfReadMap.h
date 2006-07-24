#ifndef SMFREADMAP_H
#define SMFREADMAP_H

#include "Map/ReadMap.h"
#include <string>
#include "mapfile.h"

class CBFGroundDrawer;

class CSmfReadMap : public CReadMap
{
public:
	CSmfReadMap(std::string mapname);
	~CSmfReadMap(void);

	void HeightmapUpdated(int x1, int x2, int y1, int y2);
	unsigned int GetShadingTexture () { return shadowTex; }
	unsigned int GetGrassShadingTexture () { return minimapTex; }
	void DrawMinimap ();
	void GridVisibility(CCamera *cam, int quadSize, float maxdist, IQuadDrawer *cb, int extraSize);
	CBaseGroundDrawer* GetGroundDrawer();
	float* GetHeightmap() { return heightmap; }
	
	int GetNumFeatureTypes ();
	int GetNumFeatures ();
	void GetFeatureInfo (MapFeatureInfo* f);// returns all feature info in MapFeatureInfo[NumFeatures]
	const char *GetFeatureType (int typeID);
	
	unsigned char *GetInfoMap (const std::string& name, MapBitmapInfo* bm);
	void FreeInfoMap(const std::string& name, unsigned char *data);

	std::string detailTexName;
	unsigned int detailTex;
	MapHeader header;
	CFileHandler *ifs;

	unsigned char waterHeightColors[1024*4];
protected:
	void ReadGrassMap (void *data);
	void ReadFeatureInfo ();

	unsigned int shadowTex;
	unsigned int minimapTex;

	float* heightmap;

	MapFeatureHeader featureHeader;
	std::string* featureTypes;
	int featureFileOffset;

	CBFGroundDrawer *groundDrawer;

	float3 GetLightValue(int x, int y);
	void ParseSMD(std::string filename);

	friend class CBFGroundDrawer;
};

#endif
