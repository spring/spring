#ifndef SMFREADMAP_H
#define SMFREADMAP_H

#include "Map/ReadMap.h"
#include <string>
#include "mapfile.h"

class CBFGroundDrawer;

class CSmfReadMap : public CReadMap
{
public:
	CR_DECLARE(CSmfReadMap)

	CSmfReadMap(std::string mapname);
	~CSmfReadMap(void);

	void HeightmapUpdated(int x1, int x2, int y1, int y2);
	GLuint GetShadingTexture () { return shadowTex; }
	GLuint GetGrassShadingTexture () { return minimapTex; }
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
	GLuint detailTex;
	SMFHeader header;
	CFileHandler *ifs;

	bool usePBO;
	float anisotropy;

	unsigned char waterHeightColors[1024*4];
protected:
	void ReadGrassMap (void *data);
	void ReadFeatureInfo ();

	void ConfigureAnisotropy();

	GLuint shadowTex;
	GLuint minimapTex;

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
