#pragma once
#include "Object.h"
#include "Unit.h"

#define RADAR_SIZE 8

class CRadarHandler :
	public CObject
{
public:
	CRadarHandler(bool circularRadar);
	~CRadarHandler(void);
	void MoveUnit(CUnit* unit);
	void RemoveUnit(CUnit* unit);

	inline bool InRadar(const CUnit* unit,int allyteam){
		if(unit->stealth)
			return false;
		int square=max(0,min(ysize-1,(int)unit->pos.z/(SQUARE_SIZE*RADAR_SIZE)))*xsize+max(0,min(xsize-1,(int)unit->pos.x/(SQUARE_SIZE*RADAR_SIZE)));
		if(unit->isUnderWater){
			return !!sonarMaps[allyteam][square];
		}
		if(unit->useAirLos){
			return airRadarMaps[allyteam][square] && !jammerMaps[unit->allyteam][square];
		} else {
			return (radarMaps[allyteam][square] || (unit->pos.y<=1 && sonarMaps[allyteam][square])) && !jammerMaps[unit->allyteam][square];
		}
	}

	bool circularRadar;

	unsigned short* radarMaps[MAX_TEAMS];
	unsigned short* airRadarMaps[MAX_TEAMS];
	unsigned short* sonarMaps[MAX_TEAMS];
	unsigned short* jammerMaps[MAX_TEAMS];
	float radarErrorSize[MAX_TEAMS];
	float baseRadarErrorSize;

	int xsize;
	int ysize;

	float targFacEffect;

protected:
	void AddMapArea(int2 pos, int radius, unsigned short* map, int amount);

	void SafeLosRadarAdd(CUnit* unit);
};

extern CRadarHandler* radarhandler;
