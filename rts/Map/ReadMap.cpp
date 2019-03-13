/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include <cstdlib>
#include <cstring> // memcpy

#include "ReadMap.h"
#include "MapDamage.h"
#include "MapInfo.h"
#include "MetalMap.h"
#include "Rendering/Env/MapRendering.h"
#include "SMF/SMFReadMap.h"
#include "Game/LoadScreen.h"
#include "System/bitops.h"
#include "System/EventHandler.h"
#include "System/Exceptions.h"
#include "System/SpringMath.h"
#include "System/Threading/ThreadPool.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Log/ILog.h"
#include "System/Sync/HsiehHash.h"
#include "System/SafeUtil.h"
#include "System/TimeProfiler.h"

#ifdef USE_UNSYNCED_HEIGHTMAP
#include "Game/GlobalUnsynced.h"
#include "Sim/Misc/LosHandler.h"
#endif

#define MAX_UHM_RECTS_PER_FRAME static_cast<size_t>(128)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////



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
	CR_IGNORED(initHeightBounds),
	CR_IGNORED(currHeightBounds),
	CR_IGNORED(boundingRadius),
	CR_IGNORED(mapChecksum),

	CR_IGNORED(heightMapSyncedPtr),
	CR_IGNORED(heightMapUnsyncedPtr),

	/*
	CR_IGNORED(originalHeightMap),
	CR_IGNORED(centerHeightMap),
	CR_IGNORED(mipCenterHeightMaps),
	*/
	CR_IGNORED(mipPointerHeightMaps),
	/*
	CR_IGNORED(visVertexNormals),
	CR_IGNORED(faceNormalsSynced),
	CR_IGNORED(faceNormalsUnsynced),
	CR_IGNORED(centerNormalsSynced),
	CR_IGNORED(centerNormalsUnsynced),
	CR_IGNORED(centerNormals2D),
	CR_IGNORED(slopeMap),
	CR_IGNORED(typeMap),
	*/

	CR_IGNORED(sharedCornerHeightMaps),
	CR_IGNORED(sharedCenterHeightMaps),
	CR_IGNORED(sharedFaceNormals),
	CR_IGNORED(sharedCenterNormals),
	CR_IGNORED(sharedSlopeMaps),

	CR_IGNORED(unsyncedHeightMapUpdates),
	CR_IGNORED(unsyncedHeightMapUpdatesTemp),

	/*
	#ifdef USE_UNSYNCED_HEIGHTMAP
	CR_IGNORED(  syncedHeightMapDigests),
	CR_IGNORED(unsyncedHeightMapDigests),
	#endif
	*/

	CR_POSTLOAD(PostLoad),
	CR_SERIALIZER(Serialize)
))



// initialized in CGame::LoadMap
CReadMap* readMap = nullptr;

MapDimensions mapDims;

std::vector<float> CReadMap::originalHeightMap;
std::vector<float> CReadMap::centerHeightMap;
std::array<std::vector<float>, CReadMap::numHeightMipMaps - 1> CReadMap::mipCenterHeightMaps;

std::vector<float3> CReadMap::visVertexNormals;
std::vector<float3> CReadMap::faceNormalsSynced;
std::vector<float3> CReadMap::faceNormalsUnsynced;
std::vector<float3> CReadMap::centerNormalsSynced;
std::vector<float3> CReadMap::centerNormalsUnsynced;

std::vector<float> CReadMap::slopeMap;
std::vector<uint8_t> CReadMap::typeMap;
std::vector<float3> CReadMap::centerNormals2D;

#ifdef USE_UNSYNCED_HEIGHTMAP
std::vector<uint8_t> CReadMap::  syncedHeightMapDigests;
std::vector<uint8_t> CReadMap::unsyncedHeightMapDigests;
#endif



MapTexture::~MapTexture() {
	// do NOT delete a Lua-set texture here!
	glDeleteTextures(1, &texIDs[RAW_TEX_IDX]);

	texIDs[RAW_TEX_IDX] = 0;
	texIDs[LUA_TEX_IDX] = 0;
}



CReadMap* CReadMap::LoadMap(const std::string& mapName)
{
	CReadMap* rm = nullptr;

	if (FileSystem::GetExtension(mapName) == "sm3") {
		throw content_error("[CReadMap::LoadMap] SM3 maps are no longer supported as of Spring 95.0");
	} else {
		// assume SMF format by default; calls ::Initialize
		rm = new CSMFReadMap(mapName);
	}

	if (rm == nullptr)
		return nullptr;

	// read metal- and type-map
	MapBitmapInfo mbi;
	MapBitmapInfo tbi;

	unsigned char* metalmapPtr = rm->GetInfoMap("metal", &mbi);
	unsigned char* typemapPtr = rm->GetInfoMap("type", &tbi);

	assert(mbi.width == mapDims.hmapx);
	assert(mbi.height == mapDims.hmapy);
	metalMap.Init(metalmapPtr, mbi.width, mbi.height, mapInfo->map.maxMetal);

	if (metalmapPtr != nullptr)
		rm->FreeInfoMap("metal", metalmapPtr);

	if (typemapPtr != nullptr && tbi.width == mapDims.hmapx && tbi.height == mapDims.hmapy) {
		assert(!typeMap.empty());
		memcpy(typeMap.data(), typemapPtr, typeMap.size());
	} else {
		LOG_L(L_WARNING, "[CReadMap::%s] missing or illegal typemap for \"%s\" (dims=<%d,%d>)", __func__, mapName.c_str(), tbi.width, tbi.height);
	}

	if (typemapPtr != nullptr)
		rm->FreeInfoMap("type", typemapPtr);

	return rm;
}

#ifdef USING_CREG
void CReadMap::Serialize(creg::ISerializer* s)
{
	// using integers so we can xor the original heightmap with the
	// current one (affected by Lua, explosions, etc) - long runs of
	// zeros for unchanged squares should compress significantly better.
	      int32_t*  ichms = reinterpret_cast<      int32_t*>(const_cast<float*>(GetCornerHeightMapSynced()));
	const int32_t* iochms = reinterpret_cast<const int32_t*>(GetOriginalHeightMapSynced());

	// LuaSynced can also touch the typemap, serialize it (manually)
	MapBitmapInfo tbi;

	      uint8_t*  itm = typeMap.data();
	const uint8_t* iotm = GetInfoMap("type", &tbi);

	assert(!typeMap.empty());
	assert(typeMap.size() == (tbi.width * tbi.height));

	int32_t height;
	uint8_t type;

	if (s->IsWriting()) {
		for (unsigned int i = 0; i < (mapDims.mapxp1 * mapDims.mapyp1); i++) {
			height = ichms[i] ^ iochms[i];
			s->Serialize(&height, sizeof(int32_t));
		}

		for (unsigned int i = 0; i < (mapDims.hmapx * mapDims.hmapy); i++) {
			type = itm[i] ^ iotm[i];
			s->Serialize(&type, sizeof(uint8_t));
		}
	} else {
		for (unsigned int i = 0; i < (mapDims.mapxp1 * mapDims.mapyp1); i++) {
			s->Serialize(&height, sizeof(int32_t));
			ichms[i] = height ^ iochms[i];
		}

		for (unsigned int i = 0; i < (mapDims.hmapx * mapDims.hmapy); i++) {
			s->Serialize(&type, sizeof(uint8_t));
			itm[i] = type ^ iotm[i];
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

	sharedSlopeMaps[0] = &slopeMap[0]; // NO UNSYNCED VARIANT
	sharedSlopeMaps[1] = &slopeMap[0];

	//FIXME reconstruct
	/*
	mipPointerHeightMaps.fill(nullptr);
	mipPointerHeightMaps[0] = &centerHeightMap[0];
	for (int i = 1; i < numHeightMipMaps; i++) {
		mipPointerHeightMaps[i] = &mipCenterHeightMaps[i - 1][0];
	}
	*/
}
#endif //USING_CREG


CReadMap::~CReadMap()
{
	metalMap.Kill();
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
			((  mapDims.hmapx     * mapDims.hmapy           * sizeof(uint8_t))       / 1024) +   // typeMap
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

	originalHeightMap.clear();
	originalHeightMap.resize(mapDims.mapxp1 * mapDims.mapyp1);
	faceNormalsSynced.clear();
	faceNormalsSynced.resize(mapDims.mapx * mapDims.mapy * 2);
	faceNormalsUnsynced.clear();
	faceNormalsUnsynced.resize(mapDims.mapx * mapDims.mapy * 2);
	centerNormalsSynced.clear();
	centerNormalsSynced.resize(mapDims.mapx * mapDims.mapy);
	centerNormalsUnsynced.clear();
	centerNormalsUnsynced.resize(mapDims.mapx * mapDims.mapy);
	centerNormals2D.clear();
	centerNormals2D.resize(mapDims.mapx * mapDims.mapy);
	centerHeightMap.clear();
	centerHeightMap.resize(mapDims.mapx * mapDims.mapy);

	mipPointerHeightMaps.fill(nullptr);
	mipPointerHeightMaps[0] = &centerHeightMap[0];

	for (int i = 1; i < numHeightMipMaps; i++) {
		mipCenterHeightMaps[i - 1].clear();
		mipCenterHeightMaps[i - 1].resize((mapDims.mapx >> i) * (mapDims.mapy >> i));

		mipPointerHeightMaps[i] = &mipCenterHeightMaps[i - 1][0];
	}

	slopeMap.clear();
	slopeMap.resize(mapDims.hmapx * mapDims.hmapy);

	// by default, all squares are set to terrain-type 0
	typeMap.clear();
	typeMap.resize(mapDims.hmapx * mapDims.hmapy, 0);

	visVertexNormals.clear();
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

		sharedSlopeMaps[0] = &slopeMap[0]; // NO UNSYNCED VARIANT
		sharedSlopeMaps[1] = &slopeMap[0];
	}

	mapChecksum = CalcHeightmapChecksum();

	syncedHeightMapDigests.clear();
	unsyncedHeightMapDigests.clear();

	// not callable here because losHandler is still uninitialized, deferred to Game::PostLoadSim
	// InitHeightMapDigestVectors();
	UpdateHeightMapSynced(SRectangle(0, 0, mapDims.mapx, mapDims.mapy), true);

	// FIXME: sky & skyLight aren't created yet (crashes in SMFReadMap.cpp)
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
	unsigned int checksum = HsiehHash(&typeMap[0], typeMap.size() * sizeof(typeMap[0]), 0);

	for (const CMapInfo::TerrainType& tt : mapInfo->terrainTypes) {
		checksum = HsiehHash(tt.name.c_str(), tt.name.size(), checksum);
		checksum = HsiehHash(&tt.hardness, offsetof(CMapInfo::TerrainType, receiveTracks) - offsetof(CMapInfo::TerrainType, hardness), checksum);
	}

	return checksum;
}


void CReadMap::UpdateDraw(bool firstCall)
{
	SCOPED_TIMER("Update::ReadMap::UHM");

	if (unsyncedHeightMapUpdates.empty())
		return;

	#if 0
	static CRectangleOverlapHandler unsyncedHeightMapUpdatesSwap;

	{
		if (!unsyncedHeightMapUpdates.empty())
			unsyncedHeightMapUpdates.swap(unsyncedHeightMapUpdatesTemp); // swap to avoid Optimize() inside a mutex
	}
	{
		if (!firstCall) {
			if (!unsyncedHeightMapUpdatesTemp.empty()) {
				unsyncedHeightMapUpdatesTemp.Process();

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
		unsyncedHeightMapUpdates.append(unsyncedHeightMapUpdatesTemp);

	// unsyncedHeightMapUpdatesTemp is now guaranteed empty
	for (const SRectangle& rect: unsyncedHeightMapUpdatesSwap) {
		UpdateHeightMapUnsynced(rect);
	}
	for (const SRectangle& rect: unsyncedHeightMapUpdatesSwap) {
		eventHandler.UnsyncedHeightMapUpdate(rect);
	}

	unsyncedHeightMapUpdatesSwap.clear();

	#else

	// TODO: quadtree or whatever
	for (size_t i = 0, n = std::min(MAX_UHM_RECTS_PER_FRAME, unsyncedHeightMapUpdates.size()); i < n; i++) {
		UpdateHeightMapUnsynced(*(unsyncedHeightMapUpdates.begin() + i));
	}

	for (size_t i = 0, n = std::min(MAX_UHM_RECTS_PER_FRAME, unsyncedHeightMapUpdates.size()); i < n; i++) {
		eventHandler.UnsyncedHeightMapUpdate(*(unsyncedHeightMapUpdates.begin() + i));
	}

	for (size_t i = 0, n = std::min(MAX_UHM_RECTS_PER_FRAME, unsyncedHeightMapUpdates.size()); i < n; i++) {
		unsyncedHeightMapUpdates.pop_front();
	}
	#endif
}


void CReadMap::UpdateHeightMapSynced(SRectangle hmRect, bool initialize)
{
	// do not bother with zero-area updates
	if (hmRect.GetArea() <= 0)
		return;

	hmRect.x1 = std::max(             0, hmRect.x1 - 1);
	hmRect.z1 = std::max(             0, hmRect.z1 - 1);
	hmRect.x2 = std::min(mapDims.mapxm1, hmRect.x2 + 1);
	hmRect.z2 = std::min(mapDims.mapym1, hmRect.z2 + 1);

	UpdateCenterHeightmap(hmRect, initialize);
	UpdateMipHeightmaps(hmRect, initialize);
	UpdateFaceNormals(hmRect, initialize);
	UpdateSlopemap(hmRect, initialize); // must happen after UpdateFaceNormals()!

	#ifdef USE_UNSYNCED_HEIGHTMAP
	// push the unsynced update; initial one without LOS check
	if (initialize) {
		unsyncedHeightMapUpdates.push_back(hmRect);
	} else {
		#ifdef USE_HEIGHTMAP_DIGESTS
		// convert heightmap rectangle to LOS-map space
		const int2 losMapSize = losHandler->los.size;
		const SRectangle lmRect = hmRect * (SQUARE_SIZE * losHandler->los.invDiv);

		// heightmap updated, increment digests (byte-overflow is intentional!)
		for (int lmz = lmRect.z1; lmz <= lmRect.z2; ++lmz) {
			for (int lmx = lmRect.x1; lmx <= lmRect.x2; ++lmx) {
				const int losMapIdx = lmx + lmz * (losMapSize.x + 1);

				assert(losMapIdx < syncedHeightMapDigests.size());

				syncedHeightMapDigests[losMapIdx]++;
			}
		}
		#endif

		HeightMapUpdateLOSCheck(hmRect);
	}
	#else
	unsyncedHeightMapUpdates.push_back(hmRect);
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

	const int z1 = std::max(             0, rect.z1 - 1);
	const int x1 = std::max(             0, rect.x1 - 1);
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
			centerNormals2D[y * mapDims.mapx + x] = (fnTL + fnBR).Normalize2D();

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
	const int sx = std::max(0,                 (rect.x1 / 2) - 1);
	const int ex = std::min(mapDims.hmapx - 1, (rect.x2 / 2) + 1);
	const int sy = std::max(0,                 (rect.z1 / 2) - 1);
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


/// split the update into multiple invididual (los-square) chunks
void CReadMap::HeightMapUpdateLOSCheck(const SRectangle& hmRect)
{
	// size of LOS square in heightmap coords; divisor is SQUARE_SIZE * 2^mipLevel
	const int losSqSize = losHandler->los.mipDiv / SQUARE_SIZE;

	const SRectangle& lmRect = hmRect * (SQUARE_SIZE * losHandler->los.invDiv); // LOS space
	const auto PushRect = [&](SRectangle& subRect, int hmx, int hmz) {
		if (subRect.GetArea() > 0) {
			subRect.ClampIn(hmRect);
			unsyncedHeightMapUpdates.push_back(subRect);
			subRect = SRectangle(hmx + losSqSize, hmz,  hmx + losSqSize, hmz + losSqSize);
		} else {
			subRect.x1 = hmx + losSqSize;
			subRect.x2 = hmx + losSqSize;
		}
	};

	for (int lmz = lmRect.z1; lmz <= lmRect.z2; ++lmz) {
		const int hmz = lmz * losSqSize;
		      int hmx = lmRect.x1 * losSqSize;

		SRectangle subRect(hmx, hmz,  hmx, hmz + losSqSize);

		for (int lmx = lmRect.x1; lmx <= lmRect.x2; ++lmx) {
			hmx = lmx * losSqSize;

			#ifdef USE_UNSYNCED_HEIGHTMAP
			if (!(gu->spectatingFullView || losHandler->InLos(SquareToFloat3(hmx, hmz), gu->myAllyTeam))) {
				PushRect(subRect, hmx, hmz);
				continue;
			}
			#endif

			if (!HasHeightMapChanged(lmx, lmz)) {
				PushRect(subRect, hmx, hmz);
				continue;
			}

			// update rectangle size
			subRect.x2 = hmx + losSqSize;
		}

		PushRect(subRect, hmx, hmz);
	}
}


void CReadMap::InitHeightMapDigestVectors(const int2 losMapSize)
{
#if (defined(USE_HEIGHTMAP_DIGESTS) && defined(USE_UNSYNCED_HEIGHTMAP))
	assert(losHandler != nullptr);
	assert(syncedHeightMapDigests.empty());

	const int xsize = losMapSize.x + 1;
	const int ysize = losMapSize.y + 1;

	syncedHeightMapDigests.clear();
	syncedHeightMapDigests.resize(xsize * ysize, 0);
	unsyncedHeightMapDigests.clear();
	unsyncedHeightMapDigests.resize(xsize * ysize, 0);
#endif
}


bool CReadMap::HasHeightMapChanged(const int lmx, const int lmy)
{
#if (defined(USE_HEIGHTMAP_DIGESTS) && defined(USE_UNSYNCED_HEIGHTMAP))
	const int2 losMapSize = losHandler->los.size;
	const int losMapIdx = lmx + lmy * (losMapSize.x + 1);

	assert(losMapIdx < syncedHeightMapDigests.size() && losMapIdx >= 0);

	if (unsyncedHeightMapDigests[losMapIdx] != syncedHeightMapDigests[losMapIdx]) {
		unsyncedHeightMapDigests[losMapIdx] = syncedHeightMapDigests[losMapIdx];
		return true;
	}

	return false;
#else
	return true;
#endif
}


#ifdef USE_UNSYNCED_HEIGHTMAP
void CReadMap::UpdateLOS(const SRectangle& hmRect)
{
	if (gu->spectatingFullView)
		return;

	// currently we use the LOS for view updates (alternatives are AirLOS and/or radar)
	// the other maps use different resolutions, must check size here for safety
	// (if another source is used, change the res. of syncedHeightMapDigests etc)
	assert(hmRect.GetWidth() <= (losHandler->los.mipDiv / SQUARE_SIZE));
	assert(losHandler != nullptr);

	SRectangle hmPoint = hmRect;
	//HACK: UpdateLOS() is called for single LOS squares, but we use <= in HeightMapUpdateLOSCheck().
	// This would make our update area 4x as large, so we need to make the rectangle a point. Better
	// would be to use < instead of <= everywhere.
	//FIXME: this actually causes spikes in the UHM
	// hmPoint.x2 = hmPoint.x1;
	// hmPoint.z2 = hmPoint.z1;

	HeightMapUpdateLOSCheck(hmPoint);
}

void CReadMap::BecomeSpectator()
{
	HeightMapUpdateLOSCheck(SRectangle(0, 0, mapDims.mapx, mapDims.mapy));
}
#else
void CReadMap::UpdateLOS(const SRectangle& hmRect) {}
void CReadMap::BecomeSpectator() {}
#endif


bool CReadMap::HasVisibleWater() const { return (!mapRendering->voidWater && !IsAboveWater()); }
bool CReadMap::HasOnlyVoidWater() const { return (mapRendering->voidWater && IsUnderWater()); }
