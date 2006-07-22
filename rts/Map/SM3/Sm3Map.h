
#ifndef SM3_MAP_H
#define SM3_MAP_H

#include "Map/ReadMap.h"
#include "terrain/TerrainBase.h"
#include "terrain/Terrain.h"
#include "Rendering/Textures/Bitmap.h"

class CSm3GroundDrawer;


class CSm3ReadMap : public CReadMap
{
public:
	CSm3ReadMap();
	~CSm3ReadMap();
	void Initialize (const char *mapname); // throws std::runtime_exception on errors

	CBaseGroundDrawer *GetGroundDrawer ();
	void HeightmapUpdated(int x1, int x2, int y1, int y2);
	void Update();
	void Explosion(float x,float y,float strength);
	void ExplosionUpdate(int x1,int x2,int y1,int y2);
	unsigned int GetShadingTexture (); // a texture with RGB for shading and A for height
	void DrawMinimap (); // draw the minimap in a quad (with extends: (0,0)-(1,1))

	// Feature creation
	int GetNumFeatures ();
	int GetNumFeatureTypes ();
	void GetFeatureInfo (MapFeatureInfo* f); // returns MapFeatureInfo[GetNumFeatures()]
	const char *GetFeatureType (int typeID);

	// Bitmaps (such as metal map, grass map, ...), handling them with a string as type seems flexible...
	// Some map types:
	//   "metal"  -  metalmap
	//   "grass"  -  grassmap
	unsigned char *GetInfoMap (const std::string& name, MapBitmapInfo* bm);
	void FreeInfoMap(const std::string& name, unsigned char *data);

	// Determine visibility for a rectangular grid
	void GridVisibility(CCamera *cam, int quadSize, float maxdist, IQuadDrawer *cb, int extraSize=0);
protected:
	CSm3GroundDrawer *groundDrawer;
	terrain::Terrain *renderer;

	struct InfoMap {
		InfoMap ();
		~InfoMap ();
		int w,h;
		unsigned char *data;
	};

	GLuint minimapTexture;

	std::map<std::string, InfoMap> infoMaps;
	friend class CSm3GroundDrawer;
};


#endif
