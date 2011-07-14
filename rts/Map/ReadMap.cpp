/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/StdAfx.h"

#include <cstdlib>
#include "System/mmgr.h"

#include "ReadMap.h"
#include "MapDamage.h"
#include "MapInfo.h"
#include "MetalMap.h"
#include "SM3/SM3Map.h"
#include "SMF/SMFReadMap.h"
#include "Game/LoadScreen.h"
#include "System/bitops.h"
#include "System/ConfigHandler.h"
#include "System/Exceptions.h"
#include "System/LogOutput.h"
#include "System/LoadSave/LoadSaveInterface.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/myMath.h"
#include "System/TimeProfiler.h"


#ifdef USE_UNSYNCED_HEIGHTMAP
#include "Game/GlobalUnsynced.h"
#include "Sim/Misc/LosHandler.h"
#endif

using namespace std;

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

	const string extension = filesystem.GetExtension(mapname);

	CReadMap* rm = 0;

	if (extension == "sm3") {
		rm = new CSm3ReadMap(mapname);
	} else {
		rm = new CSmfReadMap(mapname);
	}

	if (!rm) {
		return 0;
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

	typeMap.clear();
	slopeMap.clear();

	for (unsigned int n = 0; n < mipCenterHeightMaps.size(); n++) {
		mipCenterHeightMaps[n].clear();
	}

	mipCenterHeightMaps.clear();
	mipPointerHeightMaps.clear();
	centerHeightMap.clear();
	originalHeightMap.clear();
	rawVertexNormals.clear();
	visVertexNormals.clear();
	faceNormalsSynced.clear();
	faceNormalsUnsynced.clear();
	centerNormalsSynced.clear();
	centerNormalsUnsynced.clear();
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
	rawVertexNormals.resize(gs->mapxp1 * gs->mapyp1);
	visVertexNormals.resize(gs->mapxp1 * gs->mapyp1);

	CalcHeightmapChecksum();
	UpdateHeightMapSynced(0, 0, gs->mapx, gs->mapy);
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

void CReadMap::UpdateDraw() {
	std::list<HeightMapUpdate> ushmu;
	std::list<HeightMapUpdate>::const_iterator ushmuIt;

	{
		GML_STDMUTEX_LOCK(map); // UpdateDraw

		static const float MIN_UNSYNCED_UPDATES_RATIO = 0.0625f;
		static const size_t MIN_UNSYNCED_UPDATES = 32U;
		       const size_t maxUpdates = unsyncedHeightMapUpdates.size();
		       const size_t minUpdates = maxUpdates * MIN_UNSYNCED_UPDATES_RATIO;
		       const size_t numUpdates = std::min(maxUpdates, std::max(MIN_UNSYNCED_UPDATES, minUpdates));

		// process the first <numUpdates> pending updates
		for (size_t n = 0; n < numUpdates; n++) {
			ushmu.push_back(unsyncedHeightMapUpdates.front());
			unsyncedHeightMapUpdates.pop_front();
		}
	}

	SCOPED_TIMER("ReadMap::UpdateHeightMapUnsynced");

	for (ushmuIt = ushmu.begin(); ushmuIt != ushmu.end(); ++ushmuIt) {
		UpdateHeightMapUnsynced(*ushmuIt);
	}
}



void CReadMap::UpdateHeightMapSynced(int x1, int z1, int x2, int z2)
{
	if ((x1 >= x2) || (z1 >= z2)) {
		// do not bother with zero-area updates
		return;
	}

	const float* heightmapSynced = GetCornerHeightMapSynced();

	x1 = std::max(           0, x1 - 1);
	z1 = std::max(           0, z1 - 1);
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

	const int decy = std::max(           0, z1 - 1);
	const int incy = std::min(gs->mapym1, z2 + 1);
	const int decx = std::max(           0, x1 - 1);
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

	//! push the unsynced update
	PushVisibleHeightMapUpdate(x1, z1,  x2, z2,  false);
}


void CReadMap::PushVisibleHeightMapUpdate(int x1, int z1,  int x2, int z2,  bool losMapCall)
{
	GML_STDMUTEX_LOCK(map); // PushVisibleHeightMapUpdate

	#ifdef USE_UNSYNCED_HEIGHTMAP
	if (gs->frameNum <= 0) {
		// NOTE:
		//     loshandler does not exist on startup when the map is loaded
		//     rather than duplicating LosHandler member variables here to
		//     avoid this special case, just push the entire update at once
		unsyncedHeightMapUpdates.push_back(HeightMapUpdate(x1, x2,  z1, z2,  true));
	} else {
		static const int losSqSizeX = loshandler->losSizeX;
		static const int losSqSizeY = loshandler->losSizeY;
		static const int losSquaresX = (gs->mapx / losSqSizeX);
		static const int losSquaresY = (gs->mapy / losSqSizeY);
		static std::vector<HeightMapUpdateFilter> updateFilters((losSquaresX + 1) * (losSquaresY + 1));

		// split the update into multiple invididual (los-square) chunks:
		// we need to do this because the reduced-rate unsynced updating
		// can mean a chunk has gone out of LOS again by the time it gets
		// processed in UpdateHeightMapUnsynced
		//
		// terrain deformations can overlap, so also check if chunks have
		// have not already been marked before pushing them
		//
		// find the losMap squares covered by the update rectangle
		const int lmxTL = x1 / losSqSizeX, lmxBR = x2 / losSqSizeX;
		const int lmzTL = z1 / losSqSizeY, lmzBR = z2 / losSqSizeY;

		for (int lmx = lmxTL; lmx <= lmxBR; ++lmx) {
			const int hmx = lmx * losSqSizeX;

			for (int lmz = lmzTL; lmz <= lmzBR; ++lmz) {
				HeightMapUpdateFilter& f = updateFilters[lmz * losSquaresX + lmx];

				// if the unsynced heightmap has not actually been modified for this LOS block, skip it
				// (assumes deformations occur first and LOS is obtained *after* ::update has been set)
				if (losMapCall && !f.update)
					continue;

				// note: calls can come from losMap even when spectatingFullView
				const int hmz = lmz * losSqSizeY;
				const bool inlos = losMapCall || gu->spectatingFullView || loshandler->InLos(hmx, hmz, gu->myAllyTeam);

				// create a new filter for this los-square
				HeightMapUpdateFilter fnew(
					Clamp(hmx, x1, x2), Clamp(hmx + losSqSizeX - 1, x1, x2),
					Clamp(hmz, z1, z2), Clamp(hmz + losSqSizeY - 1, z1, z2),
					true
				);

				if (!inlos) {
					// the heightmap has been modified, but the block is not currently in LOS
					// if this block was never visited (ever, or since coming into LOS before)
					// then set a filter for it, otherwise expand (this heightmap update might
					// cover a larger area than the previous)
					if (!f.update) {
						f = fnew;
					} else {
						f.Expand(fnew);
					}
				} else if (losMapCall) {
					// the block has now come into LOS: any subsequent deformations whose area
					// includes this block do not need to update the squares covered by it (so
					// long as it remains visible)
					fnew = f;
					f.update = false;
				}

				HeightMapUpdate hmUpdate = HeightMapUpdate(
					fnew.minx, std::min(gs->mapxm1, fnew.maxx), 
					fnew.miny, std::min(gs->mapym1, fnew.maxy),
					inlos
				);
				unsyncedHeightMapUpdates.push_back(hmUpdate);
			}
		}
	}
	#else
	// only reached from UpdateHeightMapSynced, los-state is irrelevant
	// (however, could still split chunk to save some CPU in UpdateDraw)
	unsyncedHeightMapUpdates.push_back(HeightMapUpdate(x1, x2,  z1, z2));
	#endif
}
