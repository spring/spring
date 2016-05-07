/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include <cstdlib>

#include "ReadMap.h"
#include "MapDamage.h"
#include "MapInfo.h"
#include "MetalMap.h"
// #include "SM3/SM3Map.h"
#include "SMF/SMFReadMap.h"
#include "Game/LoadScreen.h"
#include "System/bitops.h"
#include "System/EventHandler.h"
#include "System/Exceptions.h"
#include "System/myMath.h"
#include "System/TimeProfiler.h"
#include "System/ThreadPool.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Misc/RectangleOptimizer.h"
#include "System/Sync/HsiehHash.h"
#include "System/Util.h"

#ifdef USE_UNSYNCED_HEIGHTMAP
#include "Game/GlobalUnsynced.h"
#include "Sim/Misc/LosHandler.h"
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


// assigned to in CGame::CGame ("readMap = CReadMap::LoadMap(mapname)")
CReadMap* readMap = nullptr;
MapDimensions mapDims;


#ifdef USE_UNSYNCED_HEIGHTMAP
	#define	HEIGHTMAP_DIGESTS \
		CR_MEMBER(syncedHeightMapDigests), \
		CR_MEMBER(unsyncedHeightMapDigests),
#else
	#define HEIGHTMAP_DIGESTS
#endif

CR_BIND(MapDimensions, ())
CR_REG_METADATA(MapDimensions, (
	CR_MEMBER(mapx),
	CR_MEMBER(mapxm1),
	CR_MEMBER(mapxp1),

	CR_MEMBER(mapy),
	CR_MEMBER(mapym1),
	CR_MEMBER(mapyp1),

	CR_MEMBER(mapSquares),

	CR_MEMBER(hmapx),
	CR_MEMBER(hmapy),
	CR_MEMBER(pwr2mapx),
	CR_MEMBER(pwr2mapy)
))

CR_BIND_INTERFACE(CReadMap)
CR_REG_METADATA(CReadMap, (
	CR_MEMBER(metalMap),
	CR_IGNORED(initHeightBounds),
	CR_IGNORED(currHeightBounds),
	CR_IGNORED(boundingRadius),
	CR_IGNORED(mapChecksum),
	CR_IGNORED(heightMapSyncedPtr),
	CR_IGNORED(heightMapUnsyncedPtr),
	CR_IGNORED(originalHeightMap),
	CR_IGNORED(centerHeightMap),
	CR_IGNORED(mipCenterHeightMaps),
	CR_IGNORED(mipPointerHeightMaps),
	CR_IGNORED(visVertexNormals),
	CR_IGNORED(faceNormalsSynced),
	CR_IGNORED(faceNormalsUnsynced),
	CR_IGNORED(centerNormalsSynced),
	CR_IGNORED(centerNormalsUnsynced),
	CR_IGNORED(centerNormals2DSynced),
	CR_IGNORED(centerNormals2DUnsynced),
	CR_IGNORED(slopeMap),
	CR_IGNORED(sharedCornerHeightMaps),
	CR_IGNORED(sharedCenterHeightMaps),
	CR_IGNORED(sharedFaceNormals),
	CR_IGNORED(sharedCenterNormals),
	CR_IGNORED(sharedCenterNormals2D),
	CR_IGNORED(sharedSlopeMaps),
	CR_MEMBER(typeMap),
	CR_MEMBER(unsyncedHeightMapUpdates),
	CR_MEMBER(unsyncedHeightMapUpdatesTemp),
	HEIGHTMAP_DIGESTS
	CR_POSTLOAD(PostLoad),
	CR_SERIALIZER(Serialize)
))



MapTexture::~MapTexture() {
	// do NOT delete a Lua-set texture here!
	glDeleteTextures(1, &texIDs[RAW_TEX_IDX]);

	texIDs[RAW_TEX_IDX] = 0;
	texIDs[LUA_TEX_IDX] = 0;
}



CReadMap* CReadMap::LoadMap(const std::string& mapname)
{
	CReadMap* rm = nullptr;

	if (FileSystem::GetExtension(mapname) == "sm3") {
		throw content_error("[CReadMap::LoadMap] SM3 maps are no longer supported as of Spring 95.0");
		// rm = new CSM3ReadMap(mapname);
	} else {
		// assume SMF format by default
		rm = new CSMFReadMap(mapname);
	}

	if (rm == nullptr)
		return nullptr;

	/* read metal- and type-map */
	MapBitmapInfo mbi;
	MapBitmapInfo tbi;

	unsigned char* metalmapPtr = rm->GetInfoMap("metal", &mbi);
	unsigned char* typemapPtr = rm->GetInfoMap("type", &tbi);

	assert(mbi.width == (mapDims.mapx >> 1));
	assert(mbi.height == (mapDims.mapy >> 1));

	rm->metalMap = new CMetalMap(metalmapPtr, mbi.width, mbi.height, mapInfo->map.maxMetal);

	if (metalmapPtr != nullptr)
		rm->FreeInfoMap("metal", metalmapPtr);

	if (typemapPtr && tbi.width == (mapDims.mapx >> 1) && tbi.height == (mapDims.mapy >> 1)) {
		assert(mapDims.hmapx == tbi.width && mapDims.hmapy == tbi.height);
		rm->typeMap.resize(tbi.width * tbi.height);
		memcpy(&rm->typeMap[0], typemapPtr, tbi.width * tbi.height);
	} else
		throw content_error("[CReadMap::LoadMap] bad/no terrain typemap");

	if (typemapPtr != nullptr)
		rm->FreeInfoMap("type", typemapPtr);

	return rm;
}

#ifdef USING_CREG
void CReadMap::Serialize(creg::ISerializer* s)
{
	// using integers so we can xor the original heightmap with the
	// current one - should compress significantly better.
	      float* shm  = const_cast<float*>(GetCornerHeightMapSynced());
	      int*   ishm  = reinterpret_cast<int*>(shm);
	const int*   ioshm = reinterpret_cast<const int*>(GetOriginalHeightMapSynced());

	int height;
	if (s->IsWriting()) {
		for (unsigned int i = 0; i < mapDims.mapxp1 * mapDims.mapyp1; i++) {
			height = ishm[i] ^ ioshm[i];
			s->Serialize(&height, sizeof(int));
		}
	} else {
		for (unsigned int i = 0; i < mapDims.mapxp1 * mapDims.mapyp1; i++) {
			s->Serialize(&height, sizeof(int));
			ishm[i] = height ^ ioshm[i];
		}
		mapDamage->RecalcArea(2, mapDims.mapx - 3, 2, mapDims.mapy - 3);
	}

}


void CReadMap::PostLoad()
{
	#ifndef USE_UNSYNCED_HEIGHTMAP
	heightMapUnsyncedPtr = heightMapSyncedPtr;
	#endif

	sharedCornerHeightMaps[0] = &(*heightMapUnsyncedPtr)[0];
	sharedCornerHeightMaps[1] = &(*heightMapSyncedPtr)[0];

	sharedCenterHeightMaps[0] = &centerHeightMap[0]; // NO UNSYNCED VARIANT
	sharedCenterHeightMaps[1] = &centerHeightMap[0];

	sharedFaceNormals[0] = &faceNormalsUnsynced[0];
	sharedFaceNormals[1] = &faceNormalsSynced[0];

	sharedCenterNormals[0] = &centerNormalsUnsynced[0];
	sharedCenterNormals[1] = &centerNormalsSynced[0];

	sharedCenterNormals2D[0] = &centerNormals2DUnsynced[0];
	sharedCenterNormals2D[1] = &centerNormals2DSynced[0];

	sharedSlopeMaps[0] = &slopeMap[0]; // NO UNSYNCED VARIANT
	sharedSlopeMaps[1] = &slopeMap[0];

	//FIXME reconstruct
	/*mipPointerHeightMaps.resize(numHeightMipMaps, nullptr);
	mipPointerHeightMaps[0] = &centerHeightMap[0];
	for (int i = 1; i < numHeightMipMaps; i++) {
		mipPointerHeightMaps[i] = &mipCenterHeightMaps[i - 1][0];
	}*/
}
#endif //USING_CREG


CReadMap::CReadMap()
	: metalMap(nullptr)
	, heightMapSyncedPtr(nullptr)
	, heightMapUnsyncedPtr(nullptr)
	, mapChecksum(0)
	, boundingRadius(0.0f)
{
}


CReadMap::~CReadMap()
{
	SafeDelete(metalMap);
}


void CReadMap::Initialize()
{
	// set global map info
	mapDims.Initialize();

	boundingRadius = math::sqrt(Square(mapDims.mapx * SQUARE_SIZE) + Square(mapDims.mapy * SQUARE_SIZE)) * 0.5f;

	{
		char loadMsg[512];
		const char* fmtString = "Loading Map (%u MB)";
		unsigned int reqMemFootPrintKB =
			((( mapDims.mapxp1)   * mapDims.mapyp1  * 2     * sizeof(float))         / 1024) +   // cornerHeightMap{Synced, Unsynced}
			((( mapDims.mapxp1)   * mapDims.mapyp1  *         sizeof(float))         / 1024) +   // originalHeightMap
			((  mapDims.mapx      * mapDims.mapy    * 2 * 2 * sizeof(float3))        / 1024) +   // faceNormals{Synced, Unsynced}
			((  mapDims.mapx      * mapDims.mapy    * 2     * sizeof(float3))        / 1024) +   // centerNormals{Synced, Unsynced}
			((( mapDims.mapxp1)   * mapDims.mapyp1          * sizeof(float3))        / 1024) +   // VisVertexNormals
			((  mapDims.mapx      * mapDims.mapy            * sizeof(float))         / 1024) +   // centerHeightMap
			((  mapDims.hmapx     * mapDims.hmapy           * sizeof(float))         / 1024) +   // slopeMap
			((  mapDims.hmapx     * mapDims.hmapy           * sizeof(float))         / 1024) +   // MetalMap::extractionMap
			((  mapDims.hmapx     * mapDims.hmapy           * sizeof(unsigned char)) / 1024);    // MetalMap::metalMap

		// mipCenterHeightMaps[i]
		for (int i = 1; i < numHeightMipMaps; i++) {
			reqMemFootPrintKB += ((((mapDims.mapx >> i) * (mapDims.mapy >> i)) * sizeof(float)) / 1024);
		}

		sprintf(loadMsg, fmtString, reqMemFootPrintKB / 1024);
		loadscreen->SetLoadMessage(loadMsg);
	}

	float3::maxxpos = mapDims.mapx * SQUARE_SIZE - 1;
	float3::maxzpos = mapDims.mapy * SQUARE_SIZE - 1;

	originalHeightMap.resize(mapDims.mapxp1 * mapDims.mapyp1);
	faceNormalsSynced.resize(mapDims.mapx * mapDims.mapy * 2);
	faceNormalsUnsynced.resize(mapDims.mapx * mapDims.mapy * 2);
	centerNormalsSynced.resize(mapDims.mapx * mapDims.mapy);
	centerNormalsUnsynced.resize(mapDims.mapx * mapDims.mapy);
	centerNormals2DSynced.resize(mapDims.mapx * mapDims.mapy);
	centerNormals2DUnsynced.resize(mapDims.mapx * mapDims.mapy);
	centerHeightMap.resize(mapDims.mapx * mapDims.mapy);

	mipCenterHeightMaps.resize(numHeightMipMaps - 1);
	mipPointerHeightMaps.resize(numHeightMipMaps, nullptr);
	mipPointerHeightMaps[0] = &centerHeightMap[0];

	for (int i = 1; i < numHeightMipMaps; i++) {
		mipCenterHeightMaps[i - 1].resize((mapDims.mapx >> i) * (mapDims.mapy >> i));
		mipPointerHeightMaps[i] = &mipCenterHeightMaps[i - 1][0];
	}

	slopeMap.resize(mapDims.hmapx * mapDims.hmapy);
	visVertexNormals.resize(mapDims.mapxp1 * mapDims.mapyp1);

	// note: if USE_UNSYNCED_HEIGHTMAP is false, then
	// heightMapUnsyncedPtr points to an empty vector
	// for SMF maps so indexing it is forbidden (!)
	assert(heightMapSyncedPtr != nullptr);
	assert(heightMapUnsyncedPtr != nullptr);

	{
		#ifndef USE_UNSYNCED_HEIGHTMAP
		heightMapUnsyncedPtr = heightMapSyncedPtr;
		#endif

		sharedCornerHeightMaps[0] = &(*heightMapUnsyncedPtr)[0];
		sharedCornerHeightMaps[1] = &(*heightMapSyncedPtr)[0];

		sharedCenterHeightMaps[0] = &centerHeightMap[0]; // NO UNSYNCED VARIANT
		sharedCenterHeightMaps[1] = &centerHeightMap[0];

		sharedFaceNormals[0] = &faceNormalsUnsynced[0];
		sharedFaceNormals[1] = &faceNormalsSynced[0];

		sharedCenterNormals[0] = &centerNormalsUnsynced[0];
		sharedCenterNormals[1] = &centerNormalsSynced[0];

		sharedCenterNormals2D[0] = &centerNormals2DUnsynced[0];
		sharedCenterNormals2D[1] = &centerNormals2DSynced[0];

		sharedSlopeMaps[0] = &slopeMap[0]; // NO UNSYNCED VARIANT
		sharedSlopeMaps[1] = &slopeMap[0];
	}

	mapChecksum = CalcHeightmapChecksum();
	UpdateHeightMapSynced(SRectangle(0, 0, mapDims.mapx, mapDims.mapy), true);

	// FIXME can't call that yet cause sky & skyLight aren't created yet (crashes in SMFReadMap.cpp)
	// UpdateDraw(true);
}


unsigned int CReadMap::CalcHeightmapChecksum()
{
	const float* heightmap = GetCornerHeightMapSynced();

	initHeightBounds.x =  std::numeric_limits<float>::max();
	initHeightBounds.y = -std::numeric_limits<float>::max();

	unsigned int checksum = 0;

	for (int i = 0; i < (mapDims.mapxp1 * mapDims.mapyp1); ++i) {
		originalHeightMap[i] = heightmap[i];

		initHeightBounds.x = std::min(initHeightBounds.x, heightmap[i]);
		initHeightBounds.y = std::max(initHeightBounds.y, heightmap[i]);

		checksum = HsiehHash(&heightmap[i], sizeof(heightmap[i]), checksum);
	}

	checksum = HsiehHash(mapInfo->map.name.c_str(), mapInfo->map.name.size(), checksum);

	currHeightBounds.x = initHeightBounds.x;
	currHeightBounds.y = initHeightBounds.y;

	return checksum;
}


unsigned int CReadMap::CalcTypemapChecksum()
{
	unsigned int checksum = 0;
	checksum = HsiehHash(&typeMap[0], typeMap.size() * sizeof(typeMap[0]), checksum);

	for (unsigned int i = 0; i < CMapInfo::NUM_TERRAIN_TYPES; i++) {
		const CMapInfo::TerrainType& tt = mapInfo->terrainTypes[i];

		checksum = HsiehHash(tt.name.c_str(), tt.name.size(), checksum);
		checksum = HsiehHash(&tt.hardness, offsetof(CMapInfo::TerrainType, receiveTracks) - offsetof(CMapInfo::TerrainType, hardness), checksum);
	}

	return checksum;
}


void CReadMap::UpdateDraw(bool firstCall)
{
	if (unsyncedHeightMapUpdates.empty())
		return;

	CRectangleOptimizer::container unsyncedHeightMapUpdatesSwap;

	{
		if (!unsyncedHeightMapUpdates.empty())
			unsyncedHeightMapUpdates.swap(unsyncedHeightMapUpdatesTemp); // swap to avoid Optimize() inside a mutex
	}
	{
		if (!firstCall) {
			if (!unsyncedHeightMapUpdatesTemp.empty()) {
				unsyncedHeightMapUpdatesTemp.Optimize();

				int updateArea = unsyncedHeightMapUpdatesTemp.GetTotalArea() * 0.0625f + (50 * 50);

				while (updateArea > 0 && !unsyncedHeightMapUpdatesTemp.empty()) {
					const SRectangle& rect = unsyncedHeightMapUpdatesTemp.front();
					updateArea -= rect.GetArea();
					unsyncedHeightMapUpdatesSwap.push_back(rect);
					unsyncedHeightMapUpdatesTemp.pop_front();
				}
			}
		} else {
			// first update is full map
			unsyncedHeightMapUpdatesTemp.swap(unsyncedHeightMapUpdatesSwap);
		}
	}

	if (!unsyncedHeightMapUpdatesTemp.empty())
		unsyncedHeightMapUpdates.splice(unsyncedHeightMapUpdates.end(), unsyncedHeightMapUpdatesTemp);

	// unsyncedHeightMapUpdatesTemp is now guaranteed empty
	for (const SRectangle& rect: unsyncedHeightMapUpdatesSwap) {
		UpdateHeightMapUnsynced(rect);
	}
	for (const SRectangle& rect: unsyncedHeightMapUpdatesSwap) {
		eventHandler.UnsyncedHeightMapUpdate(rect);
	}
}


void CReadMap::UpdateHeightMapSynced(SRectangle rect, bool initialize)
{
	if (rect.GetArea() <= 0) {
		// do not bother with zero-area updates
		return;
	}

	rect.x1 = std::max(         0, rect.x1 - 1);
	rect.z1 = std::max(         0, rect.z1 - 1);
	rect.x2 = std::min(mapDims.mapxm1, rect.x2 + 1);
	rect.z2 = std::min(mapDims.mapym1, rect.z2 + 1);

	UpdateCenterHeightmap(rect, initialize);
	UpdateMipHeightmaps(rect, initialize);
	UpdateFaceNormals(rect, initialize);
	UpdateSlopemap(rect, initialize); // must happen after UpdateFaceNormals()!

#ifdef USE_UNSYNCED_HEIGHTMAP
	// push the unsynced update
	if (initialize) {
		// push 1st update through without LOS check
		unsyncedHeightMapUpdates.push_back(rect);
	} else {
		InitHeightMapDigestsVectors();

		const int losSquaresX = losHandler->los.size.x; // size of LOS square in heightmap coords
		const SRectangle& lm = rect * (SQUARE_SIZE * losHandler->los.invDiv); // LOS space

		// we updated the heightmap so change their digest (byte-overflow is intentional!)
		for (int lmx = lm.x1; lmx <= lm.x2; ++lmx) {
			for (int lmz = lm.z1; lmz <= lm.z2; ++lmz) {
				const int idx = lmx + lmz * (losSquaresX + 1);
				assert(idx < syncedHeightMapDigests.size());
				syncedHeightMapDigests[idx]++;
			}
		}

		HeightMapUpdateLOSCheck(rect);
	}
#else
	unsyncedHeightMapUpdates.push_back(rect);
#endif
}


void CReadMap::UpdateCenterHeightmap(const SRectangle& rect, bool initialize)
{
	const float* heightmapSynced = GetCornerHeightMapSynced();

	for (int y = rect.z1; y <= rect.z2; y++) {
		for (int x = rect.x1; x <= rect.x2; x++) {
			const int idxTL = (y    ) * mapDims.mapxp1 + x;
			const int idxTR = (y    ) * mapDims.mapxp1 + x + 1;
			const int idxBL = (y + 1) * mapDims.mapxp1 + x;
			const int idxBR = (y + 1) * mapDims.mapxp1 + x + 1;

			const float height =
				heightmapSynced[idxTL] +
				heightmapSynced[idxTR] +
				heightmapSynced[idxBL] +
				heightmapSynced[idxBR];
			centerHeightMap[y * mapDims.mapx + x] = height * 0.25f;
		}
	}
}


void CReadMap::UpdateMipHeightmaps(const SRectangle& rect, bool initialize)
{
	for (int i = 0; i < numHeightMipMaps - 1; i++) {
		const int hmapx = mapDims.mapx >> i;

		const int sx = (rect.x1 >> i) & (~1);
		const int ex = (rect.x2 >> i);
		const int sy = (rect.z1 >> i) & (~1);
		const int ey = (rect.z2 >> i);
		float* topMipMap = mipPointerHeightMaps[i];
		float* subMipMap = mipPointerHeightMaps[i + 1];

		for (int y = sy; y < ey; y += 2) {
			for (int x = sx; x < ex; x += 2) {
				const float height =
					topMipMap[(x    ) + (y    ) * hmapx] +
					topMipMap[(x    ) + (y + 1) * hmapx] +
					topMipMap[(x + 1) + (y    ) * hmapx] +
					topMipMap[(x + 1) + (y + 1) * hmapx];
				subMipMap[(x / 2) + (y / 2) * hmapx / 2] = height * 0.25f;
			}
		}
	}
}


void CReadMap::UpdateFaceNormals(const SRectangle& rect, bool initialize)
{
	const float* heightmapSynced = GetCornerHeightMapSynced();

	const int z1 = std::max(         0, rect.z1 - 1);
	const int x1 = std::max(         0, rect.x1 - 1);
	const int z2 = std::min(mapDims.mapym1, rect.z2 + 1);
	const int x2 = std::min(mapDims.mapxm1, rect.x2 + 1);

	for_mt(z1, z2+1, [&](const int y) {
		float3 fnTL;
		float3 fnBR;

		for (int x = x1; x <= x2; x++) {
			const int idxTL = (y    ) * mapDims.mapxp1 + x; // TL
			const int idxBL = (y + 1) * mapDims.mapxp1 + x; // BL

			const float& hTL = heightmapSynced[idxTL    ];
			const float& hTR = heightmapSynced[idxTL + 1];
			const float& hBL = heightmapSynced[idxBL    ];
			const float& hBR = heightmapSynced[idxBL + 1];

			// normal of top-left triangle (face) in square
			//
			//  *---> e1
			//  |
			//  |
			//  v
			//  e2
			//const float3 e1( SQUARE_SIZE, hTR - hTL,           0);
			//const float3 e2(           0, hBL - hTL, SQUARE_SIZE);
			//const float3 fnTL = (e2.cross(e1)).Normalize();
			fnTL.y = SQUARE_SIZE;
			fnTL.x = - (hTR - hTL);
			fnTL.z = - (hBL - hTL);
			fnTL.Normalize();

			// normal of bottom-right triangle (face) in square
			//
			//         e3
			//         ^
			//         |
			//         |
			//  e4 <---*
			//const float3 e3(-SQUARE_SIZE, hBL - hBR,           0);
			//const float3 e4(           0, hTR - hBR,-SQUARE_SIZE);
			//const float3 fnBR = (e4.cross(e3)).Normalize();
			fnBR.y = SQUARE_SIZE;
			fnBR.x = (hBL - hBR);
			fnBR.z = (hTR - hBR);
			fnBR.Normalize();

			faceNormalsSynced[(y * mapDims.mapx + x) * 2    ] = fnTL;
			faceNormalsSynced[(y * mapDims.mapx + x) * 2 + 1] = fnBR;
			// square-normal
			centerNormalsSynced[y * mapDims.mapx + x] = (fnTL + fnBR).Normalize();
			centerNormals2DSynced[y * mapDims.mapx + x] = (fnTL + fnBR).Normalize2D();

			#ifdef USE_UNSYNCED_HEIGHTMAP
			if (initialize) {
				faceNormalsUnsynced[(y * mapDims.mapx + x) * 2    ] = faceNormalsSynced[(y * mapDims.mapx + x) * 2    ];
				faceNormalsUnsynced[(y * mapDims.mapx + x) * 2 + 1] = faceNormalsSynced[(y * mapDims.mapx + x) * 2 + 1];
				centerNormalsUnsynced[y * mapDims.mapx + x] = centerNormalsSynced[y * mapDims.mapx + x];
				centerNormals2DUnsynced[y * mapDims.mapx + x] = centerNormals2DSynced[y * mapDims.mapx + x];
			}
			#endif
		}
	});
}


void CReadMap::UpdateSlopemap(const SRectangle& rect, bool initialize)
{
	const int sx = std::max(0, (rect.x1 / 2) - 1);
	const int ex = std::min(mapDims.hmapx - 1, (rect.x2 / 2) + 1);
	const int sy = std::max(0, (rect.z1 / 2) - 1);
	const int ey = std::min(mapDims.hmapy - 1, (rect.z2 / 2) + 1);

	for (int y = sy; y <= ey; y++) {
		for (int x = sx; x <= ex; x++) {
			const int idx0 = (y*2    ) * (mapDims.mapx) + x*2;
			const int idx1 = (y*2 + 1) * (mapDims.mapx) + x*2;

			float avgslope = 0.0f;
			avgslope += faceNormalsSynced[(idx0    ) * 2    ].y;
			avgslope += faceNormalsSynced[(idx0    ) * 2 + 1].y;
			avgslope += faceNormalsSynced[(idx0 + 1) * 2    ].y;
			avgslope += faceNormalsSynced[(idx0 + 1) * 2 + 1].y;
			avgslope += faceNormalsSynced[(idx1    ) * 2    ].y;
			avgslope += faceNormalsSynced[(idx1    ) * 2 + 1].y;
			avgslope += faceNormalsSynced[(idx1 + 1) * 2    ].y;
			avgslope += faceNormalsSynced[(idx1 + 1) * 2 + 1].y;
			avgslope *= 0.125f;

			float maxslope =              faceNormalsSynced[(idx0    ) * 2    ].y;
			maxslope = std::min(maxslope, faceNormalsSynced[(idx0    ) * 2 + 1].y);
			maxslope = std::min(maxslope, faceNormalsSynced[(idx0 + 1) * 2    ].y);
			maxslope = std::min(maxslope, faceNormalsSynced[(idx0 + 1) * 2 + 1].y);
			maxslope = std::min(maxslope, faceNormalsSynced[(idx1    ) * 2    ].y);
			maxslope = std::min(maxslope, faceNormalsSynced[(idx1    ) * 2 + 1].y);
			maxslope = std::min(maxslope, faceNormalsSynced[(idx1 + 1) * 2    ].y);
			maxslope = std::min(maxslope, faceNormalsSynced[(idx1 + 1) * 2 + 1].y);

			// smooth it a bit, so small holes don't block huge tanks
			const float lerp = maxslope / avgslope;
			const float slope = mix(maxslope, avgslope, lerp);

			slopeMap[y * mapDims.hmapx + x] = 1.0f - slope;
		}
	}
}


/// split the update into multiple invididual (los-square) chunks:
void CReadMap::HeightMapUpdateLOSCheck(const SRectangle& rect)
{
	InitHeightMapDigestsVectors();
	const int losSqSize = losHandler->los.divisor / SQUARE_SIZE; // size of LOS square in heightmap coords
	const SRectangle& lm = rect * (SQUARE_SIZE * losHandler->los.invDiv); // LOS space

	for (int lmz = lm.z1; lmz <= lm.z2; ++lmz) {
		const int hmz = lmz * losSqSize;
		      int hmx = lm.x1 * losSqSize;

		SRectangle subrect(hmx, hmz, hmx, hmz + losSqSize);
		#define PUSH_RECT \
			if (subrect.GetArea() > 0) { \
				subrect.ClampIn(rect); \
				unsyncedHeightMapUpdates.push_back(subrect); \
				subrect = SRectangle(hmx + losSqSize, hmz, hmx + losSqSize, hmz + losSqSize); \
			} else { \
				subrect.x1 = hmx + losSqSize; \
				subrect.x2 = hmx + losSqSize; \
			}

		for (int lmx = lm.x1; lmx <= lm.x2; ++lmx) {
			hmx = lmx * losSqSize;

			#ifdef USE_UNSYNCED_HEIGHTMAP
			if (!(gu->spectatingFullView || losHandler->InLos(SquareToFloat3(hmx, hmz), gu->myAllyTeam))) {
				PUSH_RECT
				continue;
			}
			#endif

			if (!HasHeightMapChanged(lmx, lmz)) {
				PUSH_RECT
				continue;
			}

			// update rectangle size
			subrect.x2 = hmx + losSqSize;
		}

		PUSH_RECT
	}
}


void CReadMap::InitHeightMapDigestsVectors()
{
#ifdef USE_UNSYNCED_HEIGHTMAP
	if (syncedHeightMapDigests.empty()) {
		const int size = (losHandler->los.size.x + 1) * (losHandler->los.size.y + 1);
		syncedHeightMapDigests.resize(size, 0);
		unsyncedHeightMapDigests.resize(size, 0);
	}
#endif
}


bool CReadMap::HasHeightMapChanged(const int lmx, const int lmy)
{
#ifdef USE_UNSYNCED_HEIGHTMAP
	const int losSquaresX = losHandler->los.size.x;
	const int idx = lmx + lmy * (losSquaresX + 1);
	assert(idx < syncedHeightMapDigests.size() && idx >= 0);
	const bool heightmapChanged = (unsyncedHeightMapDigests[idx] != syncedHeightMapDigests[idx]);
	if (heightmapChanged) {
		unsyncedHeightMapDigests[idx] = syncedHeightMapDigests[idx];
	}
	return heightmapChanged;
#else
	return true;
#endif
}


void CReadMap::UpdateLOS(const SRectangle& rect)
{
#ifdef USE_UNSYNCED_HEIGHTMAP
	if (gu->spectatingFullView)
		return;

	// currently we use the LOS for view updates (alternatives are AirLOS and/or radar)
	// cause the others use different resolutions we must check it here for safety
	// (if you want to use another source you need to change the res. of syncedHeightMapDigests etc.)
	assert(rect.GetWidth() <= losHandler->los.divisor / SQUARE_SIZE);

	//HACK: UpdateLOS() is called for single LOS squares, but we use <= in HeightMapUpdateLOSCheck().
	// This would make our update area 4x as large, so we need to make the rectangle a point. Better
	// would be to use < instead of <= everywhere.
	SRectangle r = rect;
	r.x2 = r.x1;
	r.z2 = r.z1;

	HeightMapUpdateLOSCheck(rect);
#endif
}


void CReadMap::BecomeSpectator()
{
#ifdef USE_UNSYNCED_HEIGHTMAP
	HeightMapUpdateLOSCheck(SRectangle(0, 0, mapDims.mapx, mapDims.mapy));
#endif
}

bool CReadMap::HasVisibleWater() const { return (!mapInfo->map.voidWater && !IsAboveWater()); }
bool CReadMap::HasOnlyVoidWater() const { return (mapInfo->map.voidWater && IsUnderWater()); }

