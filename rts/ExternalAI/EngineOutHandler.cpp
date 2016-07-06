/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "EngineOutHandler.h"

#include "ExternalAI/SkirmishAIWrapper.h"
#include "ExternalAI/SkirmishAIData.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include "ExternalAI/IAILibraryManager.h"
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
#include "System/Util.h"

#include "System/creg/STL_Map.h"

CR_BIND_DERIVED(CEngineOutHandler, CObject, )
CR_REG_METADATA(CEngineOutHandler, (
	// FIXME:
	//   "constexpr std::pair<_T1, _T2>::pair(const std::pair<_T1, _T2>&)
	//   [with _T1 = const unsigned char; _T2 = std::unique_ptr<CSkirmishAIWrapper>]’
	//   is implicitly deleted because the default definition would be ill-formed"
	CR_IGNORED(id_skirmishAI),
	CR_MEMBER(team_skirmishAIs)
))


static inline bool IsUnitInLosOrRadarOfAllyTeam(const CUnit& unit, const int allyTeamId) {
	// NOTE:
	//     we check for globalLOS because the LOS-state of a
	//     new unit has not yet been set when it is created
	//     (thus UnitCreated will not produce EnemyCreated,
	//     etc. without this, even when globalLOS is enabled
	//     (for non-cheating AI's))
	return (losHandler->globalLOS[allyTeamId] || (unit.losStatus[allyTeamId] & (LOS_INLOS | LOS_INRADAR)));
}

static CEngineOutHandler* singleton = NULL;
static unsigned int numInstances = 0;

CEngineOutHandler* CEngineOutHandler::GetInstance() {
	// if more than one instance, some code called eoh->func()
	// and created another after Destroy was already executed
	// from ~CGame --> usually dtors
	assert(numInstances == 1);
	return singleton;
}

void CEngineOutHandler::Create() {
	if (singleton == NULL) {
		singleton = new CEngineOutHandler();
		numInstances += 1;
	}
}

void CEngineOutHandler::Destroy() {
	if (singleton != NULL) {
		singleton->PreDestroy();

		SafeDelete(singleton);
		IAILibraryManager::Destroy();

		numInstances -= 1;
	}
}


CEngineOutHandler::~CEngineOutHandler() {
	// id_skirmishAI should be empty already, but this can not hurt
	// (it releases only those AI's that were not already released)
	while (!id_skirmishAI.empty()) {
		DestroySkirmishAI((id_skirmishAI.begin())->first);
	}
}


// This macro should be insterted at the start of each method sending AI events
#define AI_EVT_MTH()            \
	if (id_skirmishAI.empty())  \
		return;                 \
	SCOPED_TIMER("AI Total");


#define DO_FOR_SKIRMISH_AIS(FUNC)     \
	for (auto& ai: id_skirmishAI) {   \
		(ai.second)->FUNC;        \
	}


void CEngineOutHandler::PostLoad() {}

void CEngineOutHandler::PreDestroy() {
	AI_EVT_MTH();

	DO_FOR_SKIRMISH_AIS(PreDestroy())
}

void CEngineOutHandler::Load(std::istream* s) {
	AI_EVT_MTH();

	DO_FOR_SKIRMISH_AIS(Load(s))
}

void CEngineOutHandler::Save(std::ostream* s) {
	AI_EVT_MTH();

	DO_FOR_SKIRMISH_AIS(Save(s))
}


void CEngineOutHandler::Update() {
	AI_EVT_MTH();

	const int frame = gs->frameNum;

	DO_FOR_SKIRMISH_AIS(Update(frame))
}



// Do only if the unit is not allied, in which case we know
// everything about it anyway, and do not need to be informed
#define DO_FOR_ALLIED_SKIRMISH_AIS(FUNC, ALLY_TEAM_ID, UNIT_ALLY_TEAM_ID)            \
	if (!teamHandler->Ally(ALLY_TEAM_ID, UNIT_ALLY_TEAM_ID)) {                       \
		for (auto& ai: id_skirmishAI) {                                              \
			const int aiAllyTeam = teamHandler->AllyTeam((ai.second)->GetTeamId());  \
			if (teamHandler->Ally(aiAllyTeam, ALLY_TEAM_ID)) {                       \
				(ai.second)->FUNC;                                               \
			}                                                                        \
		}                                                                            \
	}


void CEngineOutHandler::UnitEnteredLos(const CUnit& unit, int allyTeamId) {
	AI_EVT_MTH();

	const int unitId         = unit.id;
	const int unitAllyTeamId = unit.allyteam;

	DO_FOR_ALLIED_SKIRMISH_AIS(EnemyEnterLOS(unitId), allyTeamId, unitAllyTeamId)
}

void CEngineOutHandler::UnitLeftLos(const CUnit& unit, int allyTeamId) {
	AI_EVT_MTH();

	const int unitId         = unit.id;
	const int unitAllyTeamId = unit.allyteam;

	DO_FOR_ALLIED_SKIRMISH_AIS(EnemyLeaveLOS(unitId), allyTeamId, unitAllyTeamId)
}

void CEngineOutHandler::UnitEnteredRadar(const CUnit& unit, int allyTeamId) {
	AI_EVT_MTH();

	const int unitId         = unit.id;
	const int unitAllyTeamId = unit.allyteam;

	DO_FOR_ALLIED_SKIRMISH_AIS(EnemyEnterRadar(unitId), allyTeamId, unitAllyTeamId)
}

void CEngineOutHandler::UnitLeftRadar(const CUnit& unit, int allyTeamId) {
	AI_EVT_MTH();

	const int unitId         = unit.id;
	const int unitAllyTeamId = unit.allyteam;

	DO_FOR_ALLIED_SKIRMISH_AIS(EnemyLeaveRadar(unitId), allyTeamId, unitAllyTeamId)
}



#define DO_FOR_TEAM_SKIRMISH_AIS(FUNC, TEAM_ID)                     \
	if (team_skirmishAIs.find(TEAM_ID) == team_skirmishAIs.end())   \
		return;                                                     \
                                                                    \
	for (auto ai: team_skirmishAIs[TEAM_ID]) {                      \
		id_skirmishAI[ai]->FUNC;                                \
	}                                                               \



// Send to all teams which the unit is not allied to,
// and which have cheat-events enabled, or the unit in sensor range.
#define DO_FOR_ENEMY_SKIRMISH_AIS(FUNC, ALLY_TEAM_ID, UNIT)                     \
	for (auto& ai: id_skirmishAI) {                                             \
		CSkirmishAIWrapper* saw = (ai.second).get();                            \
                                                                                \
		const int aiAllyTeam = teamHandler->AllyTeam(saw->GetTeamId());         \
                                                                                \
		const bool alliedAI = teamHandler->Ally(aiAllyTeam, ALLY_TEAM_ID);      \
		const bool cheatingAI = saw->IsCheatEventsEnabled();                    \
                                                                                \
		if (alliedAI)                                                           \
			continue;                                                           \
		if (cheatingAI || IsUnitInLosOrRadarOfAllyTeam(UNIT, aiAllyTeam)) {     \
			saw->FUNC;                                                      \
		}                                                                       \
	}



void CEngineOutHandler::UnitIdle(const CUnit& unit) {
	AI_EVT_MTH();

	const int teamId = unit.team;
	const int unitId = unit.id;

	DO_FOR_TEAM_SKIRMISH_AIS(UnitIdle(unitId), teamId);
}

void CEngineOutHandler::UnitCreated(const CUnit& unit, const CUnit* builder) {
	AI_EVT_MTH();

	const int teamId     = unit.team;
	const int allyTeamId = unit.allyteam;
	const int unitId     = unit.id;
	const int builderId  = builder? builder->id: -1;

	// enemies first, do_for_team can return early
	DO_FOR_ENEMY_SKIRMISH_AIS(EnemyCreated(unitId), allyTeamId, unit);
	DO_FOR_TEAM_SKIRMISH_AIS(UnitCreated(unitId, builderId), teamId);
}

void CEngineOutHandler::UnitFinished(const CUnit& unit) {
	AI_EVT_MTH();

	const int teamId     = unit.team;
	const int allyTeamId = unit.allyteam;
	const int unitId     = unit.id;

	DO_FOR_ENEMY_SKIRMISH_AIS(EnemyFinished(unitId), allyTeamId, unit);
	DO_FOR_TEAM_SKIRMISH_AIS(UnitFinished(unitId), teamId);
}


void CEngineOutHandler::UnitMoveFailed(const CUnit& unit) {
	AI_EVT_MTH();

	const int teamId = unit.team;
	const int unitId = unit.id;

	DO_FOR_TEAM_SKIRMISH_AIS(UnitMoveFailed(unitId), teamId);
}

void CEngineOutHandler::UnitGiven(const CUnit& unit, int oldTeam, int newTeam) {
	AI_EVT_MTH();

	const int unitId        = unit.id;
	const int newAllyTeamId = unit.allyteam;
	const int oldAllyTeamId = teamHandler->AllyTeam(oldTeam);

	for (auto& ai: id_skirmishAI) {
		CSkirmishAIWrapper* saw = (ai.second).get();

		const int aiTeam     = saw->GetTeamId();
		const int aiAllyTeam = teamHandler->AllyTeam(aiTeam);
		const bool alliedOld = teamHandler->Ally(aiAllyTeam, oldAllyTeamId);
		const bool alliedNew = teamHandler->Ally(aiAllyTeam, newAllyTeamId);

		// inform the new team and its allies
		// (except those also allied with the old team,
		// which were already informed by UnitCaptured)
		bool informAI = ((aiTeam == newTeam) || (alliedNew && !alliedOld));

		if (informAI) {
			informAI  = false;
			informAI |= alliedOld;
			informAI |= alliedNew;
			informAI |= saw->IsCheatEventsEnabled();
			informAI |= IsUnitInLosOrRadarOfAllyTeam(unit, aiAllyTeam);
		}

		if (!informAI)
			continue;

		saw->UnitGiven(unitId, oldTeam, newTeam);
	}
}

void CEngineOutHandler::UnitCaptured(const CUnit& unit, int oldTeam, int newTeam) {
	AI_EVT_MTH();

	const int unitId        = unit.id;
	const int newAllyTeamId = unit.allyteam;
	const int oldAllyTeamId = teamHandler->AllyTeam(oldTeam);

	for (auto& ai: id_skirmishAI) {
		CSkirmishAIWrapper* saw = (ai.second).get();

		const int aiTeam     = saw->GetTeamId();
		const int aiAllyTeam = teamHandler->AllyTeam(aiTeam);

		const bool alliedOld = teamHandler->Ally(aiAllyTeam, oldAllyTeamId);
		const bool alliedNew = teamHandler->Ally(aiAllyTeam, newAllyTeamId);

		// inform the old team and its allies
		// (except those also allied with the new team,
		// which will be informed by UnitGiven instead)
		bool informAI = ((aiTeam == oldTeam) || (alliedOld && !alliedNew));

		if (informAI) {
			informAI  = false;
			informAI |= alliedOld;
			informAI |= alliedNew;
			informAI |= saw->IsCheatEventsEnabled();
			informAI |= IsUnitInLosOrRadarOfAllyTeam(unit, aiAllyTeam);
		}

		if (!informAI)
			continue;

		saw->UnitCaptured(unitId, oldTeam, newTeam);
	}
}

void CEngineOutHandler::UnitDestroyed(const CUnit& destroyed, const CUnit* attacker) {
	AI_EVT_MTH();

	const int destroyedId = destroyed.id;
	const int attackerId  = attacker ? attacker->id : -1;
	const int dt          = destroyed.team;

	// inform destroyed units team (not allies)
	if (team_skirmishAIs.find(dt) != team_skirmishAIs.end()) {
		const bool attackerInLosOrRadar = attacker && IsUnitInLosOrRadarOfAllyTeam(*attacker, destroyed.allyteam);

		for (auto ai: team_skirmishAIs[dt]) {
			CSkirmishAIWrapper* saw = id_skirmishAI[ai].get();
			int visibleAttackerId = -1;

			if (attackerInLosOrRadar || saw->IsCheatEventsEnabled()) {
				visibleAttackerId = attackerId;
			}
			saw->UnitDestroyed(destroyedId, visibleAttackerId);
		}
	}

	// inform all enemy teams
	for (auto& ai: id_skirmishAI) {
		CSkirmishAIWrapper* saw = (ai.second).get();

		const int aiTeam = saw->GetTeamId();
		const int aiAllyTeam = teamHandler->AllyTeam(aiTeam);

		if (teamHandler->Ally(aiAllyTeam, destroyed.allyteam))
			continue;

		if (!saw->IsCheatEventsEnabled() && !IsUnitInLosOrRadarOfAllyTeam(destroyed, aiAllyTeam))
			continue;

		int myAttackerId = -1;

		if ((attacker != NULL) && teamHandler->Ally(aiAllyTeam, attacker->allyteam)) {
			myAttackerId = attackerId;
		}
		saw->EnemyDestroyed(destroyedId, myAttackerId);
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
	AI_EVT_MTH();

	const int damagedUnitId  = damaged.id;
	const int dt             = damaged.team;
	const int attackerUnitId = attacker ? attacker->id : -1;

	// inform damaged units team (not allies)
	if (team_skirmishAIs.find(dt) != team_skirmishAIs.end()) {
		float3 attackDir_damagedsView = ZeroVector;
		if (attacker) {
			attackDir_damagedsView =
					attacker->GetErrorPos(damaged.allyteam)
					- damaged.pos;
			attackDir_damagedsView.ANormalize();
		}

		const bool attackerInLosOrRadar = attacker && IsUnitInLosOrRadarOfAllyTeam(*attacker, damaged.allyteam);

		for (auto ai: team_skirmishAIs[dt]) {
			const CSkirmishAIWrapper* saw = id_skirmishAI[ai].get();
			int visibleAttackerUnitId = -1;

			if (attackerInLosOrRadar || saw->IsCheatEventsEnabled())
				visibleAttackerUnitId = attackerUnitId;

			id_skirmishAI[ai]->UnitDamaged(damagedUnitId, visibleAttackerUnitId, damage, attackDir_damagedsView, weaponDefID, paralyzer);
		}
	}

	// inform attacker units team (not allies)
	if (attacker) {
		const int at = attacker ? attacker->team : -1;

		if (teamHandler->Ally(attacker->allyteam, damaged.allyteam))
			return;
		if (team_skirmishAIs.find(at) == team_skirmishAIs.end())
			return;

		// direction from the attacker's view
		const float3 attackDir = (attacker->pos - damaged.GetErrorPos(attacker->allyteam)).ANormalize();
		const bool damagedInLosOrRadar = IsUnitInLosOrRadarOfAllyTeam(damaged, attacker->allyteam);

		for (auto ai: team_skirmishAIs[at]) {
			CSkirmishAIWrapper* saw = id_skirmishAI[ai].get();

			if (damagedInLosOrRadar || saw->IsCheatEventsEnabled()) {
				saw->EnemyDamaged(damagedUnitId, attackerUnitId, damage,
						attackDir, weaponDefID, paralyzer);
			}
		}
	}
}


void CEngineOutHandler::SeismicPing(
	int allyTeamId,
	const CUnit& unit,
	const float3& pos,
	float strength
) {
	AI_EVT_MTH();

	const int unitId         = unit.id;
	const int unitAllyTeamId = unit.allyteam;

	DO_FOR_ALLIED_SKIRMISH_AIS(SeismicPing(allyTeamId, unitId, pos, strength), allyTeamId, unitAllyTeamId)
}

void CEngineOutHandler::WeaponFired(const CUnit& unit, const WeaponDef& def) {
	AI_EVT_MTH();

	const int teamId = unit.team;
	const int unitId = unit.id;
	const int defId  = def.id;

	DO_FOR_TEAM_SKIRMISH_AIS(WeaponFired(unitId, defId), teamId);
}

void CEngineOutHandler::PlayerCommandGiven(
		const std::vector<int>& selectedUnitIds, const Command& c, int playerId)
{
	AI_EVT_MTH();

	const int teamId = playerHandler->Player(playerId)->team;

	DO_FOR_TEAM_SKIRMISH_AIS(PlayerCommandGiven(selectedUnitIds, c, playerId), teamId);
}

void CEngineOutHandler::CommandFinished(const CUnit& unit, const Command& command) {
	AI_EVT_MTH();

	const int teamId           = unit.team;
	const int unitId           = unit.id;
	const int aiCommandTopicId = extractAICommandTopic(&command, unitHandler->MaxUnits());

	DO_FOR_TEAM_SKIRMISH_AIS(CommandFinished(unitId, command.aiCommandId, aiCommandTopicId), teamId);
}

void CEngineOutHandler::SendChatMessage(const char* msg, int fromPlayerId) {
	AI_EVT_MTH();

	DO_FOR_SKIRMISH_AIS(SendChatMessage(msg, fromPlayerId))
}

bool CEngineOutHandler::SendLuaMessages(int aiTeam, const char* inData, std::vector<const char*>& outData) {
	SCOPED_TIMER("AI Total");

	if (id_skirmishAI.empty()) {
		return false;
	}

	unsigned int n = 0;

	if (aiTeam != -1) {
		// get the AI's for team <aiTeam>
		const team_ais_t::iterator aiTeamIter = team_skirmishAIs.find(aiTeam);

		if (aiTeamIter == team_skirmishAIs.end()) {
			return false;
		}

		// get the vector of ID's for the AI's
		ids_t& aiIDs = aiTeamIter->second;

		outData.resize(aiIDs.size(), "");

		// send only to AI's in team <aiTeam>
		for (auto aiIDsIter = aiIDs.begin(); aiIDsIter != aiIDs.end(); ++aiIDsIter) {
			CSkirmishAIWrapper* wrapperAI = id_skirmishAI[*aiIDsIter].get();
			wrapperAI->SendLuaMessage(inData, &outData[n++]);
		}
	} else {
		outData.resize(id_skirmishAI.size(), "");

		// broadcast to all AI's across all teams
		//
		// since neither AI ID's nor AI teams are
		// necessarily consecutive, store responses
		// in calling order
		for (auto it = id_skirmishAI.begin(); it != id_skirmishAI.end(); ++it) {
			CSkirmishAIWrapper* wrapperAI = (it->second).get();
			wrapperAI->SendLuaMessage(inData, &outData[n++]);
		}
	}

	return true;
}



void CEngineOutHandler::CreateSkirmishAI(const size_t skirmishAIId) {
	SCOPED_TIMER("AI Total");

	//const bool unpauseAfterAIInit = configHandler->GetBool("AI_UnpauseAfterInit");

	// Pause the game for letting the AI initialize,
	// as this can take quite some time.
	/*bool weDoPause = !gs->paused;
	if (weDoPause) {
		const std::string& myPlayerName = playerHandler->Player(gu->myPlayerNum)->name;
		LOG(
				"Player %s (auto)-paused the game for letting Skirmish AI"
				" %s initialize for controlling team %i.%s",
				myPlayerName.c_str(), key.GetShortName().c_str(), teamId,
				(unpauseAfterAIInit ?
				 " The game is auto-unpaused as soon as the AI is ready." :
				 ""));
		clientNet->Send(CBaseNetProtocol::Get().SendPause(gu->myPlayerNum, true));
	}*/

	const SkirmishAIData* aiData = skirmishAIHandler.GetSkirmishAI(skirmishAIId);

	if (aiData->status != SKIRMAISTATE_RELOADING) {
		clientNet->Send(CBaseNetProtocol::Get().SendAIStateChanged(gu->myPlayerNum, skirmishAIId, SKIRMAISTATE_INITIALIZING));
	}

	if (aiData->isLuaAI) {
		// currently, we need to do nothing for Lua AIs
		clientNet->Send(CBaseNetProtocol::Get().SendAIStateChanged(gu->myPlayerNum, skirmishAIId, SKIRMAISTATE_ALIVE));
	} else {
		CSkirmishAIWrapper* aiWrapper = nullptr;

		id_skirmishAI[skirmishAIId].reset(new CSkirmishAIWrapper(skirmishAIId));

		aiWrapper = id_skirmishAI[skirmishAIId].get();

		team_skirmishAIs[aiWrapper->GetTeamId()].push_back(skirmishAIId);

		aiWrapper->Init();

		if (aiWrapper == nullptr)
			return;
		if (skirmishAIHandler.IsLocalSkirmishAIDieing(skirmishAIId))
			return;

		if (!gs->PreSimFrame()) {
			// We will only get here if the AI is created mid-game.
			aiWrapper->Update(gs->frameNum);
		}

		// Send a UnitCreated event for each unit of the team.
		// This will only do something if the AI is created mid-game.
		const CTeam* team = teamHandler->Team(aiWrapper->GetTeamId());

		for (CUnit* u: team->units) {
			aiWrapper->UnitCreated(u->id, -1);
			aiWrapper->UnitFinished(u->id);
		}

		clientNet->Send(CBaseNetProtocol::Get().SendAIStateChanged(gu->myPlayerNum, skirmishAIId, SKIRMAISTATE_ALIVE));
	}
}

void CEngineOutHandler::SetSkirmishAIDieing(const size_t skirmishAIId) {
	SCOPED_TIMER("AI Total");

	// if exiting before start, AI's have not been loaded yet
	// and std::map<>::operator[] would insert a NULL instance
	if (id_skirmishAI.find(skirmishAIId) == id_skirmishAI.end())
		return;

	assert(id_skirmishAI[skirmishAIId].get() != nullptr);
	id_skirmishAI[skirmishAIId]->Dieing();
}

static void internal_aiErase(std::vector<unsigned char>& ais, const unsigned char skirmishAIId) {
	auto it = std::find(ais.begin(), ais.end(), skirmishAIId);

	if (it != ais.end()) {
		ais.erase(it);
		return;
	}

	// failed to remove Skirmish AI ID
	assert(false);
}

void CEngineOutHandler::DestroySkirmishAI(const size_t skirmishAIId) {
	SCOPED_TIMER("AI Total");

	CSkirmishAIWrapper* aiWrapper = id_skirmishAI[skirmishAIId].get();

	internal_aiErase(team_skirmishAIs[aiWrapper->GetTeamId()], skirmishAIId);
	id_skirmishAI.erase(skirmishAIId);

	clientNet->Send(CBaseNetProtocol::Get().SendAIStateChanged(gu->myPlayerNum, skirmishAIId, SKIRMAISTATE_DEAD));
}

