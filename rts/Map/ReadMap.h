/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef READ_MAP_H
#define READ_MAP_H

#include <list>

#include "System/creg/creg_cond.h"
#include "System/float3.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/GlobalSynced.h"

#define USE_UNSYNCED_HEIGHTMAP

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

struct HeightMapUpdate {
	HeightMapUpdate(int tlx, int brx,  int tly, int bry, bool inLOS = false): x1(tlx), x2(brx), y1(tly), y2(bry), los(inLOS) {
	}

	int x1, x2;
	int y1, y2;
	bool los;
};


class CReadMap
{
protected:
	CReadMap();

	/// called by implementations of CReadMap
	void Initialize();
	void CalcHeightmapChecksum();

	virtual void UpdateHeightMapUnsynced(const HeightMapUpdate&) = 0;

public:
	CR_DECLARE(CReadMap);

	static CReadMap* LoadMap(const std::string& mapname);
	static inline unsigned char EncodeHeight(const float h) {
		return std::max(0, 255 + int(10.0f * h));
	}

	/// creg serialize callback
	void Serialize(creg::ISerializer& s);


	/**
	 * calculates derived heightmap information
	 * such as normals, centerheightmap and slopemap
	 */
	void UpdateHeightMapSynced(int x1, int z1, int x2, int z2);
	void PushVisibleHeightMapUpdate(int x1, int z1, int x2, int z2, bool);
	void UpdateDraw();

	virtual ~CReadMap();

	virtual void Update() {}
	virtual void UpdateShadingTexture() {}
	virtual void Explosion(float x, float y, float strength) {}

	virtual void NewGroundDrawer() = 0;
	virtual CBaseGroundDrawer* GetGroundDrawer() { return 0; }

	virtual unsigned int GetGrassShadingTexture() const { return 0; }
	/**
	 * a texture with RGB for shading and A for height
	 * (0 := above water; 1-255 := under water = 255+height*10)
	 */
	virtual unsigned int GetShadingTexture() const = 0;

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
		virtual ~IQuadDrawer() {}
		virtual void DrawQuad (int x,int y) = 0;
	};
	virtual void GridVisibility(CCamera* cam, int quadSize, float maxdist, IQuadDrawer* cb, int extraSize = 0) = 0;


	virtual const float* GetCornerHeightMapSynced() const = 0;
	virtual       float* GetCornerHeightMapUnsynced() = 0;

	const float* GetOriginalHeightMapSynced() const { return &originalHeightMap[0]; }
	const float* GetCenterHeightMapSynced() const { return &centerHeightMap[0]; }
	const float* GetMIPHeightMapSynced(unsigned int mip) const { return mipPointerHeightMaps[mip]; }
	const float* GetSlopeMapSynced() const { return &slopeMap[0]; }
	const unsigned char* GetTypeMapSynced() const { return &typeMap[0]; }
	      unsigned char* GetTypeMapSynced()       { return &typeMap[0]; }
	const float3* GetRawVertexNormalsUnsynced() const { return &rawVertexNormals[0]; }
	const float3* GetVisVertexNormalsUnsynced() const { return &visVertexNormals[0]; }
	const float3* GetFaceNormalsSynced() const { return &faceNormalsSynced[0]; }
	const float3* GetFaceNormalsUnsynced() const { return &faceNormalsUnsynced[0]; }
	const float3* GetCenterNormalsSynced() const { return &centerNormalsSynced[0]; }
	const float3* GetCenterNormalsUnsynced() const { return &centerNormalsUnsynced[0]; }


	/// if you modify the heightmap through these, call UpdateHeightMapSynced
	inline void SetHeight(const int& idx, const float& h) {
		static float* hm = const_cast<float*>(GetCornerHeightMapSynced());

		hm[idx] = h;
		currMinHeight = std::min(h, currMinHeight);
		currMaxHeight = std::max(h, currMaxHeight);
	}

	inline void AddHeight(const int& idx, const float& a) {
		static float* hm = const_cast<float*>(GetCornerHeightMapSynced());

		hm[idx] += a;
		currMinHeight = std::min(hm[idx], currMinHeight);
		currMaxHeight = std::max(hm[idx], currMaxHeight);
	}


	/// number of heightmap mipmaps, including full resolution
	static const int numHeightMipMaps = 7;

	/// Metal-density/height-map
	CMetalMap* metalMap;

	int width, height;
	float initMinHeight, initMaxHeight; //! initial minimum- and maximum-height (before any deformations)
	float currMinHeight, currMaxHeight; //! current minimum- and maximum-height 

	unsigned int mapChecksum;


protected:
	std::vector<float> originalHeightMap;    /// size: (mapx+1)*(mapy+1) (per vertex) [SYNCED, does NOT update on terrain deformation]
	std::vector<float> centerHeightMap;      /// size: (mapx  )*(mapy  ) (per face) [SYNCED, updates on terrain deformation]
	std::vector< std::vector<float> > mipCenterHeightMaps;

	/**
	 * array of pointers to heightmaps in different resolutions,
	 * mipPointerHeightMaps[0  ] is full resolution (centerHeightMap),
	 * mipPointerHeightMaps[n+1] is half resolution of mipPointerHeightMaps[n] (mipCenterHeightMaps[n - 1])
	 */
	std::vector< float* > mipPointerHeightMaps;

	std::vector<float3> rawVertexNormals;      /// size:  (mapx + 1) * (mapy + 1), contains one vertex normal per corner-heightmap pixel [UNSYNCED]
	std::vector<float3> visVertexNormals;      /// size:  (mapx + 1) * (mapy + 1), contains one vertex normal per corner-heightmap pixel [UNSYNCED]
	std::vector<float3> faceNormalsSynced;     /// size: 2*mapx      *  mapy     , contains 2 normals per quad -> triangle strip [SYNCED]
	std::vector<float3> faceNormalsUnsynced;   /// size: 2*mapx      *  mapy     , contains 2 normals per quad -> triangle strip [UNSYNCED]
	std::vector<float3> centerNormalsSynced;   /// size:   mapx      *  mapy     , contains 1 interpolated normal per quad, same as (facenormal0+facenormal1).Normalize()) [SYNCED]
	std::vector<float3> centerNormalsUnsynced;

	std::vector<float> slopeMap;               /// size: (mapx/2)    * (mapy/2)  , same as 1.0 - interpolate(centernomal[i]).y [SYNCED]
	std::vector<unsigned char> typeMap;

	std::list<HeightMapUpdate> unsyncedHeightMapUpdates;

#ifdef USE_UNSYNCED_HEIGHTMAP
	struct HeightMapUpdateFilter {
		HeightMapUpdateFilter(bool upd = false) : minx(0), maxx(0), miny(0), maxy(0), update(upd) {}
		HeightMapUpdateFilter(int mnx, int mxx, int mny, int mxy, bool upd = false) : minx(mnx), maxx(mxx), miny(mny), maxy(mxy), update(upd) {}

		void Expand(const HeightMapUpdateFilter& fnew) {
			minx = std::min(minx, fnew.minx);
			maxx = std::max(maxx, fnew.maxx);
			miny = std::min(miny, fnew.miny);
			maxy = std::max(maxy, fnew.maxy);
			update = fnew.update;
		}

		int minx;
		int maxx;
		int miny;
		int maxy;
		bool update;
	};
#endif
};

extern CReadMap* readmap;



/// Converts a map-square into a float3-position.
inline float3 SquareToFloat3(int xSquare, int zSquare) {
	const float* hm = readmap->GetCenterHeightMapSynced();
	const float& h = hm[(zSquare * gs->mapx) + xSquare];
	return float3(xSquare * SQUARE_SIZE, h, zSquare * SQUARE_SIZE);
}

/// TODO: use in SM3 renderer also
inline float GetVisibleVertexHeight(int idx) {

	#ifdef USE_UNSYNCED_HEIGHTMAP
	static const float* hm = readmap->GetCornerHeightMapUnsynced();
	#else
	static const float* hm = readmap->GetCornerHeightMapSynced();
	#endif

	return hm[idx];
}


#endif /* READ_MAP_H */
