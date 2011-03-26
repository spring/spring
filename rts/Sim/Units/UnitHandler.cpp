/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include <cassert>
#include "mmgr.h"

#include "lib/gml/gml.h"
#include "UnitHandler.h"
#include "Unit.h"
#include "UnitDefHandler.h"
#include "CommandAI/BuilderCAI.h"
#include "CommandAI/Command.h"
#include "Game/GameSetup.h"
#include "Lua/LuaUnsyncedCtrl.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Misc/AirBaseHandler.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/MoveTypes/MoveType.h"
#include "System/EventHandler.h"
#include "System/EventBatchHandler.h"
#include "System/GlobalUnsynced.h"
#include "System/LogOutput.h"
#include "System/TimeProfiler.h"
#include "System/myMath.h"
#include "System/Sync/SyncTracer.h"
#include "System/creg/STL_Deque.h"
#include "System/creg/STL_List.h"
#include "System/creg/STL_Set.h"
using std::min;
using std::max;


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CUnitHandler* uh = NULL;

CR_BIND(CUnitHandler, );
CR_REG_METADATA(CUnitHandler, (
	CR_MEMBER(activeUnits),
	CR_MEMBER(units),
	CR_MEMBER(freeUnitIDs),
	CR_MEMBER(maxUnits),
	CR_MEMBER(maxUnitsPerTeam),
	CR_MEMBER(maxUnitRadius),
	CR_MEMBER(unitsToBeRemoved),
	CR_MEMBER(morphUnitToFeature),
	CR_MEMBER(builderCAIs),
	CR_MEMBER(unitsByDefs),
	CR_POSTLOAD(PostLoad),
	CR_SERIALIZER(Serialize)
));



void CUnitHandler::PostLoad()
{
	// reset any synced stuff that is not saved
	slowUpdateIterator = activeUnits.end();
}


CUnitHandler::CUnitHandler()
:
	maxUnitRadius(0.0f),
	morphUnitToFeature(true)
{
	assert(teamHandler->ActiveTeams() > 0);

	// note: the number of active teams can change at run-time, so
	// the team unit limit should be recalculated whenever one dies
	// or spawns (but would get complicated)
	maxUnits = std::min(gameSetup->maxUnits * teamHandler->ActiveTeams(), MAX_UNITS);
	maxUnitsPerTeam = maxUnits / teamHandler->ActiveTeams();

	// ensure each team can make at least one unit
	assert(maxUnits >= teamHandler->ActiveTeams());
	assert(maxUnitsPerTeam >= 1);

	units.resize(maxUnits, NULL);
	unitsByDefs.resize(teamHandler->ActiveTeams(), std::vector<CUnitSet>(unitDefHandler->unitDefs.size()));

	{
		std::vector<unsigned int> freeIDs(units.size());

		// id's are used as indices, so they must lie in [0, units.size() - 1]
		// (furthermore all id's are treated equally, none have special status)
		for (unsigned int id = 0; id < units.size(); id++) {
			freeIDs[id] = id;
		}

		// randomize the unit ID's so that Lua widgets can not
		// easily determine enemy unit counts from ID's alone
		// (shuffle twice for good measure)
		SyncedRNG rng;

		std::random_shuffle(freeIDs.begin(), freeIDs.end(), rng);
		std::random_shuffle(freeIDs.begin(), freeIDs.end(), rng);
		std::copy(freeIDs.begin(), freeIDs.end(), std::front_inserter(freeUnitIDs));
	}

	slowUpdateIterator = activeUnits.end();
	airBaseHandler = new CAirBaseHandler();
}


CUnitHandler::~CUnitHandler()
{
	for (std::list<CUnit*>::iterator usi = activeUnits.begin(); usi != activeUnits.end(); ++usi) {
		// ~CUnit dereferences featureHandler which is destroyed already
		(*usi)->delayedWreckLevel = -1;
		delete (*usi);
	}

	delete airBaseHandler;
}


bool CUnitHandler::AddUnit(CUnit *unit)
{
	if (freeUnitIDs.empty()) {
		// should be unreachable (all code that goes through
		// UnitLoader::LoadUnit --> Unit::PreInit checks the
		// unit limit first)
		assert(false);
		return false;
	}

	unit->id = freeUnitIDs.front();
	units[unit->id] = unit;

	std::list<CUnit*>::iterator ui = activeUnits.begin();

	if (ui != activeUnits.end()) {
		// randomize this to make the slow-update order random (good if one
		// builds say many buildings at once and then many mobile ones etc)
		const unsigned int insertionPos = gs->randFloat() * activeUnits.size();

		for (unsigned int n = 0; n < insertionPos; ++n) {
			++ui;
		}
	}

	activeUnits.insert(ui, unit);
	freeUnitIDs.pop_front();

	teamHandler->Team(unit->team)->AddUnit(unit, CTeam::AddBuilt);
	unitsByDefs[unit->team][unit->unitDef->id].insert(unit);

	maxUnitRadius = max(unit->radius, maxUnitRadius);
	return true;
}

void CUnitHandler::DeleteUnit(CUnit* unit)
{
	unitsToBeRemoved.push_back(unit);
	(eventBatchHandler->GetUnitCreatedDestroyedBatch()).dequeue_synced(unit);
}


void CUnitHandler::DeleteUnitNow(CUnit* delUnit)
{
#if defined(USE_GML) && GML_ENABLE_SIM
	{
		GML_STDMUTEX_LOCK(dque); // DeleteUnitNow

		LuaUnsyncedCtrl::drawCmdQueueUnits.erase(delUnit);
	}
#endif
	
	int delTeam = 0;
	int delType = 0;
	std::list<CUnit*>::iterator usi;
	for (usi = activeUnits.begin(); usi != activeUnits.end(); ++usi) {
		if (*usi == delUnit) {
			if (slowUpdateIterator != activeUnits.end() && *usi == *slowUpdateIterator) {
				++slowUpdateIterator;
			}
			delTeam = delUnit->team;
			delType = delUnit->unitDef->id;

			activeUnits.erase(usi);
			units[delUnit->id] = 0;
			freeUnitIDs.push_back(delUnit->id);
			teamHandler->Team(delTeam)->RemoveUnit(delUnit, CTeam::RemoveDied);

			unitsByDefs[delTeam][delType].erase(delUnit);

			delete delUnit;
			break;
		}
	}

#ifdef _DEBUG
	for (usi = activeUnits.begin(); usi != activeUnits.end(); /* no post-op */) {
		if (*usi == delUnit) {
			logOutput.Print("Error: Duplicated unit found in active units on erase");
			usi = activeUnits.erase(usi);
		} else {
			++usi;
		}
	}
#endif
}


void CUnitHandler::Update()
{
	{
		GML_STDMUTEX_LOCK(runit); // Update

		if (!unitsToBeRemoved.empty()) {
			GML_RECMUTEX_LOCK(unit); // Update - for anti-deadlock purposes.
			GML_RECMUTEX_LOCK(sel);  // Update - unit is removed from selectedUnits in ~CObject, which is too late.
			GML_RECMUTEX_LOCK(quad); // Update - make sure unit does not get partially deleted before before being removed from the quadfield
			GML_STDMUTEX_LOCK(proj); // Update - projectile drawing may access owner() and lead to crash

			eventHandler.DeleteSyncedUnits();

			while (!unitsToBeRemoved.empty()) {
				CUnit* delUnit = unitsToBeRemoved.back();
				unitsToBeRemoved.pop_back();

				DeleteUnitNow(delUnit);
			}
		}

		eventHandler.UpdateUnits();
	}

	GML_UPDATE_TICKS();

	{
		SCOPED_TIMER("Unit::MoveType::Update");
		std::list<CUnit*>::iterator usi;
		for (usi = activeUnits.begin(); usi != activeUnits.end(); ++usi) {
			CUnit* unit = *usi;
			AMoveType* moveType = unit->moveType;

			if (moveType->Update()) {
				eventHandler.UnitMoved(unit);
			}

			GML_GET_TICKS(unit->lastUnitUpdate);
		}
	}

	{
		SCOPED_TIMER("Unit::Update");
		std::list<CUnit*>::iterator usi;
		for (usi = activeUnits.begin(); usi != activeUnits.end(); ++usi) {
			CUnit* unit = *usi;

			if (unit->deathScriptFinished) {
				// there are many ways to fiddle with "deathScriptFinished", so a unit may
				// arrive here without having been properly killed (and isDead still false),
				// which can result in MT deadlocking -- FIXME verify this
				// (KU returns early if isDead)
				unit->KillUnit(false, true, NULL);

				DeleteUnit(unit);
			} else {
				unit->Update();
			}
		}
	}

	{
		SCOPED_TIMER("Unit::SlowUpdate");
		if (!(gs->frameNum & (UNIT_SLOWUPDATE_RATE - 1))) {
			slowUpdateIterator = activeUnits.begin();
		}

		int n = (activeUnits.size() / UNIT_SLOWUPDATE_RATE) + 1;
		for (; slowUpdateIterator != activeUnits.end() && n != 0; ++ slowUpdateIterator) {
			(*slowUpdateIterator)->SlowUpdate(); n--;
		}
	} // for timer destruction
}


float CUnitHandler::GetBuildHeight(const float3& pos, const UnitDef* unitdef)
{
	float minh = -5000.0f;
	float maxh =  5000.0f;
	int numBorder=0;
	float borderh=0;
	const float* heightmap=readmap->GetHeightmap();

	int xsize=1;
	int zsize=1;

	float maxDif=unitdef->maxHeightDif;
	int x1 = (int)max(0.f,(pos.x-(xsize*0.5f*SQUARE_SIZE))/SQUARE_SIZE);
	int x2 = min(gs->mapx,x1+xsize);
	int z1 = (int)max(0.f,(pos.z-(zsize*0.5f*SQUARE_SIZE))/SQUARE_SIZE);
	int z2 = min(gs->mapy,z1+zsize);

	if (x1 > gs->mapx) x1 = gs->mapx;
	if (x2 < 0) x2 = 0;
	if (z1 > gs->mapy) z1 = gs->mapy;
	if (z2 < 0) z2 = 0;

	for(int x=x1; x<=x2; x++){
		for(int z=z1; z<=z2; z++){
			float orgh=readmap->orgheightmap[z*(gs->mapx+1)+x];
			float h=heightmap[z*(gs->mapx+1)+x];
			if(x==x1 || x==x2 || z==z1 || z==z2){
				numBorder++;
				borderh+=h;
			}
			if(minh<min(h,orgh)-maxDif)
				minh=min(h,orgh)-maxDif;
			if(maxh>max(h,orgh)+maxDif)
				maxh=max(h,orgh)+maxDif;
		}
	}
	float h=borderh/numBorder;

	if(h<minh && minh<maxh)
		h=minh+0.01f;
	if(h>maxh && maxh>minh)
		h=maxh-0.01f;

	return h;
}


int CUnitHandler::TestUnitBuildSquare(
	const BuildInfo& buildInfo,
	CFeature*& feature,
	int allyteam,
	std::vector<float3>* canbuildpos,
	std::vector<float3>* featurepos,
	std::vector<float3>* nobuildpos,
	const std::vector<Command>* commands)
{
	feature = NULL;

	const int xsize = buildInfo.GetXSize();
	const int zsize = buildInfo.GetZSize();
	const float3 pos = buildInfo.pos;

	const int x1 = (int) (pos.x - (xsize * 0.5f * SQUARE_SIZE));
	const int x2 = x1 + xsize * SQUARE_SIZE;
	const int z1 = (int) (pos.z - (zsize * 0.5f * SQUARE_SIZE));
	const int z2 = z1 + zsize * SQUARE_SIZE;
	const float h = GetBuildHeight(pos, buildInfo.def);

	int canBuild = 2;

	if (buildInfo.def->needGeo) {
		canBuild = 0;
		const std::vector<CFeature*> &features = qf->GetFeaturesExact(pos, max(xsize, zsize) * 6);

		for (std::vector<CFeature*>::const_iterator fi = features.begin(); fi != features.end(); ++fi) {
			if ((*fi)->def->geoThermal
				&& fabs((*fi)->pos.x - pos.x) < (xsize * 4 - 4)
				&& fabs((*fi)->pos.z - pos.z) < (zsize * 4 - 4)) {
				canBuild = 2;
				break;
			}
		}
	}

	if (commands != NULL) {
		//! unsynced code
		for (int x = x1; x < x2; x += SQUARE_SIZE) {
			for (int z = z1; z < z2; z += SQUARE_SIZE) {
				int tbs = TestBuildSquare(float3(x, pos.y, z), buildInfo.def, feature, gu->myAllyTeam);

				if (tbs) {
					std::vector<Command>::const_iterator ci = commands->begin();

					for (; ci != commands->end() && tbs; ++ci) {
						BuildInfo bc(*ci);

						if (std::max(bc.pos.x - x - SQUARE_SIZE, x - bc.pos.x) * 2 < bc.GetXSize() * SQUARE_SIZE &&
							std::max(bc.pos.z - z - SQUARE_SIZE, z - bc.pos.z) * 2 < bc.GetZSize() * SQUARE_SIZE) {
							tbs = 0;
						}
					}

					if (!tbs) {
						nobuildpos->push_back(float3(x, h, z));
						canBuild = 0;
					} else if (feature || tbs == 1) {
						featurepos->push_back(float3(x, h, z));
					} else {
						canbuildpos->push_back(float3(x, h, z));
					}

					canBuild = min(canBuild, tbs);
				} else {
					nobuildpos->push_back(float3(x, h, z));
					canBuild = 0;
				}
			}
		}
	} else {
		for (int x = x1; x < x2; x += SQUARE_SIZE) {
			for (int z = z1; z < z2; z += SQUARE_SIZE) {
				canBuild = min(canBuild, TestBuildSquare(float3(x, h, z), buildInfo.def, feature, allyteam));

				if (canBuild == 0) {
					return 0;
				}
			}
		}
	}

	return canBuild;
}


int CUnitHandler::TestBuildSquare(const float3& pos, const UnitDef* unitdef, CFeature*& feature, int allyteam)
{
	if (pos.x < 0 || pos.x >= gs->mapx * SQUARE_SIZE || pos.z < 0 || pos.z >= gs->mapy * SQUARE_SIZE) {
		return 0;
	}

	int ret = 2;
	int yardxpos = int(pos.x + 4) / SQUARE_SIZE;
	int yardypos = int(pos.z + 4) / SQUARE_SIZE;
	CSolidObject* s;

	if ((s = groundBlockingObjectMap->GroundBlocked(yardypos * gs->mapx + yardxpos))) {
		CFeature* f;
		if ((f = dynamic_cast<CFeature*>(s))) {
			if ((allyteam < 0) || f->IsInLosForAllyTeam(allyteam)) {
				if (!f->def->reclaimable) {
					return 0;
				}
				feature = f;
			}
		} else if (!dynamic_cast<CUnit*>(s) || (allyteam < 0) ||
			(((CUnit*) s)->losStatus[allyteam] & LOS_INLOS)) {
			if (s->immobile) {
				return 0;
			} else {
				ret = 1;
			}
		}
	}

	const float groundHeight = ground->GetHeightReal(pos.x, pos.z);

	if (!unitdef->floater || groundHeight > 0.0f) {
		// if we are capable of floating, only test local
		// height difference if terrain is above sea-level
		const float* heightmap = readmap->GetHeightmap();
		int x = (int) (pos.x / SQUARE_SIZE);
		int z = (int) (pos.z / SQUARE_SIZE);
		float orgh = readmap->orgheightmap[z * (gs->mapx + 1) + x];
		float h = heightmap[z * (gs->mapx + 1) + x];
		float hdif = unitdef->maxHeightDif;

		if (pos.y > orgh + hdif && pos.y > h + hdif) { return 0; }
		if (pos.y < orgh - hdif && pos.y < h - hdif) { return 0; }
	}

	if (!unitdef->IsAllowedTerrainHeight(groundHeight))
		ret = 0;

	return ret;
}



void CUnitHandler::AddBuilderCAI(CBuilderCAI* b)
{
	GML_STDMUTEX_LOCK(cai); // AddBuilderCAI

	builderCAIs.insert(builderCAIs.end(), b);
}

void CUnitHandler::RemoveBuilderCAI(CBuilderCAI* b)
{
	GML_STDMUTEX_LOCK(cai); // RemoveBuilderCAI

	ListErase<CBuilderCAI*>(builderCAIs, b);
}



/**
 * Returns a build Command that intersects the ray described by pos and dir from
 * the command queues of the units 'units' on team number 'team'.
 * @brief returns a build Command that intersects the ray described by pos and dir
 * @return the build Command, or a Command with id 0 if none is found
 */
Command CUnitHandler::GetBuildCommand(const float3& pos, const float3& dir) {
	float3 tempF1 = pos;

	GML_STDMUTEX_LOCK(cai); // GetBuildCommand

	CCommandQueue::iterator ci;
	for (std::list<CUnit*>::const_iterator ui = activeUnits.begin(); ui != activeUnits.end(); ++ui) {
		const CUnit* unit = *ui;

		if (unit->team != gu->myTeam) {
			continue;
		}

		ci = unit->commandAI->commandQue.begin();

		for (; ci != unit->commandAI->commandQue.end(); ++ci) {
			const Command& cmd = *ci;

			if (cmd.id < 0 && cmd.params.size() >= 3) {
				BuildInfo bi(cmd);
				tempF1 = pos + dir * ((bi.pos.y - pos.y) / dir.y) - bi.pos;

				if (bi.def && (bi.GetXSize() / 2) * SQUARE_SIZE > fabs(tempF1.x) && (bi.GetZSize() / 2) * SQUARE_SIZE > fabs(tempF1.z)) {
					return cmd;
				}
			}
		}
	}

	Command c;
	c.id = CMD_STOP;
	return c;
}


bool CUnitHandler::CanBuildUnit(const UnitDef* unitdef, int team)
{
	if (teamHandler->Team(team)->units.size() >= maxUnitsPerTeam) {
		return false;
	}
	if (unitsByDefs[team][unitdef->id].size() >= unitdef->maxThisUnit) {
		return false;
	}

	return true;
}
