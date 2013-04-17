/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef RADARHANDLER_H
#define RADARHANDLER_H

#include <boost/noncopyable.hpp>

#include "Sim/Misc/LosMap.h"
#include "Sim/Units/Unit.h"
#include "System/myMath.h"

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
		const int colIdx = Clamp(gx, 0, xsize - 1);
		const int rowIdx = Clamp(gz, 0, zsize - 1);
		return (rowIdx * xsize) + colIdx;
	}

	bool InRadar(const float3& pos, int allyTeam) const {
		const int square = GetSquare(pos);

		if (pos.y < 0.0f) {
			// position is underwater, only sonar can see it
			return (sonarMaps[allyTeam][square] != 0 && commonSonarJammerMap[square] == 0);
		}
		if (circularRadar) {
			// position is not in water, but height is irrelevant for this mode
			return (airRadarMaps[allyTeam][square] != 0 && commonJammerMap[square] == 0);
		}

		return (radarMaps[allyTeam][square] != 0 && commonJammerMap[square] == 0);
	}

	bool InRadar(const CUnit* unit, int allyTeam) const {
		const int square = GetSquare(unit->pos);

		if (unit->IsUnderWater()) {
			// unit is completely submerged, only sonar can see it
			if (unit->sonarStealth && !unit->beingBuilt) {
				return false;
			}

			return (sonarMaps[allyTeam][square] != 0 && commonSonarJammerMap[square] == 0);
		}
		if (circularRadar && unit->useAirLos) {
			// circular mode and unit is an aircraft (and currently not landed)
			if (unit->stealth && !unit->beingBuilt) {
				return false;
			}

			return (airRadarMaps[allyTeam][square] != 0 && commonJammerMap[square] == 0);
		}

		// (surface) units that are not completely submerged can potentially
		// be seen by both radar and sonar (by sonar iff the lowest point on
		// the model is still inside water)
		const bool radarVisible =
			(!unit->stealth || unit->beingBuilt) &&
			(radarMaps[allyTeam][square] != 0) &&
			(commonJammerMap[square] == 0);
		const bool sonarVisible =
			(unit->pos.y < 0.0f) &&
			(!unit->sonarStealth || unit->beingBuilt) &&
			(sonarMaps[allyTeam][square] != 0) &&
			(commonSonarJammerMap[square] == 0);

		return (radarVisible || sonarVisible);
	}


	// returns whether a square is being radar- or sonar-jammed
	// (even when the square is not in radar- or sonar-coverage)
	bool InJammer(const float3& pos, int allyTeam) const {
		const int square = GetSquare(pos);
		bool ret = false;

		#if 1
		if (pos.y < 0.0f) {
			ret = (commonSonarJammerMap[square] != 0);
		} else {
			ret = (commonJammerMap[square] != 0);
		}
		#else
		if (pos.y < 0.0f) {
			#ifdef SONAR_JAMMER_MAPS
			ret = (sonarJammerMaps[allyTeam][square] != 0);
			#endif
		} else {
			ret = (jammerMaps[allyTeam][square] != 0);
		}
		#endif

		return ret;
	}
	bool InJammer(const CUnit* unit, int allyTeam) const {
		return (InJammer(unit->pos, allyTeam));
	}


	bool InSeismicDistance(const CUnit* unit, int allyTeam) const {
		return (seismicMaps[allyTeam][GetSquare(unit->pos)] != 0);
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
