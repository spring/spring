#include "StdAfx.h"
// LosHandler.cpp: implementation of the CLosHandler class.
//
//////////////////////////////////////////////////////////////////////
#include <list>
#include <cstdlib>
#include <cstring>
#include "mmgr.h"

#include "LosHandler.h"

#include "ModInfo.h"
#include "Sim/Units/Unit.h"
#include "Sim/Misc/TeamHandler.h"
#include "Map/ReadMap.h"
#include "TimeProfiler.h"
#include "LogOutput.h"
#include "Platform/errorhandler.h"
#include "creg/STL_Deque.h"
#include "creg/STL_List.h"

using std::min;
using std::max;

CR_BIND(LosInstance, );
CR_BIND(CLosHandler, );
CR_BIND(CLosHandler::DelayedInstance, );

CR_REG_METADATA(LosInstance,(
//		CR_MEMBER(losSquares),
		CR_MEMBER(losSize),
		CR_MEMBER(airLosSize),
		CR_MEMBER(refCount),
		CR_MEMBER(allyteam),
		CR_MEMBER(basePos),
		CR_MEMBER(baseSquare),
		CR_MEMBER(baseAirPos),
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
				LosAdd(*li);
			}
}

CR_REG_METADATA(CLosHandler,(
		CR_MEMBER(instanceHash),
		CR_MEMBER(toBeDeleted),
		CR_MEMBER(delayQue),
		CR_RESERVED(8),
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


CLosHandler::CLosHandler() :
	losMap(teamHandler->ActiveAllyTeams()),
	airLosMap(teamHandler->ActiveAllyTeams()),
	//airAlgo(int2(airSizeX, airSizeY), -1e6f, 15, readmap->mipHeightmap[airMipLevel]),
	losMipLevel(modInfo.losMipLevel),
	airMipLevel(modInfo.airMipLevel),
	losDiv(SQUARE_SIZE * (1 << losMipLevel)),
	airDiv(SQUARE_SIZE * (1 << airMipLevel)),
	invLosDiv(1.0f / losDiv),
	invAirDiv(1.0f / airDiv),
	airSizeX(std::max(1, gs->mapx >> airMipLevel)),
	airSizeY(std::max(1, gs->mapy >> airMipLevel)),
	losSizeX(std::max(1, gs->mapx >> losMipLevel)),
	losSizeY(std::max(1, gs->mapy >> losMipLevel)),
	requireSonarUnderWater(modInfo.requireSonarUnderWater),
	losAlgo(int2(losSizeX, losSizeY), -1e6f, 15, readmap->mipHeightmap[losMipLevel])
{
	for (int a = 0; a < teamHandler->ActiveAllyTeams(); ++a) {
		losMap[a].SetSize(losSizeX, losSizeY);
		airLosMap[a].SetSize(airSizeX, airSizeY);
	}
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


void CLosHandler::MoveUnit(CUnit *unit, bool redoCurrent)
{
	SCOPED_TIMER("Los");
	const float3& losPos = unit->pos;

	const int allyteam = unit->allyteam;
	unit->lastLosUpdate = gs->frameNum;

	if (unit->losRadius <= 0) {
		return;
	}

	const int baseX = max(0, min(losSizeX - 1, (int)(losPos.x * invLosDiv)));
	const int baseY = max(0, min(losSizeY - 1, (int)(losPos.z * invLosDiv)));
	const int baseSquare = baseY * losSizeX + baseX;
	const int baseAirX = max(0, min(airSizeX - 1, (int)(losPos.x * invAirDiv)));
	const int baseAirY = max(0, min(airSizeY - 1, (int)(losPos.z * invAirDiv)));

	LosInstance* instance;
	if (redoCurrent) {
		if (!unit->los) {
			return;
		}
		instance = unit->los;
		CleanupInstance(instance);
		instance->losSquares.clear();
		instance->basePos.x = baseX;
		instance->basePos.y = baseY;
		instance->baseSquare = baseSquare; //this could be a problem if several units are sharing the same instance
		instance->baseAirPos.x = baseAirX;
		instance->baseAirPos.y = baseAirY;
	} else {
		if (unit->los && (unit->los->baseSquare == baseSquare)) {
			return;
		}
		FreeInstance(unit->los);
		int hash = GetHashNum(unit);
		std::list<LosInstance*>::iterator lii;
		for (lii = instanceHash[hash].begin(); lii != instanceHash[hash].end(); ++lii) {
			if ((*lii)->baseSquare == baseSquare         &&
			    (*lii)->losSize    == unit->losRadius    &&
			    (*lii)->airLosSize == unit->airLosRadius &&
			    (*lii)->baseHeight == unit->losHeight    &&
			    (*lii)->allyteam   == allyteam) {
				AllocInstance(*lii);
				unit->los = *lii;
				return;
			}
		}
		instance=new(mempool.Alloc(sizeof(LosInstance))) LosInstance(unit->losRadius, unit->airLosRadius, allyteam, int2(baseX,baseY), baseSquare, int2(baseAirX, baseAirY), hash, unit->losHeight);
		instanceHash[hash].push_back(instance);
		unit->los=instance;
	}

	LosAdd(instance);
}


void CLosHandler::LosAdd(LosInstance* instance)
{
	assert(instance);
	assert(instance->allyteam < teamHandler->ActiveAllyTeams());
	assert(instance->allyteam >= 0);

	losAlgo.LosAdd(instance->basePos, instance->losSize, instance->baseHeight, instance->losSquares);

	losMap[instance->allyteam].AddMapSquares(instance->losSquares, 1);
	airLosMap[instance->allyteam].AddMapArea(instance->baseAirPos, instance->airLosSize, 1);
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
	if (instance->refCount == 0) {
		LosAdd(instance);
	}
	instance->refCount++;
}


void CLosHandler::CleanupInstance(LosInstance* instance)
{
	losMap[instance->allyteam].AddMapSquares(instance->losSquares, -1);
	airLosMap[instance->allyteam].AddMapArea(instance->baseAirPos, instance->airLosSize, -1);
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
