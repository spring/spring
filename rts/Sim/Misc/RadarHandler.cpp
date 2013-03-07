/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "RadarHandler.h"
#include "LosHandler.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/TeamHandler.h"
#include "System/TimeProfiler.h"

#ifdef SONAR_JAMMER_MAPS
	#define SONAR_MAPS CR_MEMBER(sonarJammerMaps),
#else
	#define SONAR_MAPS
#endif

CR_BIND(CRadarHandler, (false));

CR_REG_METADATA(CRadarHandler, (
	CR_MEMBER(radarErrorSize),
	CR_MEMBER(baseRadarErrorSize),
	CR_MEMBER(xsize),
	CR_MEMBER(zsize),
	CR_MEMBER(targFacEffect),

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
));


CRadarHandler* radarhandler = NULL;


CRadarHandler::CRadarHandler(bool circularRadar)
: radarMipLevel(3),
  radarDiv(SQUARE_SIZE * (1 << radarMipLevel)),
  invRadarDiv(1.0f / radarDiv),
  circularRadar(circularRadar),
  baseRadarErrorSize(96),
  xsize(std::max(1, gs->mapx >> radarMipLevel)),
  zsize(std::max(1, gs->mapy >> radarMipLevel)),
  targFacEffect(2),
  radarAlgo(int2(xsize, zsize), -1000, 20, readmap->GetMIPHeightMapSynced(radarMipLevel))
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
#ifdef SONAR_JAMMER_MAPS
	sonarJammerMaps.resize(teamHandler->ActiveAllyTeams(), tmp);
#endif
	radarErrorSize.resize(teamHandler->ActiveAllyTeams(), 96);
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

	if (!unit->hasRadarPos ||
		(newPos.x != unit->oldRadarPos.x) ||
	    (newPos.y != unit->oldRadarPos.y)) {
		RemoveUnit(unit);

		if (unit->jammerRadius) {
			jammerMaps[unit->allyteam].AddMapArea(newPos, -123, unit->jammerRadius, 1);
			commonJammerMap.AddMapArea(newPos, -123, unit->jammerRadius, 1);
		}
		if (unit->sonarJamRadius) {
#ifdef SONAR_JAMMER_MAPS
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
#ifdef SONAR_JAMMER_MAPS
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
