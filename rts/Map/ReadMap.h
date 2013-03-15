/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef READ_MAP_H
#define READ_MAP_H

#include <list>

#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/float3.h"
#include "System/creg/creg_cond.h"
#include "System/Misc/RectangleOptimizer.h"

#define USE_UNSYNCED_HEIGHTMAP

class CMetalMap;
class CCamera;
class CUnit;
class CSolidObject;
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



class CReadMap
{
protected:
	CReadMap();

	/// called by implementations of CReadMap
	void Initialize();
	void CalcHeightmapChecksum();

	virtual void UpdateHeightMapUnsynced(const SRectangle&) = 0;

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
	void UpdateHeightMapSynced(SRectangle rect, bool initialize = false);
	void UpdateLOS(const SRectangle& rect);
	void BecomeSpectator();
	void UpdateDraw();

	virtual ~CReadMap();

	virtual void Update() {}
	virtual void UpdateShadingTexture() {}

	virtual void NewGroundDrawer() = 0;
	virtual CBaseGroundDrawer* GetGroundDrawer() { return 0; }

	virtual unsigned int GetMiniMapTexture() const { return 0; }
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


	/// synced only
	const float* GetOriginalHeightMapSynced() const { return &originalHeightMap[0]; }
	const float* GetCenterHeightMapSynced() const { return &centerHeightMap[0]; }
	const float* GetMIPHeightMapSynced(unsigned int mip) const { return mipPointerHeightMaps[mip]; }
	const float* GetSlopeMapSynced() const { return &slopeMap[0]; }
	const unsigned char* GetTypeMapSynced() const { return &typeMap[0]; }
	      unsigned char* GetTypeMapSynced()       { return &typeMap[0]; }

	/// unsynced only
	const float3* GetVisVertexNormalsUnsynced() const { return &visVertexNormals[0]; }

	/// both
	/// synced versions
	const float* GetCornerHeightMapSynced() const { return &(*heightMapSyncedPtr)[0]; }
	const float3* GetFaceNormalsSynced()    const { return &faceNormalsSynced[0]; }
	const float3* GetCenterNormalsSynced()  const { return &centerNormalsSynced[0]; }
	/// unsynced versions
#ifdef USE_UNSYNCED_HEIGHTMAP
	const float* GetCornerHeightMapUnsynced() const { return &(*heightMapUnsyncedPtr)[0]; }
	const float3* GetFaceNormalsUnsynced()   const { return &faceNormalsUnsynced[0]; }
	const float3* GetCenterNormalsUnsynced() const { return &centerNormalsUnsynced[0]; }
#else
	const float* GetCornerHeightMapUnsynced() const { return GetCornerHeightMapSynced(); }
	const float3* GetFaceNormalsUnsynced()    const { return GetFaceNormalsSynced(); }
	const float3* GetCenterNormalsUnsynced()  const { return GetCenterNormalsSynced(); }
#endif

	/// shared interface
	/// Use when code is shared between synced & unsynced, in such cases these functions will have a better branch-prediciton behavior.
	static const float* GetCornerHeightMap(const bool& synced);
	static const float* GetCenterHeightMap(const bool& synced);
	static const float3* GetFaceNormals(const bool& synced);
	static const float3* GetCenterNormals(const bool& synced);
	static const float* GetSlopeMap(const bool& synced);

	/// if you modify the heightmap through these, call UpdateHeightMapSynced
	float SetHeight(const int idx, const float h, const int add = 0);
	float AddHeight(const int idx, const float a);

public:
	/// number of heightmap mipmaps, including full resolution
	static const int numHeightMipMaps = 7;

	/// Metal-density/height-map
	CMetalMap* metalMap;

	int width, height;
	float initMinHeight, initMaxHeight; //< initial minimum- and maximum-height (before any deformations)
	float currMinHeight, currMaxHeight; //< current minimum- and maximum-height 

	unsigned int mapChecksum;

private:
	void UpdateCenterHeightmap(const SRectangle& rect);
	void UpdateMipHeightmaps(const SRectangle& rect);
	void UpdateFaceNormals(const SRectangle& rect);
	void UpdateSlopemap(const SRectangle& rect);
	
	inline void HeightMapUpdateLOSCheck(const SRectangle& rect);
	inline bool HasHeightMapChanged(const int lmx, const int lmy);
	inline void InitHeightMapDigestsVectors();

protected:
	std::vector<float>* heightMapSyncedPtr;      //< size: (mapx+1)*(mapy+1) (per vertex) [SYNCED, updates on terrain deformation]
	std::vector<float>* heightMapUnsyncedPtr;    //< size: (mapx+1)*(mapy+1) (per vertex) [UNSYNCED]
	std::vector<float> originalHeightMap;        //< size: (mapx+1)*(mapy+1) (per vertex) [SYNCED, does NOT update on terrain deformation]
	std::vector<float> centerHeightMap;          //< size: (mapx  )*(mapy  ) (per face) [SYNCED, updates on terrain deformation]
	std::vector< std::vector<float> > mipCenterHeightMaps;

	/**
	 * array of pointers to heightmaps in different resolutions,
	 * mipPointerHeightMaps[0  ] is full resolution (centerHeightMap),
	 * mipPointerHeightMaps[n+1] is half resolution of mipPointerHeightMaps[n] (mipCenterHeightMaps[n - 1])
	 */
	std::vector< float* > mipPointerHeightMaps;

	std::vector<float3> visVertexNormals;      //< size:  (mapx + 1) * (mapy + 1), contains one vertex normal per corner-heightmap pixel [UNSYNCED]
	std::vector<float3> faceNormalsSynced;     //< size: 2*mapx      *  mapy     , contains 2 normals per quad -> triangle strip [SYNCED]
	std::vector<float3> faceNormalsUnsynced;   //< size: 2*mapx      *  mapy     , contains 2 normals per quad -> triangle strip [UNSYNCED]
	std::vector<float3> centerNormalsSynced;   //< size:   mapx      *  mapy     , contains 1 interpolated normal per quad, same as (facenormal0+facenormal1).Normalize()) [SYNCED]
	std::vector<float3> centerNormalsUnsynced;

	std::vector<float> slopeMap;               //< size: (mapx/2)    * (mapy/2)  , same as 1.0 - interpolate(centernomal[i]).y [SYNCED]
	std::vector<unsigned char> typeMap;

	CRectangleOptimizer unsyncedHeightMapUpdates;
	CRectangleOptimizer unsyncedHeightMapUpdatesTemp;

#ifdef USE_UNSYNCED_HEIGHTMAP
	/// used to filer LOS updates (so only update UHM on LOS updates when the heightmap was changed beforehand)
	/// size: in LOS resolution
	std::vector<unsigned char>   syncedHeightMapDigests;
	std::vector<unsigned char> unsyncedHeightMapDigests;
#endif
};

extern CReadMap* readmap;



/**
 * Inlined functions
 */
inline const float* CReadMap::GetCornerHeightMap(const bool& synced) {
	// Note, this doesn't save a branch compared to `(synced) ? SHM : UHM`. Cause static vars always check if they were already initialized.
	//  But they are only initialized once, so this branch will always fail. And so branch-prediction will have easier going.
	assert(readmap && readmap->mapChecksum);
	return synced ? readmap->GetCornerHeightMapSynced() : readmap->GetCornerHeightMapUnsynced();
}
inline const float* CReadMap::GetCenterHeightMap(const bool& synced) {
	assert(readmap && readmap->mapChecksum);
	// TODO: add unsynced variant
	return synced ? readmap->GetCenterHeightMapSynced() : readmap->GetCenterHeightMapSynced();
}
inline const float3* CReadMap::GetFaceNormals(const bool& synced) {
	assert(readmap && readmap->mapChecksum);
	return synced ? readmap->GetFaceNormalsSynced() : readmap->GetFaceNormalsUnsynced();
}
inline const float3* CReadMap::GetCenterNormals(const bool& synced) {
	assert(readmap && readmap->mapChecksum);
	return synced ? readmap->GetCenterNormalsSynced() : readmap->GetCenterNormalsUnsynced();
}
inline const float* CReadMap::GetSlopeMap(const bool& synced) {
	assert(readmap && readmap->mapChecksum);
	// TODO: add unsynced variant (or add a LOS check foreach square access?)
	return synced ? readmap->GetSlopeMapSynced() : readmap->GetSlopeMapSynced();
}






inline float CReadMap::SetHeight(const int idx, const float h, const int add) {
	float& x = (*heightMapSyncedPtr)[idx];

	// add=0 <--> x = x*0 + h =   h
	// add=1 <--> x = x*1 + h = x+h
	x = x * add + h;

	currMinHeight = std::min(x, currMinHeight);
	currMaxHeight = std::max(x, currMaxHeight);

	return x;
}

inline float CReadMap::AddHeight(const int idx, const float a) {
	return SetHeight(idx, a, 1);
}





/// Converts a map-square into a float3-position.
inline float3 SquareToFloat3(int xSquare, int zSquare) {
	const float* hm = readmap->GetCenterHeightMapSynced();
	const float& h = hm[(zSquare * gs->mapx) + xSquare];
	return float3(xSquare * SQUARE_SIZE, h, zSquare * SQUARE_SIZE);
}

/// TODO: use in SM3 renderer also
inline float GetVisibleVertexHeight(int idx) {
	const float* hm = readmap->GetCornerHeightMapUnsynced();
	return hm[idx];
}


#endif /* READ_MAP_H */
