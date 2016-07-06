/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ExternalAI/AICallback.h"

#include "Game/Game.h"
#include "Game/Camera/CameraController.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/GameHelper.h"
#include "Game/GlobalUnsynced.h"
#include "Game/TraceRay.h"
#include "Game/GameSetup.h"
#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"
#include "Game/SelectedUnitsHandler.h"
#include "Game/InMapDraw.h"
#include "Game/UI/MiniMap.h"
#include "Lua/LuaRules.h"
#include "Lua/LuaUI.h"
#include "Map/MapInfo.h"
#include "Map/MetalMap.h"
#include "Map/ReadMap.h"
#include "Rendering/DebugDrawerAI.h"
#include "Rendering/LineDrawer.h"
#include "Rendering/Models/3DModel.h"
#include "Rendering/UnitDrawer.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/DamageArrayHandler.h"
#include "Sim/Misc/GlobalConstants.h" // needed for MAX_UNITS
#include "Sim/Misc/GeometricObjects.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/Wind.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/MoveTypes/MoveType.h"
#include "Sim/Path/IPathManager.h"
#include "Game/UI/Groups/Group.h"
#include "Game/UI/Groups/GroupHandler.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/CommandAI/CommandQueue.h"
#include "Sim/Units/UnitTypes/Factory.h"
#include "Sim/Units/BuildInfo.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Weapons/Weapon.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include "ExternalAI/EngineOutHandler.h"
#include "System/EventHandler.h"
#include "System/Log/ILog.h"
#include "Net/Protocol/NetProtocol.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/DataDirsAccess.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/FileQueryFlags.h"
#include "System/Platform/errorhandler.h"

#include <string>
#include <vector>
#include <map>

// Cast id to unsigned to catch negative ids in the same operations,
// cast MAX_* to unsigned to suppress GCC comparison between signed/unsigned warning.
#define CHECK_UNITID(id) ((unsigned)(id) < (unsigned)unitHandler->MaxUnits())
#define CHECK_GROUPID(id) ((unsigned)(id) < (unsigned)gh->groups.size())
// With some hacking you can raise an abort (assert) instead of ignoring the id,
//#define CHECK_UNITID(id) (assert(id > 0 && id < unitHandler->MaxUnits()), true)
// ...or disable the check altogether for release.
//#define CHECK_UNITID(id) true


CUnit* CAICallback::GetUnit(int unitId) const {

	CUnit* unit = NULL;

	if (CHECK_UNITID(unitId)) {
		unit = unitHandler->units[unitId];
	}

	return unit;
}

CUnit* CAICallback::GetMyTeamUnit(int unitId) const {

	CUnit* unit = NULL;

	if (CHECK_UNITID(unitId)) {
		CUnit* unitTmp = unitHandler->units[unitId];
		if (unitTmp && (unitTmp->team == team)) {
			unit = unitTmp;
		}
	}

	return unit;
}

CUnit* CAICallback::GetInSensorRangeUnit(int unitId, unsigned short losFlags) const {

	CUnit* unit = NULL;

	if (CHECK_UNITID(unitId)) {
		CUnit* unitTmp = unitHandler->units[unitId];
		// Skip in-sensor-range test if the unit is allied with our team.
		// This prevents errors where an allied unit is starting to build,
		// but is not yet (technically) in LOS, because LOS was not yet updated,
		// and thus would be invisible for us, without the ally check.
		if (unitTmp
				&& (teamHandler->AlliedTeams(unitTmp->team, team)
				||  (unitTmp->losStatus[teamHandler->AllyTeam(team)] & losFlags)))
		{
			unit = unitTmp;
		}
	}

	return unit;
}
CUnit* CAICallback::GetInLosUnit(int unitId) const {
	return GetInSensorRangeUnit(unitId, LOS_INLOS);
}
CUnit* CAICallback::GetInLosAndRadarUnit(int unitId) const {
	return GetInSensorRangeUnit(unitId, (LOS_INLOS | LOS_INRADAR));
}


CAICallback::CAICallback(int teamId)
	: team(teamId)
	, noMessages(false)
	, gh(grouphandlers[teamId])
{}

void CAICallback::SendStartPos(bool ready, float3 startPos)
{
	if (ready) {
		clientNet->Send(CBaseNetProtocol::Get().SendStartPos(gu->myPlayerNum, team, CPlayer::PLAYER_RDYSTATE_READIED, startPos.x, startPos.y, startPos.z));
	} else {
		clientNet->Send(CBaseNetProtocol::Get().SendStartPos(gu->myPlayerNum, team, CPlayer::PLAYER_RDYSTATE_UPDATED, startPos.x, startPos.y, startPos.z));
	}
}

void CAICallback::SendTextMsg(const char* text, int zone)
{
	const CSkirmishAIHandler::ids_t& teamAIs = skirmishAIHandler.GetSkirmishAIsInTeam(this->team);
	const SkirmishAIData* aiData = skirmishAIHandler.GetSkirmishAI(*(teamAIs.begin())); // FIXME is there a better way?

	if (!game->ProcessCommandText(-1, text)) {
		LOG("<SkirmishAI: %s %s (team %d)>: %s", aiData->shortName.c_str(), aiData->version.c_str(), team, text);
	}
}

void CAICallback::SetLastMsgPos(const float3& pos)
{
	eventHandler.LastMessagePosition(pos);
}

void CAICallback::AddNotification(const float3& pos, const float3& color, float alpha)
{
	minimap->AddNotification(pos, color, alpha);
}



bool CAICallback::SendResources(float mAmount, float eAmount, int receivingTeamId)
{
	typedef unsigned char ubyte;
	bool ret = false;

	if ((team != receivingTeamId)
			&& teamHandler->IsValidTeam(receivingTeamId)
			&& teamHandler->Team(receivingTeamId)
			&& teamHandler->Team(team)
			&& !teamHandler->Team(receivingTeamId)->isDead
			&& !teamHandler->Team(team)->isDead)
	{
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

		clientNet->Send(CBaseNetProtocol::Get().SendAIShare(ubyte(gu->myPlayerNum), skirmishAIHandler.GetCurrentAIID(), ubyte(team), ubyte(receivingTeamId), mAmount, eAmount, empty));
	}

	return ret;
}

int CAICallback::SendUnits(const std::vector<int>& unitIds, int receivingTeamId)
{
	typedef unsigned char ubyte;
	std::vector<short> sentUnitIDs;

	if ((team != receivingTeamId)
			&& teamHandler->IsValidTeam(receivingTeamId)
			&& teamHandler->Team(receivingTeamId)
			&& teamHandler->Team(team)
			&& !teamHandler->Team(receivingTeamId)->isDead
			&& !teamHandler->Team(team)->isDead)
	{
		// we must iterate over the ID's to check if
		// all of them really belong to the AI's team
		std::vector<int>::const_iterator uid;
		for (uid = unitIds.begin(); uid != unitIds.end(); ++uid) {
			const int unitId = *uid;

			const CUnit* unit = GetUnit(unitId);
			if (unit && unit->team == team) {
				// we own this unit, save it
				// (note: safe cast since MAX_UNITS currently fits in a short)
				sentUnitIDs.push_back(short(unitId));

				// stop whatever this unit is doing
				Command c(CMD_STOP);
				GiveOrder(unitId, &c);
			}
		}

		if (!sentUnitIDs.empty()) {
			// we ca not use SendShare() here either, since
			// AIs do not have a notion of "selected units"
			clientNet->Send(CBaseNetProtocol::Get().SendAIShare(ubyte(gu->myPlayerNum), skirmishAIHandler.GetCurrentAIID(), ubyte(team), ubyte(receivingTeamId), 0.0f, 0.0f, sentUnitIDs));
		}
	}

	// return how many units were actually put up for transfer
	return (sentUnitIDs.size());
}



bool CAICallback::PosInCamera(const float3& pos, float radius)
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
	int playerTeamId = -1;

	if (playerHandler->IsValidPlayer(playerId)) {
		CPlayer* pl = playerHandler->Player(playerId);
		if (!pl->spectator) {
			playerTeamId = pl->team;
		}
	}

	return playerTeamId;
}

const char* CAICallback::GetTeamSide(int teamId)
{
	if (teamHandler->IsValidTeam(teamId)) {
		return (teamHandler->Team(teamId)->GetSide().c_str());
	}

	return NULL;
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

	CUnit* unit = GetMyTeamUnit(unitId);
	if (unit) {
		if (CHECK_GROUPID(groupId) && gh->groups[groupId]) {
			added = unit->SetGroup(gh->groups[groupId]);
		}
	}

	return added;
}

bool CAICallback::RemoveUnitFromGroup(int unitId)
{
	bool removed = false;

	CUnit* unit = GetMyTeamUnit(unitId);
	if (unit) {
		unit->SetGroup(0);
		removed = true;
	}

	return removed;
}

int CAICallback::GetUnitGroup(int unitId)
{
	int groupId = -1;

	const CUnit* unit = GetMyTeamUnit(unitId);
	if (unit) {
		const CGroup* group = unit->group;
		if (group) {
			groupId = group->id;
		}
	}

	return groupId;
}

const std::vector<const SCommandDescription*>* CAICallback::GetGroupCommands(int groupId)
{
	static std::vector<const SCommandDescription*> tempcmds;
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

	const CUnit * unit = unitHandler->units[unitId];

	if (!unit) {
		return -3;
	}

	if (unit->team != team) {
		return -5;
	}

	clientNet->Send(CBaseNetProtocol::Get().SendAICommand(gu->myPlayerNum, skirmishAIHandler.GetCurrentAIID(), unitId, c->GetID(), c->aiCommandId, c->options, c->params));

	return 0;
}

const std::vector<const SCommandDescription*>* CAICallback::GetUnitCommands(int unitId)
{
	const std::vector<const SCommandDescription*>* unitCommands = NULL;

	const CUnit* unit = GetMyTeamUnit(unitId);
	if (unit) {
		unitCommands = &unit->commandAI->possibleCommands;
	}

	return unitCommands;
}

const CCommandQueue* CAICallback::GetCurrentUnitCommands(int unitId)
{
	const CCommandQueue* currentUnitCommands = NULL;

	const CUnit* unit = GetMyTeamUnit(unitId);
	if (unit) {
		currentUnitCommands = &unit->commandAI->commandQue;
	}

	return currentUnitCommands;
}

int CAICallback::GetUnitAiHint(int unitId)
{
	return -1;
}

int CAICallback::GetUnitTeam(int unitId)
{
	int unitTeam = -1;

	verify();
	const CUnit* unit = GetInLosUnit(unitId);
	if (unit) {
		unitTeam = unit->team;
	}

	return unitTeam;
}

int CAICallback::GetUnitAllyTeam(int unitId)
{
	int unitAllyTeam = -1;

	verify();
	const CUnit* unit = GetInLosUnit(unitId);
	if (unit) {
		unitAllyTeam = unit->allyteam;
	}

	return unitAllyTeam;
}

float CAICallback::GetUnitHealth(int unitId)
{
	float health = -1;

	verify();
	if (CHECK_UNITID(unitId)) {
		const CUnit* unit = unitHandler->units[unitId];

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
		const CUnit* unit = unitHandler->units[unitId];
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
		const CUnit* unit = unitHandler->units[unitId];
		if (unit) {
			const int allyTeam = teamHandler->AllyTeam(team);
			if (teamHandler->Ally(unit->allyteam, allyTeam)) {
				speed = unit->moveType->GetMaxSpeed();
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
		const CUnit* unit = unitHandler->units[unitId];
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
	const CUnit* unit = GetInLosUnit(unitId);
	if (unit) {
		experience = unit->experience;
	}

	return experience;
}

float CAICallback::GetUnitMaxRange(int unitId)
{
	float maxRange = -1;

	verify();
	if (CHECK_UNITID(unitId)) {
		const CUnit* unit = unitHandler->units[unitId];
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
		const CUnit* unit = unitHandler->units[unitId];
		if (unit) {
			const UnitDef* unitDef = unit->unitDef;
			const int allyTeam = teamHandler->AllyTeam(team);
			if (teamHandler->Ally(unit->allyteam, allyTeam)) {
				def = unitDef;
			} else {
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
	}

	return def;
}

const UnitDef* CAICallback::GetUnitDef(const char* unitName)
{
	return unitDefHandler->GetUnitDefByName(unitName);
}
const UnitDef* CAICallback::GetUnitDefById(int unitDefId)
{
	// NOTE: this function is never called, implemented in SSkirmishAICallbackImpl
	return NULL;
}


float3 CAICallback::GetUnitPos(int unitId)
{
	verify();
	const CUnit* unit = GetInLosAndRadarUnit(unitId);
	if (unit) {
		return unit->GetErrorPos(teamHandler->AllyTeam(team));
	}
	return ZeroVector;
}

float3 CAICallback::GetUnitVelocity(int unitId)
{
	verify();
	const CUnit* unit = GetInLosAndRadarUnit(unitId);
	if (unit) {
		return unit->speed;
	}
	return ZeroVector;
}



int CAICallback::GetBuildingFacing(int unitId) {

	int buildFacing = -1;

	verify();
	const CUnit* unit = GetInLosUnit(unitId);
	if (unit) {
		buildFacing = unit->buildFacing;
	}

	return buildFacing;
}

bool CAICallback::IsUnitCloaked(int unitId) {

	bool isCloaked = false;

	verify();
	const CUnit* unit = GetInLosUnit(unitId);
	if (unit) {
		isCloaked = unit->isCloaked;
	}

	return isCloaked;
}

bool CAICallback::IsUnitParalyzed(int unitId) {

	bool isParalyzed = false;

	verify();
	const CUnit* unit = GetInLosUnit(unitId);
	if (unit) {
		isParalyzed = unit->IsStunned();
	}

	return isParalyzed;
}

bool CAICallback::IsUnitNeutral(int unitId) {

	bool isNeutral = false;

	verify();
	const CUnit* unit = GetInLosUnit(unitId);
	if (unit) {
		isNeutral = unit->IsNeutral();
	}

	return isNeutral;
}



int CAICallback::InitPath(const float3& start, const float3& end, int pathType, float goalRadius)
{
	assert(((size_t)pathType) < moveDefHandler->GetNumMoveDefs());
	return pathManager->RequestPath(NULL, moveDefHandler->GetMoveDefByPathType(pathType), start, end, goalRadius, false);
}

float3 CAICallback::GetNextWaypoint(int pathId)
{
	return pathManager->NextWayPoint(NULL, pathId, 0, ZeroVector, 0.0f, false);
}

void CAICallback::FreePath(int pathId)
{
	pathManager->DeletePath(pathId);
}

float CAICallback::GetPathLength(float3 start, float3 end, int pathType, float goalRadius)
{
	const int pathID  = InitPath(start, end, pathType, goalRadius);
	float     pathLen = -1.0f;

	if (pathID == 0) {
		return pathLen;
	}

	std::vector<float3> points;
	std::vector<int>    lengths;

	pathManager->GetPathWayPoints(pathID, points, lengths);

	if (points.empty()) {
		return 0.0f;
	}

	// distance to first intermediate node
	pathLen = start.distance(points[0]);

	// we don't care which path segment has
	// what resolution, just lump all points
	// together
	for (size_t i = 1; i < points.size(); i++) {
		pathLen += points[i].distance(points[i - 1]);
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

bool CAICallback::SetPathNodeCost(unsigned int x, unsigned int z, float cost) {
	return pathManager->SetNodeExtraCost(x, z, cost, false);
}

float CAICallback::GetPathNodeCost(unsigned int x, unsigned int z) {
	return pathManager->GetNodeExtraCost(x, z, false);
}



static int FilterUnitsVector(const std::vector<CUnit*>& units, int* unitIds, int unitIds_max, bool (*includeUnit)(const CUnit*) = NULL)
{
	int a = 0;

	if (unitIds_max < 0) {
		unitIds = NULL;
		unitIds_max = MAX_UNITS;
	}

	std::vector<CUnit*>::const_iterator ui;
	for (ui = units.begin(); (ui != units.end()) && (a < unitIds_max); ++ui) {
		CUnit* u = *ui;

		if ((includeUnit == NULL) || (*includeUnit)(u)) {
			if (unitIds != NULL) {
				unitIds[a] = u->id;
			}
			a++;
		}
	}

	return a;
}


static inline bool unit_IsNeutral(const CUnit* unit) {
	return unit->IsNeutral();
}

static int myAllyTeamId = -1;

/// You have to set myAllyTeamId before calling this function. NOT thread safe!
static inline bool unit_IsEnemy(const CUnit* unit) {
	return (!teamHandler->Ally(unit->allyteam, myAllyTeamId)
			&& !unit_IsNeutral(unit));
}

/// You have to set myAllyTeamId before calling this function. NOT thread safe!
static inline bool unit_IsFriendly(const CUnit* unit) {
	return (teamHandler->Ally(unit->allyteam, myAllyTeamId)
			&& !unit_IsNeutral(unit));
}

/// You have to set myAllyTeamId before calling this function. NOT thread safe!
static inline bool unit_IsInLos(const CUnit* unit) {

	// Skip in-sensor-range test if the unit is allied with our team.
	// This prevents errors where an allied unit is starting to build,
	// but is not yet (technically) in LOS, because LOS was not yet updated,
	// and thus would be invisible for us, without the ally check.
	return (teamHandler->Ally(myAllyTeamId, unit->allyteam)
			|| ((unit->losStatus[myAllyTeamId] & LOS_INLOS) != 0));
}

/// You have to set myAllyTeamId before calling this function. NOT thread safe!
static inline bool unit_IsInRadar(const CUnit* unit) {

	// Skip in-sensor-range test if the unit is allied with our team.
	// This prevents errors where an allied unit is starting to build,
	// but is not yet (technically) in LOS, because LOS was not yet updated,
	// and thus would be invisible for us, without the ally check.
	return (teamHandler->Ally(myAllyTeamId, unit->allyteam)
			|| ((unit->losStatus[myAllyTeamId] & LOS_INRADAR) != 0));
}

/// You have to set myAllyTeamId before calling this function. NOT thread safe!
static inline bool unit_IsEnemyAndInLos(const CUnit* unit) {
	return (unit_IsEnemy(unit) && unit_IsInLos(unit));
}

/// You have to set myAllyTeamId before calling this function. NOT thread safe!
static inline bool unit_IsEnemyAndInLosOrRadar(const CUnit* unit) {
	return (unit_IsEnemy(unit) && (unit_IsInLos(unit) || unit_IsInRadar(unit)));
}

/// You have to set myAllyTeamId before calling this function. NOT thread safe!
static inline bool unit_IsNeutralAndInLos(const CUnit* unit) {
	return (unit_IsNeutral(unit) && unit_IsInLos(unit));
}

int CAICallback::GetEnemyUnits(int* unitIds, int unitIds_max)
{
	verify();
	myAllyTeamId = teamHandler->AllyTeam(team);
	return FilterUnitsVector(unitHandler->activeUnits, unitIds, unitIds_max, &unit_IsEnemyAndInLos);
}

int CAICallback::GetEnemyUnitsInRadarAndLos(int* unitIds, int unitIds_max)
{
	verify();
	myAllyTeamId = teamHandler->AllyTeam(team);
	return FilterUnitsVector(unitHandler->activeUnits, unitIds, unitIds_max, &unit_IsEnemyAndInLosOrRadar);
}

int CAICallback::GetEnemyUnits(int* unitIds, const float3& pos, float radius,
		int unitIds_max)
{
	verify();
	const std::vector<CUnit*>& units = quadField->GetUnitsExact(pos, radius);
	myAllyTeamId = teamHandler->AllyTeam(team);
	return FilterUnitsVector(units, unitIds, unitIds_max, &unit_IsEnemyAndInLos);
}


int CAICallback::GetFriendlyUnits(int* unitIds, int unitIds_max)
{
	verify();
	myAllyTeamId = teamHandler->AllyTeam(team);
	return FilterUnitsVector(unitHandler->activeUnits, unitIds, unitIds_max, &unit_IsFriendly);
}

int CAICallback::GetFriendlyUnits(int* unitIds, const float3& pos, float radius,
		int unitIds_max)
{
	verify();
	const std::vector<CUnit*>& units = quadField->GetUnitsExact(pos, radius);
	myAllyTeamId = teamHandler->AllyTeam(team);
	return FilterUnitsVector(units, unitIds, unitIds_max, &unit_IsFriendly);
}


int CAICallback::GetNeutralUnits(int* unitIds, int unitIds_max)
{
	verify();
	myAllyTeamId = teamHandler->AllyTeam(team);
	return FilterUnitsVector(unitHandler->activeUnits, unitIds, unitIds_max, &unit_IsNeutralAndInLos);
}

int CAICallback::GetNeutralUnits(int* unitIds, const float3& pos, float radius, int unitIds_max)
{
	verify();
	const std::vector<CUnit*>& units = quadField->GetUnitsExact(pos, radius);
	myAllyTeamId = teamHandler->AllyTeam(team);
	return FilterUnitsVector(units, unitIds, unitIds_max, &unit_IsNeutralAndInLos);
}




int CAICallback::GetMapWidth()
{
	return mapDims.mapx;
}

int CAICallback::GetMapHeight()
{
	return mapDims.mapy;
}



float CAICallback::GetMaxMetal() const {
	return mapInfo->map.maxMetal;
}

float CAICallback::GetExtractorRadius() const {
	return mapInfo->map.extractorRadius;
}

float CAICallback::GetMinWind() const {
	return wind.GetMinWind();
}

float CAICallback::GetMaxWind() const {
	return wind.GetMaxWind();
}

float CAICallback::GetCurWind() const {
	return wind.GetCurrentStrength();
}

float CAICallback::GetTidalStrength() const {
	return mapInfo->map.tidalStrength;
}

float CAICallback::GetGravity() const {
	return mapInfo->map.gravity;
}


const float* CAICallback::GetHeightMap()
{
	return &readMap->GetCenterHeightMapSynced()[0];
}

const float* CAICallback::GetCornersHeightMap()
{
	return readMap->GetCornerHeightMapSynced();
}

float CAICallback::GetMinHeight() { return readMap->GetInitMinHeight(); }
float CAICallback::GetMaxHeight() { return readMap->GetInitMaxHeight(); }

const float* CAICallback::GetSlopeMap()
{
	return readMap->GetSlopeMapSynced();
}

const unsigned short* CAICallback::GetLosMap()
{
	return &losHandler->los.losMaps[teamHandler->AllyTeam(team)].front();
}

const unsigned short* CAICallback::GetRadarMap()
{
	return &losHandler->radar.losMaps[teamHandler->AllyTeam(team)].front();
}

const unsigned short* CAICallback::GetJammerMap()
{
	const int jammerAllyTeam = modInfo.separateJammers ? teamHandler->AllyTeam(team) : 0;
	return &losHandler->jammer.losMaps[jammerAllyTeam].front();
}

const unsigned char* CAICallback::GetMetalMap()
{
	return (readMap->metalMap->GetDistributionMap());
}

float CAICallback::GetElevation(float x, float z)
{
	return CGround::GetHeightReal(x, z);
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


int CAICallback::CreateSplineFigure(const float3& pos1, const float3& pos2,
		const float3& pos3, const float3& pos4, float width, int arrow,
		int lifetime, int group)
{
	return geometricObjects->AddSpline(pos1, pos2, pos3, pos4, width, arrow, lifetime, group);
}

int CAICallback::CreateLineFigure(const float3& pos1, const float3& pos2,
		float width, int arrow, int lifetime, int group)
{
	return geometricObjects->AddLine(pos1, pos2, width, arrow, lifetime, group);
}

void CAICallback::SetFigureColor(int group, float red, float green, float blue, float alpha)
{
	geometricObjects->SetColor(group, red, green, blue, alpha);
}

void CAICallback::DeleteFigureGroup(int group)
{
	geometricObjects->DeleteGroup(group);
}



void CAICallback::DrawUnit(
	const char* unitName,
	const float3& pos,
	float rotation,
	int lifetime,
	int teamId,
	bool transparent,
	bool drawBorder,
	int facing
) {
	CUnitDrawer::TempDrawUnit tdu;
	tdu.unitDef = unitDefHandler->GetUnitDefByName(unitName);

	if (tdu.unitDef == nullptr) {
		LOG_L(L_WARNING, "Unknown unit in CAICallback::DrawUnit %s", unitName);
		return;
	}

	tdu.team = teamId;
	tdu.facing = facing;
	tdu.timeout = gs->frameNum + lifetime;

	tdu.pos = pos;
	tdu.rotation = rotation;

	tdu.drawAlpha = transparent;
	tdu.drawBorder = drawBorder;

	unitDrawer->AddTempDrawUnit(tdu);
}



bool CAICallback::CanBuildAt(const UnitDef* unitDef, const float3& pos, int facing)
{
	CFeature* blockingF = NULL;
	BuildInfo bi(unitDef, pos, facing);
	bi.pos = CGameHelper::Pos2BuildPos(bi, false);
	return !!CGameHelper::TestUnitBuildSquare(bi, blockingF, teamHandler->AllyTeam(team), false);
}


float3 CAICallback::ClosestBuildSite(const UnitDef* unitDef, const float3& pos, float searchRadius, int minDist, int facing)
{
	return CGameHelper::ClosestBuildSite(team, unitDef, pos, searchRadius, minDist, facing);
}


float CAICallback::GetMetal()
{
	return teamHandler->Team(team)->res.metal;
}

float CAICallback::GetMetalIncome()
{
	return teamHandler->Team(team)->resPrevIncome.metal;
}

float CAICallback::GetMetalUsage()
{
	return teamHandler->Team(team)->resPrevExpense.metal;
}

float CAICallback::GetMetalStorage()
{
	return teamHandler->Team(team)->resStorage.metal;
}

float CAICallback::GetEnergy()
{
	return teamHandler->Team(team)->res.energy;
}

float CAICallback::GetEnergyIncome()
{
	return teamHandler->Team(team)->resPrevIncome.energy;
}

float CAICallback::GetEnergyUsage()
{
	return teamHandler->Team(team)->resPrevExpense.energy;
}

float CAICallback::GetEnergyStorage()
{
	return teamHandler->Team(team)->resStorage.energy;
}

bool CAICallback::GetUnitResourceInfo(int unitId, UnitResourceInfo* unitResInf)
{
	bool fetchOk = false;

	verify();
	const CUnit* unit = GetInLosUnit(unitId);
	if (unit) {
		unitResInf->energyMake = unit->resourcesMake.energy;
		unitResInf->energyUse  = unit->resourcesUse.energy;
		unitResInf->metalMake  = unit->resourcesMake.metal;
		unitResInf->metalUse   = unit->resourcesUse.metal;
		fetchOk = true;
	}

	return fetchOk;
}

bool CAICallback::IsUnitActivated(int unitId)
{
	bool activated = false;

	verify();
	const CUnit* unit = GetInLosUnit(unitId);
	if (unit) {
		activated = unit->activated;
	}

	return activated;
}

bool CAICallback::UnitBeingBuilt(int unitId)
{
	bool beingBuilt = false;

	verify();
	const CUnit* unit = GetInLosUnit(unitId);
	if (unit) {
		beingBuilt = unit->beingBuilt;
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
			// if it is NULL, the caller only wants to know
			// the number of features
			if (featureIds != NULL) {
				featureIds[featureIds_size] = f->id;
			}
			featureIds_size++;
		}
	}

	return featureIds_size;
}

int CAICallback::GetFeatures(int* featureIds, int featureIds_sizeMax, const float3& pos, float radius)
{
	int featureIds_size = 0;

	verify();
	const std::vector<CFeature*>& ft = quadField->GetFeaturesExact(pos, radius);
	const int allyteam = teamHandler->AllyTeam(team);

	std::vector<CFeature*>::const_iterator it;
	for (it = ft.begin(); (it != ft.end()) && (featureIds_size < featureIds_sizeMax); ++it) {
		const CFeature* f = *it;
		assert(f);

		if (f->IsInLosForAllyTeam(allyteam)) {
			// if it is NULL, the caller only wants to know
			// the number of features
			if (featureIds != NULL) {
				featureIds[featureIds_size] = f->id;
			}
			featureIds_size++;
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
	// NOTE: this function is never called, implemented in SSkirmishAICallbackImpl
	return NULL;
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
		}case AIVAL_MAP_CHECKSUM:{
			*(unsigned int*)data = readMap->GetMapChecksum();
			return true;
		}case AIVAL_DEBUG_MODE:{
			*(bool*)data = globalRendering->drawdebug;
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
			*(static_cast<float3*>(data)) = camHandler->GetCurrentController().GetDir();
			return true;
		}case AIVAL_GUI_CAMERA_POS:{
			*(static_cast<float3*>(data)) = camHandler->GetCurrentController().GetPos();
			return true;
		}case AIVAL_LOCATE_FILE_R:{
			std::string f((char*) data);
			f = dataDirsAccess.LocateFile(f);
			strcpy((char*) data, f.c_str());
			return FileSystem::IsReadableFile(f);
		}case AIVAL_LOCATE_FILE_W:{
			std::string f((char*) data);
			std::string f_abs = dataDirsAccess.LocateFile(f, FileQueryFlags::WRITE | FileQueryFlags::CREATE_DIRS);
			if (!FileSystem::IsAbsolutePath(f_abs)) {
				return false;
			} else {
				strcpy((char*) data, f.c_str());
				return true;
			}
		}
		case AIVAL_UNIT_LIMIT: {
			*(int*) data = teamHandler->Team(team)->GetMaxUnits();
			return true;
		}
		case AIVAL_SCRIPT: {
			*(const char**) data = gameSetup ? gameSetup->setupText.c_str() : "";
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
			const AIHCAddMapPoint* cmdData = static_cast<AIHCAddMapPoint*>(data);
			/*
			   TODO: gu->myPlayerNum makes the command to look like as it comes from the local player,
			   "team" should be used (but needs some major changes in other engine parts)
			*/
			clientNet->Send(CBaseNetProtocol::Get().SendMapDrawPoint(gu->myPlayerNum, (short)cmdData->pos.x, (short)cmdData->pos.z, std::string(cmdData->label), false));
			return 1;
		} break;
		case AIHCAddMapLineId: {
			const AIHCAddMapLine* cmdData = static_cast<AIHCAddMapLine*>(data);
			// see TODO above
			clientNet->Send(CBaseNetProtocol::Get().SendMapDrawLine(gu->myPlayerNum, (short)cmdData->posfrom.x, (short)cmdData->posfrom.z, (short)cmdData->posto.x, (short)cmdData->posto.z, false));
			return 1;
		} break;
		case AIHCRemoveMapPointId: {
			const AIHCRemoveMapPoint* cmdData = static_cast<AIHCRemoveMapPoint*>(data);
			// see TODO above
			clientNet->Send(CBaseNetProtocol::Get().SendMapErase(gu->myPlayerNum, (short)cmdData->pos.x, (short)cmdData->pos.z));
			return 1;
		} break;
		case AIHCSendStartPosId:
		case AIHCGetUnitDefByIdId:
		case AIHCGetWeaponDefByIdId:
		case AIHCGetFeatureDefByIdId:
		case AIHCGetDataDirId:
		{
			// NOTE: these commands should never arrive, handled in SSkirmishAICallbackImpl
			assert(false);
			return 0;
		} break;

		case AIHCTraceRayId: {
			AIHCTraceRay* cmdData = static_cast<AIHCTraceRay*>(data);

			if (CHECK_UNITID(cmdData->srcUID)) {
				const CUnit* srcUnit = unitHandler->units[cmdData->srcUID];

				if (srcUnit != NULL) {
					CUnit* hitUnit = NULL;
					CFeature* hitFeature = NULL;
					//FIXME add COLLISION_NOFEATURE?
					const float realLen = TraceRay::TraceRay(cmdData->rayPos, cmdData->rayDir, cmdData->rayLen, cmdData->flags, srcUnit, hitUnit, hitFeature);

					if (hitUnit != NULL) {
						myAllyTeamId = teamHandler->AllyTeam(team);
						const bool isUnitVisible = unit_IsInLos(hitUnit);
						if (isUnitVisible) {
							cmdData->rayLen = realLen;
							cmdData->hitUID = hitUnit->id;
						}
					}
				}
			}

			return 1;
		} break;

		case AIHCFeatureTraceRayId: {
			AIHCFeatureTraceRay* cmdData = static_cast<AIHCFeatureTraceRay*>(data);

			if (CHECK_UNITID(cmdData->srcUID)) {
				const CUnit* srcUnit = unitHandler->units[cmdData->srcUID];

				if (srcUnit != NULL) {
					CUnit* hitUnit = NULL;
					CFeature* hitFeature = NULL;
					//FIXME add COLLISION_NOENEMIES || COLLISION_NOFRIENDLIES || COLLISION_NONEUTRALS?
					const float realLen = TraceRay::TraceRay(cmdData->rayPos, cmdData->rayDir, cmdData->rayLen, cmdData->flags, srcUnit, hitUnit, hitFeature);

					if (hitFeature != NULL) {
						const bool isFeatureVisible = hitFeature->IsInLosForAllyTeam(teamHandler->AllyTeam(team));
						if (isFeatureVisible) {
							cmdData->rayLen = realLen;
							cmdData->hitFID = hitFeature->id;
						}
					}
				}
			}

			return 1;
		} break;

		case AIHCPauseId: {
			AIHCPause* cmdData = static_cast<AIHCPause*>(data);

			clientNet->Send(CBaseNetProtocol::Get().SendPause(gu->myPlayerNum, cmdData->enable));
			LOG("Skirmish AI controlling team %i paused the game, reason: %s",
					team,
					cmdData->reason != NULL ? cmdData->reason : "UNSPECIFIED");

			return 1;
		} break;

		case AIHCDebugDrawId: {
			AIHCDebugDraw* cmdData = static_cast<AIHCDebugDraw*>(data);

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

		default: {
			return 0;
		}
	}
}

bool CAICallback::IsDebugDrawerEnabled() const
{
	// this function will never be called,
	// as it is handled in the C layer directly
	// see eg. Debug_Drawer_isEnabled in rts/ExternalAI/Interface/SSkirmishAICallback.h
	return debugDrawerAI->GetDraw();
}

int CAICallback::GetNumUnitDefs ()
{
	// defid=0 is not valid, that's why "-1"
	return unitDefHandler->unitDefs.size() - 1;
}

void CAICallback::GetUnitDefList (const UnitDef** list)
{
	for (int ud = 1; ud < unitDefHandler->unitDefs.size(); ud++) {
		list[ud-1] = unitDefHandler->GetUnitDefByID(ud);
	}
}


float CAICallback::GetUnitDefRadius(int def)
{
	const UnitDef* ud = unitDefHandler->GetUnitDefByID(def);
	S3DModel* mdl = ud->LoadModel();
	return mdl->radius;
}

float CAICallback::GetUnitDefHeight(int def)
{
	const UnitDef* ud = unitDefHandler->GetUnitDefByID(def);
	S3DModel* mdl = ud->LoadModel();
	return mdl->height;
}


bool CAICallback::GetProperty(int unitId, int property, void* data)
{
	verify();
	if (CHECK_UNITID(unitId)) {
		const CUnit* unit = unitHandler->units[unitId];
		const int allyTeam = teamHandler->AllyTeam(team);
		myAllyTeamId = allyTeam;
		if (!(unit && unit_IsInLos(unit))) {
			// the unit does not exist or can not be seen
			return false;
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
			case AIVAL_CURRENT_FUEL: { //Deprecated
				(*(float*)data) = 0;
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
				(*(float*) data) = unit->moveType->GetMaxSpeed();
				return true;
			}
			default:
				return false;
		}
	}
	return false;
}


int CAICallback::GetFileSize(const char *filename)
{
	CFileHandler fh (filename);

	if (!fh.FileExists ())
		return -1;

	return fh.FileSize();
}


int CAICallback::GetFileSize(const char* filename, const char* modes)
{
	CFileHandler fh (filename, modes);

	if (!fh.FileExists ())
		return -1;

	return fh.FileSize();
}


bool CAICallback::ReadFile(const char* filename, void* buffer, int bufferLength)
{
	CFileHandler fh (filename);
	int fs;
	if (!fh.FileExists() || bufferLength < (fs = fh.FileSize()))
		return false;

	fh.Read (buffer, fs);
	return true;
}


bool CAICallback::ReadFile(const char* filename, const char* modes,
		void* buffer, int bufferLength)
{
	CFileHandler fh (filename, modes);
	int fs;
	if (!fh.FileExists() || bufferLength < (fs = fh.FileSize()))
		return false;

	fh.Read (buffer, fs);
	return true;
}


// Additions to the interface by Alik
int CAICallback::GetSelectedUnits(int* unitIds, int unitIds_max)
{
	verify();
	int a = 0;

	// check if the allyteam of the player running
	// the AI lib matches the AI's actual allyteam
	if (gu->myAllyTeam == teamHandler->AllyTeam(team)) {
		for (CUnitSet::iterator ui = selectedUnitsHandler.selectedUnits.begin();
				(ui != selectedUnitsHandler.selectedUnits.end()) && (a < unitIds_max); ++ui) {
			if (unitIds != NULL) {
				unitIds[a] = (*ui)->id;
			}
			a++;
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


void CAICallback::GetMapPoints(std::vector<PointMarker>& pm, int pm_sizeMax, bool includeAllies)
{
	verify();

	// If the AI is not in the local player's ally team, the draw
	// information for the AIs ally team will not be available to
	// prevent cheating.
	/*
	if (gu->myAllyTeam != teamHandler->AllyTeam(team)) {
		return 0;
	}
	*/

	std::list<int> includeTeamIDs;
	// include our team
	includeTeamIDs.push_back(team);

	// include the team colors of all our allies
	for (int t = 0; t < teamHandler->ActiveTeams(); ++t) {
		if (teamHandler->AlliedTeams(team, t)) {
			includeTeamIDs.push_back(t);
		}
	}

	inMapDrawer->GetPoints(pm, pm_sizeMax, includeTeamIDs);
}

void CAICallback::GetMapLines(std::vector<LineMarker>& lm, int lm_sizeMax, bool includeAllies)
{
	verify();

	// If the AI is not in the local player's ally team, the draw
	// information for the AIs ally team will not be available to
	// prevent cheating.
	/*
	if (gu->myAllyTeam != teamHandler->AllyTeam(team)) {
		return 0;
	}
	*/

	std::list<int> includeTeamIDs;
	// include our team
	includeTeamIDs.push_back(team);

	// include the team colors of all our allies
	for (int t = 0; t < teamHandler->ActiveTeams(); ++t) {
		if (teamHandler->AlliedTeams(team, t)) {
			includeTeamIDs.push_back(t);
		}
	}

	inMapDrawer->GetLines(lm, lm_sizeMax, includeTeamIDs);
}


const WeaponDef* CAICallback::GetWeapon(const char* weaponName)
{
	return weaponDefHandler->GetWeaponDef(weaponName);
}
const WeaponDef* CAICallback::GetWeaponDefById(int weaponDefId)
{
	// NOTE: this function is never called, implemented in SSkirmishAICallbackImpl
	return NULL;
}


bool CAICallback::CanBuildUnit(int unitDefID)
{
	const UnitDef* ud = unitDefHandler->GetUnitDefByID(unitDefID);
	if (ud == NULL) {
		return false;
	}
	return unitHandler->CanBuildUnit(ud, team);
}


const float3* CAICallback::GetStartPos()
{
	return &teamHandler->Team(team)->GetStartPos();
}



#define AICALLBACK_CALL_LUA(HandleName)                                              \
	const char* CAICallback::CallLua ## HandleName(const char* inData, int inSize) { \
		if (lua ## HandleName == NULL) {                                             \
			return NULL;                                                             \
		}                                                                            \
		return lua ## HandleName->RecvSkirmishAIMessage(team, inData, inSize);       \
	}

AICALLBACK_CALL_LUA(Rules)
AICALLBACK_CALL_LUA(UI)

#undef AICALLBACK_CALL_LUA
