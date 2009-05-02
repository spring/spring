#ifndef READMAP_H
#define READMAP_H
// ReadMap.h: interface for the CReadMap class.
//
//////////////////////////////////////////////////////////////////////

#include "Rendering/GL/myGL.h"
#include "creg/creg_cond.h"
#include "float3.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/GlobalSynced.h"

class CMetalMap;
class CCamera;
class CFileHandler;
class CUnit;
class CSolidObject;
class CFileHandler;
class CLoadSaveInterface;
class CBaseGroundDrawer;


struct MapFeatureInfo
{
	float3 pos;
	int featureType;	//index to one of the strings above
	float rotation;
};


struct MapBitmapInfo
{
	MapBitmapInfo() {}
	MapBitmapInfo(int w, int h) : width(w), height(h) {}

	int width;
	int height;
};

struct HeightmapUpdate {
	HeightmapUpdate(int x, int xx, int y, int yy) : x1(x),x2(xx),y1(y),y2(yy) {}
	int x1;
	int x2;
	int y1;
	int y2;
};

class CReadMap
{
public:
	CR_DECLARE(CReadMap);

	virtual ~CReadMap();

	void Serialize(creg::ISerializer& s); // creg serialize callback

	static CReadMap* LoadMap (const std::string& mapname);

protected:
	void Initialize(); // called by implementations of CReadMap
public:
	// calculates derived heightmap information such as normals, centerheightmap and slopemap
	void CalcHeightfieldData();

	virtual const float* GetHeightmap() = 0;
	// if you modify the heightmap, call HeightmapUpdated
	virtual void SetHeight(int idx, float h) = 0;
	virtual void AddHeight(int idx, float a) = 0;

	float* orgheightmap;
	float* centerheightmap;
	static const int numHeightMipMaps = 7;	//number of heightmap mipmaps, including full resolution
	float* mipHeightmap[numHeightMipMaps];	//array of pointers to heightmap in different resolutions, mipHeightmap[0] is full resolution, mipHeightmap[n+1] is half resolution of mipHeightmap[n]
	float* slopemap;
	float3* facenormals;
	unsigned char* typemap;

	CMetalMap *metalMap;					//Metal-density/height-map

	int width, height;

	unsigned int mapChecksum;
protected:
	CReadMap(); // use LoadMap
public:
	virtual CBaseGroundDrawer *GetGroundDrawer () { return 0; }
	std::vector<HeightmapUpdate> heightmapUpdates;
	void HeightmapUpdated(int x1, int x2, int y1, int y2);
	virtual void HeightmapUpdatedNow(int x1, int x2, int y1, int y2)=0;
	void UpdateDraw();
	virtual void Update(){};
	virtual void Explosion(float x,float y,float strength){};
	virtual GLuint GetShadingTexture () = 0; // a texture with RGB for shading and A for height
	static inline unsigned char EncodeHeight(float h) { return std::max(0, (int)(255+10.0f*h)); }

	virtual void DrawMinimap () = 0; // draw the minimap in a quad (with extends: (0,0)-(1,1))

	virtual GLuint GetGrassShadingTexture() { return 0; }

	// Feature creation
	virtual int GetNumFeatures () = 0;
	virtual int GetNumFeatureTypes () = 0;
	virtual void GetFeatureInfo (MapFeatureInfo* f) = 0; // returns MapFeatureInfo[GetNumFeatures()]
	virtual const char *GetFeatureType (int typeID) = 0;

	// Infomaps (such as metal map, grass map, ...), handling them with a string as type seems flexible...
	// Some map types:
	//   "metal"  -  metalmap
	//   "grass"  -  grassmap
	virtual unsigned char* GetInfoMap (const std::string& name, MapBitmapInfo* bm) = 0;
	virtual void FreeInfoMap (const std::string& name, unsigned char *data) = 0;

	// Determine visibility for a rectangular grid
	struct IQuadDrawer
	{
		virtual ~IQuadDrawer();
		virtual void DrawQuad (int x,int y) = 0;
	};
	virtual void GridVisibility(CCamera *cam, int quadSize, float maxdist, IQuadDrawer *cb, int extraSize=0) = 0;

	float minheight, maxheight;
	float currMinHeight, currMaxHeight;
};

extern CReadMap* readmap;

//Converts a map-square into a float3-position.
inline float3 SquareToFloat3(int xSquare, int zSquare) {return float3(((xSquare))*SQUARE_SIZE, readmap->centerheightmap[(xSquare) + (zSquare) * gs->mapx], ((zSquare))*SQUARE_SIZE);};

#endif /* READMAP_H */
