#include "StdAfx.h"
// ReadMap.cpp: implementation of the CReadMap class.
//
//////////////////////////////////////////////////////////////////////

#include "Rendering/GL/myGL.h"
#include "ReadMap.h"
#include <stdlib.h>
#include <string>
#include "Rendering/Textures/Bitmap.h"
#include "Ground.h"
#include "Platform/ConfigHandler.h"
#include "MapDamage.h"
#include "MapInfo.h"
#include "MetalMap.h"
#include "Sim/Path/PathManager.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/Unit.h"
#include "SM3/Sm3Map.h"
#include "SMF/SmfReadMap.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/LoadSaveInterface.h"
#include "System/LogOutput.h"
#include "System/Platform/errorhandler.h"
#include "mmgr.h"

using namespace std;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CReadMap* readmap=0;

CR_BIND_INTERFACE(CReadMap)
CR_REG_METADATA(CReadMap, (
				CR_SERIALIZER(Serialize)
				));


CReadMap* CReadMap::LoadMap (const std::string& mapname)
{
	if (mapname.length() < 3)
		throw std::runtime_error("CReadMap::LoadMap(): mapname '" + mapname + "' too short");

	string extension = mapname.substr(mapname.length()-3);

	CReadMap *rm = 0;
	if (extension=="sm3") {
		rm = SAFE_NEW CSm3ReadMap;
		((CSm3ReadMap*)rm)->Initialize (mapname.c_str());
	} else
		rm = SAFE_NEW CSmfReadMap(mapname);

	if (!rm)
		return 0;

	/* Read metal map */
	MapBitmapInfo mbi;
	unsigned char  *metalmap = rm->GetInfoMap ("metal", &mbi);
	if (metalmap && mbi.width == rm->width/2 && mbi.height == rm->height/2)
	{
		int size = mbi.width*mbi.height;
		unsigned char *map = SAFE_NEW unsigned char[size];
		memcpy(map, metalmap, size);
		rm->metalMap = SAFE_NEW CMetalMap(map, mbi.width, mbi.height, mapInfo->map.maxMetal);
	}
	if (metalmap) rm->FreeInfoMap ("metal", metalmap);

	if (!rm->metalMap) {
		unsigned char *zd = SAFE_NEW unsigned char[rm->width*rm->height/4];
		memset(zd,0,rm->width*rm->height/4);
		rm->metalMap = SAFE_NEW CMetalMap(zd, rm->width/2,rm->height/2, 1.0f);
	}

	/* Read type map */
	MapBitmapInfo tbi;
	unsigned char *typemap = rm->GetInfoMap ("type", &tbi);
	if (typemap && tbi.width == rm->width/2 && tbi.height == rm->height/2)
	{
		assert (gs->hmapx == tbi.width && gs->hmapy == tbi.height);
		rm->typemap = SAFE_NEW unsigned char[tbi.width*tbi.height];
		memcpy(rm->typemap, typemap, tbi.width*tbi.height);
	} else
		throw content_error("Bad/no terrain type map.");
	if (typemap) rm->FreeInfoMap ("type", typemap);

	return rm;
}

CReadMap::CReadMap() :
		orgheightmap(NULL),
		centerheightmap(NULL),
		slopemap(NULL),
		facenormals(NULL),
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
	delete[] centerheightmap;
	for(int i=1; i<numHeightMipMaps; i++)	//don't delete first pointer since it points to centerheightmap
		delete[] mipHeightmap[i];

	delete[] orgheightmap;
}

void CReadMap::Serialize(creg::ISerializer& s)
{
	float *hm = GetHeightmap();
	s.Serialize(hm, 4 * (gs->mapx+1) * (gs->mapy+1));

	if (!s.IsWriting())
		mapDamage->RecalcArea(2,gs->mapx-3,2,gs->mapy-3);
}

void CReadMap::Initialize()
{
	PrintLoadMsg("Loading Map");

	orgheightmap=SAFE_NEW float[(gs->mapx+1)*(gs->mapy+1)];

	//	normals=SAFE_NEW float3[(gs->mapx+1)*(gs->mapy+1)];
	facenormals=SAFE_NEW float3[gs->mapx*gs->mapy*2];
	centerheightmap=SAFE_NEW float[gs->mapx*gs->mapy];
	//halfHeightmap=SAFE_NEW float[gs->hmapx*gs->hmapy];
	mipHeightmap[0] = centerheightmap;
	for(int i=1; i<numHeightMipMaps; i++)
		mipHeightmap[i] = SAFE_NEW float[(gs->mapx>>i)*(gs->mapy>>i)];

	slopemap=SAFE_NEW float[gs->hmapx*gs->hmapy];

	CalcHeightfieldData();
}

void CReadMap::CalcHeightfieldData()
{
	float* heightmap = GetHeightmap();

	minheight=1000;
	maxheight=-1000;
	mapChecksum=0;
	for(int y=0;y<(gs->mapy+1)*(gs->mapx+1);++y){
		orgheightmap[y]=heightmap[y];
		if(heightmap[y]<minheight)
			minheight=heightmap[y];
		if(heightmap[y]>maxheight)
			maxheight=heightmap[y];
		mapChecksum+=(unsigned int)(heightmap[y]*100);
		mapChecksum^=*(unsigned int*)&heightmap[y];
	}

//	PrintLoadMsg("Creating surface normals");

	for(int y=0;y<(gs->mapy);y++){
		for(int x=0;x<(gs->mapx);x++){
			float height=heightmap[(y)*(gs->mapx+1)+x];
			height+=heightmap[(y)*(gs->mapx+1)+x+1];
			height+=heightmap[(y+1)*(gs->mapx+1)+x];
			height+=heightmap[(y+1)*(gs->mapx+1)+x+1];
			centerheightmap[y*gs->mapx+x]=height/4;
		}
	}

	for(int i=0; i<numHeightMipMaps-1; i++){
		int hmapx = gs->mapx>>i;
		int hmapy = gs->mapy>>i;
		for(int y=0;y<hmapy;y+=2){
			for(int x=0;x<hmapx;x+=2){
				float height = mipHeightmap[i][(x)+(y)*hmapx];
				height += mipHeightmap[i][(x)+(y+1)*hmapx];
				height += mipHeightmap[i][(x+1)+(y)*hmapx];
				height += mipHeightmap[i][(x+1)+(y+1)*hmapx];
				mipHeightmap[i+1][(x/2)+(y/2)*hmapx/2] = height/4.0f;
			}
		}
	}

	for(int y=0;y<gs->mapy;y++){
		for(int x=0;x<gs->mapx;x++){

			float3 e1(-SQUARE_SIZE,heightmap[y*(gs->mapx+1)+x]-heightmap[y*(gs->mapx+1)+x+1],0);
			float3 e2( 0,heightmap[y*(gs->mapx+1)+x]-heightmap[(y+1)*(gs->mapx+1)+x],-SQUARE_SIZE);

			float3 n=e2.cross(e1);
			n.Normalize();

			facenormals[(y*gs->mapx+x)*2]=n;

			e1=float3( SQUARE_SIZE,heightmap[(y+1)*(gs->mapx+1)+x+1]-heightmap[(y+1)*(gs->mapx+1)+x],0);
			e2=float3( 0,heightmap[(y+1)*(gs->mapx+1)+x+1]-heightmap[(y)*(gs->mapx+1)+x+1],SQUARE_SIZE);

			n=e2.cross(e1);
			n.Normalize();

			facenormals[(y*gs->mapx+x)*2+1]=n;
		}
	}

	for(int y=0;y<gs->hmapy;y++){
		for(int x=0;x<gs->hmapx;x++){
			slopemap[y*gs->hmapx+x]=1;
		}
	}

	for(int y=2;y<gs->mapy-2;y+=2){
		for(int x=2;x<gs->mapx-2;x+=2){
			float3 e1(-SQUARE_SIZE*4,heightmap[(y-1)*(gs->mapx+1)+(x-1)]-heightmap[(y-1)*(gs->mapx+1)+x+3],0);
			float3 e2( 0,heightmap[(y-1)*(gs->mapx+1)+(x-1)]-heightmap[(y+3)*(gs->mapx+1)+(x-1)],-SQUARE_SIZE*4);

			float3 n=e2.cross(e1);
			n.Normalize();

			e1=float3( SQUARE_SIZE*4,heightmap[(y+3)*(gs->mapx+1)+x+3]-heightmap[(y+3)*(gs->mapx+1)+x-1],0);
			e2=float3( 0,heightmap[(y+3)*(gs->mapx+1)+x+3]-heightmap[(y-1)*(gs->mapx+1)+x+3],SQUARE_SIZE*4);

			float3 n2=e2.cross(e1);
			n2.Normalize();

			slopemap[(y/2)*gs->hmapx+(x/2)]=1-(n.y+n2.y)*0.5f;
		}
	}
}


CReadMap::IQuadDrawer::~IQuadDrawer() {
}
