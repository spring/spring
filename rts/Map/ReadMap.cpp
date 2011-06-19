/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"

#include <stdlib.h>
#include <string>
#include "mmgr.h"

#include "ReadMap.h"
#include "MapDamage.h"
#include "MapInfo.h"
#include "MetalMap.h"
#include "SM3/Sm3Map.h"
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

	if (metalmapPtr && mbi.width == (rm->width >> 1) && mbi.height == (rm->height >> 1)) {
		int size = mbi.width * mbi.height;
		unsigned char* mtlMap = new unsigned char[size];
		memcpy(mtlMap, metalmapPtr, size);
		rm->metalMap = new CMetalMap(mtlMap, mbi.width, mbi.height, mapInfo->map.maxMetal);
	}
	if (metalmapPtr)
		rm->FreeInfoMap("metal", metalmapPtr);

	if (!rm->metalMap) {
		unsigned char* mtlMap = new unsigned char[rm->width * rm->height / 4];
		memset(mtlMap, 0, rm->width * rm->height / 4);
		rm->metalMap = new CMetalMap(mtlMap, rm->width / 2,rm->height / 2, 1.0f);
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

	s.Serialize(shm, 4 * (gs->mapx + 1) * (gs->mapy + 1));

	if (!s.IsWriting())
		mapDamage->RecalcArea(2, gs->mapx - 3, 2, gs->mapy - 3);
}


CReadMap::CReadMap():
	metalMap(NULL),
	width(0),
	height(0),
	minheight(0.0f),
	maxheight(0.0f),
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

	for (int i = 1; i < numHeightMipMaps; i++) {
		// don't delete mipHeightMaps[0] since it points to centerHeightMap
		delete[] mipHeightMaps[i];
	}

	mipHeightMaps.clear();
	centerHeightMap.clear();
	originalHeightMap.clear();
	vertexNormals.clear();
	faceNormals.clear();
	centerNormals.clear();
}


void CReadMap::Initialize()
{
	loadscreen->SetLoadMessage("Loading Map");

	// set global map info
	gs->mapx = width;
	gs->mapy = height;
	gs->mapSquares = gs->mapx * gs->mapy;
	gs->hmapx = gs->mapx >> 1;
	gs->hmapy = gs->mapy >> 1;
	gs->pwr2mapx = next_power_of_2(gs->mapx);
	gs->pwr2mapy = next_power_of_2(gs->mapy);

	float3::maxxpos = gs->mapx * SQUARE_SIZE - 1;
	float3::maxzpos = gs->mapy * SQUARE_SIZE - 1;

	originalHeightMap.resize((gs->mapx + 1) * (gs->mapy + 1));
	faceNormals.resize(gs->mapx * gs->mapy * 2);
	centerNormals.resize(gs->mapx * gs->mapy);
	centerHeightMap.resize(gs->mapx * gs->mapy);

	mipHeightMaps.resize(numHeightMipMaps);
	mipHeightMaps[0] = &centerHeightMap[0];

	for (int i = 1; i < numHeightMipMaps; i++) {
		mipHeightMaps[i] = new float[(gs->mapx >> i) * (gs->mapy >> i)];
	}

	slopeMap.resize(gs->hmapx * gs->hmapy);
	vertexNormals.resize((gs->mapx + 1) * (gs->mapy + 1));

	CalcHeightmapChecksum();
	HeightmapUpdated(0, 0, gs->mapx, gs->mapy);
}


void CReadMap::CalcHeightmapChecksum()
{
	const float* heightmap = GetCornerHeightMapSynced();

	minheight = +123456.0f;
	maxheight = -123456.0f;

	mapChecksum = 0;
	for (int i = 0; i < ((gs->mapx + 1) * (gs->mapy + 1)); ++i) {
		originalHeightMap[i] = heightmap[i];
		if (heightmap[i] < minheight) { minheight = heightmap[i]; }
		if (heightmap[i] > maxheight) { maxheight = heightmap[i]; }
		mapChecksum +=  (unsigned int) (heightmap[i] * 100);
		mapChecksum ^= *(unsigned int*) &heightmap[i];
	}

	currMinHeight = minheight;
	currMaxHeight = maxheight;
}


void CReadMap::UpdateHeightmapSynced(int x1, int y1, int x2, int y2)
{
	const float* heightmapSynced = GetCornerHeightMapSynced();
	      float* heightmapUnsynced = GetCornerHeightMapUnsynced();

	x1 = std::max(           0, x1 - 1);
	y1 = std::max(           0, y1 - 1);
	x2 = std::min(gs->mapx - 1, x2 + 1);
	y2 = std::min(gs->mapy - 1, y2 + 1);

	for (int y = y1; y <= y2; y++) {
		for (int x = x1; x <= x2; x++) {
			const int idxTL = (y    ) * (gs->mapx + 1) + x;
			const int idxTR = (y    ) * (gs->mapx + 1) + x + 1;
			const int idxBL = (y + 1) * (gs->mapx + 1) + x;
			const int idxBR = (y + 1) * (gs->mapx + 1) + x + 1;

			const float height =
				heightmapSynced[idxTL] +
				heightmapSynced[idxTR] +
				heightmapSynced[idxBL] +
				heightmapSynced[idxBR];
			centerHeightMap[y * gs->mapx + x] = height * 0.25f;
		}
	}

	for (int i = 0; i < numHeightMipMaps - 1; i++) {
		int hmapx = gs->mapx >> i;
		for (int y = ((y1 >> i) & (~1)); y < (y2 >> i); y += 2) {
			for (int x = ((x1 >> i) & (~1)); x < (x2 >> i); x += 2) {
				const float height =
					mipHeightMaps[i][(x    ) + (y    ) * hmapx] +
					mipHeightMaps[i][(x    ) + (y + 1) * hmapx] +
					mipHeightMaps[i][(x + 1) + (y    ) * hmapx] +
					mipHeightMaps[i][(x + 1) + (y + 1) * hmapx];
				mipHeightMaps[i + 1][(x / 2) + (y / 2) * hmapx / 2] = height * 0.25f;
			}
		}
	}

	const int decy = std::max(           0, y1 - 1);
	const int incy = std::min(gs->mapy - 1, y2 + 1);
	const int decx = std::max(           0, x1 - 1);
	const int incx = std::min(gs->mapx - 1, x2 + 1);

	//! create the surface normals
	for (int y = decy; y <= incy; y++) {
		for (int x = decx; x <= incx; x++) {
			const int idx0 = (y    ) * (gs->mapx + 1) + x;
			const int idx1 = (y + 1) * (gs->mapx + 1) + x;

			float3 e1(-SQUARE_SIZE, heightmapSynced[idx0] - heightmapSynced[idx0 + 1],            0);
			float3 e2(           0, heightmapSynced[idx0] - heightmapSynced[idx1    ], -SQUARE_SIZE);

			const float3 n1 = e2.cross(e1).Normalize();

			//! triangle topright
			faceNormals[(y * gs->mapx + x) * 2] = n1;

			e1 = float3( SQUARE_SIZE, heightmapSynced[idx1 + 1] - heightmapSynced[idx1    ],           0);
			e2 = float3(           0, heightmapSynced[idx1 + 1] - heightmapSynced[idx0 + 1], SQUARE_SIZE);

			const float3 n2 = e2.cross(e1).Normalize();

			//! triangle bottomleft
			faceNormals[(y * gs->mapx + x) * 2 + 1] = n2;

			//! face normal
			centerNormals[y * gs->mapx + x] = (n1 + n2).Normalize();
		}
	}

	for (int y = std::max(0, (y1/2)-1); y <= std::min(gs->hmapy - 1, (y2/2)+1); y++) {
		for (int x = std::max(0, (x1/2)-1); x <= std::min(gs->hmapx - 1, (x2/2)+1); x++) {
			const int idx0 = (y*2    ) * (gs->mapx) + x*2;
			const int idx1 = (y*2 + 1) * (gs->mapx) + x*2;

			float avgslope = 0.0f;
			avgslope += faceNormals[(idx0    ) * 2    ].y;
			avgslope += faceNormals[(idx0    ) * 2 + 1].y;
			avgslope += faceNormals[(idx0 + 1) * 2    ].y;
			avgslope += faceNormals[(idx0 + 1) * 2 + 1].y;
			avgslope += faceNormals[(idx1    ) * 2    ].y;
			avgslope += faceNormals[(idx1    ) * 2 + 1].y;
			avgslope += faceNormals[(idx1 + 1) * 2    ].y;
			avgslope += faceNormals[(idx1 + 1) * 2 + 1].y;
			avgslope /= 8.0f;

			float maxslope =              faceNormals[(idx0    ) * 2    ].y;
			maxslope = std::min(maxslope, faceNormals[(idx0    ) * 2 + 1].y);
			maxslope = std::min(maxslope, faceNormals[(idx0 + 1) * 2    ].y);
			maxslope = std::min(maxslope, faceNormals[(idx0 + 1) * 2 + 1].y);
			maxslope = std::min(maxslope, faceNormals[(idx1    ) * 2    ].y);
			maxslope = std::min(maxslope, faceNormals[(idx1    ) * 2 + 1].y);
			maxslope = std::min(maxslope, faceNormals[(idx1 + 1) * 2    ].y);
			maxslope = std::min(maxslope, faceNormals[(idx1 + 1) * 2 + 1].y);

			//! smooth it a bit, so small holes don't block huge tanks
			const float lerp = maxslope / avgslope;
			const float slope = maxslope * (1.0f - lerp) + avgslope * lerp;

			slopeMap[y * gs->hmapx + x] = 1.0f - slope;
		}
	}
}

void CReadMap::UpdateDraw() {
	GML_STDMUTEX_LOCK(map); // UpdateDraw

	for (std::vector<HeightmapUpdate>::iterator i = heightmapUpdates.begin(); i != heightmapUpdates.end(); ++i)
		UpdateHeightmapUnsynced(i->x1, i->y1, i->x2, i->y2);

	heightmapUpdates.clear();
}

void CReadMap::HeightmapUpdated(const int& x1, const int& y1, const int& x2, const int& y2) {
	GML_STDMUTEX_LOCK(map); // HeightmapUpdated

	// only update the heightmap if the affected area has a size > 0
	if ((x1 < x2) && (y1 < y2)) {
		//! synced
		UpdateHeightmapSynced(x1, y1, x2, y2);

		//! unsynced
		heightmapUpdates.push_back(HeightmapUpdate(x1, x2, y1, y2));
	}
}
