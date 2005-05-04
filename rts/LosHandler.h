// LosHandler.h: interface for the CLosHandler class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_LOSHANDLER_H__99DB739D_46CA_4AAF_A69C_C975AA1E8566__INCLUDED_)
#define AFX_LOSHANDLER_H__99DB739D_46CA_4AAF_A69C_C975AA1E8566__INCLUDED_

#pragma warning(disable:4786)

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <vector>
#include <list>
#include <deque>
#include "mempool.h"
#include "ground.h"
#include "worldobject.h"
#include "unit.h"
#include "radarhandler.h"
//#include <windows.h>

struct LosInstance{
	inline void* operator new(size_t size){return mempool.Alloc(size);};
	inline void operator delete(void* p,size_t size){mempool.Free(p,size);};
 	std::vector<int> losSquares;
	inline LosInstance(int lossize,int allyteam,int baseSquare,int hashNum,float baseHeight,int airLosSize)
		: losSize(lossize),
			airLosSize(airLosSize),
			allyteam(allyteam),
			baseSquare(baseSquare),
			hashNum(hashNum),
			baseHeight(baseHeight),
			refCount(1){}
	int losSize;
	int airLosSize;
	int refCount;
	int allyteam;
	int baseSquare;
	int hashNum;
	float baseHeight;
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
			return !!airLosMap[allyteam][(max(0,min(airSizeY-1,((int(unit->pos.z))/(SQUARE_SIZE*4))))*airSizeX) + max(0,min(airSizeX-1,((int(unit->pos.x))/(SQUARE_SIZE*4))))];
		} else {
			if(unit->isUnderWater && !radarhandler->InRadar(unit,allyteam))
				return false;
			return !!losMap[allyteam][max(0,min(gs->hmapy-1,((int)unit->pos.z)/(SQUARE_SIZE*2)))*gs->hmapx+ max(0,min(gs->hmapx-1,((int)unit->pos.x)/(SQUARE_SIZE*2)))];
		}
	}
	inline bool InLos(const CWorldObject* object, int allyteam){
		if(object->useAirLos)
			return !!airLosMap[allyteam][(max(0,min(airSizeY-1,((int(object->pos.z))/(SQUARE_SIZE*4))))*airSizeX) + max(0,min(airSizeX-1,((int(object->pos.x))/(SQUARE_SIZE*4))))] | object->alwaysVisible;
		else
			return !!losMap[allyteam][max(0,min(gs->hmapy-1,((int)object->pos.z)/(SQUARE_SIZE*2)))*gs->hmapx+ max(0,min(gs->hmapx-1,((int)object->pos.x)/(SQUARE_SIZE*2)))] | object->alwaysVisible;
	}
	inline bool InLos(float3 pos, int allyteam){
		pos.CheckInBounds();
		return !!losMap[allyteam][((int)pos.z)/(SQUARE_SIZE*2)*gs->hmapx+((int)pos.x)/(SQUARE_SIZE*2)];
	}
	CLosHandler();
	virtual ~CLosHandler();

	unsigned short* losMap[MAX_TEAMS];
	unsigned short* airLosMap[MAX_TEAMS];

	friend class CRadarHandler;
private:
	friend CRadarHandler;

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

	int airSizeX;
	int airSizeY;

	int Round(float num);
	void DrawLine(int x,int y,int Size);
	LosLine OutputLine(int x,int y,int line);
	void OutputTable(int table);
public:
	void Update(void);
	void DelayedFreeInstance(LosInstance* instance);
};

extern CLosHandler* loshandler;

#endif // !defined(AFX_LOSHANDLER_H__99DB739D_46CA_4AAF_A69C_C975AA1E8566__INCLUDED_)
