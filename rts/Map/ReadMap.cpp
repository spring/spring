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
#include "SMF/SmfReadMap.h"
#include "System/bitops.h"
#include "System/ConfigHandler.h"
#include "System/Exceptions.h"
#include "System/LogOutput.h"
#include "System/LoadSave/LoadSaveInterface.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/ArchiveScanner.h"

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

	string extension = mapname.substr(mapname.length() - 3);

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
		rm->typemap = new unsigned char[tbi.width * tbi.height];
		memcpy(rm->typemap, typemapPtr, tbi.width * tbi.height);
	} else
		throw content_error("Bad/no terrain type map.");

	if (typemapPtr)
		rm->FreeInfoMap("type", typemapPtr);

	return rm;
}


void CReadMap::Serialize(creg::ISerializer& s)
{
	// remove the const
	float* hm = (float*) GetHeightmap();
	s.Serialize(hm, 4 * (gs->mapx + 1) * (gs->mapy + 1));

	if (!s.IsWriting())
		mapDamage->RecalcArea(2, gs->mapx - 3, 2, gs->mapy - 3);
}


CReadMap::CReadMap():
	orgheightmap(NULL),
	centerheightmap(NULL),
	slopemap(NULL),
	facenormals(NULL),
	centernormals(NULL),
	typemap(NULL),
	metalMap(NULL)
{
	memset(mipHeightmap, 0, sizeof(mipHeightmap));
}


CReadMap::~CReadMap()
{
	delete metalMap;
	delete[] typemap;
	delete[] slopemap;

	delete[] facenormals;
	delete[] centernormals;

	delete[] centerheightmap;
	for (int i = 1; i < numHeightMipMaps; i++) {
		// don't delete mipHeightmap[0] since it points to centerheightmap
		delete[] mipHeightmap[i];
	}

	delete[] orgheightmap;

	vertexNormals.clear();
}


void CReadMap::Initialize()
{
	PrintLoadMsg("Loading Map");

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

	orgheightmap = new float[(gs->mapx + 1) * (gs->mapy + 1)];
	facenormals = new float3[gs->mapx * gs->mapy * 2];
	centernormals = new float3[gs->mapx * gs->mapy];
	centerheightmap = new float[gs->mapx * gs->mapy];
	mipHeightmap[0] = centerheightmap;
	for (int i = 1; i < numHeightMipMaps; i++)
		mipHeightmap[i] = new float[(gs->mapx >> i) * (gs->mapy >> i)];

	slopemap = new float[gs->hmapx * gs->hmapy];
	vertexNormals.resize((gs->mapx + 1) * (gs->mapy + 1));

	CalcHeightmapChecksum();
	HeightmapUpdated(0, 0, gs->mapx, gs->mapy);
}


void CReadMap::CalcHeightmapChecksum()
{
	const float* heightmap = GetHeightmap();

	minheight = +123456.0f;
	maxheight = -123456.0f;

	mapChecksum = 0;
	for (int i = 0; i < ((gs->mapx + 1) * (gs->mapy + 1)); ++i) {
		orgheightmap[i] = heightmap[i];
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
	const float* heightmap = GetHeightmap();

	x1 = std::max(           0, x1 - 1);
	y1 = std::max(           0, y1 - 1);
	x2 = std::min(gs->mapx - 1, x2 + 1);
	y2 = std::min(gs->mapy - 1, y2 + 1);

	for (int y = y1; y <= y2; y++) {
		for (int x = x1; x <= x2; x++) {
			float height = heightmap[(y) * (gs->mapx + 1) + x];
			height += heightmap[(y    ) * (gs->mapx + 1) + x + 1];
			height += heightmap[(y + 1) * (gs->mapx + 1) + x    ];
			height += heightmap[(y + 1) * (gs->mapx + 1) + x + 1];
			centerheightmap[y * gs->mapx + x] = height * 0.25f;
		}
	}

	for (int i = 0; i < numHeightMipMaps - 1; i++) {
		int hmapx = gs->mapx >> i;
		for (int y = ((y1 >> i) & (~1)); y < (y2 >> i); y += 2) {
			for (int x = ((x1 >> i) & (~1)); x < (x2 >> i); x += 2) {
				float height = mipHeightmap[i][(x) + (y) * hmapx];
				height += mipHeightmap[i][(x    ) + (y + 1) * hmapx];
				height += mipHeightmap[i][(x + 1) + (y    ) * hmapx];
				height += mipHeightmap[i][(x + 1) + (y + 1) * hmapx];
				mipHeightmap[i + 1][(x / 2) + (y / 2) * hmapx / 2] = height * 0.25f;
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

			float3 e1(-SQUARE_SIZE, heightmap[idx0] - heightmap[idx0 + 1],            0);
			float3 e2(           0, heightmap[idx0] - heightmap[idx1    ], -SQUARE_SIZE);

			const float3 n1 = e2.cross(e1).Normalize();

			//! triangle topright
			facenormals[(y * gs->mapx + x) * 2] = n1;

			e1 = float3( SQUARE_SIZE, heightmap[idx1 + 1] - heightmap[idx1    ],           0);
			e2 = float3(           0, heightmap[idx1 + 1] - heightmap[idx0 + 1], SQUARE_SIZE);

			const float3 n2 = e2.cross(e1).Normalize();

			//! triangle bottomleft
			facenormals[(y * gs->mapx + x) * 2 + 1] = n2;

			//! face normal
			centernormals[y * gs->mapx + x] = (n1 + n2).Normalize();
		}
	}

	for (int y = std::max(0, (y1/2)-1); y <= std::min(gs->hmapy - 1, (y2/2)+1); y++) {
		for (int x = std::max(0, (x1/2)-1); x <= std::min(gs->hmapx - 1, (x2/2)+1); x++) {
			const int idx0 = (y*2    ) * (gs->mapx) + x*2;
			const int idx1 = (y*2 + 1) * (gs->mapx) + x*2;

			float avgslope = 0;
			avgslope += facenormals[(idx0    ) * 2    ].y;
			avgslope += facenormals[(idx0    ) * 2 + 1].y;
			avgslope += facenormals[(idx0 + 1) * 2    ].y;
			avgslope += facenormals[(idx0 + 1) * 2 + 1].y;
			avgslope += facenormals[(idx1    ) * 2    ].y;
			avgslope += facenormals[(idx1    ) * 2 + 1].y;
			avgslope += facenormals[(idx1 + 1) * 2    ].y;
			avgslope += facenormals[(idx1 + 1) * 2 + 1].y;
			avgslope /= 8;

			float maxslope =              facenormals[(idx0    ) * 2    ].y;
			maxslope = std::min(maxslope, facenormals[(idx0    ) * 2 + 1].y);
			maxslope = std::min(maxslope, facenormals[(idx0 + 1) * 2    ].y);
			maxslope = std::min(maxslope, facenormals[(idx0 + 1) * 2 + 1].y);
			maxslope = std::min(maxslope, facenormals[(idx1    ) * 2    ].y);
			maxslope = std::min(maxslope, facenormals[(idx1    ) * 2 + 1].y);
			maxslope = std::min(maxslope, facenormals[(idx1 + 1) * 2    ].y);
			maxslope = std::min(maxslope, facenormals[(idx1 + 1) * 2 + 1].y);

			//! smooth it a bit, so small holes don't block huge tanks
			const float lerp = maxslope / avgslope;
			const float slope = maxslope * (1.0f - lerp) + avgslope * lerp;

			slopemap[y * gs->hmapx + x] = 1.0f - slope;
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

CReadMap::IQuadDrawer::~IQuadDrawer() {
}
