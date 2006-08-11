#ifndef LOSHANDLER_H
#define LOSHANDLER_H
// LosHandler.h: interface for the CLosHandler class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)

#include <vector>
#include <list>
#include <deque>
#include "MemPool.h"
#include "Map/Ground.h"
#include "Sim/Objects/WorldObject.h"
#include "Sim/Units/Unit.h"
#include "RadarHandler.h"

#define MAX_LOS_TABLE 110

struct LosInstance{
	inline void* operator new(size_t size){return mempool.Alloc(size);};
	inline void operator delete(void* p,size_t size){mempool.Free(p,size);};
 	std::vector<int> losSquares;
	inline LosInstance(int lossize,int allyteam,int baseSquare,int baseAirSquare,int hashNum,float baseHeight,int airLosSize)
		: losSize(lossize),
			airLosSize(airLosSize),
			refCount(1),
			allyteam(allyteam),
			baseSquare(baseSquare),
			baseAirSquare(baseAirSquare),
			hashNum(hashNum),
			baseHeight(baseHeight),
			toBeDeleted(false){}
	int losSize;
	int airLosSize;
	int refCount;
	int allyteam;
	int baseSquare;
	int baseAirSquare;
	int hashNum;
	float baseHeight;
	bool toBeDeleted;
};

class CRadarHandler;

class CLosHandler  
{
public:
	void MoveUnit(CUnit* unit,bool redoCurrent);
	void FreeInstance(LosInstance* instance);
	inline bool InLos(const CUnit* unit, int allyteam){
		if(unit->isCloaked)
			return false;
		if(unit->useAirLos){
			return !!airLosMap[allyteam][(max(0,min(airSizeY-1,((int(unit->pos.z*invAirDiv)))))*airSizeX) + max(0,min(airSizeX-1,((int(unit->pos.x*invAirDiv)))))];
		} else {
			if(unit->isUnderWater && !radarhandler->InRadar(unit,allyteam))
				return false;
			return !!losMap[allyteam][max(0,min(losSizeY-1,((int)(unit->pos.z*invLosDiv))))*losSizeX+ max(0,min(losSizeX-1,((int)(unit->pos.x*invLosDiv))))];
		}
	}
	inline bool InLos(const CWorldObject* object, int allyteam){
		if(object->useAirLos)
			return !!airLosMap[allyteam][(max(0,min(airSizeY-1,((int(object->pos.z*invAirDiv)))))*airSizeX) + max(0,min(airSizeX-1,((int(object->pos.x*invAirDiv)))))] | object->alwaysVisible;
		else
			return !!losMap[allyteam][max(0,min(losSizeY-1,((int)(object->pos.z*invLosDiv))))*losSizeX+ max(0,min(losSizeX-1,((int)(object->pos.x*invLosDiv))))] | object->alwaysVisible;
	}
	inline bool InLos(float3 pos, int allyteam){
		pos.CheckInBounds();
		return !!losMap[allyteam][((int)(pos.z*invLosDiv))*losSizeX+((int)(pos.x*invLosDiv))];
	}
	CLosHandler();
	virtual ~CLosHandler();

	unsigned short* losMap[MAX_TEAMS];
	unsigned short* airLosMap[MAX_TEAMS];

	friend class CRadarHandler;

	int losMipLevel;
	int airMipLevel;
	float invLosDiv;
	float invAirDiv;
	int airSizeX;
	int airSizeY;
	int losSizeX;
	int losSizeY;

private:

	void SafeLosAdd(LosInstance* instance,int xm,int ym);
	void LosAdd(LosInstance* instance);
	int GetHashNum(CUnit* unit);
	void AllocInstance(LosInstance* instance);
	void CleanupInstance(LosInstance* instance);
	void LosAddAir(LosInstance* instance);

	std::list<LosInstance*> instanceHash[2309+1];

	std::deque<LosInstance*> toBeDeleted;

	struct DelayedInstance {
		LosInstance* instance;
		int timeoutTime;
	};

	std::deque<DelayedInstance> delayQue;

	struct CPoint
	{
		CPoint(){};
		CPoint(int x,int y):x(x),y(y){};

		int x;
		int y;
		
		int operator < (const CPoint &a) const
		{
			if(x!=a.x)
				return x<a.x;
			else
				return y<a.y;
		}
	};
	char *PaintTable;
	typedef std::list<CPoint> TPoints;
	TPoints Points;
	float terrainHeight[256];

	typedef std::vector<CPoint> LosLine;
	typedef std::vector<LosLine> LosTable;

	std::vector<LosTable> lostables;

	int Round(float num);
	void DrawLine(int x,int y,int Size);
	LosLine OutputLine(int x,int y,int line);
	void OutputTable(int table);
public:
	void Update(void);
	void DelayedFreeInstance(LosInstance* instance);
};

extern CLosHandler* loshandler;

#endif /* LOSHANDLER_H */
