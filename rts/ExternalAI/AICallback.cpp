/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ExternalAI/AICallback.h"

#include "Game/Game.h"
#include "Game/Camera.h"
#include "Game/GameHelper.h"
#include "Game/GlobalUnsynced.h"
#include "Game/TraceRay.h"
#include "Game/GameSetup.h"
#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"
#include "Game/SelectedUnitsHandler.h"
#include "Game/InMapDraw.h"
#include "Game/UI/MiniMap.h"
#include "Game/UI/MouseHandler.h"
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
#define CHECK_UNITID(id) ((unsigned)(id) < (unsigned)unitHandler.MaxUnits())
#define CHECK_GROUPID(id) (uiGroupHandlers[team].HasGroup(id))
// With some hacking you can raise an abort (assert) instead of ignoring the id,
//#define CHECK_UNITID(id) (assert(id > 0 && id < unitHandler.MaxUnits()), true)
// ...or disable the check altogether for release.
//#define CHECK_UNITID(id) true


CUnit* CAICallback::GetUnit(int unitId) const
{
	if (CHECK_UNITID(unitId))
		return unitHandler.GetUnit(unitId);

	return nullptr;
}

CUnit* CAICallback::GetMyTeamUnit(int unitId) const
{
	if (!CHECK_UNITID(unitId))
		return nullptr;

	CUnit* unit = unitHandler.GetUnit(unitId);

	if (unit != nullptr && (unit->team == team))
		return unit;

	return nullptr;
}

CUnit* CAICallback::GetInSensorRangeUnit(int unitId, unsigned short losFlags) const
{
	if (!CHECK_UNITID(unitId))
		return nullptr;

	CUnit* unit = unitHandler.GetUnit(unitId);
	// Skip in-sensor-range test if the unit is allied with our team.
	// This prevents errors where an allied unit is starting to build,
	// but is not yet (technically) in LOS, because LOS was not yet updated,
	// and thus would be invisible for us, without the ally check.
	if (unit == nullptr)
		return nullptr;

	if ((teamHandler.AlliedTeams(unit->team, team) || (unit->losStatus[teamHandler.AllyTeam(team)] & losFlags)))
		return unit;

	return nullptr;
}
CUnit* CAICallback::GetInLosUnit(int unitId) const {
	return GetInSensorRangeUnit(unitId, LOS_INLOS);
}
CUnit* CAICallback::GetInLosAndRadarUnit(int unitId) const {
	return GetInSensorRangeUnit(unitId, (LOS_INLOS | LOS_INRADAR));
}


CAICallback::CAICallback(int teamId): team(teamId)
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
	const std::vector<uint8_t>& teamAIs = skirmishAIHandler.GetSkirmishAIsInTeam(this->team);
	const SkirmishAIData* aiData = skirmishAIHandler.GetSkirmishAI(teamAIs[0]); // FIXME is there a better way?

	if (!game->ProcessCommandText(-1, text))
		return;

	LOG("<SkirmishAI: %s %s (team %d)>: %s", aiData->shortName.c_str(), aiData->version.c_str(), team, text);
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
			&& teamHandler.IsValidTeam(receivingTeamId)
			&& teamHandler.Team(receivingTeamId)
			&& teamHandler.Team(team)
			&& !teamHandler.Team(receivingTeamId)->isDead
			&& !teamHandler.Team(team)->isDead)
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
			&& teamHandler.IsValidTeam(receivingTeamId)
			&& teamHandler.Team(receivingTeamId)
			&& teamHandler.Team(team)
			&& !teamHandler.Team(receivingTeamId)->isDead
			&& !teamHandler.Team(team)->isDead)
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
	return teamHandler.AllyTeam(team);
}

int CAICallback::GetPlayerTeam(int playerId)
{
	if (!playerHandler.IsValidPlayer(playerId))
		return -1;

	const CPlayer* pl = playerHandler.Player(playerId);

	if (!pl->spectator)
		return pl->team;

	return -1;
}

const char* CAICallback::GetTeamSide(int teamId)
{
	if (teamHandler.IsValidTeam(teamId))
		return (teamHandler.Team(teamId)->GetSideName());

	return nullptr;
}

void* CAICallback::CreateSharedMemArea(char* name, int size)
{
	handleerror (0, "AI wants to use deprecated function \"CreateSharedMemArea\"",
				"Spring is closing:", MBF_OK | MBF_EXCL);
	return nullptr;
}

void CAICallback::ReleasedSharedMemArea(char* name)
{
	handleerror (0, "AI wants to use deprecated function \"ReleasedSharedMemArea\"",
				"Spring is closing:", MBF_OK | MBF_EXCL);
}

int CAICallback::CreateGroup()
{
	const CGroup* g = uiGroupHandlers[team].CreateNewGroup();
	return g->id;
}

void CAICallback::EraseGroup(int groupId)
{
	if (!CHECK_GROUPID(groupId))
		return;

	uiGroupHandlers[team].RemoveGroup(uiGroupHandlers[team].GetGroup(groupId));
}

bool CAICallback::AddUnitToGroup(int unitId, int groupId)
{
	CUnit* unit = GetMyTeamUnit(unitId);

	if (unit == nullptr)
		return false;

	return (CHECK_GROUPID(groupId) && unit->SetGroup(uiGroupHandlers[team].GetGroup(groupId)));
}

bool CAICallback::RemoveUnitFromGroup(int unitId)
{
	CUnit* unit = GetMyTeamUnit(unitId);

	if (unit == nullptr)
		return false;

	unit->SetGroup(nullptr);
	return true;
}

int CAICallback::GetUnitGroup(int unitId)
{
	const CUnit* unit = GetMyTeamUnit(unitId);

	if (unit == nullptr)
		return -1;

	const CGroup* group = unit->GetGroup();

	if (group != nullptr)
		return group->id;

	return -1;
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

	if (!CHECK_UNITID(unitId) || c == nullptr)
		return -1;

	if (!allowOrders)
		return -2;

	const CUnit * unit = unitHandler.GetUnit(unitId);

	if (unit == nullptr)
		return -3;

	if (unit->team != team)
		return -5;

	clientNet->Send(CBaseNetProtocol::Get().SendAICommand(gu->myPlayerNum, skirmishAIHandler.GetCurrentAIID(), team, unitId, c->GetID(false), c->GetID(true), c->GetTimeOut(), c->GetOpts(), c->GetNumParams(), c->GetParams()));
	return 0;
}

const std::vector<const SCommandDescription*>* CAICallback::GetUnitCommands(int unitId)
{
	const CUnit* unit = GetMyTeamUnit(unitId);

	if (unit != nullptr)
		return &unit->commandAI->possibleCommands;

	return nullptr;
}

const CCommandQueue* CAICallback::GetCurrentUnitCommands(int unitId)
{
	const CUnit* unit = GetMyTeamUnit(unitId);

	if (unit != nullptr)
		return &unit->commandAI->commandQue;

	return nullptr;
}

int CAICallback::GetUnitAiHint(int unitId)
{
	return -1;
}

int CAICallback::GetUnitTeam(int unitId)
{
	verify();

	const CUnit* unit = GetInLosUnit(unitId);

	if (unit != nullptr)
		return unit->team;

	return -1;
}

int CAICallback::GetUnitAllyTeam(int unitId)
{
	verify();

	const CUnit* unit = GetInLosUnit(unitId);

	if (unit != nullptr)
		return unit->allyteam;

	return -1;
}

float CAICallback::GetUnitHealth(int unitId)
{
	float health = -1.0f;

	verify();
	if (CHECK_UNITID(unitId)) {
		const CUnit* unit = unitHandler.GetUnit(unitId);

		if (unit == nullptr)
			return health;

		const int allyTeam = teamHandler.AllyTeam(team);

		if (teamHandler.Ally(unit->allyteam, allyTeam)) {
			health = unit->health;
		} else if (unit->losStatus[allyTeam] & LOS_INLOS) {
			const UnitDef* unitDef = unit->unitDef;
			const UnitDef* decoyDef = unitDef->decoyDef;

			if (decoyDef == nullptr) {
				health = unit->health;
			} else {
				health = unit->health * (decoyDef->health / unitDef->health);
			}
		}
	}

	return health;
}

float CAICallback::GetUnitMaxHealth(int unitId)
{
	float maxHealth = -1.0f;

	verify();
	if (CHECK_UNITID(unitId)) {
		const CUnit* unit = unitHandler.GetUnit(unitId);

		if (unit == nullptr)
			return maxHealth;

		const int allyTeam = teamHandler.AllyTeam(team);
		if (teamHandler.Ally(unit->allyteam, allyTeam)) {
			maxHealth = unit->maxHealth;
		} else if (unit->losStatus[allyTeam] & LOS_INLOS) {
			const UnitDef* unitDef = unit->unitDef;
			const UnitDef* decoyDef = unitDef->decoyDef;
			if (decoyDef == nullptr) {
				maxHealth = unit->maxHealth;
			} else {
				const float scale = (decoyDef->health / unitDef->health);
				maxHealth = unit->maxHealth * scale;
			}
		}
	}

	return maxHealth;
}

float CAICallback::GetUnitSpeed(int unitId)
{
	float speed = 0.0f;

	verify();
	if (CHECK_UNITID(unitId)) {
		const CUnit* unit = unitHandler.GetUnit(unitId);

		if (unit == nullptr)
			return speed;

		const int allyTeam = teamHandler.AllyTeam(team);
		if (teamHandler.Ally(unit->allyteam, allyTeam)) {
			speed = unit->moveType->GetMaxSpeed();
		} else if (unit->losStatus[allyTeam] & LOS_INLOS) {
			const UnitDef* unitDef = unit->unitDef;
			const UnitDef* decoyDef = unitDef->decoyDef;
			if (decoyDef == nullptr) {
				speed = unitDef->speed;
			} else {
				speed = decoyDef->speed;
			}
		}
	}

	return speed;
}

float CAICallback::GetUnitPower(int unitId)
{
	float power = -1.0f;

	verify();
	if (CHECK_UNITID(unitId)) {
		const CUnit* unit = unitHandler.GetUnit(unitId);

		if (unit == nullptr)
			return power;

		const int allyTeam = teamHandler.AllyTeam(team);
		if (teamHandler.Ally(unit->allyteam, allyTeam)) {
			power = unit->power;
		} else if (unit->losStatus[allyTeam] & LOS_INLOS) {
			const UnitDef* unitDef = unit->unitDef;
			const UnitDef* decoyDef = unitDef->decoyDef;
			if (decoyDef == nullptr) {
				power = unit->power;
			} else {
				const float scale = (decoyDef->power / unitDef->power);
				power = unit->power * scale;
			}
		}
	}

	return power;
}

float CAICallback::GetUnitExperience(int unitId)
{
	verify();
	const CUnit* unit = GetInLosUnit(unitId);

	if (unit != nullptr)
		return unit->experience;

	return -1.0f;
}

float CAICallback::GetUnitMaxRange(int unitId)
{
	float maxRange = -1.0f;

	verify();
	if (CHECK_UNITID(unitId)) {
		const CUnit* unit = unitHandler.GetUnit(unitId);

		if (unit == nullptr)
			return maxRange;

		const int allyTeam = teamHandler.AllyTeam(team);
		if (teamHandler.Ally(unit->allyteam, allyTeam)) {
			maxRange = unit->maxRange;
		} else if (unit->losStatus[allyTeam] & LOS_INLOS) {
			const UnitDef* unitDef = unit->unitDef;
			const UnitDef* decoyDef = unitDef->decoyDef;
			if (decoyDef == nullptr) {
				maxRange = unit->maxRange;
			} else {
				maxRange = decoyDef->maxWeaponRange;
			}
		}
	}

	return maxRange;
}

const UnitDef* CAICallback::GetUnitDef(int unitId)
{
	const UnitDef* def = nullptr;

	verify();
	if (CHECK_UNITID(unitId)) {
		const CUnit* unit = unitHandler.GetUnit(unitId);

		if (unit == nullptr)
			return def;

		const UnitDef* unitDef = unit->unitDef;
		const int allyTeam = teamHandler.AllyTeam(team);
		if (teamHandler.Ally(unit->allyteam, allyTeam)) {
			def = unitDef;
		} else {
			const unsigned short losStatus = unit->losStatus[allyTeam];
			const unsigned short prevMask = (LOS_PREVLOS | LOS_CONTRADAR);
			if (((losStatus & LOS_INLOS) != 0) ||
					((losStatus & prevMask) == prevMask)) {
				const UnitDef* decoyDef = unitDef->decoyDef;
				if (decoyDef == nullptr) {
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
const UnitDef* CAICallback::GetUnitDefById(int unitDefId)
{
	// NOTE: this function is never called, implemented in SSkirmishAICallbackImpl
	return nullptr;
}


float3 CAICallback::GetUnitPos(int unitId)
{
	verify();

	const CUnit* unit = GetInLosAndRadarUnit(unitId);

	if (unit != nullptr)
		return (unit->GetErrorPos(teamHandler.AllyTeam(team)));

	return ZeroVector;
}

float3 CAICallback::GetUnitVelocity(int unitId)
{
	verify();

	const CUnit* unit = GetInLosAndRadarUnit(unitId);

	if (unit != nullptr)
		return unit->speed;

	return ZeroVector;
}



int CAICallback::GetBuildingFacing(int unitId) {
	verify();

	const CUnit* unit = GetInLosUnit(unitId);

	if (unit != nullptr)
		return unit->buildFacing;

	return -1;
}

bool CAICallback::IsUnitCloaked(int unitId) {
	verify();

	const CUnit* unit = GetInLosUnit(unitId);

	if (unit != nullptr)
		return unit->isCloaked;

	return false;
}

bool CAICallback::IsUnitParalyzed(int unitId) {
	verify();

	const CUnit* unit = GetInLosUnit(unitId);

	if (unit != nullptr)
		return (unit->IsStunned());

	return false;
}

bool CAICallback::IsUnitNeutral(int unitId) {
	verify();

	const CUnit* unit = GetInLosAndRadarUnit(unitId);

	if (unit != nullptr)
		return unit->IsNeutral();

	return false;
}



int CAICallback::InitPath(const float3& start, const float3& end, int pathType, float goalRadius)
{
	assert(((size_t)pathType) < moveDefHandler.GetNumMoveDefs());
	return pathManager->RequestPath(nullptr, moveDefHandler.GetMoveDefByPathType(pathType), start, end, goalRadius, false);
}

float3 CAICallback::GetNextWaypoint(int pathId)
{
	return pathManager->NextWayPoint(nullptr, pathId, 0, ZeroVector, 0.0f, false);
}

void CAICallback::FreePath(int pathId)
{
	pathManager->DeletePath(pathId);
}

float CAICallback::GetPathLength(float3 start, float3 end, int pathType, float goalRadius)
{
	const int pathID  = InitPath(start, end, pathType, goalRadius);
	float     pathLen = -1.0f;

	if (pathID == 0)
		return pathLen;

	std::vector<float3> points;
	std::vector<int>    lengths;

	pathManager->GetPathWayPoints(pathID, points, lengths);

	// non-zero pathID means at least a partial path was found
	// but only raw search does not add waypoints, just return
	// the Euclidean estimate
	if (points.empty())
		return (start.distance(end));

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



static int FilterUnitsVector(const std::vector<CUnit*>& units, int* unitIds, int maxUnitIds, bool (*includeUnit)(const CUnit*) = nullptr)
{
	int a = 0;

	if (maxUnitIds < 0) {
		unitIds = nullptr;
		maxUnitIds = MAX_UNITS;
	}

	for (auto ui = units.begin(); (ui != units.end()) && (a < maxUnitIds); ++ui) {
		const CUnit* u = *ui;

		if ((includeUnit == nullptr) || (*includeUnit)(u)) {
			if (unitIds != nullptr)
				unitIds[a] = u->id;

			a++;
		}
	}

	return a;
}


static int myAllyTeamId = -1;

/// You have to set myAllyTeamId before calling this function. NOT thread safe!
static inline bool unit_IsEnemy(const CUnit* unit) {
	return (!teamHandler.Ally(unit->allyteam, myAllyTeamId) && !unit->IsNeutral());
}

/// You have to set myAllyTeamId before calling this function. NOT thread safe!
static inline bool unit_IsFriendly(const CUnit* unit) {
	return (teamHandler.Ally(unit->allyteam, myAllyTeamId) && !unit->IsNeutral());
}

/// You have to set myAllyTeamId before calling this function. NOT thread safe!
static inline bool unit_IsInSensor(const CUnit* unit, const unsigned short losFlags) {
	// Skip in-sensor-range test if the unit is allied with our team.
	// This prevents errors where an allied unit is starting to build,
	// but is not yet (technically) in LOS, because LOS was not yet updated,
	// and thus would be invisible for us, without the ally check.
	return (teamHandler.Ally(myAllyTeamId, unit->allyteam) || ((unit->losStatus[myAllyTeamId] & losFlags) != 0));
}

/// You have to set myAllyTeamId before calling this function. NOT thread safe!
static inline bool unit_IsInLos(const CUnit* unit) {
	return unit_IsInSensor(unit, LOS_INLOS);
}

/// You have to set myAllyTeamId before calling this function. NOT thread safe!
static inline bool unit_IsEnemyAndInLos(const CUnit* unit) {
	return (unit_IsEnemy(unit) && unit_IsInLos(unit));
}

/// You have to set myAllyTeamId before calling this function. NOT thread safe!
static inline bool unit_IsEnemyAndInLosOrRadar(const CUnit* unit) {
	return (unit_IsEnemy(unit) && ((unit->losStatus[myAllyTeamId] & (LOS_INLOS | LOS_INRADAR)) != 0));
}

/// You have to set myAllyTeamId before calling this function. NOT thread safe!
static inline bool unit_IsNeutralAndInLosOrRadar(const CUnit* unit) {
	return (unit->IsNeutral() && (unit_IsInSensor(unit, LOS_INLOS | LOS_INRADAR)));
}

int CAICallback::GetEnemyUnits(int* unitIds, int unitIds_max)
{
	verify();
	myAllyTeamId = teamHandler.AllyTeam(team);
	return FilterUnitsVector(unitHandler.GetActiveUnits(), unitIds, unitIds_max, &unit_IsEnemyAndInLos);
}

int CAICallback::GetEnemyUnitsInRadarAndLos(int* unitIds, int unitIds_max)
{
	verify();
	myAllyTeamId = teamHandler.AllyTeam(team);
	return FilterUnitsVector(unitHandler.GetActiveUnits(), unitIds, unitIds_max, &unit_IsEnemyAndInLosOrRadar);
}

int CAICallback::GetEnemyUnits(int* unitIds, const float3& pos, float radius,
		int unitIds_max)
{
	verify();
	QuadFieldQuery qfQuery;
	quadField.GetUnitsExact(qfQuery, pos, radius);
	myAllyTeamId = teamHandler.AllyTeam(team);
	return FilterUnitsVector(*qfQuery.units, unitIds, unitIds_max, &unit_IsEnemyAndInLos);
}


int CAICallback::GetFriendlyUnits(int* unitIds, int unitIds_max)
{
	verify();
	myAllyTeamId = teamHandler.AllyTeam(team);
	return FilterUnitsVector(unitHandler.GetActiveUnits(), unitIds, unitIds_max, &unit_IsFriendly);
}

int CAICallback::GetFriendlyUnits(int* unitIds, const float3& pos, float radius,
		int unitIds_max)
{
	verify();
	QuadFieldQuery qfQuery;
	quadField.GetUnitsExact(qfQuery, pos, radius);
	myAllyTeamId = teamHandler.AllyTeam(team);
	return FilterUnitsVector(*qfQuery.units, unitIds, unitIds_max, &unit_IsFriendly);
}


int CAICallback::GetNeutralUnits(int* unitIds, int unitIds_max)
{
	verify();
	myAllyTeamId = teamHandler.AllyTeam(team);
	return FilterUnitsVector(unitHandler.GetActiveUnits(), unitIds, unitIds_max, &unit_IsNeutralAndInLosOrRadar);
}

int CAICallback::GetNeutralUnits(int* unitIds, const float3& pos, float radius, int unitIds_max)
{
	verify();
	QuadFieldQuery qfQuery;
	quadField.GetUnitsExact(qfQuery, pos, radius);
	myAllyTeamId = teamHandler.AllyTeam(team);
	return FilterUnitsVector(*qfQuery.units, unitIds, unitIds_max, &unit_IsNeutralAndInLosOrRadar);
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
	return envResHandler.GetMinWindStrength();
}

float CAICallback::GetMaxWind() const {
	return envResHandler.GetMaxWindStrength();
}

float CAICallback::GetCurWind() const {
	return envResHandler.GetCurrentWindStrength();
}

float CAICallback::GetTidalStrength() const {
	return envResHandler.GetCurrentTidalStrength();
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
	return &losHandler->los.losMaps[teamHandler.AllyTeam(team)].front();
}

const unsigned short* CAICallback::GetRadarMap()
{
	return &losHandler->radar.losMaps[teamHandler.AllyTeam(team)].front();
}

const unsigned short* CAICallback::GetJammerMap()
{
	const int jammerAllyTeam = modInfo.separateJammers ? teamHandler.AllyTeam(team) : 0;
	return &losHandler->jammer.losMaps[jammerAllyTeam].front();
}

const unsigned char* CAICallback::GetMetalMap()
{
	return (metalMap.GetDistributionMap());
}

float CAICallback::GetElevation(float x, float z)
{
	return CGround::GetHeightReal(x, z);
}




void CAICallback::LineDrawerStartPath(const float3& pos, const float* color)
{
	lineDrawer.StartPath(pos, color);
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
	CFeature* blockingF = nullptr;
	BuildInfo bi(unitDef, pos, facing);
	bi.pos = CGameHelper::Pos2BuildPos(bi, false);
	return !!CGameHelper::TestUnitBuildSquare(bi, blockingF, teamHandler.AllyTeam(team), false);
}


float3 CAICallback::ClosestBuildSite(const UnitDef* unitDef, const float3& pos, float searchRadius, int minDist, int facing)
{
	return CGameHelper::ClosestBuildPos(team, unitDef, pos, searchRadius, minDist, facing);
}


float CAICallback::GetMetal()
{
	return teamHandler.Team(team)->res.metal;
}

float CAICallback::GetMetalIncome()
{
	return teamHandler.Team(team)->resPrevIncome.metal;
}

float CAICallback::GetMetalUsage()
{
	return teamHandler.Team(team)->resPrevExpense.metal;
}

float CAICallback::GetMetalStorage()
{
	return teamHandler.Team(team)->resStorage.metal;
}

float CAICallback::GetEnergy()
{
	return teamHandler.Team(team)->res.energy;
}

float CAICallback::GetEnergyIncome()
{
	return teamHandler.Team(team)->resPrevIncome.energy;
}

float CAICallback::GetEnergyUsage()
{
	return teamHandler.Team(team)->resPrevExpense.energy;
}

float CAICallback::GetEnergyStorage()
{
	return teamHandler.Team(team)->resStorage.energy;
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



int CAICallback::GetFeatures(int* featureIds, int maxFeatureIDs)
{
	int numFeatureIDs = 0;

	verify();
	const int allyteam = teamHandler.AllyTeam(team);

	// non-spatial query
	for (const int featureID: featureHandler.GetActiveFeatureIDs()) {
		if (numFeatureIDs >= maxFeatureIDs)
			break;

		const CFeature* f = featureHandler.GetFeature(featureID);

		assert(f != nullptr);

		if (!f->IsInLosForAllyTeam(allyteam))
			continue;

		// if array is nullptr, caller only wants to know the number of features
		if (featureIds != nullptr)
			featureIds[numFeatureIDs] = f->id;

		numFeatureIDs++;
	}

	return numFeatureIDs;
}

int CAICallback::GetFeatures(int* featureIds, int maxFeatureIDs, const float3& pos, float radius)
{
	int numFeatureIDs = 0;

	verify();
	QuadFieldQuery qfQuery;
	quadField.GetFeaturesExact(qfQuery, pos, radius);
	const int allyteam = teamHandler.AllyTeam(team);

	for (const CFeature* f: *qfQuery.features) {
		if (numFeatureIDs >= maxFeatureIDs)
			break;

		if (!f->IsInLosForAllyTeam(allyteam))
			continue;

		// if array is nullptr, caller only wants to know the number of features
		if (featureIds != nullptr)
			featureIds[numFeatureIDs] = f->id;

		numFeatureIDs++;
	}

	return numFeatureIDs;
}



const FeatureDef* CAICallback::GetFeatureDef(int featureId)
{
	verify();

	const FeatureDef* featureDef = nullptr;
	const CFeature* f = featureHandler.GetFeature(featureId);

	if (f != nullptr && f->IsInLosForAllyTeam(teamHandler.AllyTeam(team)))
		featureDef = f->def;

	return featureDef;
}
const FeatureDef* CAICallback::GetFeatureDefById(int featureDefId)
{
	// NOTE: this function is never called, implemented in SSkirmishAICallbackImpl
	return nullptr;
}

float CAICallback::GetFeatureHealth(int featureId)
{
	verify();

	const CFeature* f = featureHandler.GetFeature(featureId);

	if (f == nullptr)
		return 0.0f;

	const int allyteam = teamHandler.AllyTeam(team);

	if (f->IsInLosForAllyTeam(allyteam))
		return f->health;

	return 0.0f;
}

float CAICallback::GetFeatureReclaimLeft(int featureId)
{
	verify();

	const CFeature* f = featureHandler.GetFeature(featureId);

	if (f == nullptr)
		return 0.0f;

	const int allyteam = teamHandler.AllyTeam(team);

	if (f->IsInLosForAllyTeam(allyteam))
		return f->reclaimLeft;

	return 0.0f;
}

float3 CAICallback::GetFeaturePos(int featureId)
{
	verify();

	const CFeature* f = featureHandler.GetFeature(featureId);

	if (f == nullptr)
		return ZeroVector;

	const int allyteam = teamHandler.AllyTeam(team);

	if (f->IsInLosForAllyTeam(allyteam))
		return f->pos;

	return ZeroVector;
}

bool CAICallback::GetValue(int id, void* data)
{
	verify();

	switch (id) {
		case AIVAL_NUMDAMAGETYPES: {
			*((int*) data) = damageArrayHandler.GetNumTypes();
		} break;
		case AIVAL_MAP_CHECKSUM: {
			*(unsigned int*) data = readMap->GetMapChecksum();
		} break;
		case AIVAL_DEBUG_MODE: {
			*(bool*) data = globalRendering->drawDebug;
		} break;

		case AIVAL_GAME_PAUSED: {
			*(bool*) data = gs->paused;
		} break;
		case AIVAL_GAME_SPEED_FACTOR: {
			*(float*) data = gs->speedFactor;
		} break;

		case AIVAL_GUI_VIEW_RANGE: {
			*(float*) data = camera->GetFarPlaneDist();
		} break;
		case AIVAL_GUI_SCREENX: {
			*(float*) data = globalRendering->viewSizeX;
		} break;
		case AIVAL_GUI_SCREENY: {
			*(float*) data = globalRendering->viewSizeY;
		} break;

		case AIVAL_LOCATE_FILE_R: {
			const std::string frel((char*) data);
			const std::string fabs(dataDirsAccess.LocateFile(frel));

			strcpy((char*) data, fabs.c_str());
			return FileSystem::IsReadableFile(fabs);
		} break;
		case AIVAL_LOCATE_FILE_W: {
			const std::string frel((char*) data);
			const std::string fabs(dataDirsAccess.LocateFile(frel, FileQueryFlags::WRITE | FileQueryFlags::CREATE_DIRS));

			if (!FileSystem::IsAbsolutePath(fabs))
				return false;

			strcpy((char*) data, frel.c_str());
		} break;

		case AIVAL_UNIT_LIMIT: {
			*(int*) data = teamHandler.Team(team)->GetMaxUnits();
		} break;
		case AIVAL_SCRIPT: {
			*(const char**) data = gameSetup->setupText.c_str();
		} break;

		default: {
			return false;
		} break;
	}

	return true;
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
				const CUnit* srcUnit = unitHandler.GetUnit(cmdData->srcUID);

				if (srcUnit != nullptr) {
					CUnit* hitUnit = nullptr;
					CFeature* hitFeature = nullptr;

					//FIXME add COLLISION_NOFEATURE?
					const float realLen = TraceRay::TraceRay(cmdData->rayPos, cmdData->rayDir, cmdData->rayLen, cmdData->flags, srcUnit, hitUnit, hitFeature);

					if (hitUnit != nullptr) {
						myAllyTeamId = teamHandler.AllyTeam(team);

						if (unit_IsInLos(hitUnit)) {
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
				const CUnit* srcUnit = unitHandler.GetUnit(cmdData->srcUID);

				if (srcUnit != nullptr) {
					CUnit* hitUnit = nullptr;
					CFeature* hitFeature = nullptr;

					//FIXME add COLLISION_NOENEMIES || COLLISION_NOFRIENDLIES || COLLISION_NONEUTRALS?
					const float realLen = TraceRay::TraceRay(cmdData->rayPos, cmdData->rayDir, cmdData->rayLen, cmdData->flags, srcUnit, hitUnit, hitFeature);

					if (hitFeature != nullptr) {
						if (hitFeature->IsInLosForAllyTeam(teamHandler.AllyTeam(team))) {
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
					cmdData->reason != nullptr ? cmdData->reason : "UNSPECIFIED");

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
	return debugDrawerAI->IsEnabled();
}

int CAICallback::GetNumUnitDefs()
{
	return (unitDefHandler->NumUnitDefs());
}

void CAICallback::GetUnitDefList(const UnitDef** list)
{
	for (unsigned int i = 0, n = unitDefHandler->NumUnitDefs(); i < n; i++) {
		list[i] = unitDefHandler->GetUnitDefByID(i + 1);
	}
}


float CAICallback::GetUnitDefRadius(int def)
{
	const UnitDef* ud = unitDefHandler->GetUnitDefByID(def);
	const S3DModel* mdl = ud->LoadModel();
	return mdl->radius;
}

float CAICallback::GetUnitDefHeight(int def)
{
	const UnitDef* ud = unitDefHandler->GetUnitDefByID(def);
	const S3DModel* mdl = ud->LoadModel();
	return mdl->height;
}


bool CAICallback::GetProperty(int unitId, int property, void* data)
{
	verify();
	if (CHECK_UNITID(unitId)) {
		const CUnit* unit = unitHandler.GetUnit(unitId);
		const int allyTeam = teamHandler.AllyTeam(team);

		myAllyTeamId = allyTeam;

		// the unit does not exist or can not be seen
		if (unit == nullptr || !unit_IsInLos(unit))
			return false;

		switch (property) {
			case AIVAL_UNITDEF: {
				if (teamHandler.Ally(unit->allyteam, allyTeam)) {
					(*(const UnitDef**)data) = unit->unitDef;
				} else {
					const UnitDef* unitDef = unit->unitDef;
					const UnitDef* decoyDef = unitDef->decoyDef;
					if (decoyDef == nullptr) {
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
				if (!unit->stockpileWeapon || !teamHandler.Ally(unit->allyteam, allyTeam)) {
					return false;
				}
				(*(int*)data) = unit->stockpileWeapon->numStockpiled;
				return true;
			}
			case AIVAL_STOCKPILE_QUED: {
				if (!unit->stockpileWeapon || !teamHandler.Ally(unit->allyteam, allyTeam)) {
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


int CAICallback::GetFileSize(const char* filename)
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
	if (gu->myAllyTeam == teamHandler.AllyTeam(team)) {
		const auto& selUnits = selectedUnitsHandler.selectedUnits;

		for (auto ui = selUnits.begin(); (ui != selUnits.end()) && (a < unitIds_max); ++ui) {
			if (unitIds != nullptr)
				unitIds[a] = (unitHandler.GetUnit(*ui))->id;

			a++;
		}
	}

	return a;
}


float3 CAICallback::GetMousePos() {
	verify();
	if (gu->myAllyTeam == teamHandler.AllyTeam(team))
		return mouse->GetWorldMapPos();

	return ZeroVector;
}


void CAICallback::GetMapPoints(std::vector<PointMarker>& pm, int maxPoints, bool includeAllies)
{
	verify();

	// If the AI is not in the local player's ally team, the draw
	// information for the AIs ally team will not be available to
	// prevent cheating.
	/*
	if (gu->myAllyTeam != teamHandler.AllyTeam(team))
		return 0;
	*/

	std::array<int, MAX_TEAMS> includeTeamIDs;

	// include our team; first non-entry is indicated by -1
	includeTeamIDs[0] = team;
	includeTeamIDs[1] = -1;

	// include the teams of all our allies, exclude Gaia
	for (int i = 1, t = 0, n = std::min(teamHandler.ActiveTeams(), MAX_TEAMS - 1); t < n; ++t) {
		if (!teamHandler.AlliedTeams(team, t))
			continue;

		includeTeamIDs[i++] = t;
		includeTeamIDs[i  ] = -1;
	}

	inMapDrawer->GetPoints(pm, maxPoints, includeTeamIDs);
}

void CAICallback::GetMapLines(std::vector<LineMarker>& lm, int maxLines, bool includeAllies)
{
	verify();

	// If the AI is not in the local player's ally team, the draw
	// information for the AIs ally team will not be available to
	// prevent cheating.
	/*
	if (gu->myAllyTeam != teamHandler.AllyTeam(team))
		return 0;
	*/

	std::array<int, MAX_TEAMS> includeTeamIDs;

	// include our team; first non-entry is indicated by -1
	includeTeamIDs[0] = team;
	includeTeamIDs[1] = -1;

	// include the teams of all our allies, exclude Gaia
	for (int i = 1, t = 0, n = std::min(teamHandler.ActiveTeams(), MAX_TEAMS - 1); t < n; ++t) {
		if (!teamHandler.AlliedTeams(team, t))
			continue;

		includeTeamIDs[i++] = t;
		includeTeamIDs[i  ] = -1;
	}

	inMapDrawer->GetLines(lm, maxLines, includeTeamIDs);
}


const WeaponDef* CAICallback::GetWeapon(const char* weaponName)
{
	return weaponDefHandler->GetWeaponDef(weaponName);
}
const WeaponDef* CAICallback::GetWeaponDefById(int weaponDefId)
{
	// NOTE: this function is never called, implemented in SSkirmishAICallbackImpl
	return nullptr;
}


bool CAICallback::CanBuildUnit(int unitDefID)
{
	const UnitDef* ud = unitDefHandler->GetUnitDefByID(unitDefID);

	if (ud == nullptr)
		return false;

	return unitHandler.CanBuildUnit(ud, team);
}


const float3* CAICallback::GetStartPos()
{
	return &teamHandler.Team(team)->GetStartPos();
}



#define AICALLBACK_CALL_LUA(HandleName)                                                               \
	const char* CAICallback::CallLua ## HandleName(const char* inData, int inSize, size_t* outSize) { \
		if (lua ## HandleName == nullptr)                                                             \
			return nullptr;                                                                           \
                                                                                                      \
		return lua ## HandleName->RecvSkirmishAIMessage(team, inData, inSize, outSize);               \
	}

AICALLBACK_CALL_LUA(Rules)
AICALLBACK_CALL_LUA(UI)

#undef AICALLBACK_CALL_LUA
