#include "StdAfx.h"
// ReadMap.cpp: implementation of the CReadMap class.
//
//////////////////////////////////////////////////////////////////////

#include "Rendering/GL/myGL.h"
#include <GL/glu.h>			// Header file for the gLu32 library
#include "ReadMap.h"
#include <stdlib.h>
#include <math.h>
//#include <ostream>
#include "Bitmap.h"
#include "Ground.h"
#include "Platform/ConfigHandler.h"
#ifdef _WIN32
#include <process.h>
#endif
#include "FileSystem/FileHandler.h"
#include "SmfReadMap.h"
#include "Rendering/Map/BFGroundDrawer.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Game/UI/InfoConsole.h"
#include "MetalMap.h"
//#include "multipath.h"
#include "Sim/Path/PathManager.h"
#include "Sim/Misc/Wind.h"
#include "Sim/Misc/GeometricObjects.h"
#include "LoadSaveInterface.h"
#include "MapDamage.h"
#include "Platform/errorhandler.h"
#include <string>
#include "mmgr.h"

using namespace std;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CReadMap* readmap;
CReadMap* CReadMap::_instance=0;
string stupidGlobalMapname;
int stupidGlobalMapId=0;

CReadMap* CReadMap::Instance()
{
	if(_instance==0){
		_instance=new CSmfReadMap(stupidGlobalMapname/*/"maps\\map2.smf"/**/);
		readmap=_instance;
		PUSH_CODE_MODE;
		ENTER_UNSYNCED;
		groundDrawer=new CBFGroundDrawer();
		POP_CODE_MODE;
	}

	return _instance;
}
int CReadMap::LoadInt(CFileHandler* ifs)
{
	int i;
	ifs->Read(&i,4);
	return i;
}

static void* myNew(int size)
{
	int msize=size+16000;
	unsigned char* ret=new unsigned char[msize];
	for(int a=0;a<8000;++a)
		ret[a]=0xbb;

	return ret+8000;
}

static void myDelete(void* p)
{
	unsigned char* p2=((unsigned char*)p)-8000;

	for(int a=0;a<8000;++a)
		if(p2[a]!=0xbb){
			handleerror(0,"Write before allocated mem","Error",0);
			break;
		}

	delete[] 	p2;
}

CReadMap::CReadMap()
{
}

CReadMap::~CReadMap()
{
	delete metalMap;
}

void CReadMap::AddGroundBlockingObject(CSolidObject *object) {
	object->mapPos=object->GetMapPos();
	if(object->immobile){
		object->mapPos.x&=0xfffffe;
		object->mapPos.y&=0xfffffe;
	}
	int bx=object->mapPos.x;
	int bz=object->mapPos.y;

	int minXSqr = bx;
	int minZSqr = bz;
	int maxXSqr = bx + object->xsize;
	int maxZSqr = bz + object->ysize;

	for(int zSqr = minZSqr; zSqr < maxZSqr; zSqr++)
		for(int xSqr = minXSqr; xSqr < maxXSqr; xSqr++)
			if(!groundBlockingObjectMap[xSqr + zSqr*gs->mapx])
				groundBlockingObjectMap[xSqr + zSqr*gs->mapx] = object;
	if(!object->mobility && pathManager)
		pathManager->TerrainChange(minXSqr, minZSqr, maxXSqr, maxZSqr);
}

void CReadMap::AddGroundBlockingObject(CSolidObject *object, unsigned char *yardMap) {
	object->mapPos=object->GetMapPos();
	if(object->immobile){
		object->mapPos.x&=0xfffffe;
		object->mapPos.y&=0xfffffe;
	}
	int bx=object->mapPos.x;
	int bz=object->mapPos.y;

	int minXSqr = bx;
	int minZSqr = bz;
	int maxXSqr = bx + object->xsize;
	int maxZSqr = bz + object->ysize;

	for(int z = 0; minZSqr + z < maxZSqr; z++) {
		for(int x = 0; minXSqr + x < maxXSqr; x++) {
			if(!groundBlockingObjectMap[minXSqr + x + (minZSqr + z)*gs->mapx]){
				if(yardMap[x + z*object->xsize]) {
					groundBlockingObjectMap[minXSqr + x + (minZSqr + z)*gs->mapx] = object;
				}
			}
		}
	}
	if(!object->mobility && pathManager)
		pathManager->TerrainChange(minXSqr, minZSqr, maxXSqr, maxZSqr);
}

void CReadMap::RemoveGroundBlockingObject(CSolidObject *object) 
{
	int bx=object->mapPos.x;
	int bz=object->mapPos.y;
	int sx=object->xsize;
	int sz=object->ysize;
	for(int z = bz; z < bz+sz; ++z)
		for(int x = bx; x < bx+sx; ++x)
			if(groundBlockingObjectMap[x + z*gs->mapx]==object)
				groundBlockingObjectMap[x + z*gs->mapx] = 0;

	if(!object->mobility)
		pathManager->TerrainChange(bx, bz, bx+sx, bz+sz);
}


/*
Moves a ground blocking object from old position to the current on map.
*/
void CReadMap::MoveGroundBlockingObject(CSolidObject *object, float3 oldPos) {
	RemoveGroundBlockingObject(object);
	AddGroundBlockingObject(object);
}


/*
Checks if a ground-square is blocked.
If it's not blocked, then 0 is returned.
If it's blocked, then a pointer to the blocking object is returned.
*/
CSolidObject* CReadMap::GroundBlocked(int mapSquare) {
	if(mapSquare < 0 || mapSquare >= gs->mapSquares)
		return 0;
	return groundBlockingObjectMap[mapSquare];
}


/*
Checks if a ground-square is blocked.
If it's not blocked, then 0 is returned.
If it's blocked, then a pointer to the blocking object is returned.
*/
CSolidObject* CReadMap::GroundBlocked(float3 pos) {
	int xSqr = int(pos.x / SQUARE_SIZE) % gs->mapx;
	int zSqr = int(pos.z / SQUARE_SIZE) / gs->mapx;
	return GroundBlocked(xSqr + zSqr*gs->mapx);
}


/*
Opens up a yard in a blocked area.
When a factory opens up, for example.
*/
void CReadMap::OpenBlockingYard(CSolidObject *yard, unsigned char *blockingMap) {
	RemoveGroundBlockingObject(yard);
	AddGroundBlockingObject(yard, blockingMap);
}


/*
Closes a yard, blocking the area.
When a factory closes, for example.
*/
void CReadMap::CloseBlockingYard(CSolidObject *yard) {
	RemoveGroundBlockingObject(yard);
	AddGroundBlockingObject(yard);
}


/*
Used to look for dead references in the groundBlockingMap.
*/
void CReadMap::CleanBlockingMap(CSolidObject* object) {
	int i, counter = 0;
	for(i = 0; i < gs->mapSquares; i++)
		if(groundBlockingObjectMap[i] == object) {
			groundBlockingObjectMap[i] = 0;
			counter++;
		}

	if(counter > 0) {
		*info << "Dead references: " << counter << "\n";
		if(dynamic_cast<CUnit*>(object)){
			info->AddLine("From %s",((CUnit*)object)->unitDef->humanName.c_str());
		}
	}
}

//this function assumes that the correct map has been loaded and only load/saves new height values
void CReadMap::LoadSaveMap(CLoadSaveInterface* file,bool loading)
{
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
