#ifndef RADARHANDLER_H
#define RADARHANDLER_H

#include <boost/noncopyable.hpp>

#include "Object.h"
#include "Sim/Misc/LosMap.h"
#include "Sim/Units/Unit.h"
#include <assert.h>

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
		const int gx = (int)(pos.x * invRadarDiv);
		const int gz = (int)(pos.z * invRadarDiv);
		const int rowIdx = std::max(0, std::min(zsize - 1, gz));
		const int colIdx = std::max(0, std::min(xsize - 1, gx));
		return (rowIdx * xsize) + colIdx;
	}

	bool InRadar(const float3& pos, int allyTeam) {
		const int square = GetSquare(pos);
		if (pos.y < -0.5f) {
			return (sonarMaps[allyTeam][square] && !commonSonarJammerMap[square]);
		}
		else if (circularRadar && (pos.y > 0.5f)) {
			return (airRadarMaps[allyTeam][square] && !commonJammerMap[square]);
		}
		else {
			return (radarMaps[allyTeam][square] && !commonJammerMap[square]);
		}
	}

	bool InRadar(const CUnit* unit, int allyTeam) {
		if (unit->isUnderWater) {
			if (unit->sonarStealth) {
				return false;
			}
			const int square = GetSquare(unit->pos);
			return !!sonarMaps[allyTeam][square] && !commonSonarJammerMap[square];
		}
		else if (circularRadar && unit->useAirLos) {
			if (unit->stealth) {
				return false;
			}
			const int square = GetSquare(unit->pos);
			return airRadarMaps[allyTeam][square] && !commonJammerMap[square];
		}
		else {
			const int square = GetSquare(unit->pos);
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
		return !!seismicMaps[allyTeam][square];
	}

	const int radarMipLevel;
	const int radarDiv;
	const float invRadarDiv;
	const bool circularRadar;

	std::vector<CLosMap> radarMaps;
	std::vector<CLosMap> airRadarMaps;
	std::vector<CLosMap> sonarMaps;
	std::vector<CLosMap> jammerMaps;
#ifdef SONAR_JAMMER_MAPS
	std::vector<CLosMap> sonarJammerMaps;
#endif
	std::vector<CLosMap> seismicMaps;
	CLosMap commonJammerMap;
	CLosMap commonSonarJammerMap;
	std::vector<float> radarErrorSize;
	float baseRadarErrorSize;

	int xsize;
	int zsize;

	float targFacEffect;

private:
	CLosAlgorithm radarAlgo;

	void Serialize(creg::ISerializer& s);
};

extern CRadarHandler* radarhandler;

#endif /* RADARHANDLER_H */
