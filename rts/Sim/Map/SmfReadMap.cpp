#include "StdAfx.h"
#include "SmfReadMap.h"
#include "mapfile.h"
#include "Rendering/GL/myGL.h"
#include <GL/glu.h>
#include "FileSystem/FileHandler.h"
#include "Platform/ConfigHandler.h"
#include "Rendering/Map/BFGroundTextures.h"
#include "Game/UI/InfoConsole.h"
#include "MetalMap.h"
#include "Sim/Misc/FeatureHandler.h"
#include "myMath.h"
#include "Platform/errorhandler.h"
#include "Rendering/Textures/Bitmap.h"
#include "bitops.h"
#include "mmgr.h"

using namespace std;

CSmfReadMap::CSmfReadMap(std::string mapname)
{
	readmap=this;//not entirely legal probably

	PrintLoadMsg("Opening map file");

	ParseSMD(string("maps/")+mapname.substr(0,mapname.find_last_of('.'))+".smd");
	
	mapChecksum=0;

	PUSH_CODE_MODE;
	ENTER_MIXED;
	ifs=new CFileHandler(string("maps/")+mapname);
	if(!ifs->FileExists()){
		char t[500];
		sprintf(t,"Error couldnt find map file %s",mapname.c_str());
		handleerror(0,t,"Error when reading map",0);
		exit(0);
	}
	POP_CODE_MODE;
	READPTR_MAPHEADER(header,ifs);

	if(strcmp(header.magic,"spring map file")!=0 || header.version!=1 || header.tilesize!=32 || header.texelPerSquare!=8 || header.squareSize!=8){
		char t[500];
		sprintf(t,"Error couldnt open map file %s",mapname.c_str());
		handleerror(0,t,"Error when reading map",0);
		exit(0);
	}

	gs->mapx=header.mapx;
	gs->mapy=header.mapy;
	gs->mapSquares = gs->mapx*gs->mapy;
	gs->hmapx=gs->mapx/2;
	gs->hmapy=gs->mapy/2;
	int mapx=gs->mapx+1;
	int mapy=gs->mapy+1;
	gs->pwr2mapx=next_power_of_2(gs->mapx);
	gs->pwr2mapy=next_power_of_2(gs->mapy);

//	info->AddLine("%i %i",gs->mapx,gs->mapy);
	float3::maxxpos=gs->mapx*SQUARE_SIZE-1;
	float3::maxzpos=gs->mapy*SQUARE_SIZE-1;

	heightmap=new float[(gs->mapx+1)*(gs->mapy+1)*4];//new float[(gs->mapx+1)*(gs->mapy+1)];
	orgheightmap=new float[(gs->mapx+1)*(gs->mapy+1)];
	//	normals=new float3[(gs->mapx+1)*(gs->mapy+1)];
	facenormals=new float3[gs->mapx*gs->mapy*2];
	centerheightmap=new float[gs->mapx*gs->mapy];
	halfHeightmap=new float[gs->hmapx*gs->hmapy];
	slopemap=new float[gs->hmapx*gs->hmapy];
	typemap=new unsigned char[gs->mapx/2*gs->mapy/2];
	groundBlockingObjectMap = new CSolidObject*[gs->mapSquares];
	memset(groundBlockingObjectMap, 0, gs->mapSquares*sizeof(CSolidObject*));

	//CFileHandler ifs((string("maps/")+stupidGlobalMapname).c_str());

	float base=header.minHeight;
	float mod=(header.maxHeight-header.minHeight)/65536.0f;

	unsigned short* temphm=new unsigned short[mapx*mapy];
	ifs->Seek(header.heightmapPtr);
	ifs->Read(temphm,mapx*mapy*2);

	for(int y=0;y<mapx*mapy;++y){
		heightmap[y]=base+swabword(temphm[y])*mod;
	}

	delete[] temphm;

	minheight=1000;
	maxheight=-1000;
	for(int y=0;y<(gs->mapy+1)*(gs->mapx+1);++y){
		orgheightmap[y]=heightmap[y];
		if(heightmap[y]<minheight)
			minheight=heightmap[y];
		if(heightmap[y]>maxheight)
			maxheight=heightmap[y];
		mapChecksum+=(unsigned int)(heightmap[y]*100);
		mapChecksum^=*(unsigned int*)&heightmap[y];
	}

	for(int y=0;y<(gs->mapy);y++){
		for(int x=0;x<(gs->mapx);x++){
			float height=heightmap[(y)*(gs->mapx+1)+x];
			height+=heightmap[(y)*(gs->mapx+1)+x+1];
			height+=heightmap[(y+1)*(gs->mapx+1)+x];
			height+=heightmap[(y+1)*(gs->mapx+1)+x+1];
			centerheightmap[y*gs->mapx+x]=height/4;
		}
	}

	for(int y=0;y<gs->hmapy;++y){
		for(int x=0;x<gs->hmapx;++x){
			halfHeightmap[y*gs->hmapx+x]=heightmap[(y*2+1)*mapx+x*2+1];
		}
	}

	ifs->Seek(header.typeMapPtr);
	ifs->Read(typemap,gs->mapx*gs->mapy/4);

	for(int a=0;a<256;++a){
		char tname[200];
		sprintf(tname,"TerrainType%i\\",a);
		string section="map\\";
		section+=tname;
		mapDefParser.GetDef(terrainTypes[a].name,"Default",section+"name");
		mapDefParser.GetDef(terrainTypes[a].hardness,"1",section+"hardness");
		mapDefParser.GetDef(terrainTypes[a].tankSpeed,"1",section+"tankmovespeed");
		mapDefParser.GetDef(terrainTypes[a].kbotSpeed,"1",section+"kbotmovespeed");
		mapDefParser.GetDef(terrainTypes[a].hoverSpeed,"1",section+"hovermovespeed");
		mapDefParser.GetDef(terrainTypes[a].shipSpeed,"1",section+"shipmovespeed");
		mapDefParser.GetDef(terrainTypes[a].receiveTracks,"1",section+"receivetracks");
	}
	PrintLoadMsg("Creating surface normals");

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

			slopemap[(y/2)*gs->hmapx+(x/2)]=1-(n.y+n2.y)*0.5;
		}
	}

	PUSH_CODE_MODE;
	ENTER_MIXED;
	heightLinePal=new unsigned char[3*256];
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
	POP_CODE_MODE;

	for(unsigned int a=0;a<mapname.size();++a){
		mapChecksum+=mapname[a];
		mapChecksum*=mapname[a];
	}

	PrintLoadMsg("Loading detail textures");

	CBitmap bm(detailTexName);
	glGenTextures(1, &detailTex);
	glBindTexture(GL_TEXTURE_2D, detailTex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
	gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGBA8 ,bm.xsize, bm.ysize, GL_RGBA, GL_UNSIGNED_BYTE, bm.mem);

	PrintLoadMsg("Creating overhead texture");

	unsigned char* buf=new unsigned char[MINIMAP_SIZE];
	ifs->Seek(header.minimapPtr);
	ifs->Read(buf,MINIMAP_SIZE);
	glGenTextures(1, &minimapTex);
	glBindTexture(GL_TEXTURE_2D, minimapTex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
	//glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8 ,512, 512, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf);
	int offset=0;
	for(unsigned int i=0; i<MINIMAP_NUM_MIPMAP; i++)
	{
		int mipsize = 1024>>i;

		int size = ((mipsize+3)/4)*((mipsize+3)/4)*8;

		glCompressedTexImage2DARB(GL_TEXTURE_2D, i, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, mipsize, mipsize, 0, size, buf + offset);

		offset += size;
	}
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, MINIMAP_NUM_MIPMAP-1 );
	bigtex[0]=minimapTex;

	delete[] buf;

	unsigned char *map = new unsigned char[gs->mapx/2*gs->mapy/2];
	ifs->Seek(header.metalmapPtr);
	ifs->Read(map,gs->mapx/2*gs->mapy/2);
	metalMap = new CMetalMap(map, gs->mapx/2, gs->mapy/2, maxMetal);

	PUSH_CODE_MODE;
	ENTER_UNSYNCED;
	groundTextures=new CBFGroundTextures(ifs);
	POP_CODE_MODE;

	PrintLoadMsg("Creating ground shading");

	glGenTextures(1, &shadowTex);
	glBindTexture(GL_TEXTURE_2D, shadowTex);
//	unsigned char* tempMem=new unsigned char[1024*1024*4];
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8 ,gs->pwr2mapx, gs->pwr2mapy, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
//	delete [] tempMem;

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

	RecalcTexture(0, gs->mapx, 0, gs->mapy);

	for(int a=0;a<256*3;++a){
		yardmapPal[a]=255;
	}
	yardmapPal[0]=0;
	yardmapPal[1]=0;
	yardmapPal[2]=0;
	yardmapPal[5]=0;

}
extern GLfloat FogLand[];

void CSmfReadMap::ParseSMD(std::string filename)
{
	mapDefParser.LoadFile(filename);
	
	gs->sunVector=mapDefParser.GetFloat3(float3(0,1,2),"MAP\\LIGHT\\SunDir");
	gs->sunVector.Normalize();
	gs->sunVector4[0]=gs->sunVector[0];
	gs->sunVector4[1]=gs->sunVector[1];
	gs->sunVector4[2]=gs->sunVector[2];
	gs->sunVector4[3]=0;

	gs->gravity=-atof(mapDefParser.SGetValueDef("130","MAP\\Gravity").c_str())/(GAME_SPEED*GAME_SPEED);

	float3 fogColor=mapDefParser.GetFloat3(float3(0.7,0.7,0.8),"MAP\\ATMOSPHERE\\FogColor");
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
		waterPlaneColor = mapDefParser.GetFloat3(float3(0.0,0.4,0.0),"MAP\\WATER\\WaterPlaneColor");
	}
	mapDefParser.GetDef(tidalStrength, "0", "MAP\\TidalStrength");


	waterSurfaceColor=mapDefParser.GetFloat3(float3(0.75,0.8,0.85),"MAP\\WATER\\WaterSurfaceColor");
	waterAbsorb=mapDefParser.GetFloat3(float3(0,0,0),"MAP\\WATER\\WaterAbsorb");
	waterBaseColor=mapDefParser.GetFloat3(float3(0,0,0),"MAP\\WATER\\WaterBaseColor");
	waterMinColor=mapDefParser.GetFloat3(float3(0,0,0),"MAP\\WATER\\WaterMinColor");
	mapDefParser.GetDef(waterTexture, "", "MAP\\WATER\\WaterTexture");
	TdfParser resources("gamedata/resources.tdf");
	if(waterTexture.empty())	//default water is ocean.jpg in bitmaps, map specific water textures is saved in the map dir
		waterTexture = "bitmaps/"+resources.SGetValueDef("ocean.jpg","resources\\graphics\\maps\\watertex");
	else
		waterTexture = "maps/" + waterTexture;

	for(int a=0;a<1024;++a){
		for(int b=0;b<3;++b){
			float c=max(waterMinColor[b],waterBaseColor[b]-waterAbsorb[b]*a);
			waterHeightColors[a*4+b]=(unsigned char)(c*210);
		}
		waterHeightColors[a*4+3]=1;
	}

	ambientColor=mapDefParser.GetFloat3(float3(0.5,0.5,0.5),"MAP\\LIGHT\\GroundAmbientColor");
	sunColor=mapDefParser.GetFloat3(float3(0.5,0.5,0.5),"MAP\\LIGHT\\GroundSunColor");
	mapDefParser.GetDef(shadowDensity, "0.8", "MAP\\LIGHT\\GroundShadowDensity");

	mapDefParser.GetDef(maxMetal,"0.02","MAP\\MaxMetal");
	mapDefParser.GetDef(extractorRadius,"500","MAP\\ExtractorRadius");

	mapDefParser.GetDef(detailTexName, "", "MAP\\DetailTex");
	if(detailTexName.empty())
		detailTexName = "bitmaps/"+resources.SGetValueDef("detailtex2.bmp","resources\\graphics\\maps\\detailtex");
	else
		detailTexName = "maps/" + detailTexName;
	
	mapDefParser.GetDef(voidWater, "0", "MAP\\voidWater");	
}

CSmfReadMap::~CSmfReadMap(void)
{
	delete ifs;
	if (detailTex) glDeleteTextures (1, &detailTex);
	if (minimapTex) glDeleteTextures (1, &minimapTex);
	if (shadowTex) glDeleteTextures (1, &shadowTex);

	delete groundTextures;

	//myDelete(heightmap);
	delete[] heightmap;
//	delete[] normals;
	delete[] facenormals;
	delete[] typemap;
	delete[] slopemap;
	//myDelete(teammap);
	delete[] centerheightmap;
	delete[] halfHeightmap;
	delete[] orgheightmap;
	delete[] heightLinePal;
	delete[] groundBlockingObjectMap;
}

void CSmfReadMap::RecalcTexture(int x1, int x2, int y1, int y2)
{
	x1-=x1&3;
	x2+=(20004-x2)&3;

	y1-=y1&3;
	y2+=(20004-y2)&3;

	int xsize=x2-x1;
	int ysize=y2-y1;

	//info->AddLine("%i %i %i %i",x1,x2,y1,y2);
	unsigned char* tempMem=new unsigned char[xsize*ysize*4];
	for(int y=0;y<ysize;++y){
		for(int x=0;x<xsize;++x){
			float height = centerheightmap[(x+x1)+(y+y1)*gs->mapx];

			if(height<0){
				int h=(int)-height;

				if(height>-10){
					float3 light = GetLightValue(x+x1,y+y1)*210.0f;
					float wc=-height*0.1;
					tempMem[(y*xsize+x)*4+0] = (unsigned char)(waterHeightColors[h*4+0]*wc+light.x*(1-wc));
					tempMem[(y*xsize+x)*4+1] = (unsigned char)(waterHeightColors[h*4+1]*wc+light.y*(1-wc));
					tempMem[(y*xsize+x)*4+2] = (unsigned char)(waterHeightColors[h*4+2]*wc+light.z*(1-wc));
					tempMem[(y*xsize+x)*4+3] = (unsigned char)((1-wc)*255);
					
				} else if(h<1024){
					tempMem[(y*xsize+x)*4+0] = waterHeightColors[h*4+0];
					tempMem[(y*xsize+x)*4+1] = waterHeightColors[h*4+1];
					tempMem[(y*xsize+x)*4+2] = waterHeightColors[h*4+2];
					tempMem[(y*xsize+x)*4+3] = 0;
				} else {
					tempMem[(y*xsize+x)*4+0] = waterHeightColors[1023*4+0];
					tempMem[(y*xsize+x)*4+1] = waterHeightColors[1023*4+1];
					tempMem[(y*xsize+x)*4+2] = waterHeightColors[1023*4+2];
					tempMem[(y*xsize+x)*4+3] = 0;
				}
				;//waterHeightColors[h*4+3];
			} else {
				float3 light = GetLightValue(x+x1,y+y1)*210.0f;
				tempMem[(y*xsize+x)*4] = (unsigned char)light.x;
				tempMem[(y*xsize+x)*4+1] = (unsigned char)light.y;
				tempMem[(y*xsize+x)*4+2] = (unsigned char)light.z;
				tempMem[(y*xsize+x)*4+3] = 255;
			}
		}
	}
	glBindTexture(GL_TEXTURE_2D, shadowTex);
	glTexSubImage2D(GL_TEXTURE_2D,0,x1,y1,xsize,ysize,GL_RGBA,GL_UNSIGNED_BYTE,tempMem);

	delete[] tempMem;

}

float3 CSmfReadMap::GetLightValue(int x, int y)
{
	float3 n1=facenormals[(y*gs->mapx+x)*2]+facenormals[(y*gs->mapx+x)*2+1];
	n1.Normalize();

	float3 light=sunColor*gs->sunVector.dot(n1);
	for(int a=0;a<3;++a)
		if(light[a]<0)
			light[a]=0;

	light+=ambientColor;
	for(int a=0;a<3;++a)
		if(light[a]>1)
			light[a]=1;

	return light;
}

