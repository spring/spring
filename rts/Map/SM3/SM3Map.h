/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SM3_MAP_H
#define SM3_MAP_H

#include "Frustum.h"
#include "terrain/TerrainBase.h"
#include "terrain/Terrain.h"
#include "Map/ReadMap.h"
#include "Rendering/Textures/Bitmap.h"

#include <map>

class CSM3GroundDrawer;


class CSM3ReadMap : public CReadMap
{
public:
	CR_DECLARE(CSM3ReadMap);

	CSM3ReadMap(const std::string& rm);
	~CSM3ReadMap();

	void ConfigNotify(const std::string& key, const std::string& value);

	void InitGroundDrawer();
	void KillGroundDrawer();

	CBaseGroundDrawer* GetGroundDrawer();

	void UpdateHeightMapUnsynced(const SRectangle& hmu);

	unsigned int GetMiniMapTexture() const { return minimapTexture; }
	unsigned int GetShadingTexture() const { return 0; }
	void DrawMinimap() const; // draw the minimap in a quad (with extends: (0,0)-(1,1))

	// Feature creation
	int GetNumFeatures();
	int GetNumFeatureTypes();
	void GetFeatureInfo(MapFeatureInfo* f); // returns MapFeatureInfo[GetNumFeatures()]
	const char* GetFeatureTypeName(int typeID);

	// Bitmaps (such as metal map, grass map, ...), handling them with a string as type seems flexible...
	// Some map types:
	//   "metal"  -  metalmap
	//   "grass"  -  grassmap
	unsigned char* GetInfoMap (const std::string& name, MapBitmapInfo* bm);
	void FreeInfoMap(const std::string& name, unsigned char* data);

	void GridVisibility(CCamera* cam, IQuadDrawer* cb, float maxDist, int quadSize, int extraSize = 0) override;

protected:
	CSM3GroundDrawer* groundDrawer;
	terrain::Terrain* renderer;

	struct InfoMap {
		InfoMap();
		~InfoMap();
		int w;
		int h;
		unsigned char* data;
	};

	unsigned int minimapTexture;

	std::map<std::string, InfoMap> infoMaps;
	friend class CSM3GroundDrawer;

	std::vector<std::string*> featureTypes;
	MapFeatureInfo* featureInfo;
	unsigned int numFeatures;
	void LoadFeatureData();

	Frustum tmpFrustum;
};

#endif
