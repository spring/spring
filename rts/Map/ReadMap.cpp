/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include <cstdlib>

#include "ReadMap.h"
#include "MapDamage.h"
#include "MapInfo.h"
#include "MetalMap.h"
#include "SM3/SM3Map.h"
#include "SMF/SMFReadMap.h"
#include "lib/gml/gmlmut.h"
#include "Game/LoadScreen.h"
#include "System/bitops.h"
#include "System/EventHandler.h"
#include "System/Exceptions.h"
#include "System/myMath.h"
#include "System/TimeProfiler.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/LoadSave/LoadSaveInterface.h"
#include "System/Misc/RectangleOptimizer.h"

#ifdef USE_UNSYNCED_HEIGHTMAP
#include "Game/GlobalUnsynced.h"
#include "Sim/Misc/LosHandler.h"
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

// assigned to in CGame::CGame ("readmap = CReadMap::LoadMap(mapname)")
CReadMap* readmap = NULL;

CR_BIND_INTERFACE(CReadMap)
CR_REG_METADATA(CReadMap, (
	CR_SERIALIZER(Serialize)
));


CReadMap* CReadMap::LoadMap(const std::string& mapname)
{
	const std::string extension = FileSystem::GetExtension(mapname);
	CReadMap* rm = NULL;

	if (extension.empty()) {
		throw content_error("CReadMap::LoadMap(): missing file extension in mapname '" + mapname + "'");
	}

	if (extension == "sm3") {
		rm = new CSM3ReadMap(mapname);
	} else {
		rm = new CSMFReadMap(mapname);
	}

	if (!rm) {
		return NULL;
	}

	/* Read metal map */
	MapBitmapInfo mbi;
	unsigned char* metalmapPtr = rm->GetInfoMap("metal", &mbi);

	assert(mbi.width == (rm->width >> 1));
	assert(mbi.height == (rm->height >> 1));

	rm->metalMap = new CMetalMap(metalmapPtr, mbi.width, mbi.height, mapInfo->map.maxMetal);

	if (metalmapPtr != NULL) {
		rm->FreeInfoMap("metal", metalmapPtr);
	}


	/* Read type map */
	MapBitmapInfo tbi;
	unsigned char* typemapPtr = rm->GetInfoMap("type", &tbi);

	if (typemapPtr && tbi.width == (rm->width >> 1) && tbi.height == (rm->height >> 1)) {
		assert(gs->hmapx == tbi.width && gs->hmapy == tbi.height);
		rm->typeMap.resize(tbi.width * tbi.height);
		memcpy(&rm->typeMap[0], typemapPtr, tbi.width * tbi.height);
	} else
		throw content_error("Bad/no terrain type map.");

	if (typemapPtr)
		rm->FreeInfoMap("type", typemapPtr);

	return rm;
}


void CReadMap::Serialize(creg::ISerializer& s)
{
	// remove the const
	const float* cshm = GetCornerHeightMapSynced();
	      float*  shm = const_cast<float*>(cshm);

	s.Serialize(shm, 4 * gs->mapxp1 * gs->mapyp1);

	if (!s.IsWriting())
		mapDamage->RecalcArea(2, gs->mapx - 3, 2, gs->mapy - 3);
}


CReadMap::CReadMap()
	: metalMap(NULL)
	, width(0)
	, height(0)
	, initMinHeight(0.0f)
	, initMaxHeight(0.0f)
	, currMinHeight(0.0f)
	, currMaxHeight(0.0f)
	, mapChecksum(0)
	, heightMapSyncedPtr(NULL)
	, heightMapUnsyncedPtr(NULL)
{
}


CReadMap::~CReadMap()
{
	delete metalMap;
}


void CReadMap::Initialize()
{
	// set global map info (TODO: move these to ReadMap!)
	gs->mapx = width;
	gs->mapxm1 = width - 1;
	gs->mapxp1 = width + 1;
	gs->mapy = height;
	gs->mapym1 = height - 1;
	gs->mapyp1 = height + 1;
	gs->mapSquares = gs->mapx * gs->mapy;
	gs->hmapx = gs->mapx >> 1;
	gs->hmapy = gs->mapy >> 1;
	gs->pwr2mapx = next_power_of_2(gs->mapx);
	gs->pwr2mapy = next_power_of_2(gs->mapy);

	{
		char loadMsg[512];
		const char* fmtString = "Loading Map (%u MB)";
		unsigned int reqMemFootPrintKB =
			((( gs->mapxp1)   * gs->mapyp1  * 2     * sizeof(float))         / 1024) +   // cornerHeightMap{Synced, Unsynced}
			((( gs->mapxp1)   * gs->mapyp1  *         sizeof(float))         / 1024) +   // originalHeightMap
			((  gs->mapx      * gs->mapy    * 2 * 2 * sizeof(float3))        / 1024) +   // faceNormals{Synced, Unsynced}
			((  gs->mapx      * gs->mapy    * 2     * sizeof(float3))        / 1024) +   // centerNormals{Synced, Unsynced}
			((( gs->mapxp1)   * gs->mapyp1          * sizeof(float3))        / 1024) +   // VisVertexNormals
			((  gs->mapx      * gs->mapy            * sizeof(float))         / 1024) +   // centerHeightMap
			((  gs->hmapx     * gs->hmapy           * sizeof(float))         / 1024) +   // slopeMap
			((  gs->hmapx     * gs->hmapy           * sizeof(float))         / 1024) +   // MetalMap::extractionMap
			((  gs->hmapx     * gs->hmapy           * sizeof(unsigned char)) / 1024);    // MetalMap::metalMap

		// mipCenterHeightMaps[i]
		for (int i = 1; i < numHeightMipMaps; i++) {
			reqMemFootPrintKB += ((((gs->mapx >> i) * (gs->mapy >> i)) * sizeof(float)) / 1024);
		}

		sprintf(loadMsg, fmtString, reqMemFootPrintKB / 1024);
		loadscreen->SetLoadMessage(loadMsg);
	}

	float3::maxxpos = gs->mapx * SQUARE_SIZE - 1;
	float3::maxzpos = gs->mapy * SQUARE_SIZE - 1;

	originalHeightMap.resize(gs->mapxp1 * gs->mapyp1);
	faceNormalsSynced.resize(gs->mapx * gs->mapy * 2);
	faceNormalsUnsynced.resize(gs->mapx * gs->mapy * 2);
	centerNormalsSynced.resize(gs->mapx * gs->mapy);
	centerNormalsUnsynced.resize(gs->mapx * gs->mapy);
	centerHeightMap.resize(gs->mapx * gs->mapy);

	mipCenterHeightMaps.resize(numHeightMipMaps - 1);
	mipPointerHeightMaps.resize(numHeightMipMaps, NULL);
	mipPointerHeightMaps[0] = &centerHeightMap[0];

	for (int i = 1; i < numHeightMipMaps; i++) {
		mipCenterHeightMaps[i - 1].resize((gs->mapx >> i) * (gs->mapy >> i));
		mipPointerHeightMaps[i] = &mipCenterHeightMaps[i - 1][0];
	}

	slopeMap.resize(gs->hmapx * gs->hmapy);
	visVertexNormals.resize(gs->mapxp1 * gs->mapyp1);

	assert(heightMapSyncedPtr != NULL);
	assert(heightMapUnsyncedPtr != NULL);

	CalcHeightmapChecksum();
	UpdateHeightMapSynced(SRectangle(0, 0, gs->mapx, gs->mapy), true);
	//FIXME can't call that yet cause sky & skyLight aren't created yet (crashes in SMFReadMap.cpp)
	//UpdateDraw(); 
}


void CReadMap::CalcHeightmapChecksum()
{
	const float* heightmap = GetCornerHeightMapSynced();

	initMinHeight =  std::numeric_limits<float>::max();
	initMaxHeight = -std::numeric_limits<float>::max();

	mapChecksum = 0;
	for (int i = 0; i < (gs->mapxp1 * gs->mapyp1); ++i) {
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


void CReadMap::UpdateDraw()
{
	if (unsyncedHeightMapUpdates.empty())
		return;

	std::list<SRectangle> ushmu;
	std::list<SRectangle>::const_iterator ushmuIt;

	{
		GML_STDMUTEX_LOCK(map); // UpdateDraw
		
		if (!unsyncedHeightMapUpdates.empty())
			unsyncedHeightMapUpdates.swap(unsyncedHeightMapUpdatesTemp); // swap to avoid Optimize() inside a mutex
	}
	{

		SCOPED_TIMER("ReadMap::UpdateHeightMapOptimize");

		static bool first = true;
		if (!first) {
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
			first = false;
		}
	}
	if (!unsyncedHeightMapUpdatesTemp.empty()) {
		GML_STDMUTEX_LOCK(map); // UpdateDraw

		unsyncedHeightMapUpdates.splice(unsyncedHeightMapUpdates.end(), unsyncedHeightMapUpdatesTemp);
	}
	// unsyncedHeightMapUpdatesTemp is now guaranteed empty

	SCOPED_TIMER("ReadMap::UpdateHeightMapUnsynced");

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

	SCOPED_TIMER("ReadMap::UpdateHeightMapSynced");

	rect.x1 = std::max(         0, rect.x1 - 1);
	rect.z1 = std::max(         0, rect.z1 - 1);
	rect.x2 = std::min(gs->mapxm1, rect.x2 + 1);
	rect.z2 = std::min(gs->mapym1, rect.z2 + 1);

	UpdateCenterHeightmap(rect);
	UpdateMipHeightmaps(rect);
	UpdateFaceNormals(rect);
	UpdateSlopemap(rect); // must happen after UpdateFaceNormals()!

#ifdef USE_UNSYNCED_HEIGHTMAP
	// push the unsynced update
	if (initialize) {
		// push 1st update through without LOS check
		GML_STDMUTEX_LOCK(map); // UpdateHeightMapSynced
		unsyncedHeightMapUpdates.push_back(rect);
	} else {
		InitHeightMapDigestsVectors();
		const int losSquaresX = loshandler->losSizeX; // size of LOS square in heightmap coords
		const SRectangle& lm = rect * (SQUARE_SIZE * loshandler->invLosDiv); // LOS space

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
	GML_STDMUTEX_LOCK(map); // UpdateHeightMapSynced
	unsyncedHeightMapUpdates.push_back(rect);
#endif
}


void CReadMap::UpdateCenterHeightmap(const SRectangle& rect)
{
	const float* heightmapSynced = GetCornerHeightMapSynced();

	for (int y = rect.z1; y <= rect.z2; y++) {
		for (int x = rect.x1; x <= rect.x2; x++) {
			const int idxTL = (y    ) * gs->mapxp1 + x;
			const int idxTR = (y    ) * gs->mapxp1 + x + 1;
			const int idxBL = (y + 1) * gs->mapxp1 + x;
			const int idxBR = (y + 1) * gs->mapxp1 + x + 1;

			const float height =
				heightmapSynced[idxTL] +
				heightmapSynced[idxTR] +
				heightmapSynced[idxBL] +
				heightmapSynced[idxBR];
			centerHeightMap[y * gs->mapx + x] = height * 0.25f;
		}
	}
}


void CReadMap::UpdateMipHeightmaps(const SRectangle& rect)
{
	for (int i = 0; i < numHeightMipMaps - 1; i++) {
		const int hmapx = gs->mapx >> i;

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


void CReadMap::UpdateFaceNormals(const SRectangle& rect)
{
	const float* heightmapSynced = GetCornerHeightMapSynced();

	const int z1 = std::max(         0, rect.z1 - 1);
	const int x1 = std::max(         0, rect.x1 - 1);
	const int z2 = std::min(gs->mapym1, rect.z2 + 1);
	const int x2 = std::min(gs->mapxm1, rect.x2 + 1);

	int y;
	#pragma omp parallel for private(y)
	for (y = z1; y <= z2; y++) {
		float3 fnTL;
		float3 fnBR;

		for (int x = x1; x <= x2; x++) {
			const int idxTL = (y    ) * gs->mapxp1 + x; // TL
			const int idxBL = (y + 1) * gs->mapxp1 + x; // BL

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

			faceNormalsSynced[(y * gs->mapx + x) * 2    ] = fnTL;
			faceNormalsSynced[(y * gs->mapx + x) * 2 + 1] = fnBR;

			// square-normal
			centerNormalsSynced[y * gs->mapx + x] = (fnTL + fnBR).Normalize();
		}
	}
}


void CReadMap::UpdateSlopemap(const SRectangle& rect)
{
	const int sx = std::max(0, (rect.x1 / 2) - 1);
	const int ex = std::min(gs->hmapx - 1, (rect.x2 / 2) + 1);
	const int sy = std::max(0, (rect.z1 / 2) - 1);
	const int ey = std::min(gs->hmapy - 1, (rect.z2 / 2) + 1);
	
	for (int y = sy; y <= ey; y++) {
		for (int x = sx; x <= ex; x++) {
			const int idx0 = (y*2    ) * (gs->mapx) + x*2;
			const int idx1 = (y*2 + 1) * (gs->mapx) + x*2;

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

			slopeMap[y * gs->hmapx + x] = 1.0f - slope;
		}
	}
}


/// split the update into multiple invididual (los-square) chunks:
void CReadMap::HeightMapUpdateLOSCheck(const SRectangle& rect)
{
	GML_STDMUTEX_LOCK(map); // HeightMapUpdateLOSCheck

	InitHeightMapDigestsVectors();
	const int losSqSize = loshandler->losDiv / SQUARE_SIZE; // size of LOS square in heightmap coords
	const SRectangle& lm = rect * (SQUARE_SIZE * loshandler->invLosDiv); // LOS space

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

			const bool inlos = (gu->spectatingFullView || loshandler->InLos(hmx, hmz, gu->myAllyTeam));
			if (!inlos) {
				PUSH_RECT
				continue;
			}

			const bool heightmapChanged = HasHeightMapChanged(lmx, lmz);
			if (!heightmapChanged) {
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
		const int losSquaresX = loshandler->losSizeX;
		const int losSquaresY = loshandler->losSizeY;
		const int size = (losSquaresX + 1) * (losSquaresY + 1);
		syncedHeightMapDigests.resize(size, 0);
		unsyncedHeightMapDigests.resize(size, 0);
	}
#endif
}


bool CReadMap::HasHeightMapChanged(const int lmx, const int lmy)
{
#ifdef USE_UNSYNCED_HEIGHTMAP
	const int losSquaresX = loshandler->losSizeX;
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
	assert(rect.GetWidth() <= loshandler->losDiv / SQUARE_SIZE);

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
	HeightMapUpdateLOSCheck(SRectangle(0, 0, gs->mapx, gs->mapy));
#endif
}
