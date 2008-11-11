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

#include "GroupAIWrapper.h"

#include "StdAfx.h"
#include "Game/GlobalSynced.h"
#include "IGlobalAI.h"
#include "GroupAI.h"
#include "GlobalAICallback.h"
#include "GlobalAIHandler.h"
#include "Platform/FileSystem.h"
#include "Platform/errorhandler.h"
#include "Platform/SharedLib.h"
#include "LogOutput.h"
#include "mmgr.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Interface/AISEvents.h"
#include "Interface/AISCommands.h"
#include "Interface/SSAILibrary.h"

#include <sstream>

CR_BIND_DERIVED(CGroupAIWrapper, CObject, (0, SSAIKey()))
CR_REG_METADATA(CGroupAIWrapper, (
	CR_MEMBER(teamId),
	CR_MEMBER(groupId),
	CR_MEMBER(cheatEvents),
	CR_MEMBER(key),
	CR_MEMBER(optionKeys),
	CR_MEMBER(optionValues),
	CR_SERIALIZER(Serialize),
	CR_POSTLOAD(PostLoad)
));

void AIException(const char *what);

#define HANDLE_EXCEPTION					\
	catch (const std::exception& e) {		\
		if (globalAI->CatchException()) {	\
			AIException(e.what());			\
			throw;							\
		} else throw;						\
	}										\
	catch (const char *s) {	\
		if (globalAI->CatchException()) {	\
			AIException(s);					\
			throw;							\
		} else throw;						\
	}										\
	catch (...) {							\
		if (globalAI->CatchException()) {	\
			AIException(0);					\
			throw;							\
		} else throw;						\
	}


CGroupAIWrapper::CGroupAIWrapper(int teamId, int groupId, const SSAIKey& key,
		const std::map<std::string, std::string>& options)
		: teamId(teamId), groupId(groupId), cheatEvents(false), key(key) {

	std::map<std::string, std::string>::const_iterator op;
	for (op = options.begin(); op != options.end(); ++op) {
		optionKeys.push_back(op->first);
		optionValues.push_back(op->second);
	}

	LoadGroupAI(false);

	Init();
}

void CGroupAIWrapper::PreDestroy() {
	callback->noMessages = true;
}

CGroupAIWrapper::~CGroupAIWrapper() {

	if (ai) {
		Release();
		delete c_callback;
		delete callback;
		delete ai;
	}
}

void CGroupAIWrapper::Serialize(creg::ISerializer* s) {}


void CGroupAIWrapper::PostLoad() {
	LoadGroupAI(true);
}


void CGroupAIWrapper::LoadGroupAI(bool postLoad) {

	ai = SAFE_NEW CGroupAI(teamId, groupId, key);

	IAILibraryManager* libManager = IAILibraryManager::GetInstance();
	libManager->FetchGroupAILibrary(key);
	const CGroupAILibraryInfo* infos =
			libManager->GetGroupAIInfos()->at(key);
	bool loadSupported =
			infos->GetInfo(GROUP_AI_PROPERTY_LOAD_SUPPORTED) == "yes";
	libManager->ReleaseGroupAILibrary(key);

	if (postLoad && !loadSupported) {
		// fallback code to help the AI if it
		// doesn't implement load/save support
		for (int a = 0; a < MAX_UNITS; a++) {
			if (!uh->units[a])
				continue;

			if (uh->units[a]->team == teamId) {
				try {
					AddUnit(a);
				} HANDLE_EXCEPTION;
				if (!uh->units[a]->beingBuilt)
					try {
						UnitFinished(a);
					} HANDLE_EXCEPTION;
			} else {
				if ((uh->units[a]->allyteam == gs->AllyTeam(teamId))
						|| gs->Ally(gs->AllyTeam(teamId), uh->units[a]->allyteam)) {
					// do nothing
				} else {
					if (uh->units[a]->losStatus[gs->AllyTeam(teamId)] & (LOS_INRADAR | LOS_INLOS)) {
						try {
							EnemyEnterRadar(a);
						} HANDLE_EXCEPTION;
					}
					if (uh->units[a]->losStatus[gs->AllyTeam(teamId)] & LOS_INLOS) {
						try {
							EnemyEnterLOS(a);
						} HANDLE_EXCEPTION;
					}
				}
			}
		}
	}
}

static const char** allocCStrArray(std::vector<std::string> strVec) {

	const char** strArr = NULL;

	unsigned int size = strVec.size();
	strArr = (const char**) calloc(size, sizeof(char*));
	unsigned int i;
	for (i = 0; i < size; ++i) {
		strArr[i] = strVec[i].c_str();
	}

	return strArr;
}

void CGroupAIWrapper::Init() {

	callback = SAFE_NEW CGroupAICallback(this);
	c_callback = initSAICallback(teamId, callback);
	optionKeys_c = allocCStrArray(optionKeys);
	optionValues_c = allocCStrArray(optionValues);

	SInitEvent evtData = {teamId, c_callback, optionKeys.size(), optionKeys_c, optionValues_c};
	ai->HandleEvent(EVENT_INIT, &evtData);
}

void CGroupAIWrapper::Release() {

	SReleaseEvent evtData = {teamId};
	ai->HandleEvent(EVENT_RELEASE, &evtData);

	free(optionKeys_c);
	free(optionValues_c);

	// further cleanup is done in the destructor
}


void CGroupAIWrapper::Load(std::istream* s) {

/* TODO
	SLoadAIEvent evtData = {s.TO_FILENAME(TODO), callback};
	ai->HandleEvent(EVENT_LOAD_AI, &evtData);
*/
}

void CGroupAIWrapper::Save(std::ostream* s) {

/* TODO
	SSaveAIEvent evtData = {s.TO_FILENAME(TODO)};
	ai->HandleEvent(EVENT_SAVE_AI, &evtData);
*/
}

void CGroupAIWrapper::UnitIdle(int unitId) {

	SUnitIdleEvent evtData = {unitId};
	ai->HandleEvent(EVENT_UNIT_IDLE, &evtData);
}

void CGroupAIWrapper::UnitCreated(int unitId) {

	SUnitCreatedEvent evtData = {unitId};
	ai->HandleEvent(EVENT_UNIT_CREATED, &evtData);
}

void CGroupAIWrapper::UnitFinished(int unitId) {

	SUnitFinishedEvent evtData = {unitId};
	ai->HandleEvent(EVENT_UNIT_FINISHED, &evtData);
}

void CGroupAIWrapper::UnitDestroyed(int unitId, int attackerUnitId) {

	SUnitDestroyedEvent evtData = {unitId, attackerUnitId};
	ai->HandleEvent(EVENT_UNIT_DESTROYED, &evtData);
}

void CGroupAIWrapper::UnitDamaged(int unitId, int attackerUnitId,
		float damage, const float3& dir) {

	SUnitDamagedEvent evtData = {unitId, attackerUnitId, damage,
			dir.toSAIFloat3()};
	ai->HandleEvent(EVENT_UNIT_DAMAGED, &evtData);
}

void CGroupAIWrapper::UnitMoveFailed(int unitId) {

	SUnitMoveFailedEvent evtData = {unitId};
	ai->HandleEvent(EVENT_UNIT_MOVE_FAILED, &evtData);
}

void CGroupAIWrapper::UnitGiven(int unitId, int oldTeam, int newTeam) {

	SUnitGivenEvent evtData = {unitId, oldTeam, newTeam};
	ai->HandleEvent(EVENT_UNIT_GIVEN, &evtData);
}

void CGroupAIWrapper::UnitCaptured(int unitId, int oldTeam, int newTeam) {

	SUnitCapturedEvent evtData = {unitId, oldTeam, newTeam};
	ai->HandleEvent(EVENT_UNIT_CAPTURED, &evtData);
}

void CGroupAIWrapper::EnemyEnterLOS(int unitId) {

	SEnemyEnterLOSEvent evtData = {unitId};
	ai->HandleEvent(EVENT_ENEMY_ENTER_LOS, &evtData);
}

void CGroupAIWrapper::EnemyLeaveLOS(int unitId) {

	SEnemyLeaveLOSEvent evtData = {unitId};
	ai->HandleEvent(EVENT_ENEMY_LEAVE_LOS, &evtData);
}

void CGroupAIWrapper::EnemyEnterRadar(int unitId) {

	SEnemyEnterRadarEvent evtData = {unitId};
	ai->HandleEvent(EVENT_ENEMY_ENTER_RADAR, &evtData);
}

void CGroupAIWrapper::EnemyLeaveRadar(int unitId) {

	SEnemyLeaveRadarEvent evtData = {unitId};
	ai->HandleEvent(EVENT_ENEMY_LEAVE_RADAR, &evtData);
}

void CGroupAIWrapper::EnemyDestroyed(int enemyUnitId, int attackerUnitId) {

	SEnemyDestroyedEvent evtData = {enemyUnitId, attackerUnitId};
	ai->HandleEvent(EVENT_ENEMY_DESTROYED, &evtData);
}

void CGroupAIWrapper::EnemyDamaged(int enemyUnitId, int attackerUnitId,
		float damage, const float3& dir) {

	SEnemyDamagedEvent evtData = {enemyUnitId, attackerUnitId, damage,
			dir.toSAIFloat3()};
	ai->HandleEvent(EVENT_ENEMY_DAMAGED, &evtData);
}

void CGroupAIWrapper::Update(int frame) {

	SUpdateEvent evtData = {frame};
	ai->HandleEvent(EVENT_UPDATE, &evtData);
}

void CGroupAIWrapper::GotChatMsg(const char* msg, int fromPlayerId) {

	SMessageEvent evtData = {fromPlayerId, msg};
	ai->HandleEvent(EVENT_MESSAGE, &evtData);
}

void CGroupAIWrapper::WeaponFired(int unitId, int weaponDefId) {

	SWeaponFiredEvent evtData = {unitId, weaponDefId};
	ai->HandleEvent(EVENT_WEAPON_FIRED, &evtData);
}

void CGroupAIWrapper::PlayerCommandGiven(
		const std::vector<int>& selectedUnits, const Command& c, int playerId) {

	unsigned int numUnits = selectedUnits.size();
	int unitIds[numUnits];
	for (unsigned int i=0; i < numUnits; ++i) {
		unitIds[i] = selectedUnits.at(i);
	}
	int sCommandId;
	void* sCommandData = mallocSUnitCommand(-1, -1, &c, &sCommandId);

	SPlayerCommandEvent evtData = {unitIds, numUnits, sCommandId, sCommandData,
			playerId};
	ai->HandleEvent(EVENT_PLAYER_COMMAND, &evtData);
}

void CGroupAIWrapper::SeismicPing(int allyTeam, int unitId,
		const float3& pos, float strength) {

	SSeismicPingEvent evtData = {pos.toSAIFloat3(), strength};
	ai->HandleEvent(EVENT_SEISMIC_PING, &evtData);
}


int CGroupAIWrapper::GetTeamId() const {
	return teamId;
}
int CGroupAIWrapper::GetGroupId() const {
	return groupId;
}

void CGroupAIWrapper::SetCheatEventsEnabled(bool enable) {
	cheatEvents = enable;
}
bool CGroupAIWrapper::IsCheatEventsEnabled() const {
	return cheatEvents;
}

int CGroupAIWrapper::HandleEvent(int topic, const void* data) const
{
	return ai->HandleEvent(topic, data);
}
