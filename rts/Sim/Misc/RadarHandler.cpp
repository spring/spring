#include "StdAfx.h"
#include "RadarHandler.h"
#include "TimeProfiler.h"
#include "LosHandler.h"
#include "Rendering/UnitModels/3DOParser.h"
#include "Map/ReadMap.h"
#include "mmgr.h"

CR_BIND(CRadarHandler, (false));

CR_REG_METADATA(CRadarHandler, (
	CR_SERIALIZER(Serialize),
	// radarMaps, airRadarMaps, sonarMaps, jammerMaps,
	// seismicMaps, commonJammerMap, commonSonarJammerMap
	CR_MEMBER(circularRadar),
	CR_MEMBER(radarErrorSize),
	CR_MEMBER(baseRadarErrorSize),
	CR_MEMBER(xsize),
	CR_MEMBER(ysize),
	CR_MEMBER(targFacEffect),
	CR_RESERVED(32)
));


void CRadarHandler::Serialize(creg::ISerializer& s)
{
	const int size = xsize*ysize*2;

	// NOTE This could be tricky if gs is serialized after radarHandler.
	for(int a = 0; a < gs->activeAllyTeams; ++a) {
		s.Serialize(&radarMaps[a].front(), size);
		if (!circularRadar)
			s.Serialize(&airRadarMaps[a].front(), size);
		s.Serialize(&sonarMaps[a].front(), size);
		s.Serialize(&jammerMaps[a].front(), size);
		s.Serialize(&seismicMaps[a].front(), size);
	}
	s.Serialize(&commonJammerMap.front(), size);
	s.Serialize(&commonSonarJammerMap.front(), size);
}


CRadarHandler* radarhandler;


CRadarHandler::CRadarHandler(bool circularRadar):
		circularRadar(circularRadar),
		baseRadarErrorSize(96),
		xsize(gs->mapx / RADAR_SIZE),
		ysize(gs->mapy / RADAR_SIZE),
		targFacEffect(2)
{
	commonJammerMap.resize(xsize*ysize,0);
	commonSonarJammerMap.resize(xsize*ysize,0);

	for(int a=0;a<gs->activeAllyTeams;++a){
		radarMaps[a].resize(xsize*ysize,0);
		sonarMaps[a].resize(xsize*ysize,0);
		seismicMaps[a].resize(xsize*ysize,0);
		airRadarMaps[a].resize(xsize*ysize,0);
		jammerMaps[a].resize(xsize*ysize,0);
		radarErrorSize[a]=96;
	}
}

CRadarHandler::~CRadarHandler()
{
}

//todo: add the optimizations that is in loshandler
void CRadarHandler::MoveUnit(CUnit* unit)
{
	int2 newPos;
	newPos.x=(int) (unit->pos.x/(SQUARE_SIZE*RADAR_SIZE));
	newPos.y=(int) (unit->pos.z/(SQUARE_SIZE*RADAR_SIZE));

	if(newPos.x!=unit->oldRadarPos.x || newPos.y!=unit->oldRadarPos.y){
		RemoveUnit(unit);
		SCOPED_TIMER("Radar");
		if(unit->jammerRadius){
			AddMapArea(newPos,unit->jammerRadius,jammerMaps[unit->allyteam],1);
			AddMapArea(newPos,unit->jammerRadius,commonJammerMap,1);
		}
		if(unit->sonarJamRadius){
			AddMapArea(newPos,unit->sonarJamRadius,commonSonarJammerMap,1);
		}
		if(unit->radarRadius){
			AddMapArea(newPos,unit->radarRadius,airRadarMaps[unit->allyteam],1);
			if(!circularRadar){
				SafeLosRadarAdd(unit);
			}
		}
		if(unit->sonarRadius){
			AddMapArea(newPos,unit->sonarRadius,sonarMaps[unit->allyteam],1);
		}
		if(unit->seismicRadius){
			AddMapArea(newPos,unit->seismicRadius,seismicMaps[unit->allyteam],1);
		}
		unit->oldRadarPos=newPos;
	}
}

void CRadarHandler::RemoveUnit(CUnit* unit)
{
	SCOPED_TIMER("Radar");

	if(unit->oldRadarPos.x>=0){
		if(unit->jammerRadius){
			AddMapArea(unit->oldRadarPos,unit->jammerRadius,jammerMaps[unit->allyteam],-1);
			AddMapArea(unit->oldRadarPos,unit->jammerRadius,commonJammerMap,-1);
		}
		if(unit->sonarJamRadius){
			AddMapArea(unit->oldRadarPos,unit->sonarJamRadius,commonSonarJammerMap,-1);
		}
		if(unit->radarRadius){
			AddMapArea(unit->oldRadarPos,unit->radarRadius,airRadarMaps[unit->allyteam],-1);
			if(!circularRadar){
				for(std::vector<int>::iterator ri=unit->radarSquares.begin();ri!=unit->radarSquares.end();++ri){
					--radarMaps[unit->allyteam][*ri];
				}
				unit->radarSquares.clear();
			}
		}
		if(unit->sonarRadius){
			AddMapArea(unit->oldRadarPos,unit->sonarRadius,sonarMaps[unit->allyteam],-1);
		}
		if(unit->seismicRadius){
			AddMapArea(unit->oldRadarPos,unit->seismicRadius,seismicMaps[unit->allyteam],-1);
		}
		unit->oldRadarPos.x=-1;
	}
}

void CRadarHandler::AddMapArea(int2 pos, int radius, std::vector<unsigned short>& map, int amount)
{
	int sx = std::max(0,pos.x-radius);
	int ex = std::min(xsize-1,pos.x+radius);
	int sy = std::max(0,pos.y-radius);
	int ey = std::min(ysize-1,pos.y+radius);

	int rr = radius * radius;
	for (int y = sy; y <= ey; ++y) {
		int rrx = rr - (pos.y - y) * (pos.y - y);
		for (int x = sx; x <= ex; ++x) {
			if ((pos.x - x) * (pos.x - x) <= rrx) {
				map[y * xsize + x] += amount;
			}
		}
	}
}

void CRadarHandler::SafeLosRadarAdd(CUnit* unit)
{
	float3 pos=unit->pos;
	pos.CheckInBounds();
	int xradar=(int) (pos.x/(SQUARE_SIZE*RADAR_SIZE));
	int yradar=(int) (pos.z/(SQUARE_SIZE*RADAR_SIZE));
	int xmap=(int) (pos.x/(SQUARE_SIZE*2));
	int ymap=(int) (pos.z/(SQUARE_SIZE*2));
	int allyteam=unit->allyteam;

	int tablenum=unit->radarRadius;
	if(tablenum>MAX_LOS_TABLE){
		tablenum=MAX_LOS_TABLE;
	}
	CLosHandler::LosTable& table=loshandler->lostables[tablenum-1];

	CLosHandler::LosTable::iterator li;
	for(li=table.begin();li!=table.end();++li){
		CLosHandler::LosLine& line=*li;
		CLosHandler::LosLine::iterator linei;
		float baseHeight=pos.y+unit->model->height-20;
		float maxAng1=-1000;
		float maxAng2=-1000;
		float maxAng3=-1000;
		float maxAng4=-1000;
		float r=1;
		unit->radarSquares.push_back(yradar*xsize+xradar);
		radarMaps[allyteam][yradar*xsize+xradar]++;

		for(linei=line.begin();linei!=line.end();++linei){
			if(xradar+linei->x < xsize && yradar+linei->y < ysize){
				int rsquare=(xradar+linei->x)+(yradar+linei->y)*xsize;
				int msquare=(xmap+linei->x*4)+(ymap+linei->y*4)*gs->hmapx;
				float dh=readmap->mipHeightmap[1][msquare]-baseHeight;
				float ang=dh/r;
				if(ang>maxAng1){
					unit->radarSquares.push_back(rsquare);
					radarMaps[allyteam][rsquare]++;
				}
				dh-=20;
				ang=dh/r;
				if(ang>maxAng1){
					maxAng1=ang;
				}
			}
			if(xradar-linei->x >= 0 && yradar-linei->y >= 0){
				int rsquare=(xradar-linei->x)+(yradar-linei->y)*xsize;
				int msquare=(xmap-linei->x*4)+(ymap-linei->y*4)*gs->hmapx;
				float dh=readmap->mipHeightmap[1][msquare]-baseHeight;
				float ang=dh/r;
				if(ang>maxAng2){
					unit->radarSquares.push_back(rsquare);
					radarMaps[allyteam][rsquare]++;
				}
				dh-=20;
				ang=dh/r;
				if(ang>maxAng2){
					maxAng2=ang;
				}
			}
			if(xradar+linei->y < xsize && yradar-linei->x >= 0){
				int rsquare=(xradar+linei->y)+(yradar-linei->x)*xsize;
				int msquare=(xmap+linei->y*4)+(ymap-linei->x*4)*gs->hmapx;
				float dh=readmap->mipHeightmap[1][msquare]-baseHeight;
				float ang=dh/r;
				if(ang>maxAng3){
					unit->radarSquares.push_back(rsquare);
					radarMaps[allyteam][rsquare]++;
				}
				dh-=20;
				ang=dh/r;
				if(ang>maxAng3){
					maxAng3=ang;
				}
			}
			if(xradar-linei->y >= 0 && yradar+linei->x < ysize){
				int rsquare=(xradar-linei->y)+(yradar+linei->x)*xsize;
				int msquare=(xmap-linei->y*4)+(ymap+linei->x*4)*gs->hmapx;
				float dh=readmap->mipHeightmap[1][msquare]-baseHeight;
				float ang=dh/r;
				if(ang>maxAng4){
					unit->radarSquares.push_back(rsquare);
					radarMaps[allyteam][rsquare]++;
				}
				dh-=20;
				ang=dh/r;
				if(ang>maxAng4){
					maxAng4=ang;
				}
			}
			r++;
		}
	}
}
