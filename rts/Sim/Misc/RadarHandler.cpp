/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "RadarHandler.h"
#include "LosHandler.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/TeamHandler.h"
#include "System/TimeProfiler.h"

#ifdef RADARHANDLER_SONAR_JAMMER_MAPS
	#define SONAR_MAPS CR_MEMBER(sonarJammerMaps),
#else
	#define SONAR_MAPS
#endif

CR_BIND(CRadarHandler, (false))

CR_REG_METADATA(CRadarHandler, (
	CR_MEMBER(radarErrorSizes),
	CR_MEMBER(xsize),
	CR_MEMBER(zsize),
	CR_MEMBER(baseRadarErrorSize),
	CR_MEMBER(baseRadarErrorMult),

	CR_MEMBER(radarMipLevel),
	CR_MEMBER(radarDiv),
	CR_MEMBER(invRadarDiv),
	CR_MEMBER(circularRadar),
	CR_MEMBER(radarAlgo),

	CR_MEMBER(radarMaps),
	CR_MEMBER(airRadarMaps),
	CR_MEMBER(sonarMaps),
	CR_MEMBER(jammerMaps),
	SONAR_MAPS
	CR_MEMBER(seismicMaps),
	CR_MEMBER(commonJammerMap),
	CR_MEMBER(commonSonarJammerMap)
))


CRadarHandler* radarHandler = NULL;


CRadarHandler::CRadarHandler(bool circularRadar)
: radarMipLevel(3),
  radarDiv(SQUARE_SIZE * (1 << radarMipLevel)),
  invRadarDiv(1.0f / radarDiv),
  circularRadar(circularRadar),
  xsize(std::max(1, mapDims.mapx >> radarMipLevel)),
  zsize(std::max(1, mapDims.mapy >> radarMipLevel)),
  radarAlgo(int2(xsize, zsize), -1000, 20, readMap->GetMIPHeightMapSynced(radarMipLevel)),
  baseRadarErrorSize(96.0f),
  baseRadarErrorMult(2.0f)
{
	commonJammerMap.SetSize(xsize, zsize, false);
	commonSonarJammerMap.SetSize(xsize, zsize, false);

	CLosMap tmp;
	tmp.SetSize(xsize, zsize, false);
	radarMaps.resize(teamHandler->ActiveAllyTeams(), tmp);
	sonarMaps.resize(teamHandler->ActiveAllyTeams(), tmp);
	seismicMaps.resize(teamHandler->ActiveAllyTeams(), tmp);
	airRadarMaps.resize(teamHandler->ActiveAllyTeams(), tmp);
	jammerMaps.resize(teamHandler->ActiveAllyTeams(), tmp);
#ifdef RADARHANDLER_SONAR_JAMMER_MAPS
	sonarJammerMaps.resize(teamHandler->ActiveAllyTeams(), tmp);
#endif
	radarErrorSizes.resize(teamHandler->ActiveAllyTeams(), baseRadarErrorSize);
}


CRadarHandler::~CRadarHandler()
{
}


// TODO: add the LosHandler optimizations (instance-sharing)
void CRadarHandler::MoveUnit(CUnit* unit)
{
	if (gs->globalLOS[unit->allyteam])
		return;
	if (!unit->hasRadarCapacity)
		return;
	// NOTE:
	//   when stunned, we are not called during Unit::SlowUpdate's
	//   but units can in principle still be given on/off commands
	//   this creates an exploit via Unit::Activate if the unit is
	//   a transported radar/jammer and leaves a detached coverage
	//   zone behind
	if (!unit->activated || unit->IsStunned())
		return;

	int2 newPos;
	newPos.x = (int) (unit->pos.x * invRadarDiv);
	newPos.y = (int) (unit->pos.z * invRadarDiv);

	if (!unit->hasRadarPos || (newPos != unit->oldRadarPos)) {
		RemoveUnit(unit);

		if (unit->jammerRadius) {
			jammerMaps[unit->allyteam].AddMapArea(newPos, -123, unit->jammerRadius, 1);
			commonJammerMap.AddMapArea(newPos, -123, unit->jammerRadius, 1);
		}
		if (unit->sonarJamRadius) {
#ifdef RADARHANDLER_SONAR_JAMMER_MAPS
			sonarJammerMaps[unit->allyteam].AddMapArea(newPos, -123, unit->sonarJamRadius, 1);
#endif
			commonSonarJammerMap.AddMapArea(newPos, -123, unit->sonarJamRadius, 1);
		}
		if (unit->radarRadius) {
			airRadarMaps[unit->allyteam].AddMapArea(newPos, -123, unit->radarRadius, 1);
			if (!circularRadar) {
				radarAlgo.LosAdd(newPos, unit->radarRadius, unit->radarHeight, unit->radarSquares);
				radarMaps[unit->allyteam].AddMapSquares(unit->radarSquares, -123, 1);
			}
		}
		if (unit->sonarRadius) {
			sonarMaps[unit->allyteam].AddMapArea(newPos, -123, unit->sonarRadius, 1);
		}
		if (unit->seismicRadius) {
			seismicMaps[unit->allyteam].AddMapArea(newPos, -123, unit->seismicRadius, 1);
		}
		unit->oldRadarPos = newPos;
		unit->hasRadarPos = true;
	}
}


void CRadarHandler::RemoveUnit(CUnit* unit)
{
	if (!unit->hasRadarCapacity) {
		return;
	}

	if (unit->hasRadarPos) {
		if (unit->jammerRadius) {
			jammerMaps[unit->allyteam].AddMapArea(unit->oldRadarPos, -123, unit->jammerRadius, -1);
			commonJammerMap.AddMapArea(unit->oldRadarPos, -123, unit->jammerRadius, -1);
		}
		if (unit->sonarJamRadius) {
#ifdef RADARHANDLER_SONAR_JAMMER_MAPS
			sonarJammerMaps[unit->allyteam].AddMapArea(unit->oldRadarPos, -123, unit->sonarJamRadius, -1);
#endif
			commonSonarJammerMap.AddMapArea(unit->oldRadarPos, -123, unit->sonarJamRadius, -1);
		}
		if (unit->radarRadius) {
			airRadarMaps[unit->allyteam].AddMapArea(unit->oldRadarPos, -123, unit->radarRadius, -1);
			if (!circularRadar) {
				radarMaps[unit->allyteam].AddMapSquares(unit->radarSquares, -123, -1);
				unit->radarSquares.clear();
			}
		}
		if (unit->sonarRadius) {
			sonarMaps[unit->allyteam].AddMapArea(unit->oldRadarPos, -123, unit->sonarRadius, -1);
		}
		if (unit->seismicRadius) {
			seismicMaps[unit->allyteam].AddMapArea(unit->oldRadarPos, -123, unit->seismicRadius, -1);
		}
		unit->hasRadarPos = false;
	}
}


bool CRadarHandler::InRadar(const float3 pos, int allyTeam) const
{
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


bool CRadarHandler::InRadar(const CUnit* unit, int allyTeam) const
{
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


bool CRadarHandler::InJammer(const float3 pos, int allyTeam) const
{
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
		#ifdef RADARHANDLER_SONAR_JAMMER_MAPS
		ret = (sonarJammerMaps[allyTeam][square] != 0);
		#endif
	} else {
		ret = (jammerMaps[allyTeam][square] != 0);
	}
	#endif

	return ret;
}
