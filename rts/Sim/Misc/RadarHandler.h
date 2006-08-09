#ifndef RADARHANDLER_H
#define RADARHANDLER_H

#include "Object.h"
#include "Sim/Units/Unit.h"

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
			return (!!sonarMaps[allyteam][square]) && !commonSonarJammerMap[square];
		}
		if(unit->useAirLos){
			return airRadarMaps[allyteam][square] && !commonJammerMap[square];
		} else {
			return (radarMaps[allyteam][square] || (unit->pos.y<=1 && sonarMaps[allyteam][square])) && !commonJammerMap[square];
		}
	}

	inline bool InSeismicDistance(const CUnit* unit,int allyteam){
		int square=max(0,min(ysize-1,(int)unit->pos.z/(SQUARE_SIZE*RADAR_SIZE)))*xsize+max(0,min(xsize-1,(int)unit->pos.x/(SQUARE_SIZE*RADAR_SIZE)));
		return !!seismicMaps[allyteam][square];
	}

	bool circularRadar;

	unsigned short* radarMaps[MAX_TEAMS];
	unsigned short* airRadarMaps[MAX_TEAMS];
	unsigned short* sonarMaps[MAX_TEAMS];
	unsigned short* jammerMaps[MAX_TEAMS];
	unsigned short* seismicMaps[MAX_TEAMS];
	unsigned short* commonJammerMap;
	unsigned short* commonSonarJammerMap;
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

#endif /* RADARHANDLER_H */
