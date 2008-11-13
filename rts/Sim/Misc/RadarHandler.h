#ifndef RADARHANDLER_H
#define RADARHANDLER_H

#include <boost/noncopyable.hpp>

#include "Object.h"
#include "Sim/Units/Unit.h"
#include <assert.h>

#define RADAR_SIZE 8


// Because submerged units are only given LOS if they are also
// in sonar range (see LosHandler.h), sonar stealth and sonar
// jammed units can not be detected (probably why those 2 features
// are not being used by most mods). Uncommenting the following
// line will allow the LOS display mode to differentiate between
// radar and sonar coverage, and radar and sonar jammer coverage.

//#define SONAR_JAMMER_MAPS


class CRadarHandler : public boost::noncopyable
{
	CR_DECLARE(CRadarHandler);


public:
	CRadarHandler(bool circularRadar);
	~CRadarHandler();

	void MoveUnit(CUnit* unit);
	void RemoveUnit(CUnit* unit);

	inline int GetSquare(const float3& pos) const
	{
		const int gx = (int)pos.x / (SQUARE_SIZE * RADAR_SIZE);
		const int gz = (int)pos.z / (SQUARE_SIZE * RADAR_SIZE);
		const int rowIdx = std::max(0, std::min(zsize - 1, gz));
		const int colIdx = std::max(0, std::min(xsize - 1, gx));
		return (rowIdx * xsize) + colIdx;
	}

	bool InRadar(const float3& pos, int allyTeam) {
		const int square = GetSquare(pos);
		if (pos.y < -0.5f) {
			assert(static_cast<unsigned>(square) < sonarMaps[allyTeam].size());
			return (sonarMaps[allyTeam][square] && !commonSonarJammerMap[square]);
		}
		else if (!circularRadar && (pos.y > 0.5f)) {
			assert(static_cast<unsigned>(square) < airRadarMaps[allyTeam].size());
			return (airRadarMaps[allyTeam][square] && !commonJammerMap[square]);
		}
		else {
			assert(static_cast<unsigned>(square) < radarMaps[allyTeam].size());
			return (radarMaps[allyTeam][square] && !commonJammerMap[square]);
		}
	}

	bool InRadar(const CUnit* unit, int allyTeam) {

		if (unit->isUnderWater) {
			if (unit->sonarStealth) {
				return false;
			}
			const int square = GetSquare(unit->pos);
			assert(static_cast<unsigned>(square) < sonarMaps[allyTeam].size());
			return !!sonarMaps[allyTeam][square] && !commonSonarJammerMap[square];
		}
		else if (!circularRadar && unit->useAirLos) {
			if (unit->stealth) {
				return false;
			}
			const int square = GetSquare(unit->pos);
			assert(static_cast<unsigned>(square) < airRadarMaps[allyTeam].size());
			return airRadarMaps[allyTeam][square] && !commonJammerMap[square];
		}
		else {
			const int square = GetSquare(unit->pos);
			assert((static_cast<unsigned>(square) < radarMaps[allyTeam].size()) &&
					(static_cast<unsigned>(square) < sonarMaps[allyTeam].size()));
			return (radarMaps[allyTeam][square]
			        && !unit->stealth
			        && !commonJammerMap[square])
			       ||
			       ((unit->pos.y <= 1.0f)
			        && sonarMaps[allyTeam][square]
			        && !unit->sonarStealth
			        && !commonSonarJammerMap[square]);
		}
	}

	bool InSeismicDistance(const CUnit* unit, int allyTeam) {
		const int square = GetSquare(unit->pos);
		assert(static_cast<unsigned>(square) < seismicMaps[allyTeam].size());
		return !!seismicMaps[allyTeam][square];
	}

	bool circularRadar;

	std::vector<unsigned short> radarMaps[MAX_TEAMS];
	std::vector<unsigned short> airRadarMaps[MAX_TEAMS];
	std::vector<unsigned short> sonarMaps[MAX_TEAMS];
	std::vector<unsigned short> jammerMaps[MAX_TEAMS];
#ifdef SONAR_JAMMER_MAPS
	std::vector<unsigned short> sonarJammerMaps[MAX_TEAMS];
#endif
	std::vector<unsigned short> seismicMaps[MAX_TEAMS];
	std::vector<unsigned short> commonJammerMap;
	std::vector<unsigned short> commonSonarJammerMap;
	float radarErrorSize[MAX_TEAMS];
	float baseRadarErrorSize;

	int xsize;
	int zsize;

	float targFacEffect;

protected:
	void AddMapArea(int2 pos, int radius, std::vector<unsigned short>& map, int amount);

	void SafeLosRadarAdd(CUnit* unit);

private:
	void Serialize(creg::ISerializer& s);
};

extern CRadarHandler* radarhandler;

#endif /* RADARHANDLER_H */
