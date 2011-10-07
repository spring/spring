/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include <cstdlib>
#include "System/mmgr.h"

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
CReadMap* readmap = 0;

CR_BIND_INTERFACE(CReadMap)
CR_REG_METADATA(CReadMap, (
	CR_SERIALIZER(Serialize)
));


CReadMap* CReadMap::LoadMap(const std::string& mapname)
{
	if (mapname.length() < 3)
		throw content_error("CReadMap::LoadMap(): mapname '" + mapname + "' too short");

	const std::string extension = FileSystem::GetExtension(mapname);

	CReadMap* rm = NULL;

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


CReadMap::CReadMap():
	metalMap(NULL),
	width(0),
	height(0),
	initMinHeight(0.0f),
	initMaxHeight(0.0f),
	currMinHeight(0.0f),
	currMaxHeight(0.0f),
	mapChecksum(0)
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
			((( gs->mapxp1)   * gs->mapyp1  * 2     * sizeof(float3))        / 1024) +   // {raw, vis}VertexNormals
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

	CalcHeightmapChecksum();
	UpdateHeightMapSynced(0, 0, gs->mapx, gs->mapy, true);
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

		static bool first = true;
		if (!first) {
			unsyncedHeightMapUpdates.Optimize();

			int updateArea = unsyncedHeightMapUpdates.GetArea() * 0.0625f + 500;

			while (updateArea > 0 && !unsyncedHeightMapUpdates.empty()) {
				const SRectangle& rect = unsyncedHeightMapUpdates.front();
				updateArea -= rect.GetWidth() * rect.GetHeight();
				ushmu.push_back(rect);
				unsyncedHeightMapUpdates.pop_front();
			}
		} else {
			// first update is full map
			unsyncedHeightMapUpdates.swap(ushmu);
			first = false;
		}
	}

	SCOPED_TIMER("ReadMap::UpdateHeightMapUnsynced");

	for (ushmuIt = ushmu.begin(); ushmuIt != ushmu.end(); ++ushmuIt) {
		UpdateHeightMapUnsynced(*ushmuIt);
	}

	for (ushmuIt = ushmu.begin(); ushmuIt != ushmu.end(); ++ushmuIt) {
		eventHandler.UnsyncedHeightMapUpdate(*ushmuIt);
	}
}



void CReadMap::UpdateHeightMapSynced(int x1, int z1, int x2, int z2, bool initialize)
{
	if ((x1 >= x2) || (z1 >= z2)) {
		// do not bother with zero-area updates
		return;
	}

	SCOPED_TIMER("ReadMap::UpdateHeightMapSynced");

	const float* heightmapSynced = GetCornerHeightMapSynced();

	x1 = std::max(         0, x1 - 1);
	z1 = std::max(         0, z1 - 1);
	x2 = std::min(gs->mapxm1, x2 + 1);
	z2 = std::min(gs->mapym1, z2 + 1);

	for (int y = z1; y <= z2; y++) {
		for (int x = x1; x <= x2; x++) {
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

	for (int i = 0; i < numHeightMipMaps - 1; i++) {
		const int hmapx = gs->mapx >> i;

		for (int y = ((z1 >> i) & (~1)); y < (z2 >> i); y += 2) {
			for (int x = ((x1 >> i) & (~1)); x < (x2 >> i); x += 2) {
				const float height =
					mipPointerHeightMaps[i][(x    ) + (y    ) * hmapx] +
					mipPointerHeightMaps[i][(x    ) + (y + 1) * hmapx] +
					mipPointerHeightMaps[i][(x + 1) + (y    ) * hmapx] +
					mipPointerHeightMaps[i][(x + 1) + (y + 1) * hmapx];
				mipPointerHeightMaps[i + 1][(x / 2) + (y / 2) * hmapx / 2] = height * 0.25f;
			}
		}
	}

	const int decy = std::max(         0, z1 - 1);
	const int incy = std::min(gs->mapym1, z2 + 1);
	const int decx = std::max(         0, x1 - 1);
	const int incx = std::min(gs->mapxm1, x2 + 1);

	//! create the surface normals
	for (int y = decy; y <= incy; y++) {
		for (int x = decx; x <= incx; x++) {
			const int idxTL = (y    ) * gs->mapxp1 + x; // TL
			const int idxBL = (y + 1) * gs->mapxp1 + x; // BL

			//!  *---> e1
			//!  |
			//!  |
			//!  v
			//!  e2
			float3 e1( SQUARE_SIZE, heightmapSynced[idxTL + 1] - heightmapSynced[idxTL],            0);
			float3 e2(           0, heightmapSynced[idxBL    ] - heightmapSynced[idxTL],  SQUARE_SIZE);

			//! normal of top-left triangle (face) in square
			const float3 fnTL = (e2.cross(e1)).Normalize();

			//!         e1
			//!         ^
			//!         |
			//!         |
			//!  e2 <---*
			e1 = float3(-SQUARE_SIZE, heightmapSynced[idxBL    ] - heightmapSynced[idxBL + 1],            0);
			e2 = float3(           0, heightmapSynced[idxTL + 1] - heightmapSynced[idxBL + 1], -SQUARE_SIZE);

			//! normal of bottom-right triangle (face) in square
			const float3 fnBR = (e2.cross(e1)).Normalize();

			faceNormalsSynced[(y * gs->mapx + x) * 2] = fnTL;
			faceNormalsSynced[(y * gs->mapx + x) * 2 + 1] = fnBR;

			//! square-normal
			centerNormalsSynced[y * gs->mapx + x] = (fnTL + fnBR).Normalize();
		}
	}

	for (int y = std::max(0, (z1 / 2) - 1); y <= std::min(gs->hmapy - 1, (z2 / 2) + 1); y++) {
		for (int x = std::max(0, (x1 / 2) - 1); x <= std::min(gs->hmapx - 1, (x2 / 2) + 1); x++) {
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
			avgslope /= 8.0f;

			float maxslope =              faceNormalsSynced[(idx0    ) * 2    ].y;
			maxslope = std::min(maxslope, faceNormalsSynced[(idx0    ) * 2 + 1].y);
			maxslope = std::min(maxslope, faceNormalsSynced[(idx0 + 1) * 2    ].y);
			maxslope = std::min(maxslope, faceNormalsSynced[(idx0 + 1) * 2 + 1].y);
			maxslope = std::min(maxslope, faceNormalsSynced[(idx1    ) * 2    ].y);
			maxslope = std::min(maxslope, faceNormalsSynced[(idx1    ) * 2 + 1].y);
			maxslope = std::min(maxslope, faceNormalsSynced[(idx1 + 1) * 2    ].y);
			maxslope = std::min(maxslope, faceNormalsSynced[(idx1 + 1) * 2 + 1].y);

			//! smooth it a bit, so small holes don't block huge tanks
			const float lerp = maxslope / avgslope;
			const float slope = maxslope * (1.0f - lerp) + avgslope * lerp;

			slopeMap[y * gs->hmapx + x] = 1.0f - slope;
		}
	}

	// push the unsynced update
#ifdef USE_UNSYNCED_HEIGHTMAP
	if (initialize) {
		PushVisibleHeightMapUpdate(x1, z1, x2, z2, false);
	} else {
		HeightMapUpdateLOSCheck(SRectangle(x1, z1, x2, z2));
	}
#else
	PushVisibleHeightMapUpdate(x1, z1, x2, z2, false);
#endif
}


/// split the update into multiple invididual (los-square) chunks:
void CReadMap::HeightMapUpdateLOSCheck(SRectangle rect)
{
	if (gu->spectatingFullView) {
		PushVisibleHeightMapUpdate(rect.x1, rect.z1, rect.x2, rect.z2, false);
		return;
	}

	static const int losSqSize = 1 << (loshandler->losMipLevel - 1);

	// find the losMap squares covered by the update rectangle
	const int lmx1 = rect.x1 / losSqSize;
	const int lmx2 = rect.x2 / losSqSize;
	const int lmz1 = rect.z1 / losSqSize;
	const int lmz2 = rect.z2 / losSqSize;

	for (int lmx = lmx1; lmx <= lmx2; ++lmx) {
		const int hmx = lmx * losSqSize;
		for (int lmz = lmz1; lmz <= lmz2; ++lmz) {
			const int hmz = lmz * losSqSize;
			const bool inlos = loshandler->InLos(hmx, hmz, gu->myAllyTeam);

			if (inlos) {
				//TODO optimize (don't spam hundred of smaller HeightMapUpdates)
				const int lx1 = Clamp(hmx, rect.x1, rect.x2);
				const int lx2 = Clamp(hmx + losSqSize, rect.x1, rect.x2);
				const int lz1 = Clamp(hmz, rect.z1, rect.z2);
				const int lz2 = Clamp(hmz + losSqSize, rect.z1, rect.z2);
				PushVisibleHeightMapUpdate(lx1, lz1, lx2, lz2, false);
			}
		}
	}
}


void CReadMap::PushVisibleHeightMapUpdate(int x1, int z1,  int x2, int z2,  bool losMapCall)
{
	GML_STDMUTEX_LOCK(map); // PushVisibleHeightMapUpdate

	unsyncedHeightMapUpdates.push_back(SRectangle(x1, z1, x2, z2));
}
