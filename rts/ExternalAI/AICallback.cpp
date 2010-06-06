/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ExternalAI/AICallback.h"

#include "StdAfx.h"
#include "FileSystem/FileHandler.h"
#include "FileSystem/FileSystem.h"
#include "FileSystem/FileSystemHandler.h"
#include "Game/Camera/CameraController.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/GameHelper.h"
#include "Game/GameSetup.h"
#include "Game/PlayerHandler.h"
#include "Game/SelectedUnits.h"
#include "Sim/Misc/TeamHandler.h"
#include "Game/UI/MiniMap.h"
#include "Game/UI/MouseHandler.h"
#include "Lua/LuaRules.h"
#include "Map/MapInfo.h"
#include "Map/MetalMap.h"
#include "Map/ReadMap.h"
#include "NetProtocol.h"
#include "ConfigHandler.h"
#include "Platform/errorhandler.h"
#include "Rendering/DebugDrawerAI.h"
#include "Rendering/InMapDraw.h"
#include "Rendering/Models/3DModel.h"
#include "Rendering/UnitDrawer.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/DamageArrayHandler.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Misc/GeometricObjects.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/Wind.h"
#include "Sim/Path/PathManager.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/CommandAI/CommandQueue.h"
#include "Sim/Units/CommandAI/LineDrawer.h"
#include "Sim/Units/UnitTypes/Factory.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Weapons/Weapon.h"
#include "ExternalAI/AICheats.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include "ExternalAI/SkirmishAIWrapper.h"
#include "ExternalAI/EngineOutHandler.h"
#include "Sim/Units/Groups/Group.h"
#include "Sim/Units/Groups/GroupHandler.h"
#include "LogOutput.h"
#include "mmgr.h"

// Cast id to unsigned to catch negative ids in the same operations,
// cast MAX_* to unsigned to suppress GCC comparison between signed/unsigned warning.
#define CHECK_UNITID(id) ((unsigned)(id) < (unsigned)uh->MaxUnits())
#define CHECK_GROUPID(id) ((unsigned)(id) < (unsigned)gh->groups.size())
// With some hacking you can raise an abort (assert) instead of ignoring the id,
//#define CHECK_UNITID(id) (assert(id > 0 && id < uh->MaxUnits()), true)
// ...or disable the check altogether for release.
//#define CHECK_UNITID(id) true

CAICallback::CAICallback(int teamId)
	: team(teamId)
	, noMessages(false)
	, gh(grouphandlers[teamId])
{}

CAICallback::~CAICallback(void)
{}

void CAICallback::SendStartPos(bool ready, float3 startPos)
{
	if (startPos.z < gameSetup->allyStartingData[gu->myAllyTeam].startRectTop * gs->mapy * SQUARE_SIZE)
		startPos.z = gameSetup->allyStartingData[gu->myAllyTeam].startRectTop * gs->mapy * SQUARE_SIZE;

	if (startPos.z > gameSetup->allyStartingData[gu->myAllyTeam].startRectBottom * gs->mapy * SQUARE_SIZE)
		startPos.z = gameSetup->allyStartingData[gu->myAllyTeam].startRectBottom * gs->mapy * SQUARE_SIZE;

	if (startPos.x < gameSetup->allyStartingData[gu->myAllyTeam].startRectLeft * gs->mapx * SQUARE_SIZE)
		startPos.x = gameSetup->allyStartingData[gu->myAllyTeam].startRectLeft * gs->mapx * SQUARE_SIZE;

	if (startPos.x > gameSetup->allyStartingData[gu->myAllyTeam].startRectRight * gs->mapx * SQUARE_SIZE)
		startPos.x = gameSetup->allyStartingData[gu->myAllyTeam].startRectRight * gs->mapx * SQUARE_SIZE;

	unsigned char readyness = ready? 1: 0;
	net->Send(CBaseNetProtocol::Get().SendStartPos(gu->myPlayerNum, team, readyness, startPos.x, startPos.y, startPos.z));
}

void CAICallback::SendTextMsg(const char* text, int zone)
{
	const CSkirmishAIHandler::ids_t& teamAIs = skirmishAIHandler.GetSkirmishAIsInTeam(this->team);
	const SkirmishAIData* aiData = skirmishAIHandler.GetSkirmishAI(*(teamAIs.begin())); // better way?

	logOutput.Print("<SkirmishAI: %s %s (team %d)>: %s", aiData->shortName.c_str(), aiData->version.c_str(), team, text);
}

void CAICallback::SetLastMsgPos(float3 pos)
{
	logOutput.SetLastMsgPos(pos);
}

void CAICallback::AddNotification(float3 pos, float3 color, float alpha)
{
	minimap->AddNotification(pos,color,alpha);
}



bool CAICallback::SendResources(float mAmount, float eAmount, int receivingTeamId)
{
	typedef unsigned char ubyte;
	bool ret = false;

	if (team != receivingTeamId) {
		if (receivingTeamId >= 0 && receivingTeamId < teamHandler->ActiveTeams()) {
			if (teamHandler->Team(receivingTeamId) && teamHandler->Team(team)) {
				if (!teamHandler->Team(receivingTeamId)->isDead && !teamHandler->Team(team)->isDead) {
					// note: we can't use the existing SendShare()
					// since its handler in CGame uses myPlayerNum
					// (NETMSG_SHARE param) to determine which team
					// the resources came from, which is not always
					// our AI team
					ret = true;

					// cap the amounts to how much M and E we have
					mAmount = std::max(0.0f, std::min(mAmount, GetMetal()));
					eAmount = std::max(0.0f, std::min(eAmount, GetEnergy()));
					std::vector<short> empty;

					net->Send(CBaseNetProtocol::Get().SendAIShare(ubyte(gu->myPlayerNum), ubyte(team), ubyte(receivingTeamId), mAmount, eAmount, empty));
				}
			}
		}
	}

	return ret;
}

int CAICallback::SendUnits(const std::vector<int>& unitIds, int receivingTeamId)
{
	typedef unsigned char ubyte;
	std::vector<short> sentUnitIDs;

	if (team != receivingTeamId) {
		if (receivingTeamId >= 0 && receivingTeamId < teamHandler->ActiveTeams()) {
			if (teamHandler->Team(receivingTeamId) && teamHandler->Team(team)) {
				if (!teamHandler->Team(receivingTeamId)->isDead && !teamHandler->Team(team)->isDead) {
					// we must iterate over the ID's to check if
					// all of them really belong to the AI's team
					for (std::vector<int>::const_iterator it = unitIds.begin(); it != unitIds.end(); it++ ) {
						const int unitID = *it;

						if (unitID > 0 && (size_t)unitID < uh->MaxUnits()) {
							const CUnit* unit = uh->units[unitID];

							if (unit && unit->team == team) {
								// we own this unit, save it (note: safe cast
								// since MAX_UNITS currently fits in a short)
								sentUnitIDs.push_back(short(unitID));

								// stop whatever this unit is doing
								Command c;
								c.id = CMD_STOP;
								GiveOrder(unitID, &c);
							}
						}
					}

					if (sentUnitIDs.size() > 0) {
						// we can't use SendShare() here either, since
						// AI's don't have a notion of "selected units"
						net->Send(CBaseNetProtocol::Get().SendAIShare(ubyte(gu->myPlayerNum), ubyte(team), ubyte(receivingTeamId), 0.0f, 0.0f, sentUnitIDs));
					}
				}
			}
		}
	}

	// return how many units were actually put up for transfer
	return (sentUnitIDs.size());
}



bool CAICallback::PosInCamera(float3 pos, float radius)
{
	return camera->InView(pos,radius);
}

// see if the AI hasn't modified any parts of this callback
// (still completely insecure ofcourse, but it filters out the easiest way of cheating)
void CAICallback::verify()
{
	//TODO: add checks
}

int CAICallback::GetCurrentFrame()
{
	return gs->frameNum;
}

int CAICallback::GetMyTeam()
{
	return team;
}

int CAICallback::GetMyAllyTeam()
{
	return teamHandler->AllyTeam(team);
}

int CAICallback::GetPlayerTeam(int playerId)
{
	CPlayer* pl = playerHandler->Player(playerId);
	if (pl->spectator)
		return -1;
	return pl->team;
}

const char* CAICallback::GetTeamSide(int teamId)
{
	if (teamId < teamHandler->ActiveTeams()) {
		return (teamHandler->Team(teamId)->side.c_str());
	} else {
		return NULL;
	}
}

void* CAICallback::CreateSharedMemArea(char* name, int size)
{
	handleerror (0, "AI wants to use deprecated function \"CreateSharedMemArea\"",
				"Spring is closing:", MBF_OK | MBF_EXCL);
	return NULL;
}

void CAICallback::ReleasedSharedMemArea(char* name)
{
	handleerror (0, "AI wants to use deprecated function \"ReleasedSharedMemArea\"",
				"Spring is closing:", MBF_OK | MBF_EXCL);
}

int CAICallback::CreateGroup()
{
	const CGroup* g = gh->CreateNewGroup();
	return g->id;
}

void CAICallback::EraseGroup(int groupId)
{
	if (CHECK_GROUPID(groupId)) {
		if (gh->groups[groupId]) {
			gh->RemoveGroup(gh->groups[groupId]);
		}
	}
}

bool CAICallback::AddUnitToGroup(int unitId, int groupId)
{
	bool added = false;

	if (CHECK_UNITID(unitId) && CHECK_GROUPID(groupId)) {
		CUnit* u = uh->units[unitId];
		if (u && (u->team == team) && gh->groups[groupId]) {
			added = u->SetGroup(gh->groups[groupId]);
		}
	}

	return added;
}

bool CAICallback::RemoveUnitFromGroup(int unitId)
{
	bool removed = false;

	if (CHECK_UNITID(unitId)) {
		CUnit* u = uh->units[unitId];
		if (u && (u->team == team)) {
			u->SetGroup(0);
			removed = true;
		}
	}

	return removed;
}

int CAICallback::GetUnitGroup(int unitId)
{
	int groupId = -1;

	if (CHECK_UNITID(unitId)) {
		const CUnit * unit = uh->units[unitId];
		if (unit && (unit->team == team)) {
			const CGroup* g = unit->group;
			if (g) {
				groupId = g->id;
			}
		}
	}
	return groupId;
}

const std::vector<CommandDescription>* CAICallback::GetGroupCommands(int groupId)
{
	static std::vector<CommandDescription> tempcmds;
	return &tempcmds;
}

int CAICallback::GiveGroupOrder(int groupId, Command* c)
{
	return 0;
}

int CAICallback::GiveOrder(int unitId, Command* c)
{
	verify();

	if (!CHECK_UNITID(unitId) || c == NULL) {
		return -1;
	}

	if (noMessages) {
		return -2;
	}

	const CUnit * unit = uh->units[unitId];

	if (!unit) {
		return -3;
	}

	if (unit->team != team) {
		return -5;
	}

	net->Send(CBaseNetProtocol::Get().SendAICommand(gu->myPlayerNum, unitId, c->id, c->options, c->params));

	return 0;
}

const std::vector<CommandDescription>* CAICallback::GetUnitCommands(int unitId)
{
	const std::vector<CommandDescription>* unitCommands = NULL;

	if (CHECK_UNITID(unitId)) {
		const CUnit *unit = uh->units[unitId];
		if (unit && (unit->team == team)) {
			unitCommands = &unit->commandAI->possibleCommands;
		}
	}

	return unitCommands;
}

const CCommandQueue* CAICallback::GetCurrentUnitCommands(int unitId)
{
	const CCommandQueue* currentUnitCommands = NULL;

	if (CHECK_UNITID(unitId)) {
		const CUnit *unit = uh->units[unitId];
		if (unit && (unit->team == team)) {
			currentUnitCommands = &unit->commandAI->commandQue;
		}
	}

	return currentUnitCommands;
}

int CAICallback::GetUnitAiHint(int unitId)
{
	int aiHint = -1;

	verify();
	if (CHECK_UNITID(unitId)) {
		const CUnit* unit = uh->units[unitId];
		if (unit) {
			const int allyTeam = teamHandler->AllyTeam(team);
			if (teamHandler->Ally(unit->allyteam, allyTeam)) {
				aiHint = unit->aihint;
			} else if (unit->losStatus[allyTeam] & LOS_INLOS) {
				const UnitDef* unitDef  = unit->unitDef;
				const UnitDef* decoyDef = unitDef->decoyDef;
				if (decoyDef == NULL) {
					aiHint = unit->aihint;
				} else {
					aiHint = decoyDef->aihint;
				}
			}
		}
	}

	return aiHint;
}

int CAICallback::GetUnitTeam(int unitId)
{
	int unitTeam = -1;

	verify();
	if (CHECK_UNITID(unitId)) {
		const CUnit* unit = uh->units[unitId];
		if (unit && (unit->losStatus[teamHandler->AllyTeam(team)] & LOS_INLOS)){
			unitTeam = unit->team;
		}
	}

	return unitTeam;
}

int CAICallback::GetUnitAllyTeam(int unitId)
{
	int unitAllyTeam = -1;

	verify();
	if (CHECK_UNITID(unitId)) {
		const CUnit* unit = uh->units[unitId];
		if(unit && (unit->losStatus[teamHandler->AllyTeam(team)] & LOS_INLOS)){
			unitAllyTeam = unit->allyteam;
		}
	}

	return unitAllyTeam;
}

float CAICallback::GetUnitHealth(int unitId)
{
	float health = -1;

	verify();
	if (CHECK_UNITID(unitId)) {
		const CUnit* unit = uh->units[unitId];

		if (unit) {
			const int allyTeam = teamHandler->AllyTeam(team);

			if (teamHandler->Ally(unit->allyteam, allyTeam)) {
				health = unit->health;
			} else if (unit->losStatus[allyTeam] & LOS_INLOS) {
				const UnitDef* unitDef = unit->unitDef;
				const UnitDef* decoyDef = unitDef->decoyDef;

				if (decoyDef == NULL) {
					health = unit->health;
				} else {
					const float scale = (decoyDef->health / unitDef->health);
					health = unit->health * scale;
				}
			}
		}
	}

	return health;
}

float CAICallback::GetUnitMaxHealth(int unitId)
{
	float maxHealth = -1;

	verify();
	if (CHECK_UNITID(unitId)) {
		const CUnit* unit = uh->units[unitId];
		if (unit) {
			const int allyTeam = teamHandler->AllyTeam(team);
			if (teamHandler->Ally(unit->allyteam, allyTeam)) {
				maxHealth = unit->maxHealth;
			} else if (unit->losStatus[allyTeam] & LOS_INLOS) {
				const UnitDef* unitDef = unit->unitDef;
				const UnitDef* decoyDef = unitDef->decoyDef;
				if (decoyDef == NULL) {
					maxHealth = unit->maxHealth;
				} else {
					const float scale = (decoyDef->health / unitDef->health);
					maxHealth = unit->maxHealth * scale;
				}
			}
		}
	}

	return maxHealth;
}

float CAICallback::GetUnitSpeed(int unitId)
{
	float speed = 0;

	verify();
	if (CHECK_UNITID(unitId)) {
		const CUnit* unit = uh->units[unitId];
		if (unit) {
			const int allyTeam = teamHandler->AllyTeam(team);
			if (teamHandler->Ally(unit->allyteam, allyTeam)) {
				speed = unit->maxSpeed;
			} else if (unit->losStatus[allyTeam] & LOS_INLOS) {
				const UnitDef* unitDef = unit->unitDef;
				const UnitDef* decoyDef = unitDef->decoyDef;
				if (decoyDef == NULL) {
					speed = unitDef->speed;
				} else {
					speed = decoyDef->speed;
				}
			}
		}
	}

	return speed;
}

float CAICallback::GetUnitPower(int unitId)
{
	float power = -1;

	verify();
	if (CHECK_UNITID(unitId)) {
		const CUnit* unit = uh->units[unitId];
		if (unit) {
			const int allyTeam = teamHandler->AllyTeam(team);
			if (teamHandler->Ally(unit->allyteam, allyTeam)) {
				power = unit->power;
			} else if (unit->losStatus[allyTeam] & LOS_INLOS) {
				const UnitDef* unitDef = unit->unitDef;
				const UnitDef* decoyDef = unitDef->decoyDef;
				if (decoyDef == NULL) {
					power = unit->power;
				} else {
					const float scale = (decoyDef->power / unitDef->power);
					power = unit->power * scale;
				}
			}
		}
	}

	return power;
}

float CAICallback::GetUnitExperience(int unitId)
{
	float experience = -1;

	verify();
	if (CHECK_UNITID(unitId)) {
		const CUnit* unit = uh->units[unitId];
		if (unit && (unit->losStatus[teamHandler->AllyTeam(team)] & LOS_INLOS)) {
			experience = unit->experience;
		}
	}

	return experience;
}

float CAICallback::GetUnitMaxRange(int unitId)
{
	float maxRange = -1;

	verify();
	if (CHECK_UNITID(unitId)) {
		const CUnit* unit = uh->units[unitId];
		if (unit) {
			const int allyTeam = teamHandler->AllyTeam(team);
			if (teamHandler->Ally(unit->allyteam, allyTeam)) {
				maxRange = unit->maxRange;
			} else if (unit->losStatus[allyTeam] & LOS_INLOS) {
				const UnitDef* unitDef = unit->unitDef;
				const UnitDef* decoyDef = unitDef->decoyDef;
				if (decoyDef == NULL) {
					maxRange = unit->maxRange;
				} else {
					maxRange = decoyDef->maxWeaponRange;
				}
			}
		}
	}

	return maxRange;
}

const UnitDef* CAICallback::GetUnitDef(int unitId)
{
	const UnitDef* def = NULL;

	verify();
	if (CHECK_UNITID(unitId)) {
		const CUnit* unit = uh->units[unitId];
		if (unit) {
			const UnitDef* unitDef = unit->unitDef;
			const int allyTeam = teamHandler->AllyTeam(team);
			if (teamHandler->Ally(unit->allyteam, allyTeam)) {
				def = unitDef;
			}
			const unsigned short losStatus = unit->losStatus[allyTeam];
			const unsigned short prevMask = (LOS_PREVLOS | LOS_CONTRADAR);
			if (((losStatus & LOS_INLOS) != 0) ||
					((losStatus & prevMask) == prevMask)) {
				const UnitDef* decoyDef = unitDef->decoyDef;
				if (decoyDef == NULL) {
					def = unitDef;
				} else {
					def = decoyDef;
				}
			}
		}
	}

	return def;
}

const UnitDef* CAICallback::GetUnitDef(const char* unitName)
{
	return unitDefHandler->GetUnitDefByName(unitName);
}
const UnitDef* CAICallback::GetUnitDefById (int unitDefId)
{
	return unitDefHandler->GetUnitDefByID(unitDefId);
}



float3 CAICallback::GetUnitPos(int unitId)
{
	verify();
	if (CHECK_UNITID(unitId)) {
		const CUnit* unit = uh->units[unitId];
		if (unit && (unit->losStatus[teamHandler->AllyTeam(team)] & (LOS_INLOS | LOS_INRADAR))) {
			return helper->GetUnitErrorPos(unit,teamHandler->AllyTeam(team));
		}
	}
	return ZeroVector;
}

float3 CAICallback::GetUnitVelocity(int unitId)
{
	verify();
	if (CHECK_UNITID(unitId)) {
		const CUnit* unit = uh->units[unitId];
		if (unit && (unit->losStatus[teamHandler->AllyTeam(team)] & (LOS_INLOS | LOS_INRADAR))) {
			return unit->speed;
		}
	}
	return ZeroVector;
}



int CAICallback::GetBuildingFacing(int unitId) {

	int buildFacing = -1;

	verify();
	if (CHECK_UNITID(unitId)) {
		const CUnit* unit = uh->units[unitId];
		if(unit && (unit->losStatus[teamHandler->AllyTeam(team)] & LOS_INLOS)){
			buildFacing = unit->buildFacing;
		}
	}

	return buildFacing;
}

bool CAICallback::IsUnitCloaked(int unitId) {

	bool isCloaked = false;

	verify();
	if (CHECK_UNITID(unitId)) {
		const CUnit* unit = uh->units[unitId];
		if(unit && (unit->losStatus[teamHandler->AllyTeam(team)] & LOS_INLOS)){
			isCloaked = unit->isCloaked;
		}
	}

	return isCloaked;
}

bool CAICallback::IsUnitParalyzed(int unitId) {

	bool isParalyzed = false;

	verify();
	if (CHECK_UNITID(unitId)) {
		const CUnit* unit = uh->units[unitId];
		if (unit && (unit->losStatus[teamHandler->AllyTeam(team)] & LOS_INLOS)) {
			isParalyzed = unit->stunned;
		}
	}

	return isParalyzed;
}

bool CAICallback::IsUnitNeutral(int unitId) {

	bool isNeutral = false;

	verify();
	if (CHECK_UNITID(unitId)) {
		const CUnit* unit = uh->units[unitId];
		if (unit && (unit->losStatus[teamHandler->AllyTeam(team)] & LOS_INLOS)) {
			isNeutral = unit->IsNeutral();
		}
	}

	return isNeutral;
}

int CAICallback::InitPath(float3 start, float3 end, int pathType)
{
	assert(((size_t)pathType) < moveinfo->moveData.size());
	return pathManager->RequestPath(moveinfo->moveData.at(pathType), start, end, false);
}

float3 CAICallback::GetNextWaypoint(int pathId)
{
	return pathManager->NextWaypoint(pathId, ZeroVector, 0.0f, 0, 0, false);
}

void CAICallback::FreePath(int pathId)
{
	pathManager->DeletePath(pathId);
}

float CAICallback::GetPathLength(float3 start, float3 end, int pathType)
{
	const int pathID  = InitPath(start, end, pathType);
	float     pathLen = -1.0f;

	if (pathID == 0) {
		return pathLen;
	}

	std::vector<float3> points;
	std::vector<int>    lengths;

	pathManager->GetEstimatedPath(pathID, points, lengths);

	if (points.empty()) {
		return 0.0f;
	}

	// distance to first intermediate node
	pathLen = (points[0] - start).Length();

	// we don't care which path segment has
	// what resolution, just lump all points
	// together
	for (size_t i = 1; i < points.size(); i++) {
		pathLen += (points[i] - points[i - 1]).Length();
	}

	/*
	// this method does not work without a path-owner
	// TODO: add an alternate GPL() callback for this?
	bool      haveNextWP = true;
	float3    currWP     = start;
	float3    nextWP     = start;

	while (haveNextWP) {
		nextWP = GetNextWaypoint(pathID);

		if (nextWP.y == -2) {
			// next path node not yet known
			continue;
		}

		if (nextWP.y == -1) {
			if (nextWP.x >= 0.0f && nextWP.z >= 0.0f) {
				// end of path (nextWP == end)
				pathLen += (nextWP - currWP).Length2D();
			} else {
				// invalid path
				pathLen = -1.0f;
			}

			haveNextWP = false;
		} else {
			pathLen += (nextWP - currWP).Length2D();
			currWP   = nextWP;
		}
	}
	*/

	FreePath(pathID);
	return pathLen;
}




int CAICallback::GetEnemyUnits(int* unitIds, int unitIds_max)
{
	verify();

	std::list<CUnit*>::const_iterator ui;
	int a = 0;
	for (ui = uh->activeUnits.begin(); ui != uh->activeUnits.end(); ++ui) {
		const CUnit* u = *ui;

		if (!teamHandler->Ally(u->allyteam, teamHandler->AllyTeam(team)) &&
		    (u->losStatus[teamHandler->AllyTeam(team)] & LOS_INLOS)) {
			if (!IsUnitNeutral(u->id)) {
				unitIds[a++] = u->id;
				if (a >= unitIds_max) {
					break;
				}
			}
		}
	}

	return a;
}

int CAICallback::GetEnemyUnitsInRadarAndLos(int* unitIds, int unitIds_max)
{
	verify();

	std::list<CUnit*>::const_iterator ui;
	int a = 0;
	for (ui = uh->activeUnits.begin(); ui != uh->activeUnits.end(); ++ui) {
		const CUnit* u = *ui;

		if (!teamHandler->Ally(u->allyteam, teamHandler->AllyTeam(team))
				&& (u->losStatus[teamHandler->AllyTeam(team)] & (LOS_INLOS | LOS_INRADAR))) {
			if (!IsUnitNeutral(u->id)) {
				unitIds[a++] = u->id;
				if (a >= unitIds_max) {
					break;
				}
			}
		}
	}

	return a;
}

int CAICallback::GetEnemyUnits(int* unitIds, const float3& pos, float radius,
		int unitIds_max)
{
	verify();

	const std::vector<CUnit*> units = qf->GetUnitsExact(pos, radius);
	std::vector<CUnit*>::const_iterator ui;
	int a = 0;
	for (ui = units.begin(); ui != units.end(); ++ui) {
		const CUnit* u = *ui;

		if (!teamHandler->Ally(u->allyteam, teamHandler->AllyTeam(team))
				&& (u->losStatus[teamHandler->AllyTeam(team)] & LOS_INLOS)) {
			if (!IsUnitNeutral(u->id)) {
				unitIds[a++] = u->id;
				if (a >= unitIds_max) {
					break;
				}
			}
		}
	}

	return a;
}


int CAICallback::GetFriendlyUnits(int *unitIds, int unitIds_max)
{
	verify();

	std::list<CUnit*>::const_iterator ui;
	int a = 0;
	for (ui = uh->activeUnits.begin(); ui != uh->activeUnits.end(); ++ui) {
		const CUnit* u = *ui;

		if (teamHandler->Ally(u->allyteam, teamHandler->AllyTeam(team))) {
			// IsUnitNeutral does a LOS check, but inconsequential
			// since we can always see friendly units anyway
			if (!IsUnitNeutral(u->id)) {
				unitIds[a++] = u->id;
				if (a >= unitIds_max) {
					break;
				}
			}
		}
	}

	return a;
}

int CAICallback::GetFriendlyUnits(int *unitIds, const float3& pos, float radius,
		int unitIds_max)
{
	verify();

	std::vector<CUnit*> unit = qf->GetUnitsExact(pos, radius);
	std::vector<CUnit*>::const_iterator ui;
	int a = 0;
	for (ui = unit.begin(); ui != unit.end(); ++ui) {
		const CUnit* u = *ui;

		if (teamHandler->Ally(u->allyteam, teamHandler->AllyTeam(team))) {
			// IsUnitNeutral does a LOS check, but inconsequential
			// since we can always see friendly units anyway
			if (!IsUnitNeutral(u->id)) {
				unitIds[a++] = u->id;
				if (a >= unitIds_max) {
					break;
				}
			}
		}
	}

	return a;
}


int CAICallback::GetNeutralUnits(int* unitIds, int unitIds_max)
{
	verify();

	std::list<CUnit*>::const_iterator ui;
	int a = 0;
	for (ui = uh->activeUnits.begin(); ui != uh->activeUnits.end(); ++ui) {
		const CUnit* u = *ui;

		// IsUnitNeutral does the LOS check
		if (IsUnitNeutral(u->id)) {
			unitIds[a++] = u->id;
			if (a >= unitIds_max) {
				break;
			}
		}
	}

	return a;
}

int CAICallback::GetNeutralUnits(int* unitIds, const float3& pos, float radius, int unitIds_max)
{
	verify();

	std::vector<CUnit*> unit = qf->GetUnitsExact(pos, radius);
	std::vector<CUnit*>::const_iterator ui;
	int a = 0;
	for (ui = unit.begin(); ui != unit.end(); ++ui) {
		const CUnit* u = *ui;

		// IsUnitNeutral does the LOS check
		if (IsUnitNeutral(u->id)) {
			unitIds[a++] = u->id;
			if (a >= unitIds_max) {
				break;
			}
		}
	}

	return a;
}




int CAICallback::GetMapWidth()
{
	return gs->mapx;
}

int CAICallback::GetMapHeight()
{
	return gs->mapy;
}



float CAICallback::GetMaxMetal() const { return mapInfo->map.maxMetal; }
float CAICallback::GetExtractorRadius() const { return mapInfo->map.extractorRadius; }

float CAICallback::GetMinWind() const { return wind.GetMinWind(); }
float CAICallback::GetMaxWind() const { return wind.GetMaxWind(); }
float CAICallback::GetCurWind() const { return wind.GetCurrentStrength(); }

float CAICallback::GetTidalStrength() const { return mapInfo->map.tidalStrength; }
float CAICallback::GetGravity() const { return mapInfo->map.gravity; }


const float* CAICallback::GetHeightMap()
{
	return readmap->centerheightmap;
}

const float* CAICallback::GetCornersHeightMap()
{
	return readmap->GetHeightmap();
}

float CAICallback::GetMinHeight()
{
	return readmap->minheight;
}

float CAICallback::GetMaxHeight()
{
	return readmap->maxheight;
}

const float* CAICallback::GetSlopeMap()
{
	return readmap->slopemap;
}

const unsigned short* CAICallback::GetLosMap()
{
	return &loshandler->losMap[teamHandler->AllyTeam(team)].front();
}

const unsigned short* CAICallback::GetRadarMap()
{
	return &radarhandler->radarMaps[teamHandler->AllyTeam(team)].front();
}

const unsigned short* CAICallback::GetJammerMap()
{
	return &radarhandler->jammerMaps[teamHandler->AllyTeam(team)].front();
}

const unsigned char* CAICallback::GetMetalMap()
{
	return readmap->metalMap->metalMap;
}

float CAICallback::GetElevation(float x,float z)
{
	return ground->GetHeight2(x,z);
}




void CAICallback::LineDrawerStartPath(const float3& pos, const float* color)
{
	lineDrawer.StartPath(pos, color);
}

void CAICallback::LineDrawerFinishPath()
{
	lineDrawer.FinishPath();
}

void CAICallback::LineDrawerDrawLine(const float3& endPos, const float* color)
{
	lineDrawer.DrawLine(endPos,color);
}

void CAICallback::LineDrawerDrawLineAndIcon(int commandId, const float3& endPos, const float* color)
{
	lineDrawer.DrawLineAndIcon(commandId,endPos,color);
}

void CAICallback::LineDrawerDrawIconAtLastPos(int commandId)
{
	lineDrawer.DrawIconAtLastPos(commandId);
}

void CAICallback::LineDrawerBreak(const float3& endPos, const float* color)
{
	lineDrawer.Break(endPos,color);
}

void CAICallback::LineDrawerRestart()
{
	lineDrawer.Restart();
}

void CAICallback::LineDrawerRestartSameColor()
{
	lineDrawer.RestartSameColor();
}


int CAICallback::CreateSplineFigure(float3 pos1,float3 pos2,float3 pos3,float3 pos4,float width,int arrow,int lifetime,int group)
{
	return geometricObjects->AddSpline(pos1,pos2,pos3,pos4,width,arrow,lifetime,group);
}

int CAICallback::CreateLineFigure(float3 pos1,float3 pos2,float width,int arrow,int lifetime,int group)
{
	return geometricObjects->AddLine(pos1,pos2,width,arrow,lifetime,group);
}

void CAICallback::SetFigureColor(int group,float red,float green,float blue,float alpha)
{
	geometricObjects->SetColor(group,red,green,blue,alpha);
}

void CAICallback::DeleteFigureGroup(int group)
{
	geometricObjects->DeleteGroup(group);
}



void CAICallback::DrawUnit(const char* unitName,float3 pos,float rotation,int lifetime,int teamId,bool transparent,bool drawBorder,int facing)
{
	CUnitDrawer::TempDrawUnit tdu;
	tdu.unitdef=unitDefHandler->GetUnitDefByName(unitName);
	if(!tdu.unitdef){
		logOutput.Print("Unknown unit in CAICallback::DrawUnit %s",unitName);
		return;
	}
	tdu.pos=pos;
	tdu.rotation=rotation;
	tdu.team=teamId;
	tdu.drawBorder=drawBorder;
	tdu.facing=facing;
	std::pair<int,CUnitDrawer::TempDrawUnit> tp(gs->frameNum+lifetime,tdu);

	GML_STDMUTEX_LOCK(temp); // DrawUnit

	if (transparent) {
		unitDrawer->tempTransparentDrawUnits.insert(tp);
	} else {
		unitDrawer->tempDrawUnits.insert(tp);
	}
}



bool CAICallback::CanBuildAt(const UnitDef* unitDef, float3 pos, int facing)
{
	CFeature* f;
	BuildInfo bi(unitDef, pos, facing);
	bi.pos = helper->Pos2BuildPos(bi);
	return !!uh->TestUnitBuildSquare(bi,f,teamHandler->AllyTeam(team));
}


float3 CAICallback::ClosestBuildSite(const UnitDef* unitDef, float3 pos, float searchRadius, int minDist, int facing)
{
	return helper->ClosestBuildSite(team, unitDef, pos, searchRadius, minDist, facing);
}


float CAICallback::GetMetal()
{
	return teamHandler->Team(team)->metal;
}

float CAICallback::GetMetalIncome()
{
	return teamHandler->Team(team)->prevMetalIncome;
}

float CAICallback::GetMetalUsage()
{
	return teamHandler->Team(team)->prevMetalExpense;
}

float CAICallback::GetMetalStorage()
{
	return teamHandler->Team(team)->metalStorage;
}

float CAICallback::GetEnergy()
{
	return teamHandler->Team(team)->energy;
}

float CAICallback::GetEnergyIncome()
{
	return teamHandler->Team(team)->prevEnergyIncome;
}

float CAICallback::GetEnergyUsage()
{
	return teamHandler->Team(team)->prevEnergyExpense;
}

float CAICallback::GetEnergyStorage()
{
	return teamHandler->Team(team)->energyStorage;
}

bool CAICallback::GetUnitResourceInfo(int unitId, UnitResourceInfo *i)
{
	bool fetchOk = false;

	verify();
	if (CHECK_UNITID(unitId)) {
		const CUnit* unit = uh->units[unitId];
		if (unit && (unit->losStatus[teamHandler->AllyTeam(team)] & LOS_INLOS)) {
			i->energyMake = unit->energyMake;
			i->energyUse  = unit->energyUse;
			i->metalMake  = unit->metalMake;
			i->metalUse   = unit->metalUse;
			fetchOk = true;
		}
	}

	return fetchOk;
}

bool CAICallback::IsUnitActivated(int unitId)
{
	bool activated = false;

	verify();
	if (CHECK_UNITID(unitId)) {
		const CUnit* unit = uh->units[unitId];
		if (unit && (unit->losStatus[teamHandler->AllyTeam(team)] & LOS_INLOS)) {
			activated = unit->activated;
		}
	}

	return activated;
}

bool CAICallback::UnitBeingBuilt(int unitId)
{
	bool beingBuilt = false;

	verify();
	if (CHECK_UNITID(unitId)) {
		const CUnit* unit = uh->units[unitId];
		if (unit && (unit->losStatus[teamHandler->AllyTeam(team)] & LOS_INLOS)) {
			beingBuilt = unit->beingBuilt;
		}
	}

	return beingBuilt;
}

int CAICallback::GetFeatures(int* featureIds, int featureIds_sizeMax)
{
	int featureIds_size = 0;

	verify();
	const int allyteam = teamHandler->AllyTeam(team);

	const CFeatureSet& fset = featureHandler->GetActiveFeatures();
	for (CFeatureSet::const_iterator it = fset.begin(); (it != fset.end()) && (featureIds_size < featureIds_sizeMax); ++it) {
		const CFeature* f = *it;
		assert(f);

		if (f->IsInLosForAllyTeam(allyteam)) {
			featureIds[featureIds_size++] = f->id;
		}
	}

	return featureIds_size;
}

int CAICallback::GetFeatures(int* featureIds, int featureIds_sizeMax, const float3& pos, float radius)
{
	int featureIds_size = 0;

	verify();
	const std::vector<CFeature*> ft = qf->GetFeaturesExact(pos, radius);
	const int allyteam = teamHandler->AllyTeam(team);

	std::vector<CFeature*>::const_iterator it;
	for (it = ft.begin(); (it != ft.end()) && (featureIds_size < featureIds_sizeMax); ++it) {
		const CFeature* f = *it;
		assert(f);

		if (f->IsInLosForAllyTeam(allyteam)) {
			featureIds[featureIds_size++] = f->id;
		}
	}

	return featureIds_size;
}

const FeatureDef* CAICallback::GetFeatureDef(int featureId)
{
	const FeatureDef* featureDef = NULL;

	verify();

	const CFeature* f = featureHandler->GetFeature(featureId);

	if (f) {
		const int allyteam = teamHandler->AllyTeam(team);
		if (f->IsInLosForAllyTeam(allyteam)) {
			featureDef = f->def;
		}
	}

	return featureDef;
}
const FeatureDef* CAICallback::GetFeatureDefById(int featureDefId)
{
	return featureHandler->GetFeatureDefByID(featureDefId);
}

float CAICallback::GetFeatureHealth(int featureId)
{
	float health = 0.0f;

	verify();

	const CFeature* f = featureHandler->GetFeature(featureId);

	if (f) {
		const int allyteam = teamHandler->AllyTeam(team);
		if (f->IsInLosForAllyTeam(allyteam)) {
			health = f->health;
		}
	}

	return health;
}

float CAICallback::GetFeatureReclaimLeft(int featureId)
{
	float reclaimLeft = 0.0f;

	verify();

	const CFeature* f = featureHandler->GetFeature(featureId);

	if (f) {
		const int allyteam = teamHandler->AllyTeam(team);
		if (f->IsInLosForAllyTeam(allyteam)) {
			return f->reclaimLeft;
		}
	}

	return reclaimLeft;
}

float3 CAICallback::GetFeaturePos(int featureId)
{
	float3 pos = ZeroVector;

	verify();

	const CFeature* f = featureHandler->GetFeature(featureId);

	if (f) {
		const int allyteam = teamHandler->AllyTeam(team);
		if (f->IsInLosForAllyTeam(allyteam)) {
			pos = f->pos;
		}
	}

	return pos;
}

bool CAICallback::GetValue(int id, void *data)
{
	verify();
	switch (id) {
		case AIVAL_NUMDAMAGETYPES:{
			*((int*)data) = damageArrayHandler->GetNumTypes();
			return true;
		}case AIVAL_EXCEPTION_HANDLING:{
			*(bool*)data = CEngineOutHandler::IsCatchExceptions();
			return true;
		}case AIVAL_MAP_CHECKSUM:{
			*(unsigned int*)data = readmap->mapChecksum;
			return true;
		}case AIVAL_DEBUG_MODE:{
			*(bool*)data = globalRendering->drawdebug;
			return true;
		}case AIVAL_GAME_MODE:{
			*(int*)data = gameSetup->gameMode;
			return true;
		}case AIVAL_GAME_PAUSED:{
			*(bool*)data = gs->paused;
			return true;
		}case AIVAL_GAME_SPEED_FACTOR:{
			*(float*)data = gs->speedFactor;
			return true;
		}case AIVAL_GUI_VIEW_RANGE:{
			*(float*)data = globalRendering->viewRange;
			return true;
		}case AIVAL_GUI_SCREENX:{
			*(float*)data = globalRendering->viewSizeX;
			return true;
		}case AIVAL_GUI_SCREENY:{
			*(float*)data = globalRendering->viewSizeY;
			return true;
		}case AIVAL_GUI_CAMERA_DIR:{
			*(float3*)data = camHandler->GetCurrentController().GetDir();
			return true;
		}case AIVAL_GUI_CAMERA_POS:{
			*(float3*)data = camHandler->GetCurrentController().GetPos();
			return true;
		}case AIVAL_LOCATE_FILE_R:{
			std::string f((char*) data);
			f = filesystem.LocateFile(f);
			strcpy((char*) data, f.c_str());
			return FileSystemHandler::IsReadableFile(f);
		}case AIVAL_LOCATE_FILE_W:{
			std::string f((char*) data);
			std::string f_abs = filesystem.LocateFile(f, FileSystem::WRITE | FileSystem::CREATE_DIRS);
			if (!FileSystemHandler::IsAbsolutePath(f_abs)) {
				return false;
			} else {
				strcpy((char*) data, f.c_str());
				return true;
			}
		}
		case AIVAL_UNIT_LIMIT: {
			*(int*) data = uh->MaxUnitsPerTeam();
			return true;
		}
		case AIVAL_SCRIPT: {
			*(const char**) data = gameSetup ? gameSetup->gameSetupText.c_str() : "";
			return true;
		}
		default:
			return false;
	}
}

int CAICallback::HandleCommand(int commandId, void* data)
{
	switch (commandId) {
		case AIHCQuerySubVersionId: {
			return 1; // current version of Handle Command interface
		} break;
		case AIHCAddMapPointId: {
			const AIHCAddMapPoint* pnt = (AIHCAddMapPoint*) data;
			net->Send(CBaseNetProtocol::Get().SendMapDrawPoint(team, (short)pnt->pos.x, (short)pnt->pos.z, std::string(pnt->label), false));
			return 1;
		} break;
		case AIHCAddMapLineId: {
			const AIHCAddMapLine* line = (AIHCAddMapLine*) data;
			net->Send(CBaseNetProtocol::Get().SendMapDrawLine(team, (short)line->posfrom.x, (short)line->posfrom.z, (short)line->posto.x, (short)line->posto.z, false));
			return 1;
		} break;
		case AIHCRemoveMapPointId: {
			net->Send(CBaseNetProtocol::Get().SendMapErase(team, (short)((AIHCRemoveMapPoint *)data)->pos.x, (short)((AIHCRemoveMapPoint *)data)->pos.z));
			return 1;
		} break;
		case AIHCSendStartPosId: {
			SendStartPos(((AIHCSendStartPos *)data)->ready,((AIHCSendStartPos *)data)->pos);
			return 1;
		} break;
		case AIHCGetUnitDefByIdId: {
			AIHCGetUnitDefById* cmdData = (AIHCGetUnitDefById*) data;
			cmdData->ret = GetUnitDefById(cmdData->unitDefId);
			return 1;
		} break;
		case AIHCGetWeaponDefByIdId: {
			AIHCGetWeaponDefById* cmdData = (AIHCGetWeaponDefById*) data;
			cmdData->ret = GetWeaponDefById(cmdData->weaponDefId);
			return 1;
		} break;
		case AIHCGetFeatureDefByIdId: {
			AIHCGetFeatureDefById* cmdData = (AIHCGetFeatureDefById*) data;
			cmdData->ret = GetFeatureDefById(cmdData->featureDefId);
			return 1;
		} break;

		case AIHCTraceRayId: {
			AIHCTraceRay* cmdData = (AIHCTraceRay*) data;

			if (CHECK_UNITID(cmdData->srcUID)) {
				const CUnit* srcUnit = uh->units[cmdData->srcUID];
				const CUnit* hitUnit = NULL;
				float  realLen = 0.0f;
				bool   haveHit = false;
				bool   visible = true;

				if (srcUnit != NULL) {
					realLen = helper->TraceRay(cmdData->rayPos, cmdData->rayDir, cmdData->rayLen, 0.0f, srcUnit, hitUnit, cmdData->flags);

					if (hitUnit != NULL) {
						haveHit = true;
						visible = (hitUnit->losStatus[teamHandler->AllyTeam(team)] & LOS_INLOS);
					}

					cmdData->rayLen = (           visible)? realLen:     cmdData->rayLen;
					cmdData->hitUID = (haveHit && visible)? hitUnit->id: cmdData->hitUID;
				}
			}

			return 1;
		} break;

		case AIHCPauseId: {
			AIHCPause* cmdData = (AIHCPause*) data;

			net->Send(CBaseNetProtocol::Get().SendPause(gu->myPlayerNum, cmdData->enable));
			logOutput.Print(
					"Skirmish AI controlling team %i paused the game, reason: %s",
					team, cmdData->reason != NULL ? cmdData->reason : "UNSPECIFIED");

			return 1;
		} break;

		case AIHCGetDataDirId: {
			// do nothing
			// this event will never end up here, as
			// it is handled in the C layer directly
			// see Clb_DataDirs_allocatePath in rts/ExternalAI/Interface/SSkirmishAICallback.h

			return 0;
		} break;

		case AIHCDebugDrawId: {
			AIHCDebugDraw* cmdData = (AIHCDebugDraw*) data;

			switch (cmdData->cmdMode) {
				case AIHCDebugDraw::AIHC_DEBUGDRAWER_MODE_ADD_GRAPH_POINT: {
					debugDrawerAI->AddGraphPoint(this->team, cmdData->lineId, cmdData->x, cmdData->y);
				} break;
				case AIHCDebugDraw::AIHC_DEBUGDRAWER_MODE_DEL_GRAPH_POINTS: {
					debugDrawerAI->DelGraphPoints(this->team, cmdData->lineId, cmdData->numPoints);
				} break;
				case AIHCDebugDraw::AIHC_DEBUGDRAWER_MODE_SET_GRAPH_POS: {
					debugDrawerAI->SetGraphPos(this->team, cmdData->x, cmdData->y);
				} break;
				case AIHCDebugDraw::AIHC_DEBUGDRAWER_MODE_SET_GRAPH_SIZE: {
					debugDrawerAI->SetGraphSize(this->team, cmdData->w, cmdData->h);
				} break;
				case AIHCDebugDraw::AIHC_DEBUGDRAWER_MODE_SET_GRAPH_LINE_COLOR: {
					debugDrawerAI->SetGraphLineColor(this->team, cmdData->lineId, cmdData->color);
				} break;
				case AIHCDebugDraw::AIHC_DEBUGDRAWER_MODE_SET_GRAPH_LINE_LABEL: {
					debugDrawerAI->SetGraphLineLabel(this->team, cmdData->lineId, cmdData->label);
				} break;

				case AIHCDebugDraw::AIHC_DEBUGDRAWER_MODE_ADD_OVERLAY_TEXTURE: {
					cmdData->texHandle = debugDrawerAI->AddOverlayTexture(
						this->team,
						cmdData->texData,
						int(cmdData->w),   // interpret as absolute width
						int(cmdData->h)    // interpret as absolute height
					);
				} break;
				case AIHCDebugDraw::AIHC_DEBUGDRAWER_MODE_UPDATE_OVERLAY_TEXTURE: {
					debugDrawerAI->UpdateOverlayTexture(
						this->team,
						cmdData->texHandle,
						cmdData->texData,
						int(cmdData->x),    // interpret as absolute pixel col
						int(cmdData->y),    // interpret as absolute pixel row
						int(cmdData->w),    // interpret as absolute width
						int(cmdData->h)     // interpret as absolute height
					);
				} break;
				case AIHCDebugDraw::AIHC_DEBUGDRAWER_MODE_DEL_OVERLAY_TEXTURE: {
					debugDrawerAI->DelOverlayTexture(this->team, cmdData->texHandle);
				} break;
				case AIHCDebugDraw::AIHC_DEBUGDRAWER_MODE_SET_OVERLAY_TEXTURE_POS: {
					debugDrawerAI->SetOverlayTexturePos(this->team, cmdData->texHandle, cmdData->x, cmdData->y);
				} break;
				case AIHCDebugDraw::AIHC_DEBUGDRAWER_MODE_SET_OVERLAY_TEXTURE_SIZE: {
					debugDrawerAI->SetOverlayTextureSize(this->team, cmdData->texHandle, cmdData->w, cmdData->h);
				} break;
				case AIHCDebugDraw::AIHC_DEBUGDRAWER_MODE_SET_OVERLAY_TEXTURE_LABEL: {
					debugDrawerAI->SetOverlayTextureLabel(this->team, cmdData->texHandle, cmdData->label);
				} break;

				default: {
				} break;
			}

			return 1;
		} break;

		default:
			return 0;
	}
}



bool CAICallback::IsDebugDrawerEnabled() const {
	return (debugDrawerAI->GetDraw());
}



int CAICallback::GetNumUnitDefs ()
{
	return unitDefHandler->numUnitDefs;
}

void CAICallback::GetUnitDefList (const UnitDef** list)
{
	for (int a=0;a<unitDefHandler->numUnitDefs;a++)
		list [a] = unitDefHandler->GetUnitDefByID (a+1);
}


float CAICallback::GetUnitDefRadius(int def)
{
	UnitDef *ud = &unitDefHandler->unitDefs[def];
	S3DModel* mdl = ud->LoadModel();
	return mdl->radius;
}

float CAICallback::GetUnitDefHeight(int def)
{
	UnitDef *ud = &unitDefHandler->unitDefs[def];
	S3DModel* mdl = ud->LoadModel();
	return mdl->height;
}


bool CAICallback::GetProperty(int unitId, int property, void *data)
{
	verify();
	if (CHECK_UNITID(unitId)) {
		const CUnit* unit = uh->units[unitId];
		const int allyTeam = teamHandler->AllyTeam(team);
		if (!(unit && (unit->losStatus[allyTeam] & LOS_INLOS))) {
			return false;  //return if the unit doesn't exist or cant be seen
		}

		switch (property) {
			case AIVAL_UNITDEF: {
				if (teamHandler->Ally(unit->allyteam, allyTeam)) {
					(*(const UnitDef**)data) = unit->unitDef;
				} else {
					const UnitDef* unitDef = unit->unitDef;
					const UnitDef* decoyDef = unitDef->decoyDef;
					if (decoyDef == NULL) {
						(*(const UnitDef**)data) = unitDef;
					} else {
						(*(const UnitDef**)data) = decoyDef;
					}
				}
				return true;
			}
			case AIVAL_CURRENT_FUEL: {
				(*(float*)data) = unit->currentFuel;
				return true;
			}
			case AIVAL_STOCKPILED: {
				if (!unit->stockpileWeapon || !teamHandler->Ally(unit->allyteam, allyTeam)) {
					return false;
				}
				(*(int*)data) = unit->stockpileWeapon->numStockpiled;
				return true;
			}
			case AIVAL_STOCKPILE_QUED: {
				if (!unit->stockpileWeapon || !teamHandler->Ally(unit->allyteam, allyTeam)) {
					return false;
				}
				(*(int*)data) = unit->stockpileWeapon->numStockpileQued;
				return true;
			}
			case AIVAL_UNIT_MAXSPEED: {
				(*(float*) data) = unit->maxSpeed;
				return true;
			}
			default:
				return false;
		}
	}
	return false;
}


int CAICallback::GetFileSize (const char *filename)
{
	CFileHandler fh (filename);

	if (!fh.FileExists ())
		return -1;

	return fh.FileSize();
}


int CAICallback::GetFileSize (const char *filename, const char* modes)
{
	CFileHandler fh (filename, modes);

	if (!fh.FileExists ())
		return -1;

	return fh.FileSize();
}


bool CAICallback::ReadFile (const char *filename, void *buffer, int bufferLength)
{
	CFileHandler fh (filename);
	int fs;
	if (!fh.FileExists() || bufferLength < (fs = fh.FileSize()))
		return false;

	fh.Read (buffer, fs);
	return true;
}


bool CAICallback::ReadFile (const char *filename, const char* modes,
                            void *buffer, int bufferLength)
{
	CFileHandler fh (filename, modes);
	int fs;
	if (!fh.FileExists() || bufferLength < (fs = fh.FileSize()))
		return false;

	fh.Read (buffer, fs);
	return true;
}


// Additions to the interface by Alik
int CAICallback::GetSelectedUnits(int *unitIds, int unitIds_max)
{
	verify();
	int a = 0;

	GML_RECMUTEX_LOCK(sel); // GetSelectedUnit
	// check if the allyteam of the player running
	// the AI lib matches the AI's actual allyteam
	if (gu->myAllyTeam == teamHandler->AllyTeam(team)) {
		for (CUnitSet::iterator ui = selectedUnits.selectedUnits.begin();
				ui != selectedUnits.selectedUnits.end(); ++ui) {
			unitIds[a++] = (*ui)->id;
			if (a >= unitIds_max) {
				break;
			}
		}
	}

	return a;
}


float3 CAICallback::GetMousePos() {
	verify();
	if (gu->myAllyTeam == teamHandler->AllyTeam(team))
		return inMapDrawer->GetMouseMapPos();
	else
		return ZeroVector;
}


int CAICallback::GetMapPoints(PointMarker* pm, int pm_sizeMax, bool includeAllies)
{
	int pm_size = 0;

	verify();

	// If the AI is not in the local players ally team,
	// the draw information for the AIs ally team will not be available
	// for cheating prevention.
	/*if (gu->myAllyTeam != teamHandler->AllyTeam(team)) {
		return pm_size;
	}*/

	std::list<const unsigned char*> includeColors;
	// include out team color
	includeColors.push_back(teamHandler->Team(team)->color);
	// include the team colors of all our allies
	for (int t=0; t < teamHandler->ActiveTeams(); ++t) {
		if (teamHandler->AlliedTeams(team, t)) {
			includeColors.push_back(teamHandler->Team(t)->color);
		}
	}

	std::list<CInMapDraw::MapPoint>* points = NULL;
	std::list<CInMapDraw::MapPoint>::const_iterator point;
	std::list<const unsigned char*>::const_iterator ic;
	for (size_t i=0; i < inMapDrawer->numQuads && pm_size < pm_sizeMax; i++) {
		points = &(inMapDrawer->drawQuads[i].points);
		for (point = points->begin(); point != points->end()
				&& pm_size < pm_sizeMax; ++point) {
			for (ic = includeColors.begin(); ic != includeColors.end(); ++ic) {
				if (point->color == *ic) {
					pm[pm_size].pos   = point->pos;
					pm[pm_size].color = point->color;
					pm[pm_size].label = point->label.c_str();
					pm_size++;
					break;
				}
			}
		}
	}

	return pm_size;
}

int CAICallback::GetMapLines(LineMarker* lm, int lm_sizeMax, bool includeAllies)
{
	int lm_size = 0;

	verify();

	// If the AI is not in the local players ally team,
	// the draw information for the AIs ally team will not be available
	// for cheating prevention.
	/*if (gu->myAllyTeam != teamHandler->AllyTeam(team)) {
		return lm_size;
	}*/

	std::list<const unsigned char*> includeColors;
	// include out team color
	includeColors.push_back(teamHandler->Team(team)->color);
	// include the team colors of all our allies
	for (int t=0; t < teamHandler->ActiveTeams(); ++t) {
		if (teamHandler->AlliedTeams(team, t)) {
			includeColors.push_back(teamHandler->Team(t)->color);
		}
	}

	std::list<CInMapDraw::MapLine>* lines = NULL;
	std::list<CInMapDraw::MapLine>::const_iterator line;
	std::list<const unsigned char*>::const_iterator ic;
	for (size_t i=0; i < inMapDrawer->numQuads && lm_size < lm_sizeMax; i++) {
		lines = &(inMapDrawer->drawQuads[i].lines);
		for (line = lines->begin(); line != lines->end()
				&& lm_size < lm_sizeMax; ++line) {
			for (ic = includeColors.begin(); ic != includeColors.end(); ++ic) {
				if (line->color == *ic) {
					lm[lm_size].pos   = line->pos;
					lm[lm_size].pos2  = line->pos2;
					lm[lm_size].color = line->color;
					lm_size++;
					break;
				}
			}
		}
	}

	return lm_size;
}


const WeaponDef* CAICallback::GetWeapon(const char* weaponName)
{
	return weaponDefHandler->GetWeapon(weaponName);
}
const WeaponDef* CAICallback::GetWeaponDefById(int weaponDefId)
{
	return weaponDefHandler->GetWeaponById(weaponDefId);
}


bool CAICallback::CanBuildUnit(int unitDefID)
{
	const UnitDef* ud = unitDefHandler->GetUnitDefByID(unitDefID);
	if (ud == NULL) {
		return false;
	}
	return uh->CanBuildUnit(ud, team);
}


const float3 *CAICallback::GetStartPos()
{
	return &teamHandler->Team(team)->startPos;
}


const char* CAICallback::CallLuaRules(const char* data, int inSize, int* outSize)
{
	if (luaRules == NULL) {
		return NULL;
	}
	return luaRules->AICallIn(data, inSize, outSize);
}
