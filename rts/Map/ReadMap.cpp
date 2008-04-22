#include "StdAfx.h"
// ReadMap.cpp: implementation of the CReadMap class.
//
//////////////////////////////////////////////////////////////////////

#include "Rendering/GL/myGL.h"
#include "ReadMap.h"
#include <stdlib.h>
#include <string>
//#include <ostream>
#include "Rendering/Textures/Bitmap.h"
#include "Ground.h"
#include "Platform/ConfigHandler.h"
#ifdef _WIN32
#include <process.h>
#endif
#include "MapDamage.h"
#include "MetalMap.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
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
#include "System/TdfParser.h"
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

void CReadMap::OpenTDF (const std::string& mapname, TdfParser& parser)
{
	parser.LoadFile(GetTDFName(mapname));
}

std::string CReadMap::GetTDFName(const std::string& mapname)
{
	if (mapname.length() < 3)
		throw std::runtime_error("CReadMap::GetTDFName(): mapname '" + mapname + "' too short");

	string extension = mapname.substr(mapname.length()-3);
	if (extension == "smf")
		return string("maps/")+mapname.substr(0,mapname.find_last_of('.'))+".smd";
	else if(extension == "sm3")
		return string("maps/")+mapname;
	else
		throw std::runtime_error("CReadMap::GetTDFName(): Unknown extension: " + extension);
}

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

	rm->mapName = mapname;

	/* Read metal map */
	MapBitmapInfo mbi;
	unsigned char  *metalmap = rm->GetInfoMap ("metal", &mbi);
	if (metalmap && mbi.width == rm->width/2 && mbi.height == rm->height/2)
	{
		int size = mbi.width*mbi.height;
		unsigned char *map = SAFE_NEW unsigned char[size];
		memcpy(map, metalmap, size);
		rm->metalMap = SAFE_NEW CMetalMap(map, mbi.width, mbi.height, rm->maxMetal);
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

		rm->terrainTypes.resize(256);
		for(int a=0;a<256;++a){
			char tname[200];
			sprintf(tname,"TerrainType%i\\",a);
			string section="map\\";
			section+=tname;
			rm->mapDefParser.GetDef(rm->terrainTypes[a].name,"Default",section+"name");
			rm->mapDefParser.GetDef(rm->terrainTypes[a].hardness,"1",section+"hardness");
			rm->mapDefParser.GetDef(rm->terrainTypes[a].tankSpeed,"1",section+"tankmovespeed");
			rm->mapDefParser.GetDef(rm->terrainTypes[a].kbotSpeed,"1",section+"kbotmovespeed");
			rm->mapDefParser.GetDef(rm->terrainTypes[a].hoverSpeed,"1",section+"hovermovespeed");
			rm->mapDefParser.GetDef(rm->terrainTypes[a].shipSpeed,"1",section+"shipmovespeed");
			rm->mapDefParser.GetDef(rm->terrainTypes[a].receiveTracks,"1",section+"receivetracks");
		}
	} else
		throw content_error("Bad/no terrain type map.");
	if (typemap) rm->FreeInfoMap ("type", typemap);

	// FIXME: this isn't really the right place for this -> refactor sometime
	groundBlockingObjectMap = new CGroundBlockingObjectMap(gs->mapSquares);

	return rm;
}

CReadMap::CReadMap() :
		orgheightmap(NULL),
		centerheightmap(NULL),
		slopemap(NULL),
		facenormals(NULL),
		typemap(NULL),
		heightLinePal(NULL),
//		groundBlockingObjectMap(NULL),
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
//	delete[] groundBlockingObjectMap;
	delete[] heightLinePal;
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
	float* heightmap = GetHeightmap();

	orgheightmap=SAFE_NEW float[(gs->mapx+1)*(gs->mapy+1)];
	for(int y=0;y<(gs->mapy+1)*(gs->mapx+1);++y)
		orgheightmap[y]=heightmap[y];

	//	normals=SAFE_NEW float3[(gs->mapx+1)*(gs->mapy+1)];
	facenormals=SAFE_NEW float3[gs->mapx*gs->mapy*2];
	centerheightmap=SAFE_NEW float[gs->mapx*gs->mapy];
	//halfHeightmap=SAFE_NEW float[gs->hmapx*gs->hmapy];
	mipHeightmap[0] = centerheightmap;
	for(int i=1; i<numHeightMipMaps; i++)
		mipHeightmap[i] = SAFE_NEW float[(gs->mapx>>i)*(gs->mapy>>i)];

	slopemap=SAFE_NEW float[gs->hmapx*gs->hmapy];

	heightLinePal=SAFE_NEW unsigned char[3*256];
	if(configHandler.GetInt("ColorElev",1)){
		for(int a=0;a<86;++a){
			heightLinePal[a*3+0]=255-a*3;
			heightLinePal[a*3+1]=a*3;
			heightLinePal[a*3+2]=0;
		}
		for(int a=86;a<172;++a){
			heightLinePal[a*3+0]=0;
			heightLinePal[a*3+1]=255-(a-86)*3;
			heightLinePal[a*3+2]=(a-86)*3;
		}
		for(int a=172;a<256;++a){
			heightLinePal[a*3+0]=(a-172)*3;
			heightLinePal[a*3+1]=0;
			heightLinePal[a*3+2]=255-(a-172)*3;
		}
	} else {
		for(int a=0;a<29;++a){
			heightLinePal[a*3+0]=255-a*8;
			heightLinePal[a*3+1]=255-a*8;
			heightLinePal[a*3+2]=255-a*8;
		}
		for(int a=29;a<256;++a){
			heightLinePal[a*3+0]=a;
			heightLinePal[a*3+1]=a;
			heightLinePal[a*3+2]=a;
		}
	}

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


extern GLfloat FogLand[];

void CReadMap::ParseSettings(TdfParser& resources)
{
	mapHumanName = mapDefParser.SGetValueDef(mapName, "MAP\\Description");

	gs->sunVector=mapDefParser.GetFloat3(float3(0,1,2),"MAP\\LIGHT\\SunDir");
	gs->sunVector.Normalize();
	gs->sunVector4[0]=gs->sunVector[0];
	gs->sunVector4[1]=gs->sunVector[1];
	gs->sunVector4[2]=gs->sunVector[2];
	gs->sunVector4[3]=0;

	gs->gravity=-atof(mapDefParser.SGetValueDef("130","MAP\\Gravity").c_str())/(GAME_SPEED*GAME_SPEED);

	float3 fogColor=mapDefParser.GetFloat3(float3(0.7f,0.7f,0.8f),"MAP\\ATMOSPHERE\\FogColor");
	FogLand[0]=fogColor[0];
	FogLand[1]=fogColor[1];
	FogLand[2]=fogColor[2];

	mapDefParser.GetDef(skyBox, "", "MAP\\ATMOSPHERE\\SkyBox");
	std::string tmp;
	mapDefParser.GetDef(tmp, "", "MAP\\WATER\\WaterPlaneColor");
	if(tmp.empty())
		hasWaterPlane=0;
	else
	{
		hasWaterPlane = 1;
		waterPlaneColor = mapDefParser.GetFloat3(float3(0.0f,0.4f,0.0f),"MAP\\WATER\\WaterPlaneColor");
	}
	mapDefParser.GetDef(tidalStrength, "0", "MAP\\TidalStrength");

	waterSurfaceColor=mapDefParser.GetFloat3(float3(0.75f,0.8f,0.85f),"MAP\\WATER\\WaterSurfaceColor");
	waterAbsorb=mapDefParser.GetFloat3(float3(0,0,0),"MAP\\WATER\\WaterAbsorb");
	waterBaseColor=mapDefParser.GetFloat3(float3(0,0,0),"MAP\\WATER\\WaterBaseColor");
	waterMinColor=mapDefParser.GetFloat3(float3(0,0,0),"MAP\\WATER\\WaterMinColor");
	mapDefParser.GetDef(waterTexture, "", "MAP\\WATER\\WaterTexture");
	if(waterTexture.empty())	//default water is ocean.jpg in bitmaps, map specific water textures is saved in the map dir
		waterTexture = "bitmaps/"+resources.SGetValueDef("ocean.jpg","resources\\graphics\\maps\\watertex");
	else
		waterTexture = "maps/" + waterTexture;

	ambientColor=mapDefParser.GetFloat3(float3(0.5f,0.5f,0.5f),"MAP\\LIGHT\\GroundAmbientColor");
	sunColor=mapDefParser.GetFloat3(float3(0.5f,0.5f,0.5f),"MAP\\LIGHT\\GroundSunColor");
	specularColor=mapDefParser.GetFloat3(float3(0.1f,0.1f,0.1f),"MAP\\LIGHT\\GroundSpecularColor");
	mapDefParser.GetDef(shadowDensity, "0.8", "MAP\\LIGHT\\GroundShadowDensity");

	mapDefParser.GetDef(maxMetal,"0.02","MAP\\MaxMetal");
	mapDefParser.GetDef(extractorRadius,"500","MAP\\ExtractorRadius");

	mapDefParser.GetDef(voidWater, "0", "MAP\\voidWater");
}

//this function assumes that the correct map has been loaded and only load/saves new height values
void CReadMap::LoadSaveMap(CLoadSaveInterface* file,bool loading)
{
	float* heightmap=readmap->GetHeightmap();
	//load/save heightmap
	for(int y=0;y<gs->mapy+1;++y){
		for(int x=0;x<gs->mapx+1;++x){
			file->lsFloat(heightmap[y*(gs->mapx+1)+x]);
		}
	}
	//recalculate dependent values if loading
	if(loading){
		mapDamage->RecalcArea(0,gs->mapx,0,gs->mapy);
	}
}

CReadMap::IQuadDrawer::~IQuadDrawer() {
}
