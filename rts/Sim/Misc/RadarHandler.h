#ifndef RADARHANDLER_H
#define RADARHANDLER_H

#include <boost/noncopyable.hpp>

#include "Object.h"
#include "Sim/Units/Unit.h"

#define RADAR_SIZE 8

class CRadarHandler : public boost::noncopyable
{
	CR_DECLARE(CRadarHandler);

public:
	CRadarHandler(bool circularRadar);
	~CRadarHandler();

	void MoveUnit(CUnit* unit);
	void RemoveUnit(CUnit* unit);

	bool InRadar(const float3& pos, int allyteam) {
		const int gx = (int)pos.x / (SQUARE_SIZE * RADAR_SIZE);
		const int gz = (int)pos.z / (SQUARE_SIZE * RADAR_SIZE);
		const int square = std::max(0, std::min(ysize - 1, gz)) * xsize +
		                   std::max(0, std::min(xsize - 1, gx));
		if (pos.y < -0.5f) {
			return (sonarMaps[allyteam][square] && !commonSonarJammerMap[square]);
		}
		else if (!circularRadar && pos.y > +0.5f) {
			return (airRadarMaps[allyteam][square] && !commonJammerMap[square]);
		}
		else {
			return (radarMaps[allyteam][square] && !commonJammerMap[square]);
		}
	}

	bool InRadar(const CUnit* unit, int allyteam) {
		if(unit->stealth)
			return false;
		int square=std::max(0,std::min(ysize-1,(int)unit->pos.z/(SQUARE_SIZE*RADAR_SIZE)))*xsize+std::max(0,std::min(xsize-1,(int)unit->pos.x/(SQUARE_SIZE*RADAR_SIZE)));
		if(unit->isUnderWater){
			return (!!sonarMaps[allyteam][square]) && !commonSonarJammerMap[square];
		}
		if(!circularRadar && unit->useAirLos){
			return airRadarMaps[allyteam][square] && !commonJammerMap[square];
		} else {
			return (radarMaps[allyteam][square] || (unit->pos.y<=1 && sonarMaps[allyteam][square])) && !commonJammerMap[square];
		}
	}

	bool InSeismicDistance(const CUnit* unit, int allyteam) {
		int square=std::max(0,std::min(ysize-1,(int)unit->pos.z/(SQUARE_SIZE*RADAR_SIZE)))*xsize+std::max(0,std::min(xsize-1,(int)unit->pos.x/(SQUARE_SIZE*RADAR_SIZE)));
		return !!seismicMaps[allyteam][square];
	}

	bool circularRadar;

	std::vector<unsigned short> radarMaps[MAX_TEAMS];
	std::vector<unsigned short> airRadarMaps[MAX_TEAMS];
	std::vector<unsigned short> sonarMaps[MAX_TEAMS];
	std::vector<unsigned short> jammerMaps[MAX_TEAMS];
	std::vector<unsigned short> seismicMaps[MAX_TEAMS];
	std::vector<unsigned short> commonJammerMap;
	std::vector<unsigned short> commonSonarJammerMap;
	float radarErrorSize[MAX_TEAMS];
	float baseRadarErrorSize;

	int xsize;
	int ysize;

	float targFacEffect;

protected:
	void AddMapArea(int2 pos, int radius, std::vector<unsigned short>& map, int amount);

	void SafeLosRadarAdd(CUnit* unit);

private:
	void Serialize(creg::ISerializer& s);
};

extern CRadarHandler* radarhandler;

#endif /* RADARHANDLER_H */
