/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "EngineOutHandler.h"

#include "ExternalAI/SkirmishAIWrapper.h"
#include "ExternalAI/SkirmishAIData.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include "ExternalAI/AILibraryManager.h"
#include "ExternalAI/Interface/AISCommands.h"
#include "Game/GlobalUnsynced.h"
#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/Team.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/CommandAI/Command.h"
#include "Sim/Weapons/WeaponDef.h"
#include "Net/Protocol/NetProtocol.h"
#include "System/Log/ILog.h"
#include "System/TimeProfiler.h"
#include "System/SafeUtil.h"


CR_BIND(CEngineOutHandler, )
CR_REG_METADATA(CEngineOutHandler, (
	// FIXME:
	//   "constexpr std::pair<_T1, _T2>::pair(const std::pair<_T1, _T2>&)
	//   [with _T1 = const unsigned char; _T2 = std::unique_ptr<CSkirmishAIWrapper>]’
	//   is implicitly deleted because the default definition would be ill-formed"
	CR_IGNORED(hostSkirmishAIs),
	CR_IGNORED(teamSkirmishAIs),
	CR_IGNORED(activeSkirmishAIs)
))


static inline bool IsUnitInLosOrRadarOfAllyTeam(const CUnit& unit, const int allyTeamId) {
	// NOTE:
	//   we check for globalLOS because the LOS-state of a
	//   new unit has not yet been set when it is created
	//   (thus UnitCreated will not produce EnemyCreated,
	//   etc. without this, even when globalLOS is enabled
	//   (for non-cheating AI's))
	return (losHandler->globalLOS[allyTeamId] || (unit.losStatus[allyTeamId] & (LOS_INLOS | LOS_INRADAR)));
}

static CEngineOutHandler singleton;
static unsigned int numInstances = 0;

CEngineOutHandler* CEngineOutHandler::GetInstance() {
	// if more than one instance, some code called eoh->func()
	// and created another after Destroy was already executed
	// from ~CGame --> usually dtors
	assert(numInstances == 1);
	return &singleton;
}

void CEngineOutHandler::Create() {
	if (numInstances != 0)
		return;

	AILibraryManager::Create();
	singleton.Init();

	numInstances += 1;
}

void CEngineOutHandler::Destroy() {
	if (numInstances != 1)
		return;

	singleton.Kill();
	AILibraryManager::Destroy();

	numInstances -= 1;
}


// This macro should be inserted at the start of each method sending AI events
#define AI_SCOPED_TIMER()           \
	if (activeSkirmishAIs.empty())  \
		return;                     \
	SCOPED_TIMER("AI");

#define DO_FOR_SKIRMISH_AIS(FUNC)            \
	for (uint8_t aiID: activeSkirmishAIs) {  \
		hostSkirmishAIs[aiID].FUNC;          \
	}


void CEngineOutHandler::PreDestroy() {
	AI_SCOPED_TIMER();
	DO_FOR_SKIRMISH_AIS(PreDestroy())
}


void CEngineOutHandler::Load(std::istream* s, const uint8_t skirmishAIId) {
	AI_SCOPED_TIMER();
	LOG_L(L_INFO, "[EOH::%s(id=%u)] active=%d", __func__, skirmishAIId, hostSkirmishAIs[skirmishAIId].Active());

	auto& ai = hostSkirmishAIs[skirmishAIId];

	if (!ai.Active())
		return;

	ai.Load(s);
}

void CEngineOutHandler::Save(std::ostream* s, const uint8_t skirmishAIId) {
	AI_SCOPED_TIMER();
	LOG_L(L_INFO, "[EOH::%s(id=%u)] active=%d", __func__, skirmishAIId, hostSkirmishAIs[skirmishAIId].Active());

	auto& ai = hostSkirmishAIs[skirmishAIId];

	if (!ai.Active())
		return;

	ai.Save(s);
}


void CEngineOutHandler::Update() {
	AI_SCOPED_TIMER();
	DO_FOR_SKIRMISH_AIS(Update(gs->frameNum))
}



// Do only if the unit is not allied, in which case we know
// everything about it anyway, and do not need to be informed
#define DO_FOR_ALLIED_SKIRMISH_AIS(FUNC, ALLY_TEAM_ID, UNIT_ALLY_TEAM_ID)    \
	if (teamHandler.Ally(ALLY_TEAM_ID, UNIT_ALLY_TEAM_ID))                   \
		return;                                                              \
                                                                             \
	for (uint8_t aiID: activeSkirmishAIs) {                                  \
		auto& ai = hostSkirmishAIs[aiID];                                    \
		const int aiAllyTeam = teamHandler.AllyTeam(ai.GetTeamId());         \
                                                                             \
		if (teamHandler.Ally(aiAllyTeam, ALLY_TEAM_ID)) {                    \
			ai.FUNC;                                                         \
		}                                                                    \
	}


void CEngineOutHandler::UnitEnteredLos(const CUnit& unit, int allyTeamId) {
	AI_SCOPED_TIMER();

	const int unitId         = unit.id;
	const int unitAllyTeamId = unit.allyteam;

	DO_FOR_ALLIED_SKIRMISH_AIS(EnemyEnterLOS(unitId), allyTeamId, unitAllyTeamId)
}

void CEngineOutHandler::UnitLeftLos(const CUnit& unit, int allyTeamId) {
	AI_SCOPED_TIMER();

	const int unitId         = unit.id;
	const int unitAllyTeamId = unit.allyteam;

	DO_FOR_ALLIED_SKIRMISH_AIS(EnemyLeaveLOS(unitId), allyTeamId, unitAllyTeamId)
}

void CEngineOutHandler::UnitEnteredRadar(const CUnit& unit, int allyTeamId) {
	AI_SCOPED_TIMER();

	const int unitId         = unit.id;
	const int unitAllyTeamId = unit.allyteam;

	DO_FOR_ALLIED_SKIRMISH_AIS(EnemyEnterRadar(unitId), allyTeamId, unitAllyTeamId)
}

void CEngineOutHandler::UnitLeftRadar(const CUnit& unit, int allyTeamId) {
	AI_SCOPED_TIMER();

	const int unitId         = unit.id;
	const int unitAllyTeamId = unit.allyteam;

	DO_FOR_ALLIED_SKIRMISH_AIS(EnemyLeaveRadar(unitId), allyTeamId, unitAllyTeamId)
}



#define DO_FOR_TEAM_SKIRMISH_AIS(FUNC, TEAM_ID)       \
	if (teamSkirmishAIs[TEAM_ID].empty())             \
		return;                                       \
                                                      \
	for (uint8_t aiID: teamSkirmishAIs[TEAM_ID]) {    \
		hostSkirmishAIs[aiID].FUNC;                   \
	}                                                 \



// Send to all teams which the unit is not allied to,
// and which have cheat-events enabled, or the unit in sensor range.
#define DO_FOR_ENEMY_SKIRMISH_AIS(FUNC, ALLY_TEAM_ID, UNIT)                     \
	for (uint8_t aiID: activeSkirmishAIs) {                                     \
		CSkirmishAIWrapper& aiWrapper = hostSkirmishAIs[aiID];                  \
                                                                                \
		const int aiAllyTeam = teamHandler.AllyTeam(aiWrapper.GetTeamId());     \
                                                                                \
		const bool alliedAI = teamHandler.Ally(aiAllyTeam, ALLY_TEAM_ID);       \
		const bool cheatingAI = aiWrapper.CheatEventsEnabled();                 \
                                                                                \
		if (alliedAI)                                                           \
			continue;                                                           \
		if (!cheatingAI && !IsUnitInLosOrRadarOfAllyTeam(UNIT, aiAllyTeam))     \
			continue;                                                           \
                                                                                \
		aiWrapper.FUNC;                                                         \
	}



void CEngineOutHandler::UnitIdle(const CUnit& unit) {
	AI_SCOPED_TIMER();

	const int teamId = unit.team;
	const int unitId = unit.id;

	DO_FOR_TEAM_SKIRMISH_AIS(UnitIdle(unitId), teamId);
}

void CEngineOutHandler::UnitCreated(const CUnit& unit, const CUnit* builder) {
	AI_SCOPED_TIMER();

	const int teamId     = unit.team;
	const int allyTeamId = unit.allyteam;
	const int unitId     = unit.id;
	const int builderId  = (builder != nullptr)? builder->id: -1;

	// enemies first, do_for_team can return early
	DO_FOR_ENEMY_SKIRMISH_AIS(EnemyCreated(unitId), allyTeamId, unit);
	DO_FOR_TEAM_SKIRMISH_AIS(UnitCreated(unitId, builderId), teamId);
}

void CEngineOutHandler::UnitFinished(const CUnit& unit) {
	AI_SCOPED_TIMER();

	const int teamId     = unit.team;
	const int allyTeamId = unit.allyteam;
	const int unitId     = unit.id;

	DO_FOR_ENEMY_SKIRMISH_AIS(EnemyFinished(unitId), allyTeamId, unit);
	DO_FOR_TEAM_SKIRMISH_AIS(UnitFinished(unitId), teamId);
}


void CEngineOutHandler::UnitMoveFailed(const CUnit& unit) {
	AI_SCOPED_TIMER();

	const int teamId = unit.team;
	const int unitId = unit.id;

	DO_FOR_TEAM_SKIRMISH_AIS(UnitMoveFailed(unitId), teamId);
}

void CEngineOutHandler::UnitGiven(const CUnit& unit, int oldTeam, int newTeam) {
	AI_SCOPED_TIMER();

	const int unitId        = unit.id;
	const int newAllyTeamId = unit.allyteam;
	const int oldAllyTeamId = teamHandler.AllyTeam(oldTeam);

	for (uint8_t aiID: activeSkirmishAIs) {
		CSkirmishAIWrapper& aiWrapper = hostSkirmishAIs[aiID];

		const int aiTeam     = aiWrapper.GetTeamId();
		const int aiAllyTeam = teamHandler.AllyTeam(aiTeam);
		const bool alliedOld = teamHandler.Ally(aiAllyTeam, oldAllyTeamId);
		const bool alliedNew = teamHandler.Ally(aiAllyTeam, newAllyTeamId);

		// inform the new team and its allies
		// (except those also allied with the old team,
		// which were already informed by UnitCaptured)
		bool informAI = ((aiTeam == newTeam) || (alliedNew && !alliedOld));

		if (informAI) {
			informAI  = false;
			informAI |= alliedOld;
			informAI |= alliedNew;
			informAI |= aiWrapper.CheatEventsEnabled();
			informAI |= IsUnitInLosOrRadarOfAllyTeam(unit, aiAllyTeam);
		}

		if (!informAI)
			continue;

		aiWrapper.UnitGiven(unitId, oldTeam, newTeam);
	}
}

void CEngineOutHandler::UnitCaptured(const CUnit& unit, int oldTeam, int newTeam) {
	AI_SCOPED_TIMER();

	const int unitId        = unit.id;
	const int newAllyTeamId = unit.allyteam;
	const int oldAllyTeamId = teamHandler.AllyTeam(oldTeam);

	for (uint8_t aiID: activeSkirmishAIs) {
		CSkirmishAIWrapper& aiWrapper = hostSkirmishAIs[aiID];

		const int aiTeam     = aiWrapper.GetTeamId();
		const int aiAllyTeam = teamHandler.AllyTeam(aiTeam);

		const bool alliedOld = teamHandler.Ally(aiAllyTeam, oldAllyTeamId);
		const bool alliedNew = teamHandler.Ally(aiAllyTeam, newAllyTeamId);

		// inform the old team and its allies
		// (except those also allied with the new team,
		// which will be informed by UnitGiven instead)
		bool informAI = ((aiTeam == oldTeam) || (alliedOld && !alliedNew));

		if (informAI) {
			informAI  = false;
			informAI |= alliedOld;
			informAI |= alliedNew;
			informAI |= aiWrapper.CheatEventsEnabled();
			informAI |= IsUnitInLosOrRadarOfAllyTeam(unit, aiAllyTeam);
		}

		if (!informAI)
			continue;

		aiWrapper.UnitCaptured(unitId, oldTeam, newTeam);
	}
}

void CEngineOutHandler::UnitDestroyed(const CUnit& destroyed, const CUnit* attacker) {
	AI_SCOPED_TIMER();

	const int destroyedId = destroyed.id;
	const int attackerId  = (attacker != nullptr)? attacker->id : -1;
	const int dt          = destroyed.team;

	// inform destroyed units team (not allies)
	if (!teamSkirmishAIs[dt].empty()) {
		const bool attackerInLosOrRadar = attacker && IsUnitInLosOrRadarOfAllyTeam(*attacker, destroyed.allyteam);

		for (uint8_t aiID: teamSkirmishAIs[dt]) {
			int visibleAttackerId = -1;

			if (attackerInLosOrRadar || hostSkirmishAIs[aiID].CheatEventsEnabled())
				visibleAttackerId = attackerId;

			hostSkirmishAIs[aiID].UnitDestroyed(destroyedId, visibleAttackerId);
		}
	}

	// inform all enemy teams
	for (uint8_t aiID: activeSkirmishAIs) {
		CSkirmishAIWrapper& aiWrapper = hostSkirmishAIs[aiID];

		const int aiTeam = aiWrapper.GetTeamId();
		const int aiAllyTeam = teamHandler.AllyTeam(aiTeam);

		if (teamHandler.Ally(aiAllyTeam, destroyed.allyteam))
			continue;

		if (!aiWrapper.CheatEventsEnabled() && !IsUnitInLosOrRadarOfAllyTeam(destroyed, aiAllyTeam))
			continue;

		int myAttackerId = -1;

		if ((attacker != nullptr) && teamHandler.Ally(aiAllyTeam, attacker->allyteam))
			myAttackerId = attackerId;

		aiWrapper.EnemyDestroyed(destroyedId, myAttackerId);
	}
}


void CEngineOutHandler::UnitDamaged(
	const CUnit& damaged,
	const CUnit* attacker,
	float damage,
	int weaponDefID,
	int projectileID,
	bool paralyzer
) {
	AI_SCOPED_TIMER();

	const int damagedUnitId  = damaged.id;
	const int dt             = damaged.team;
	const int attackerUnitId = attacker ? attacker->id : -1;

	// inform damaged units team (not allies)
	if (!teamSkirmishAIs[dt].empty()) {
		float3 attackeeDir;

		if (attacker != nullptr)
			attackeeDir = (attacker->GetErrorPos(damaged.allyteam) - damaged.pos).ANormalize();

		const bool attackerInLosOrRadar = attacker && IsUnitInLosOrRadarOfAllyTeam(*attacker, damaged.allyteam);

		for (uint8_t aiID: teamSkirmishAIs[dt]) {
			int visibleAttackerUnitId = -1;

			if (attackerInLosOrRadar || hostSkirmishAIs[aiID].CheatEventsEnabled())
				visibleAttackerUnitId = attackerUnitId;

			hostSkirmishAIs[aiID].UnitDamaged(damagedUnitId, visibleAttackerUnitId, damage, attackeeDir, weaponDefID, paralyzer);
		}
	}

	// inform attacker units team (not allies)
	if (attacker == nullptr)
		return;

	const int at = attacker->team;

	if (teamHandler.Ally(attacker->allyteam, damaged.allyteam))
		return;
	if (teamSkirmishAIs[at].empty())
		return;

	// direction from the attacker's view
	const float3 attackerDir = (attacker->pos - damaged.GetErrorPos(attacker->allyteam)).ANormalize();
	const bool damagedInLosOrRadar = IsUnitInLosOrRadarOfAllyTeam(damaged, attacker->allyteam);

	for (uint8_t aiID: teamSkirmishAIs[at]) {
		if (!damagedInLosOrRadar && !hostSkirmishAIs[aiID].CheatEventsEnabled())
			continue;

		hostSkirmishAIs[aiID].EnemyDamaged(damagedUnitId, attackerUnitId, damage, attackerDir, weaponDefID, paralyzer);
	}
}


void CEngineOutHandler::SeismicPing(
	int allyTeamId,
	const CUnit& unit,
	const float3& pos,
	float strength
) {
	AI_SCOPED_TIMER();

	const int unitId         = unit.id;
	const int unitAllyTeamId = unit.allyteam;

	DO_FOR_ALLIED_SKIRMISH_AIS(SeismicPing(allyTeamId, unitId, pos, strength), allyTeamId, unitAllyTeamId)
}

void CEngineOutHandler::WeaponFired(const CUnit& unit, const WeaponDef& def) {
	AI_SCOPED_TIMER();

	const int teamId = unit.team;
	const int unitId = unit.id;
	const int defId  = def.id;

	DO_FOR_TEAM_SKIRMISH_AIS(WeaponFired(unitId, defId), teamId);
}

void CEngineOutHandler::PlayerCommandGiven(
		const std::vector<int>& selectedUnitIds, const Command& c, int playerId)
{
	AI_SCOPED_TIMER();

	const int teamId = playerHandler.Player(playerId)->team;

	DO_FOR_TEAM_SKIRMISH_AIS(PlayerCommandGiven(selectedUnitIds, c, playerId), teamId);
}

void CEngineOutHandler::CommandFinished(const CUnit& unit, const Command& command) {
	AI_SCOPED_TIMER();

	const int teamId           = unit.team;
	const int unitId           = unit.id;
	const int aiCommandTopicId = extractAICommandTopic(&command, unitHandler.MaxUnits());

	DO_FOR_TEAM_SKIRMISH_AIS(CommandFinished(unitId, command.GetID(true), aiCommandTopicId), teamId);
}

void CEngineOutHandler::SendChatMessage(const char* msg, int fromPlayerId) {
	AI_SCOPED_TIMER();
	DO_FOR_SKIRMISH_AIS(SendChatMessage(msg, fromPlayerId))
}

bool CEngineOutHandler::SendLuaMessages(int aiTeam, const char* inData, std::vector<const char*>& outData) {
	SCOPED_TIMER("AI");

	if (activeSkirmishAIs.empty())
		return false;

	unsigned int n = 0;

	if (aiTeam != -1) {
		// get the AI's for team <aiTeam>
		const std::vector<uint8_t>& aiIDs = teamSkirmishAIs[aiTeam];

		if (aiIDs.empty())
			return false;

		outData.resize(aiIDs.size(), "");

		// send only to AI's in team <aiTeam>
		for (uint8_t aiID: aiIDs) {
			hostSkirmishAIs[aiID].SendLuaMessage(inData, &outData[n++]);
		}
	} else {
		outData.resize(activeSkirmishAIs.size(), "");

		// broadcast to all AI's across all teams
		//
		// since neither AI ID's nor AI teams are
		// necessarily consecutive, store responses
		// in calling order
		for (uint8_t aiID: activeSkirmishAIs) {
			hostSkirmishAIs[aiID].SendLuaMessage(inData, &outData[n++]);
		}
	}

	return true;
}



void CEngineOutHandler::CreateSkirmishAI(const uint8_t skirmishAIId) {
	SCOPED_TIMER("AI");
	LOG_L(L_INFO, "[EOH::%s(id=%u)]", __func__, skirmishAIId);

	const SkirmishAIData* aiData = skirmishAIHandler.GetSkirmishAI(skirmishAIId);

	if (aiData->status != SKIRMAISTATE_RELOADING)
		clientNet->Send(CBaseNetProtocol::Get().SendAIStateChanged(gu->myPlayerNum, skirmishAIId, SKIRMAISTATE_INITIALIZING));

	if (aiData->isLuaAI) {
		// currently, we need to do nothing for Lua AIs
		clientNet->Send(CBaseNetProtocol::Get().SendAIStateChanged(gu->myPlayerNum, skirmishAIId, SKIRMAISTATE_ALIVE));
	} else {
		hostSkirmishAIs[                skirmishAIId             ].PreInit(skirmishAIId);
		teamSkirmishAIs[hostSkirmishAIs[skirmishAIId].GetTeamId()].push_back(skirmishAIId);
		hostSkirmishAIs[                skirmishAIId             ].Init();

		activeSkirmishAIs.push_back(skirmishAIId);

		if (skirmishAIHandler.HasLocalKillFlag(skirmishAIId))
			return;

		// We will only get here if the AI is created mid-game.
		if (!gs->PreSimFrame())
			hostSkirmishAIs[skirmishAIId].Update(gs->frameNum);

		// Send a UnitCreated event for each unit of the team.
		// This will only do something if the AI is created mid-game.
		const CTeam* team = teamHandler.Team(hostSkirmishAIs[skirmishAIId].GetTeamId());

		for (const CUnit* u: unitHandler.GetUnitsByTeam(team->teamNum)) {
			hostSkirmishAIs[skirmishAIId].UnitCreated(u->id, -1);
			hostSkirmishAIs[skirmishAIId].UnitFinished(u->id);
		}

		clientNet->Send(CBaseNetProtocol::Get().SendAIStateChanged(gu->myPlayerNum, skirmishAIId, SKIRMAISTATE_ALIVE));
	}
}

void CEngineOutHandler::DestroySkirmishAI(const uint8_t skirmishAIId) {
	SCOPED_TIMER("AI");
	LOG_L(L_INFO, "[EOH::%s(id=%u)]", __func__, skirmishAIId);

	const int teamID = hostSkirmishAIs[skirmishAIId].GetTeamId();

	const auto it = std::find(teamSkirmishAIs[teamID].begin(), teamSkirmishAIs[teamID].end(), skirmishAIId);
	const auto jt = std::find(activeSkirmishAIs.begin(), activeSkirmishAIs.end(), skirmishAIId);

	if (it != teamSkirmishAIs[teamID].end() && jt != activeSkirmishAIs.end()) {
		teamSkirmishAIs[teamID].erase(it);
		activeSkirmishAIs.erase(jt);

		hostSkirmishAIs[skirmishAIId].Kill();
	} else {
		assert(false);
	}

	// will trigger SkirmishAIHandler::RemoveSkirmishAI
	clientNet->Send(CBaseNetProtocol::Get().SendAIStateChanged(gu->myPlayerNum, skirmishAIId, SKIRMAISTATE_DEAD));
}

