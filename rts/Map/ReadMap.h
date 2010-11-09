/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef READ_MAP_H
#define READ_MAP_H

#ifdef     BITMAP_NO_OPENGL
// FIXME: this is hacky. this class should not depend on OpenGL stuff
typedef unsigned int GLuint;
#else  // !BITMAP_NO_OPENGL
#include "Rendering/GL/myGL.h"
#endif //  BITMAP_NO_OPENGL
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
	/// index to one of the strings above
	int featureType;
	float rotation;
};


struct MapBitmapInfo
{
	MapBitmapInfo() : width(0), height(0) {}
	MapBitmapInfo(int w, int h) : width(w), height(h) {}

	int width;
	int height;
};

struct HeightmapUpdate {
	HeightmapUpdate(int x, int xx, int y, int yy)
			: x1(x), x2(xx), y1(y), y2(yy) {}

	int x1, x2;
	int y1, y2;
};

class CReadMap
{
public:
	CR_DECLARE(CReadMap);
	/// creg serialize callback
	void Serialize(creg::ISerializer& s);

	virtual ~CReadMap();
	static CReadMap* LoadMap(const std::string& mapname);

protected:
	CReadMap();
	/// called by implementations of CReadMap
	void Initialize();

	std::vector<HeightmapUpdate> heightmapUpdates;
	virtual void UpdateHeightmapUnsynced(int x1, int y1, int x2, int y2) = 0;

	/**
	 * calculates derived heightmap information
	 * such as normals, centerheightmap and slopemap
	 */
	void UpdateHeightmapSynced(int x1, int y1, int x2, int y2);
	void CalcHeightmapChecksum();
public:
	/// Returns a float[(mapx + 1) * (mapy + 1)]
	virtual const float* GetHeightmap() const = 0;

	virtual void NewGroundDrawer() = 0;
	virtual CBaseGroundDrawer* GetGroundDrawer() { return 0; }

	virtual GLuint GetGrassShadingTexture() const { return 0; }
	/**
	 * a texture with RGB for shading and A for height
	 * (0 := above water; 1-255 := under water = 255+height*10)
	 */
	virtual GLuint GetShadingTexture() const = 0;

	/// if you modify the heightmap, call HeightmapUpdated
	virtual void SetHeight(const int& idx, const float& h) = 0;
	virtual void AddHeight(const int& idx, const float& a) = 0;
	void HeightmapUpdated(const int& x1, const int& y1, const int& x2, const int& y2);

	/// size: (mapx+1)*(mapy+1) (per vertex)
	float* orgheightmap;
	/// size: (mapx)*(mapy)     (per face)
	float* centerheightmap;
	/// number of heightmap mipmaps, including full resolution
	static const int numHeightMipMaps = 7;
	/**
	 * array of pointers to heightmap in different resolutions,
	 * mipHeightmap[0] is full resolution,
	 * mipHeightmap[n+1] is half resolution of mipHeightmap[n]
	 */
	float* mipHeightmap[numHeightMipMaps];
	/// size: (mapx/2)*(mapy/2) (1.0 - interpolate(centernomal[i]).y)
	float* slopemap;
	/// size: 2*mapx*mapy (contains 2 normals per quad -> triangle strip)
	float3* facenormals;
	/**
	 * size: mapx*mapy (contains interpolated 1 normal per quad, same as
	 * (facenormal0+facenormal1).Normalize())
	 */
	float3* centernormals;

	/**
	 * size: (mapx + 1) * (mapy + 1),
	 * contains one vertex normal per heightmap pixel
	 */
	std::vector<float3> vertexNormals;

	unsigned char* typemap;

	/// Metal-density/height-map
	CMetalMap* metalMap;

	int width, height;
	float minheight, maxheight;
	float currMinHeight, currMaxHeight;

	unsigned int mapChecksum;

public:
	void UpdateDraw();
	virtual void Update() {}
	virtual void Explosion(float x, float y, float strength) {}

	static inline unsigned char EncodeHeight(const float h) {
		return std::max(0, 255 + (int)(10.0f * h));
	}

	/// Draws the minimap in a quad (with extends: (0,0)-(1,1))
	virtual void DrawMinimap() const = 0;

	/// Feature creation
	virtual int GetNumFeatures() = 0;
	virtual int GetNumFeatureTypes() = 0;
	/// Returns MapFeatureInfo[GetNumFeatures()]
	virtual void GetFeatureInfo(MapFeatureInfo* f) = 0;
	virtual const char* GetFeatureTypeName(int typeID) = 0;

	/**
	 * Infomaps (such as metal map, grass map, ...),
	 * handling them with a string as type seems flexible...
	 * Some map types:
	 *   "metal"  -  metalmap
	 *   "grass"  -  grassmap
	 */
	virtual unsigned char* GetInfoMap(const std::string& name, MapBitmapInfo* bm) = 0;
	virtual void FreeInfoMap(const std::string& name, unsigned char* data) = 0;

	/// Determine visibility for a rectangular grid
	struct IQuadDrawer
	{
		virtual ~IQuadDrawer();
		virtual void DrawQuad (int x,int y) = 0;
	};
	virtual void GridVisibility(CCamera* cam, int quadSize, float maxdist, IQuadDrawer* cb, int extraSize = 0) = 0;
};

extern CReadMap* readmap;

/// Converts a map-square into a float3-position.
inline float3 SquareToFloat3(int xSquare, int zSquare) {
	return float3(xSquare * SQUARE_SIZE, readmap->centerheightmap[(zSquare * gs->mapx) + xSquare], zSquare * SQUARE_SIZE);
}

#endif /* READ_MAP_H */
