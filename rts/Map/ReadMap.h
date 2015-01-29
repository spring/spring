/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef READ_MAP_H
#define READ_MAP_H

#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/float3.h"
#include "System/type2.h"
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


struct MapDimensions {
public:
	CR_DECLARE_STRUCT(MapDimensions)

	MapDimensions() {
		mapx   = 0;
		mapxm1 = mapx - 1;
		mapxp1 = mapx + 1;

		mapy   = 0;
		mapym1 = mapy - 1;
		mapyp1 = mapy + 1;

		mapSquares = mapx * mapy;

		hmapx = mapx >> 1;
		hmapy = mapy >> 1;

		pwr2mapx = mapx; //next_power_of_2(mapx);
		pwr2mapy = mapy; //next_power_of_2(mapy);
	}

	/**
	* @brief map x
	*
	* The map's number of squares in the x direction
	* (note that the number of vertices is one more)
	*/
	int mapx;
	int mapxm1; // mapx minus one
	int mapxp1; // mapx plus one

	/**
	* @brief map y
	*
	* The map's number of squares in the y direction
	*/
	int mapy;
	int mapym1; // mapy minus one
	int mapyp1; // mapy plus one

	/**
	* @brief map squares
	*
	* Total number of squares on the map
	*/
	int mapSquares;

	/**
	* @brief half map x
	*
	* Contains half of the number of squares in the x direction
	*/
	int hmapx;

	/**
	* @brief half map y
	*
	* Contains half of the number of squares in the y direction
	*/
	int hmapy;

	/**
	* @brief map x power of 2
	*
	* Map's size in the x direction rounded
	* up to the next power of 2
	*/
	int pwr2mapx;

	/**
	* @brief map y power of 2
	*
	* Map's size in the y direction rounded
	* up to the next power of 2
	*/
	int pwr2mapy;
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
	CR_DECLARE(CReadMap)

	static CReadMap* LoadMap(const std::string& mapname);
	static inline unsigned char EncodeHeight(const float h) {
		return std::max(0, 255 + int(10.0f * h));
	}

	/// creg serialize callback
	void Serialize(creg::ISerializer* s);
	void PostLoad();


	/**
	 * calculates derived heightmap information
	 * such as normals, centerheightmap and slopemap
	 */
	void UpdateHeightMapSynced(SRectangle rect, bool initialize = false);
	void UpdateLOS(const SRectangle& rect);
	void BecomeSpectator();
	void UpdateDraw(bool firstCall);

	virtual ~CReadMap();

	virtual void Update() {}
	virtual void UpdateShadingTexture() {}

	virtual void NewGroundDrawer() = 0;
	virtual CBaseGroundDrawer* GetGroundDrawer() { return 0; }

	virtual unsigned int GetMiniMapTexture() const { return 0; }
	virtual int2 GetMiniMapTextureSize() const { return int2(0,0); }
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
	/// call ResetState for statically allocated drawer objects
	struct IQuadDrawer
	{
		virtual ~IQuadDrawer() {}
		virtual void ResetState() = 0;
		virtual void DrawQuad(int x, int y) = 0;
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

	/// synced versions
	const float* GetCornerHeightMapSynced() const { return sharedCornerHeightMaps[true]; }
	const float3* GetFaceNormalsSynced()    const { return sharedFaceNormals[true]; }
	const float3* GetCenterNormalsSynced()  const { return sharedCenterNormals[true]; }
	/// unsynced versions
	const float* GetCornerHeightMapUnsynced() const { return sharedCornerHeightMaps[false]; }
	const float3* GetFaceNormalsUnsynced()    const { return sharedFaceNormals[false]; }
	const float3* GetCenterNormalsUnsynced()  const { return sharedCenterNormals[false]; }


	/// shared interface
	const float* GetSharedCornerHeightMap(bool synced) const { return sharedCornerHeightMaps[synced]; }
	const float* GetSharedCenterHeightMap(bool synced) const { return sharedCenterHeightMaps[synced]; }
	const float3* GetSharedFaceNormals(bool synced) const { return sharedFaceNormals[synced]; }
	const float3* GetSharedCenterNormals(bool synced) const { return sharedCenterNormals[synced]; }
	const float* GetSharedSlopeMap(bool synced) const { return sharedSlopeMaps[synced]; }

	/// if you modify the heightmap through these, call UpdateHeightMapSynced
	float SetHeight(const int idx, const float h, const int add = 0);
	float AddHeight(const int idx, const float a);


	float GetInitMinHeight() const { return initMinHeight; }
	float GetCurrMinHeight() const { return currMinHeight; }
	float GetInitMaxHeight() const { return initMaxHeight; }
	float GetCurrMaxHeight() const { return currMaxHeight; }
	float GetBoundingRadius() const { return boundingRadius; }

	bool IsUnderWater() const { return (currMaxHeight <  0.0f); }
	bool IsAboveWater() const { return (currMinHeight >= 0.0f); }

	bool HasVisibleWater() const;
	bool HasOnlyVoidWater() const;

	unsigned int GetMapChecksum() const { return mapChecksum; }

private:
	void UpdateCenterHeightmap(const SRectangle& rect, bool initialize);
	void UpdateMipHeightmaps(const SRectangle& rect, bool initialize);
	void UpdateFaceNormals(const SRectangle& rect, bool initialize);
	void UpdateSlopemap(const SRectangle& rect, bool initialize);

	inline void HeightMapUpdateLOSCheck(const SRectangle& rect);
	inline bool HasHeightMapChanged(const int lmx, const int lmy);
	inline void InitHeightMapDigestsVectors();

public:
	/// number of heightmap mipmaps, including full resolution
	static const int numHeightMipMaps = 7;

	/// Metal-density/height-map
	CMetalMap* metalMap;

protected:
	// these point to the actual heightmap data
	// which is allocated by subclass instances
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

private:
	// these combine the various synced and unsynced arrays
	// for branch-less access: [0] = !synced, [1] = synced
	const float* sharedCornerHeightMaps[2];
	const float* sharedCenterHeightMaps[2];
	const float3* sharedFaceNormals[2];
	const float3* sharedCenterNormals[2];
	const float* sharedSlopeMaps[2];

#ifdef USE_UNSYNCED_HEIGHTMAP
	/// used to filer LOS updates (so only update UHM on LOS updates when the heightmap was changed beforehand)
	/// size: in LOS resolution
	std::vector<unsigned char>   syncedHeightMapDigests;
	std::vector<unsigned char> unsyncedHeightMapDigests;
#endif

	unsigned int mapChecksum;

	float initMinHeight, initMaxHeight; //< initial minimum- and maximum-height (before any deformations)
	float currMinHeight, currMaxHeight; //< current minimum- and maximum-height
	float boundingRadius;
};


extern CReadMap* readMap;
extern MapDimensions mapDims;



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
	const float* hm = readMap->GetCenterHeightMapSynced();
	const float h = hm[(zSquare * mapDims.mapx) + xSquare];
	return float3(xSquare * SQUARE_SIZE, h, zSquare * SQUARE_SIZE);
}

/// TODO: use in SM3 renderer also
inline float GetVisibleVertexHeight(int idx) {
	const float* hm = readMap->GetCornerHeightMapUnsynced();
	return hm[idx];
}


#endif /* READ_MAP_H */
