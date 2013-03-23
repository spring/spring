/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef RADARHANDLER_H
#define RADARHANDLER_H

#include <boost/noncopyable.hpp>

#include "System/Object.h"
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
	CR_DECLARE_STRUCT(CRadarHandler);


public:
	CRadarHandler(bool circularRadar);
	~CRadarHandler();

	void MoveUnit(CUnit* unit);
	void RemoveUnit(CUnit* unit);

	inline int GetSquare(const float3& pos) const
	{
		const int gx = pos.x * invRadarDiv;
		const int gz = pos.z * invRadarDiv;
		const int rowIdx = std::max(0, std::min(zsize - 1, gz));
		const int colIdx = std::max(0, std::min(xsize - 1, gx));
		return (rowIdx * xsize) + colIdx;
	}

	bool InRadar(const float3& pos, int allyTeam) const {
		const int square = GetSquare(pos);

		if (pos.y < 0.0f) {
			// position is underwater, only sonar can see it
			return (sonarMaps[allyTeam][square] && !commonSonarJammerMap[square]);
		}
		if (circularRadar) {
			// position is not in water, but height is irrelevant for this mode
			return (airRadarMaps[allyTeam][square] && !commonJammerMap[square]);
		}

		return (radarMaps[allyTeam][square] && !commonJammerMap[square]);
	}

	bool InRadar(const CUnit* unit, int allyTeam) const {
		const int square = GetSquare(unit->pos);

		if (unit->isUnderWater) {
			// unit is completely submerged, only sonar can see it
			if (unit->sonarStealth && !unit->beingBuilt) {
				return false;
			}

			return (!!sonarMaps[allyTeam][square] && !commonSonarJammerMap[square]);
		}
		if (circularRadar && unit->useAirLos) {
			// circular mode and unit is an aircraft (and currently not landed)
			if (unit->stealth && !unit->beingBuilt) {
				return false;
			}

			return (airRadarMaps[allyTeam][square] && !commonJammerMap[square]);
		}

		// (surface) units that are not completely submerged can potentially
		// be seen by both radar and sonar (by sonar iff the lowest point on
		// the model is still inside water)
		const bool radarVisible =
			(!unit->stealth || unit->beingBuilt) &&
			radarMaps[allyTeam][square] &&
			!commonJammerMap[square];
		const bool sonarVisible =
			(unit->pos.y < 0.0f) &&
			(!unit->sonarStealth || unit->beingBuilt) &&
			sonarMaps[allyTeam][square] &&
			!commonSonarJammerMap[square];

		return (radarVisible || sonarVisible);
	}

	bool InSeismicDistance(const CUnit* unit, int allyTeam) const {
		const int square = GetSquare(unit->pos);
		return !!seismicMaps[allyTeam][square];
	}

public:
	int radarMipLevel;
	int radarDiv;
	float invRadarDiv;
	bool circularRadar;

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
};

extern CRadarHandler* radarhandler;

#endif /* RADARHANDLER_H */
