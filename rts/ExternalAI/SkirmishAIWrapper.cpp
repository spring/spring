/*
	Copyright (c) 2008 Robin Vobruba <hoijui.quaero@gmail.com>

	This program is free software {} you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation {} either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY {} without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "SkirmishAIWrapper.h"

#include "StdAfx.h"
#include "IGlobalAI.h"
#include "SkirmishAI.h"
#include "GlobalAICallback.h"
#include "EngineOutHandler.h"
#include "IAILibraryManager.h"
#include "Platform/errorhandler.h"
#include "LogOutput.h"
#include "mmgr.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Interface/AISEvents.h"
#include "Interface/AISCommands.h"
#include "Interface/SSkirmishAILibrary.h"
#include "SkirmishAILibraryInfo.h"
#include "SSkirmishAICallbackImpl.h"

#include <sstream>

CR_BIND_DERIVED(CSkirmishAIWrapper, CObject, (0, SkirmishAIKey()))
CR_REG_METADATA(CSkirmishAIWrapper, (
	CR_MEMBER(teamId),
	CR_MEMBER(cheatEvents),
	CR_MEMBER(key),
	CR_SERIALIZER(Serialize),
	CR_POSTLOAD(PostLoad)
));

#define HANDLE_EXCEPTION								\
	catch (const std::exception& e) {					\
		if (CEngineOutHandler::IsCatchExceptions()) {	\
			handleAIException(e.what());				\
			throw;										\
		} else throw;									\
	}													\
	catch (const char *s) {								\
		if (CEngineOutHandler::IsCatchExceptions()) {	\
			handleAIException(s);						\
			throw;										\
		} else throw;									\
	}													\
	catch (...) {										\
		if (CEngineOutHandler::IsCatchExceptions()) {	\
			handleAIException(0);						\
			throw;										\
		} else throw;									\
	}


CSkirmishAIWrapper::CSkirmishAIWrapper(int teamId, const SkirmishAIKey& key)
		: teamId(teamId), cheatEvents(false), ai(NULL), c_callback(NULL), key(key) {

	if (c_callback == NULL) {
		callback = new CGlobalAICallback(this);
		c_callback = skirmishAiCallback_getInstanceFor(teamId, callback);
	}
}

void CSkirmishAIWrapper::PreDestroy() {
	callback->noMessages = true;
}

CSkirmishAIWrapper::~CSkirmishAIWrapper() {

	if (ai) {
		Release();
		skirmishAiCallback_release(teamId);
		delete callback;
		delete ai;
		c_callback = NULL;
		callback = NULL;
		ai = NULL;
	}
}

void CSkirmishAIWrapper::Serialize(creg::ISerializer* s) {}


void CSkirmishAIWrapper::PostLoad() {
	LoadSkirmishAI(true);
}



void CSkirmishAIWrapper::LoadSkirmishAI(bool postLoad) {

	ai = new CSkirmishAI(teamId, key);

	IAILibraryManager* libManager = IAILibraryManager::GetInstance();
	libManager->FetchSkirmishAILibrary(key);
	const CSkirmishAILibraryInfo* infos =
			&*libManager->GetSkirmishAIInfos().find(key)->second;
	bool loadSupported =
			infos->GetInfo(SKIRMISH_AI_PROPERTY_LOAD_SUPPORTED) == "yes";
	libManager->ReleaseSkirmishAILibrary(key);

	if (postLoad && !loadSupported) {
		// fallback code to help the AI if it
		// doesn't implement load/save support
		for (size_t a = 0; a < uh->MaxUnits(); a++) {
			if (!uh->units[a])
				continue;

			if (uh->units[a]->team == teamId) {
				try {
					UnitCreated(a);
				} HANDLE_EXCEPTION;
				if (!uh->units[a]->beingBuilt)
					try {
						UnitFinished(a);
					} HANDLE_EXCEPTION;
			} else {
				if ((uh->units[a]->allyteam == teamHandler->AllyTeam(teamId))
						|| teamHandler->Ally(teamHandler->AllyTeam(teamId), uh->units[a]->allyteam)) {
					// do nothing
				} else {
					if (uh->units[a]->losStatus[teamHandler->AllyTeam(teamId)] & (LOS_INRADAR | LOS_INLOS)) {
						try {
							EnemyEnterRadar(a);
						} HANDLE_EXCEPTION;
					}
					if (uh->units[a]->losStatus[teamHandler->AllyTeam(teamId)] & LOS_INLOS) {
						try {
							EnemyEnterLOS(a);
						} HANDLE_EXCEPTION;
					}
				}
			}
		}
	}
}


void CSkirmishAIWrapper::Init() {

	if (ai == NULL) {
		LoadSkirmishAI(false);
	}

	SInitEvent evtData = {teamId, GetCallback()};
	ai->HandleEvent(EVENT_INIT, &evtData);
}

void CSkirmishAIWrapper::Release() {

	SReleaseEvent evtData = {};
	ai->HandleEvent(EVENT_RELEASE, &evtData);

	// further cleanup is done in the destructor
}


void CSkirmishAIWrapper::Load(std::istream* s) {

/* TODO
	SLoadAIEvent evtData = {s.TO_FILENAME(TODO), callback};
	ai->HandleEvent(EVENT_LOAD_AI, &evtData);
*/
}

void CSkirmishAIWrapper::Save(std::ostream* s) {

/* TODO
	SSaveAIEvent evtData = {s.TO_FILENAME(TODO)};
	ai->HandleEvent(EVENT_SAVE_AI, &evtData);
*/
}

void CSkirmishAIWrapper::UnitIdle(int unitId) {

	SUnitIdleEvent evtData = {unitId};
	ai->HandleEvent(EVENT_UNIT_IDLE, &evtData);
}

void CSkirmishAIWrapper::UnitCreated(int unitId) {

	SUnitCreatedEvent evtData = {unitId};
	ai->HandleEvent(EVENT_UNIT_CREATED, &evtData);
}

void CSkirmishAIWrapper::UnitFinished(int unitId) {

	SUnitFinishedEvent evtData = {unitId};
	ai->HandleEvent(EVENT_UNIT_FINISHED, &evtData);
}

void CSkirmishAIWrapper::UnitDestroyed(int unitId, int attackerUnitId) {

	SUnitDestroyedEvent evtData = {unitId, attackerUnitId};
	ai->HandleEvent(EVENT_UNIT_DESTROYED, &evtData);
}

void CSkirmishAIWrapper::UnitDamaged(int unitId, int attackerUnitId,
		float damage, const float3& dir) {

	SUnitDamagedEvent evtData = {unitId, attackerUnitId, damage,
			dir.toSAIFloat3()};
	ai->HandleEvent(EVENT_UNIT_DAMAGED, &evtData);
}

void CSkirmishAIWrapper::UnitMoveFailed(int unitId) {

	SUnitMoveFailedEvent evtData = {unitId};
	ai->HandleEvent(EVENT_UNIT_MOVE_FAILED, &evtData);
}

void CSkirmishAIWrapper::UnitGiven(int unitId, int oldTeam, int newTeam) {

	SUnitGivenEvent evtData = {unitId, oldTeam, newTeam};
	ai->HandleEvent(EVENT_UNIT_GIVEN, &evtData);
}

void CSkirmishAIWrapper::UnitCaptured(int unitId, int oldTeam, int newTeam) {

	SUnitCapturedEvent evtData = {unitId, oldTeam, newTeam};
	ai->HandleEvent(EVENT_UNIT_CAPTURED, &evtData);
}

void CSkirmishAIWrapper::EnemyEnterLOS(int unitId) {

	SEnemyEnterLOSEvent evtData = {unitId};
	ai->HandleEvent(EVENT_ENEMY_ENTER_LOS, &evtData);
}

void CSkirmishAIWrapper::EnemyLeaveLOS(int unitId) {

	SEnemyLeaveLOSEvent evtData = {unitId};
	ai->HandleEvent(EVENT_ENEMY_LEAVE_LOS, &evtData);
}

void CSkirmishAIWrapper::EnemyEnterRadar(int unitId) {

	SEnemyEnterRadarEvent evtData = {unitId};
	ai->HandleEvent(EVENT_ENEMY_ENTER_RADAR, &evtData);
}

void CSkirmishAIWrapper::EnemyLeaveRadar(int unitId) {

	SEnemyLeaveRadarEvent evtData = {unitId};
	ai->HandleEvent(EVENT_ENEMY_LEAVE_RADAR, &evtData);
}

void CSkirmishAIWrapper::EnemyDestroyed(int enemyUnitId, int attackerUnitId) {

	SEnemyDestroyedEvent evtData = {enemyUnitId, attackerUnitId};
	ai->HandleEvent(EVENT_ENEMY_DESTROYED, &evtData);
}

void CSkirmishAIWrapper::EnemyDamaged(int enemyUnitId, int attackerUnitId,
		float damage, const float3& dir) {

	SEnemyDamagedEvent evtData = {enemyUnitId, attackerUnitId, damage,
			dir.toSAIFloat3()};
	ai->HandleEvent(EVENT_ENEMY_DAMAGED, &evtData);
}

void CSkirmishAIWrapper::Update(int frame) {

	SUpdateEvent evtData = {frame};
	ai->HandleEvent(EVENT_UPDATE, &evtData);
}

void CSkirmishAIWrapper::GotChatMsg(const char* msg, int fromPlayerId) {

	SMessageEvent evtData = {fromPlayerId, msg};
	ai->HandleEvent(EVENT_MESSAGE, &evtData);
}

void CSkirmishAIWrapper::WeaponFired(int unitId, int weaponDefId) {

	SWeaponFiredEvent evtData = {unitId, weaponDefId};
	ai->HandleEvent(EVENT_WEAPON_FIRED, &evtData);
}

void CSkirmishAIWrapper::PlayerCommandGiven(
		const std::vector<int>& selectedUnits, const Command& c, int playerId) {

	unsigned int numUnits = selectedUnits.size();
	int *unitIds=new int[numUnits];
	for (unsigned int i=0; i < numUnits; ++i) {
		unitIds[i] = selectedUnits.at(i);
	}
	int sCommandId;
	void* sCommandData = mallocSUnitCommand(-1, -1, &c, &sCommandId);

	SPlayerCommandEvent evtData = {unitIds, numUnits, sCommandId, sCommandData,
			playerId};
	ai->HandleEvent(EVENT_PLAYER_COMMAND, &evtData);
	delete [] unitIds;
}

void CSkirmishAIWrapper::CommandFinished(int unitId, int commandTopicId) {

	SCommandFinishedEvent evtData = {unitId, commandTopicId};
	ai->HandleEvent(EVENT_COMMAND_FINISHED, &evtData);
}

void CSkirmishAIWrapper::SeismicPing(int allyTeam, int unitId,
		const float3& pos, float strength) {

	SSeismicPingEvent evtData = {pos.toSAIFloat3(), strength};
	ai->HandleEvent(EVENT_SEISMIC_PING, &evtData);
}


int CSkirmishAIWrapper::GetTeamId() const {
	return teamId;
}
const SkirmishAIKey& CSkirmishAIWrapper::GetKey() const {
	return key;
}
const SSkirmishAICallback* CSkirmishAIWrapper::GetCallback() const {
	return c_callback;
}

void CSkirmishAIWrapper::SetCheatEventsEnabled(bool enable) {
	cheatEvents = enable;
}
bool CSkirmishAIWrapper::IsCheatEventsEnabled() const {
	return cheatEvents;
}

int CSkirmishAIWrapper::HandleEvent(int topic, const void* data) const
{
	return ai->HandleEvent(topic, data);
}
