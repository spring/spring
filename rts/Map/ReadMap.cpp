/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include <cstdlib>
#include <list>

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

#ifdef USE_UNSYNCED_HEIGHTMAP
#include "Game/GlobalUnsynced.h"
#include "Sim/Misc/LosHandler.h"
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


// assigned to in CGame::CGame ("readMap = CReadMap::LoadMap(mapname)")
CReadMap* readMap = NULL;
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
	CR_MEMBER(initMinHeight),
	CR_MEMBER(initMaxHeight),
	CR_IGNORED(currMinHeight),
	CR_IGNORED(currMaxHeight),
	CR_IGNORED(boundingRadius),
	CR_MEMBER(mapChecksum),
	CR_IGNORED(heightMapSyncedPtr),
	CR_IGNORED(heightMapUnsyncedPtr),
	CR_MEMBER(originalHeightMap),
	CR_IGNORED(centerHeightMap),
	CR_IGNORED(mipCenterHeightMaps),
	CR_IGNORED(mipPointerHeightMaps),
	CR_IGNORED(visVertexNormals),
	CR_IGNORED(faceNormalsSynced),
	CR_IGNORED(faceNormalsUnsynced),
	CR_IGNORED(centerNormalsSynced),
	CR_IGNORED(centerNormalsUnsynced),
	CR_IGNORED(slopeMap),
	CR_IGNORED(sharedCornerHeightMaps),
	CR_IGNORED(sharedCenterHeightMaps),
	CR_IGNORED(sharedFaceNormals),
	CR_IGNORED(sharedCenterNormals),
	CR_IGNORED(sharedSlopeMaps),
	CR_MEMBER(typeMap),
	CR_MEMBER(unsyncedHeightMapUpdates),
	CR_MEMBER(unsyncedHeightMapUpdatesTemp),
	HEIGHTMAP_DIGESTS
	CR_POSTLOAD(PostLoad),
	CR_SERIALIZER(Serialize)
))


CReadMap* CReadMap::LoadMap(const std::string& mapname)
{
	CReadMap* rm = NULL;

	if (FileSystem::GetExtension(mapname) == "sm3") {
		throw content_error("[CReadMap::LoadMap] SM3 maps are no longer supported as of Spring 95.0");
		// rm = new CSM3ReadMap(mapname);
	} else {
		// assume SMF format by default
		rm = new CSMFReadMap(mapname);
	}

	if (rm == NULL)
		return NULL;

	/* read metal- and type-map */
	MapBitmapInfo mbi;
	MapBitmapInfo tbi;

	unsigned char* metalmapPtr = rm->GetInfoMap("metal", &mbi);
	unsigned char* typemapPtr = rm->GetInfoMap("type", &tbi);

	assert(mbi.width == (mapDims.mapx >> 1));
	assert(mbi.height == (mapDims.mapy >> 1));

	rm->metalMap = new CMetalMap(metalmapPtr, mbi.width, mbi.height, mapInfo->map.maxMetal);

	if (metalmapPtr != NULL)
		rm->FreeInfoMap("metal", metalmapPtr);

	if (typemapPtr && tbi.width == (mapDims.mapx >> 1) && tbi.height == (mapDims.mapy >> 1)) {
		assert(mapDims.hmapx == tbi.width && mapDims.hmapy == tbi.height);
		rm->typeMap.resize(tbi.width * tbi.height);
		memcpy(&rm->typeMap[0], typemapPtr, tbi.width * tbi.height);
	} else
		throw content_error("[CReadMap::LoadMap] bad/no terrain typemap");

	if (typemapPtr != NULL)
		rm->FreeInfoMap("type", typemapPtr);

	return rm;
}


void CReadMap::Serialize(creg::ISerializer* s)
{
	// remove the const
	const float* cshm = GetCornerHeightMapSynced();
	      float*  shm = const_cast<float*>(cshm);

	s->Serialize(shm, 4 * mapDims.mapxp1 * mapDims.mapyp1);

	if (!s->IsWriting())
		mapDamage->RecalcArea(2, mapDims.mapx - 3, 2, mapDims.mapy - 3);
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

	sharedSlopeMaps[0] = &slopeMap[0]; // NO UNSYNCED VARIANT
	sharedSlopeMaps[1] = &slopeMap[0];

	//FIXME reconstruct
	/*mipPointerHeightMaps.resize(numHeightMipMaps, NULL);
	mipPointerHeightMaps[0] = &centerHeightMap[0];
	for (int i = 1; i < numHeightMipMaps; i++) {
		mipPointerHeightMaps[i] = &mipCenterHeightMaps[i - 1][0];
	}*/
}


CReadMap::CReadMap()
	: metalMap(NULL)
	, heightMapSyncedPtr(NULL)
	, heightMapUnsyncedPtr(NULL)
	, mapChecksum(0)
	, initMinHeight(0.0f)
	, initMaxHeight(0.0f)
	, currMinHeight(0.0f)
	, currMaxHeight(0.0f)
	, boundingRadius(0.0f)
{
}


CReadMap::~CReadMap()
{
	delete metalMap;
}


void CReadMap::Initialize()
{
	// set global map info (TODO: move these to ReadMap!)
	mapDims.mapxm1 = mapDims.mapx - 1;
	mapDims.mapxp1 = mapDims.mapx + 1;
	mapDims.mapym1 = mapDims.mapy - 1;
	mapDims.mapyp1 = mapDims.mapy + 1;
	mapDims.mapSquares = mapDims.mapx * mapDims.mapy;
	mapDims.hmapx = mapDims.mapx >> 1;
	mapDims.hmapy = mapDims.mapy >> 1;
	mapDims.pwr2mapx = next_power_of_2(mapDims.mapx);
	mapDims.pwr2mapy = next_power_of_2(mapDims.mapy);

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
	centerHeightMap.resize(mapDims.mapx * mapDims.mapy);

	mipCenterHeightMaps.resize(numHeightMipMaps - 1);
	mipPointerHeightMaps.resize(numHeightMipMaps, NULL);
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
	assert(heightMapSyncedPtr != NULL);
	assert(heightMapUnsyncedPtr != NULL);

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

		sharedSlopeMaps[0] = &slopeMap[0]; // NO UNSYNCED VARIANT
		sharedSlopeMaps[1] = &slopeMap[0];
	}

	CalcHeightmapChecksum();
	UpdateHeightMapSynced(SRectangle(0, 0, mapDims.mapx, mapDims.mapy), true);

	// FIXME can't call that yet cause sky & skyLight aren't created yet (crashes in SMFReadMap.cpp)
	// UpdateDraw(true);
}


void CReadMap::CalcHeightmapChecksum()
{
	const float* heightmap = GetCornerHeightMapSynced();

	initMinHeight =  std::numeric_limits<float>::max();
	initMaxHeight = -std::numeric_limits<float>::max();

	mapChecksum = 0;
	for (int i = 0; i < (mapDims.mapxp1 * mapDims.mapyp1); ++i) {
		originalHeightMap[i] = heightmap[i];
		if (heightmap[i] < initMinHeight) { initMinHeight = heightmap[i]; }
		if (heightmap[i] > initMaxHeight) { initMaxHeight = heightmap[i]; }
		mapChecksum +=  (unsigned int) (heightmap[i] * 100);
		mapChecksum ^= *(unsigned int*) &heightmap[i];
	}

	for (unsigned int a = 0; a < mapInfo->map.name.size(); ++a) {
		mapChecksum += mapInfo->map.name[a];
		mapChecksum *= mapInfo->map.name[a];
	}

	currMinHeight = initMinHeight;
	currMaxHeight = initMaxHeight;
}


void CReadMap::UpdateDraw(bool firstCall)
{
	if (unsyncedHeightMapUpdates.empty())
		return;

	std::list<SRectangle> ushmu;
	std::list<SRectangle>::const_iterator ushmuIt;

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
					ushmu.push_back(rect);
					unsyncedHeightMapUpdatesTemp.pop_front();
				}
			}
		} else {
			// first update is full map
			unsyncedHeightMapUpdatesTemp.swap(ushmu);
		}
	}
	if (!unsyncedHeightMapUpdatesTemp.empty()) {
		unsyncedHeightMapUpdates.splice(unsyncedHeightMapUpdates.end(), unsyncedHeightMapUpdatesTemp);
	}
	// unsyncedHeightMapUpdatesTemp is now guaranteed empty

	for (ushmuIt = ushmu.begin(); ushmuIt != ushmu.end(); ++ushmuIt) {
		UpdateHeightMapUnsynced(*ushmuIt);
	}
	for (ushmuIt = ushmu.begin(); ushmuIt != ushmu.end(); ++ushmuIt) {
		eventHandler.UnsyncedHeightMapUpdate(*ushmuIt);
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

		const int losSquaresX = losHandler->losSizeX; // size of LOS square in heightmap coords
		const SRectangle& lm = rect * (SQUARE_SIZE * losHandler->invLosDiv); // LOS space

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

		for (int y = sy; y < ey; y += 2) {
			for (int x = sx; x < ex; x += 2) {
				const float height =
					mipPointerHeightMaps[i][(x    ) + (y    ) * hmapx] +
					mipPointerHeightMaps[i][(x    ) + (y + 1) * hmapx] +
					mipPointerHeightMaps[i][(x + 1) + (y    ) * hmapx] +
					mipPointerHeightMaps[i][(x + 1) + (y + 1) * hmapx];
				mipPointerHeightMaps[i + 1][(x / 2) + (y / 2) * hmapx / 2] = height * 0.25f;
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

			#ifdef USE_UNSYNCED_HEIGHTMAP
			if (initialize) {
				faceNormalsUnsynced[(y * mapDims.mapx + x) * 2    ] = faceNormalsSynced[(y * mapDims.mapx + x) * 2    ];
				faceNormalsUnsynced[(y * mapDims.mapx + x) * 2 + 1] = faceNormalsSynced[(y * mapDims.mapx + x) * 2 + 1];
				centerNormalsUnsynced[y * mapDims.mapx + x] = centerNormalsSynced[y * mapDims.mapx + x];
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
	const int losSqSize = losHandler->losDiv / SQUARE_SIZE; // size of LOS square in heightmap coords
	const SRectangle& lm = rect * (SQUARE_SIZE * losHandler->invLosDiv); // LOS space

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
			if (!(gu->spectatingFullView || losHandler->InLos(hmx, hmz, gu->myAllyTeam))) {
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
		const int losSquaresX = losHandler->losSizeX;
		const int losSquaresY = losHandler->losSizeY;
		const int size = (losSquaresX + 1) * (losSquaresY + 1);
		syncedHeightMapDigests.resize(size, 0);
		unsyncedHeightMapDigests.resize(size, 0);
	}
#endif
}


bool CReadMap::HasHeightMapChanged(const int lmx, const int lmy)
{
#ifdef USE_UNSYNCED_HEIGHTMAP
	const int losSquaresX = losHandler->losSizeX;
	const int idx = lmx + lmy * (losSquaresX + 1);
	assert(idx < syncedHeightMapDigests.size());
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
	assert(rect.GetWidth() <= losHandler->losDiv / SQUARE_SIZE);

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

