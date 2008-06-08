#ifndef LOSHANDLER_H
#define LOSHANDLER_H
// LosHandler.h: interface for the CLosHandler class.
//
//////////////////////////////////////////////////////////////////////

#include <vector>
#include <list>
#include <deque>
#include <boost/noncopyable.hpp>
#include "MemPool.h"
#include "Map/Ground.h"
#include "Sim/Objects/WorldObject.h"
#include "Sim/Units/Unit.h"
#include "RadarHandler.h"

#define MAX_LOS_TABLE 110

struct LosInstance : public boost::noncopyable
{
	CR_DECLARE_STRUCT(LosInstance);
 	std::vector<int> losSquares;
	LosInstance() {} // default constructor for creg
	LosInstance(int lossize, int allyteam, int baseX, int baseY,
	            int baseSquare, int baseAirSquare, int hashNum,
	            float baseHeight, int airLosSize)
		: losSize(lossize),
			airLosSize(airLosSize),
			refCount(1),
			allyteam(allyteam),
			baseX(baseX),
			baseY(baseY),
			baseSquare(baseSquare),
			baseAirSquare(baseAirSquare),
			hashNum(hashNum),
			baseHeight(baseHeight),
			toBeDeleted(false) {}
	int losSize;
	int airLosSize;
	int refCount;
	int allyteam;
	int baseX;
	int baseY;
	int baseSquare;
	int baseAirSquare;
	int hashNum;
	float baseHeight;
	bool toBeDeleted;
};


class CLosHandler : public boost::noncopyable
{
	CR_DECLARE(CLosHandler);
	CR_DECLARE_SUB(CPoint);
	CR_DECLARE_SUB(DelayedInstance);

public:
	void MoveUnit(CUnit* unit, bool redoCurrent);
	void FreeInstance(LosInstance* instance);

	bool InLos(const CWorldObject* object, int allyTeam) {
		if (object->alwaysVisible) {
			return true;
		}
		else if (object->useAirLos) {
			const int gx = (int)(object->pos.x * invAirDiv);
			const int gz = (int)(object->pos.z * invAirDiv);
			const int rowIdx = std::max(0, std::min(airSizeY - 1, gz));
			const int colIdx = std::max(0, std::min(airSizeX - 1, gx));
			const int square = (rowIdx * airSizeX) + colIdx;
			#ifdef DEBUG
			assert(square < airLosMap[allyTeam].size());
			#endif
			return !!airLosMap[allyTeam][square];
		}
		else {
			const int gx = (int)(object->pos.x * invLosDiv);
			const int gz = (int)(object->pos.z * invLosDiv);
			const int rowIdx = std::max(0, std::min(losSizeY - 1, gz));
			const int colIdx = std::max(0, std::min(losSizeX - 1, gx));
			const int square = (rowIdx * losSizeX) + colIdx;
			#ifdef DEBUG
			assert(square < losMap[allyTeam].size());
			#endif
			return !!losMap[allyTeam][square];
		}
	}

	bool InLos(const CUnit* unit, int allyTeam) {
		if (unit->alwaysVisible) {
			return true;
		}
		if (unit->isCloaked) {
			return false;
		}
		return InLos(static_cast<const CWorldObject*>(unit), allyTeam);
	}

	bool InLos(float3 pos, int allyTeam) {
		pos.CheckInBounds();
		const int square = ((int)(pos.z * invLosDiv)) * losSizeX
		                 + ((int)(pos.x * invLosDiv));
		#ifdef DEBUG
		assert(square < losMap[allyTeam].size());
		#endif
		return !!losMap[allyTeam][square];
	}

	bool InAirLos(float3 pos, int allyTeam) {
		pos.CheckInBounds();
		const int square = ((int)(pos.z * invAirDiv)) * airSizeX
		                 + ((int)(pos.x * invAirDiv));
		#ifdef DEBUG
		assert(square < airLosMap[allyTeam].size());
		#endif
		return !!airLosMap[allyTeam][square];
	}

	CLosHandler();
	~CLosHandler();

	vector<unsigned short> losMap[MAX_TEAMS];
	vector<unsigned short> airLosMap[MAX_TEAMS];

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

	void PostLoad();
	void SafeLosAdd(LosInstance* instance,int xm,int ym);
	void LosAdd(LosInstance* instance);
	int GetHashNum(CUnit* unit);
	void AllocInstance(LosInstance* instance);
	void CleanupInstance(LosInstance* instance);
	void LosAddAir(LosInstance* instance);

	std::list<LosInstance*> instanceHash[2309+1];

	std::deque<LosInstance*> toBeDeleted;

	struct DelayedInstance {
		CR_DECLARE_STRUCT(DelayedInstance);
		LosInstance* instance;
		int timeoutTime;
	};

	std::deque<DelayedInstance> delayQue;

	struct CPoint {
		CR_DECLARE_STRUCT(CPoint);

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
	typedef std::list<CPoint> TPoints;
	TPoints Points;
	float terrainHeight[256];

	typedef std::vector<CPoint> LosLine;
	typedef std::vector<LosLine> LosTable;

	std::vector<LosTable> lostables;

	int Round(float num);
	void DrawLine(char* PaintTable, int x,int y,int Size);
	LosLine OutputLine(int x,int y,int line);
	void OutputTable(int table);
public:
	void Update(void);
	void DelayedFreeInstance(LosInstance* instance);
};

extern CLosHandler* loshandler;

#endif /* LOSHANDLER_H */
