#include "StdAfx.h"
#include "mmgr.h"

#include "RadarHandler.h"
#include "TimeProfiler.h"
#include "LosHandler.h"
#include "Rendering/UnitModels/3DOParser.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/TeamHandler.h"


CR_BIND(CRadarHandler, (false));

CR_REG_METADATA(CRadarHandler, (
	CR_SERIALIZER(Serialize),
	// radarMaps, airRadarMaps, sonarMaps, jammerMaps, sonarJammerMaps,
	// seismicMaps, commonJammerMap, commonSonarJammerMap
	//CR_MEMBER(circularRadar),
	CR_MEMBER(radarErrorSize),
	CR_MEMBER(baseRadarErrorSize),
	CR_MEMBER(xsize),
	CR_MEMBER(zsize),
	CR_MEMBER(targFacEffect),
	CR_RESERVED(32)
));


CRadarHandler* radarhandler = NULL;


void CRadarHandler::Serialize(creg::ISerializer& s)
{
	const int size = xsize*zsize*2;

	// NOTE This could be tricky if teamHandler is serialized after radarHandler.
	for (int a = 0; a < teamHandler->ActiveAllyTeams(); ++a) {
		s.Serialize(&radarMaps[a].front(), size);
		if (!circularRadar) {
			s.Serialize(&airRadarMaps[a].front(), size);
		}
		s.Serialize(&sonarMaps[a].front(), size);
		s.Serialize(&jammerMaps[a].front(), size);
#ifdef SONAR_JAMMER_MAPS
		s.Serialize(&sonarJammerMaps[a].front(), size);
#endif
		s.Serialize(&seismicMaps[a].front(), size);
	}
	s.Serialize(&commonJammerMap.front(), size);
	s.Serialize(&commonSonarJammerMap.front(), size);
}


CRadarHandler::CRadarHandler(bool circularRadar)
: radarMipLevel(3),
  radarDiv(SQUARE_SIZE * (1 << radarMipLevel)),
  invRadarDiv(1.0f / radarDiv),
  circularRadar(circularRadar),
  baseRadarErrorSize(96),
  xsize(std::max(1, gs->mapx >> radarMipLevel)),
  zsize(std::max(1, gs->mapy >> radarMipLevel)),
  targFacEffect(2),
  radarAlgo(int2(xsize, zsize), -1000, 20, readmap->mipHeightmap[radarMipLevel])
{
	commonJammerMap.SetSize(xsize, zsize);
	commonSonarJammerMap.SetSize(xsize, zsize);

	CLosMap tmp;
	tmp.SetSize(xsize, zsize);
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


//todo: add the optimizations that is in loshandler
void CRadarHandler::MoveUnit(CUnit* unit)
{
	int2 newPos;
	newPos.x = (int) (unit->pos.x * invRadarDiv);
	newPos.y = (int) (unit->pos.z * invRadarDiv);

	if (!unit->hasRadarPos ||
		(newPos.x != unit->oldRadarPos.x) ||
	    (newPos.y != unit->oldRadarPos.y)) {
		RemoveUnit(unit);
		SCOPED_TIMER("Radar");
		if (unit->jammerRadius) {
			jammerMaps[unit->allyteam].AddMapArea(newPos, unit->jammerRadius, 1);
			commonJammerMap.AddMapArea(newPos, unit->jammerRadius, 1);
		}
		if (unit->sonarJamRadius) {
#ifdef SONAR_JAMMER_MAPS
			sonarJammerMaps[unit->allyteam].AddMapArea(newPos, unit->sonarJamRadius, 1);
#endif
			commonSonarJammerMap.AddMapArea(newPos, unit->sonarJamRadius, 1);
		}
		if (unit->radarRadius) {
			airRadarMaps[unit->allyteam].AddMapArea(newPos, unit->radarRadius, 1);
			if (!circularRadar) {
				radarAlgo.LosAdd(newPos, unit->radarRadius, unit->model->height, unit->radarSquares);
				radarMaps[unit->allyteam].AddMapSquares(unit->radarSquares, 1);
			}
		}
		if (unit->sonarRadius) {
			sonarMaps[unit->allyteam].AddMapArea(newPos, unit->sonarRadius, 1);
		}
		if (unit->seismicRadius) {
			seismicMaps[unit->allyteam].AddMapArea(newPos, unit->seismicRadius, 1);
		}
		unit->oldRadarPos = newPos;
		unit->hasRadarPos = true;
	}
}


void CRadarHandler::RemoveUnit(CUnit* unit)
{
	SCOPED_TIMER("Radar");

	if (unit->hasRadarPos) {
		if (unit->jammerRadius) {
			jammerMaps[unit->allyteam].AddMapArea(unit->oldRadarPos, unit->jammerRadius, -1);
			commonJammerMap.AddMapArea(unit->oldRadarPos, unit->jammerRadius, -1);
		}
		if (unit->sonarJamRadius) {
#ifdef SONAR_JAMMER_MAPS
			sonarJammerMaps[unit->allyteam].AddMapArea(unit->oldRadarPos, unit->sonarJamRadius, -1);
#endif
			commonSonarJammerMap.AddMapArea(unit->oldRadarPos, unit->sonarJamRadius, -1);
		}
		if (unit->radarRadius) {
			airRadarMaps[unit->allyteam].AddMapArea(unit->oldRadarPos, unit->radarRadius, -1);
			if (!circularRadar) {
				radarMaps[unit->allyteam].AddMapSquares(unit->radarSquares, -1);
				unit->radarSquares.clear();
			}
		}
		if (unit->sonarRadius) {
			sonarMaps[unit->allyteam].AddMapArea(unit->oldRadarPos, unit->sonarRadius, -1);
		}
		if (unit->seismicRadius) {
			seismicMaps[unit->allyteam].AddMapArea(unit->oldRadarPos, unit->seismicRadius, -1);
		}
		unit->hasRadarPos = false;
	}
}
