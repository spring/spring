#include "StdAfx.h"
// LosHandler.cpp: implementation of the CLosHandler class.
//
//////////////////////////////////////////////////////////////////////

#include "LosHandler.h"

#include "Sim/ModInfo.h"
#include "Sim/Units/Unit.h"
#include <list>
#include "Map/ReadMap.h"
#include "TimeProfiler.h"
#include "LogOutput.h"
#include "Platform/errorhandler.h"
#include "creg/STL_Deque.h"
#include "creg/STL_List.h"
#include "mmgr.h"

using std::min;
using std::max;

CR_BIND(LosInstance, );
CR_BIND(CLosHandler, );
CR_BIND(CLosHandler::DelayedInstance, );
CR_BIND(CLosHandler::CPoint, );

CR_REG_METADATA(LosInstance,(
//		CR_MEMBER(losSquares),
		CR_MEMBER(losSize),
		CR_MEMBER(airLosSize),
		CR_MEMBER(refCount),
		CR_MEMBER(allyteam),
		CR_MEMBER(baseX),
		CR_MEMBER(baseY),
		CR_MEMBER(baseSquare),
		CR_MEMBER(baseAirSquare),
		CR_MEMBER(hashNum),
		CR_MEMBER(baseHeight),
		CR_MEMBER(toBeDeleted),
		CR_RESERVED(16)
		));

void CLosHandler::PostLoad()
{
	for (int a = 0; a < 2309; ++a)
		for (std::list<LosInstance*>::iterator li = instanceHash[a].begin(); li != instanceHash[a].end(); ++li)
			if ((*li)->refCount) {
				if ((*li)->baseX-(*li)->losSize<0 || (*li)->baseX+(*li)->losSize>=losSizeX ||
				    (*li)->baseY-(*li)->losSize<0 || (*li)->baseY+(*li)->losSize>=losSizeY)
					SafeLosAdd(*li,(*li)->baseX,(*li)->baseY);
				else
					LosAdd(*li);
			}
}

CR_REG_METADATA(CLosHandler,(
		CR_MEMBER(losMipLevel),
		CR_MEMBER(airMipLevel),
		CR_MEMBER(invLosDiv),
		CR_MEMBER(invAirDiv),
		CR_MEMBER(airSizeX),
		CR_MEMBER(airSizeY),
		CR_MEMBER(losSizeX),
		CR_MEMBER(losSizeY),
		CR_MEMBER(instanceHash),
		CR_MEMBER(toBeDeleted),
		CR_MEMBER(delayQue),
//		CR_MEMBER(Points),
		CR_MEMBER(terrainHeight),
//		CR_MEMBER(lostables)
		CR_RESERVED(32),
		CR_POSTLOAD(PostLoad)
		));

CR_REG_METADATA_SUB(CLosHandler,DelayedInstance,(
		CR_MEMBER(instance),
		CR_MEMBER(timeoutTime)));
/*
CR_REG_METADATA_SUB(CLosHandler,CPoint,(
		CR_MEMBER(x),
		CR_MEMBER(y)));
*/
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


CLosHandler* loshandler;


CLosHandler::CLosHandler()
{
	losMipLevel = modInfo.losMipLevel;
	airMipLevel = modInfo.airMipLevel;
	airSizeX = std::max(1, gs->mapx >> airMipLevel);
	airSizeY = std::max(1, gs->mapy >> airMipLevel);
	losSizeX = std::max(1, gs->mapx >> losMipLevel);
	losSizeY = std::max(1, gs->mapy >> losMipLevel);
	invLosDiv = 1 / ((float)SQUARE_SIZE * (1 << losMipLevel));
	invAirDiv = 1 / ((float)SQUARE_SIZE * (1 << airMipLevel));

	for(int a=0;a<gs->activeAllyTeams;++a){
		losMap[a].resize(losSizeX*losSizeY,0);
		airLosMap[a].resize(airSizeX*airSizeY,0);
	}

	for(int a=1;a<=MAX_LOS_TABLE;++a)
		OutputTable(a);

	for(int a=0;a<256;++a)
		terrainHeight[a]=-15;

	terrainHeight[6]=30;
	terrainHeight[7]=30;
	terrainHeight[8]=30;

	terrainHeight[26]=30;
	terrainHeight[27]=30;
	terrainHeight[28]=30;
	terrainHeight[29]=30;
	terrainHeight[31]=30;

	for(int a=32;a<64;++a)
		terrainHeight[a]=10;
}

CLosHandler::~CLosHandler()
{
	std::list<LosInstance*>::iterator li;
	for(int a=0;a<2309;++a){
		for(li=instanceHash[a].begin();li!=instanceHash[a].end();++li){
			LosInstance* i = *li;
			i->_DestructInstance(i);
			mempool.Free(i, sizeof(LosInstance));
		}
	}

}

void CLosHandler::MoveUnit(CUnit *unit,bool redoCurrent)
{
	SCOPED_TIMER("Los");
	float3 losPos=unit->pos;
	losPos.CheckInBounds();

	const int allyteam=unit->allyteam;
	unit->lastLosUpdate=gs->frameNum;

	if(unit->losRadius<=0){
		return;
	}
	const int xmap=(int)(losPos.x*invLosDiv);
	const int ymap=(int)(losPos.z*invLosDiv);
	const int baseSquare=max(0,min(losSizeY-1,(ymap)))*losSizeX + max(0,min(losSizeX-1,xmap));
	const int baseX=max(0,min(losSizeX-1,xmap));
	const int baseY=max(0,min(losSizeY-1,(ymap)));

	LosInstance* instance;
	if(redoCurrent){
		if(!unit->los){
			return;
		}
		instance=unit->los;
		CleanupInstance(instance);
		instance->losSquares.clear();
		instance->baseX=baseX;
		instance->baseY=baseY;
		instance->baseSquare=baseSquare;	//this could be a problem if several units are sharing the same instance
		int baseAirSquare=max(0,min(airSizeY-1,((int)(losPos.z*invAirDiv))))*airSizeX + max(0,min(airSizeX-1,(int)(losPos.x*invAirDiv)));
		instance->baseAirSquare=baseAirSquare;
	} else {
		if(unit->los && unit->los->baseSquare==baseSquare){
			return;
		}
		FreeInstance(unit->los);
		int hash=GetHashNum(unit);
		std::list<LosInstance*>::iterator lii;
		for(lii=instanceHash[hash].begin();lii!=instanceHash[hash].end();++lii){
			if((*lii)->baseSquare==baseSquare && (*lii)->losSize==unit->losRadius && (*lii)->airLosSize==unit->airLosRadius && (*lii)->baseHeight==unit->losHeight && (*lii)->allyteam==allyteam){
				AllocInstance(*lii);
				unit->los=*lii;
				return;
			}
		}
		int baseAirSquare=max(0,min(airSizeY-1,((int)(losPos.z*invAirDiv))))*airSizeX + max(0,min(airSizeX-1,(int)(losPos.x*invAirDiv)));
		instance=new(mempool.Alloc(sizeof(LosInstance))) LosInstance(unit->losRadius,allyteam,baseX,baseY,baseSquare,baseAirSquare,hash,unit->losHeight,unit->airLosRadius);
		instanceHash[hash].push_back(instance);
		unit->los=instance;
	}
	if(xmap-unit->losRadius<0 || xmap+unit->losRadius>=losSizeX || ymap-unit->losRadius<0 || ymap+unit->losRadius>=losSizeY)
		SafeLosAdd(instance,xmap,ymap);
	else
		LosAdd(instance);
}

void CLosHandler::LosAdd(LosInstance* instance)
{
	assert(instance);
	assert(instance->allyteam < gs->activeAllyTeams);
	assert(instance->allyteam >= 0);

	const int allyteam=instance->allyteam;
	const int mapSquare=instance->baseSquare;

	LosAddAir(instance);

	int tablenum=instance->losSize;
	if(tablenum>MAX_LOS_TABLE){
		tablenum=MAX_LOS_TABLE;
	}
	LosTable& table=lostables[tablenum-1];

	instance->losSquares.push_back(mapSquare);
	losMap[allyteam][mapSquare]++;

	for(LosTable::iterator li=table.begin();li!=table.end();++li){
		LosLine& line=*li;
		const float baseHeight=readmap->mipHeightmap[losMipLevel][mapSquare]+instance->baseHeight-15;
		float maxAng1=-1000;
		float maxAng2=-1000;
		float maxAng3=-1000;
		float maxAng4=-1000;
		float r=1;
		for(LosLine::iterator linei=line.begin();linei!=line.end();++linei){
			float invR=1.0f/r;
			int square=mapSquare+linei->x+linei->y*losSizeX;
			float dh=readmap->mipHeightmap[losMipLevel][square]-baseHeight;
			float ang=dh*invR;
			if(ang>maxAng1){
				instance->losSquares.push_back(square);
				losMap[allyteam][square]++;
			}
			dh+=terrainHeight[0];
			ang=dh*invR;
			if(ang>maxAng1){
				maxAng1=ang;
			}

			square=mapSquare-linei->x-linei->y*losSizeX;
			dh=readmap->mipHeightmap[losMipLevel][square]-baseHeight;
			ang=dh*invR;
			if(ang>maxAng2){
				instance->losSquares.push_back(square);
				losMap[allyteam][square]++;
			}
			dh+=terrainHeight[0];
			ang=dh*invR;
			if(ang>maxAng2){
				maxAng2=ang;
			}

			square=mapSquare-linei->x*losSizeX+linei->y;
			dh=readmap->mipHeightmap[losMipLevel][square]-baseHeight;
			ang=dh*invR;
			if(ang>maxAng3){
				instance->losSquares.push_back(square);
				losMap[allyteam][square]++;
			}
			dh+=terrainHeight[0];
			ang=dh*invR;
			if(ang>maxAng3){
				maxAng3=ang;
			}

			square=mapSquare+linei->x*losSizeX-linei->y;
			dh=readmap->mipHeightmap[losMipLevel][square]-baseHeight;
			ang=dh*invR;
			if(ang>maxAng4){
				instance->losSquares.push_back(square);
				losMap[allyteam][square]++;
			}
			dh+=terrainHeight[0];
			ang=dh*invR;
			if(ang>maxAng4){
				maxAng4=ang;
			}
			r++;
		}
	}
}

void CLosHandler::SafeLosAdd(LosInstance* instance,int xm,int ym)
{
	int xmap=xm;
	int ymap=ym;
	int allyteam=instance->allyteam;

	int mapSquare=instance->baseSquare;

	LosAddAir(instance);

	int tablenum=instance->losSize;
	if(tablenum>MAX_LOS_TABLE){
		tablenum=MAX_LOS_TABLE;
	}
	LosTable& table=lostables[tablenum-1];

	for(LosTable::iterator li=table.begin();li!=table.end();++li){
		LosLine& line=*li;
		float baseHeight=readmap->mipHeightmap[losMipLevel][mapSquare]+instance->baseHeight-15;
		float maxAng1=-1000;
		float maxAng2=-1000;
		float maxAng3=-1000;
		float maxAng4=-1000;
		float r=1;
		instance->losSquares.push_back(mapSquare);
		losMap[allyteam][mapSquare]++;

		for(LosLine::iterator linei=line.begin();linei!=line.end();++linei){
			if(xmap+linei->x<losSizeX && ymap+linei->y<losSizeY){
				int square=mapSquare+linei->x+linei->y*losSizeX;
				float dh=readmap->mipHeightmap[losMipLevel][square]-baseHeight;
				float ang=dh/r;
				if(ang>maxAng1){
					instance->losSquares.push_back(square);
					losMap[allyteam][square]++;
				}
				dh+=terrainHeight[0];
				ang=dh/r;
				if(ang>maxAng1){
					maxAng1=ang;
				}
			}
			if(xmap-linei->x>=0 && ymap-linei->y>=0){
				int square=mapSquare-linei->x-linei->y*losSizeX;
				float dh=readmap->mipHeightmap[losMipLevel][square]-baseHeight;
				float ang=dh/r;
				if(ang>maxAng2){
					instance->losSquares.push_back(square);
					losMap[allyteam][square]++;
				}
				dh+=terrainHeight[0];
				ang=dh/r;
				if(ang>maxAng2){
					maxAng2=ang;
				}
			}
			if(xmap+linei->y<losSizeX && ymap-linei->x>=0){
				int square=mapSquare-linei->x*losSizeX+linei->y;
				float dh=readmap->mipHeightmap[losMipLevel][square]-baseHeight;
				float ang=dh/r;
				if(ang>maxAng3){
					instance->losSquares.push_back(square);
					losMap[allyteam][square]++;
				}
				dh+=terrainHeight[0];
				ang=dh/r;
				if(ang>maxAng3){
					maxAng3=ang;
				}
			}
			if(xmap-linei->y>=0 && ymap+linei->x<losSizeY){
				int square=mapSquare+linei->x*losSizeX-linei->y;
				float dh=readmap->mipHeightmap[losMipLevel][square]-baseHeight;
				float ang=dh/r;
				if(ang>maxAng4){
					instance->losSquares.push_back(square);
					losMap[allyteam][square]++;
				}
				dh+=terrainHeight[0];
				ang=dh/r;
				if(ang>maxAng4){
					maxAng4=ang;
				}
			}
			r++;
		}
	}
}

void CLosHandler::OutputTable(int Table)
{
	LosTable lostable;

	int Radius = Table;
	char* PaintTable = SAFE_NEW char[(Radius+1)*Radius];
	memset(PaintTable, 0 , (Radius+1)*Radius);
	CPoint P;

	int x, y, r2;

	P.x = 0;
	P.y = Radius;
	Points.push_front(P);
//  DrawLine(0, Radius, Radius);
	for(float i=Radius; i>=1; i-=0.5f)
	{
		r2 = (int)(i * i);

		y = (int)i;
		x = 1;
		y = (int) (sqrt((float)r2 - 1) + 0.5f);
		while (x < y) {
			if(!PaintTable[x+y*Radius])
			{
				DrawLine(PaintTable, x, y, Radius);
				P.x = x;
				P.y = y;
				Points.push_back(P);
			}
			if(!PaintTable[y+x*Radius])
			{
				DrawLine(PaintTable, y, x, Radius);
				P.x = y;
				P.y = x;
				Points.push_back(P);
			}

			x += 1;
			y = (int) (sqrt((float)r2 - x*x) + 0.5f);
		}
		if (x == y) {
			if(!PaintTable[x+y*Radius])
			{
				DrawLine(PaintTable, x, y, Radius);
				P.x = x;
				P.y = y;
				Points.push_back(P);
			}
		}
	}

	Points.sort();

	int Line = 1;
	int Size = Points.size();
	for(int j=0; j<Size; j++)
	{
		lostable.push_back(OutputLine(Points.back().x, Points.back().y, Line));
		Points.pop_back();
		Line++;
	}

	lostables.push_back(lostable);

	delete[] PaintTable;
}

CLosHandler::LosLine CLosHandler::OutputLine(int x, int y, int Line)
{
	LosLine losline;

	int x0 = 0;
	int y0 = 0;
	int dx = x;
	int dy = y;

	if (abs(dx) > abs(dy)) {          // slope <1
		float m = (float) dy / (float) dx;      // compute slope
		float b = y0 - m*x0;
		dx = (dx < 0) ? -1 : 1;
		while (x0 != x) {
			x0 += dx;
			losline.push_back(CPoint(x0,Round(m*x0 + b)));
		}
	} else
		if (dy != 0) {                              // slope = 1
			float m = (float) dx / (float) dy;      // compute slope
			float b = x0 - m*y0;
			dy = (dy < 0) ? -1 : 1;
			while (y0 != y) {
				y0 += dy;
				losline.push_back(CPoint(Round(m*y0 + b),y0));
			}
		}
	return losline;
}

void CLosHandler::DrawLine(char* PaintTable, int x, int y, int Size)
{
	int x0 = 0;
	int y0 = 0;
	int dx = x;
	int dy = y;

	if (abs(dx) > abs(dy)) {          // slope <1
		float m = (float) dy / (float) dx;      // compute slope
		float b = y0 - m*x0;
		dx = (dx < 0) ? -1 : 1;
		while (x0 != x) {
			x0 += dx;
			PaintTable[x0+Round(m*x0 + b)*Size] = 1;
		}
	} else {
		if (dy != 0) {                              // slope = 1
			float m = (float) dx / (float) dy;      // compute slope
			float b = x0 - m*y0;
			dy = (dy < 0) ? -1 : 1;
			while (y0 != y) {
				y0 += dy;
				PaintTable[Round(m*y0 + b)+y0*Size] = 1;
			}
		}
	}
}

int CLosHandler::Round(float Num)
{
  if((Num - (int)Num) <0.5f)
    return (int)Num;
  else
    return (int)Num+1;
}

void CLosHandler::FreeInstance(LosInstance* instance)
{
	if(instance==0)
		return;
	instance->refCount--;
	if(instance->refCount==0){
		CleanupInstance(instance);
		if(!instance->toBeDeleted){
			instance->toBeDeleted=true;
			toBeDeleted.push_back(instance);
		}
		if(instance->hashNum>=2310 || instance->hashNum<0){
			logOutput.Print("bad los");
		}
		if(toBeDeleted.size()>500){
			LosInstance* i=toBeDeleted.front();
			toBeDeleted.pop_front();
//			logOutput.Print("del %i",i->hashNum);
			if(i->hashNum>=2310 || i->hashNum<0){
				logOutput.Print("bad los 2");
				return;
			}
			i->toBeDeleted=false;
			if(i->refCount==0){
				std::list<LosInstance*>::iterator lii;
				for(lii=instanceHash[i->hashNum].begin();lii!=instanceHash[i->hashNum].end();++lii){
					if((*lii)==i){
						instanceHash[i->hashNum].erase(lii);
						i->_DestructInstance(i);
						mempool.Free(i,sizeof(LosInstance));
						break;
					}
				}
			}
		}
	}
}

int CLosHandler::GetHashNum(CUnit* unit)
{
	unsigned int t=unit->mapSquare*unit->losRadius+unit->allyteam;
	t^=*(unsigned int*)&unit->losHeight;
	return t%2309;
}

void CLosHandler::AllocInstance(LosInstance* instance)
{
	if(instance->refCount==0){
		std::vector<int>::iterator lsi;
		for(lsi=instance->losSquares.begin();lsi!=instance->losSquares.end();++lsi){
			++losMap[instance->allyteam][*lsi];
		}
		LosAddAir(instance);
	}
	instance->refCount++;
}

void CLosHandler::CleanupInstance(LosInstance* instance)
{
	for(std::vector<int>::iterator lsi=instance->losSquares.begin();lsi!=instance->losSquares.end();++lsi){
		--losMap[instance->allyteam][*lsi];
	}

	int by=(instance->baseAirSquare/airSizeX);
	int bx=(instance->baseAirSquare-by*airSizeX);

	int sx=max(0,bx-instance->airLosSize);
	int ex=min(airSizeX-1,bx+instance->airLosSize);
	int sy=max(0,by-instance->airLosSize);
	int ey=min(airSizeY-1,by+instance->airLosSize);

	int rr=instance->airLosSize*instance->airLosSize;
	for(int y=sy;y<=ey;++y){
		int rrx=rr-(by-y)*(by-y);
		for(int x=sx;x<=ex;++x){
			if((bx-x)*(bx-x)<=rrx){
				--airLosMap[instance->allyteam][y*airSizeX+x];
			}
		}
	}
}

void CLosHandler::LosAddAir(LosInstance* instance)
{
	/*int by=(instance->baseSquare/gs->hmapx);
	int bx=(instance->baseSquare-by*gs->hmapx)/2;
	by/=2;*/
	int by=(instance->baseAirSquare/airSizeX);
	int bx=(instance->baseAirSquare-by*airSizeX);

	int sx=max(0,bx-instance->airLosSize);
	int ex=min(airSizeX-1,bx+instance->airLosSize);
	int sy=max(0,by-instance->airLosSize);
	int ey=min(airSizeY-1,by+instance->airLosSize);

	int rr=instance->airLosSize*instance->airLosSize;

	if (instance->allyteam < 0)
		return;

	for(int y=sy;y<=ey;++y){
		int rrx=rr-(by-y)*(by-y);
		for(int x=sx;x<=ex;++x){
			if((bx-x)*(bx-x)<=rrx){
				++airLosMap[instance->allyteam][y*airSizeX+x];
			}
		}
	}
}

void CLosHandler::Update(void)
{
	while(!delayQue.empty() && delayQue.front().timeoutTime<gs->frameNum){
		FreeInstance(delayQue.front().instance);
		delayQue.pop_front();
	}
}

void CLosHandler::DelayedFreeInstance(LosInstance* instance)
{
	DelayedInstance di;
	di.instance=instance;
	di.timeoutTime=gs->frameNum+45;

	delayQue.push_back(di);
}
