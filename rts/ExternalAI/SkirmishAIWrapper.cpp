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

CR_BIND_DERIVED(CSkirmishAIWrapper, CObject, (0, SSAIKey()))
CR_REG_METADATA(CSkirmishAIWrapper, (
	CR_MEMBER(teamId),
	CR_MEMBER(cheatEvents),
	CR_MEMBER(skirmishAIKey),
/*
	CR_MEMBER(libName),
	CR_MEMBER(IsCInterface),
	CR_MEMBER(IsLoadSupported),
*/
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

/*
CSkirmishAIWrapper::CSkirmishAIWrapper() {
    
}
*/

CSkirmishAIWrapper::CSkirmishAIWrapper(int teamId, const SSAIKey& skirmishAIKey)
		: teamId(teamId), cheatEvents(false), skirmishAIKey(skirmishAIKey) {
	
	LoadSkirmishAI(teamId, skirmishAIKey, false);
	
	Init();
}

void CSkirmishAIWrapper::PreDestroy() {
	callback->noMessages = true;
}

CSkirmishAIWrapper::~CSkirmishAIWrapper() {
	
	if (ai) {
/*
		if (!IsCInterface) {
			try {
				_ReleaseAIFunc(ai);
			} HANDLE_EXCEPTION;
		}
*/

		delete c_callback;
		delete callback;
		delete ai;
	}
}

void CSkirmishAIWrapper::Serialize(creg::ISerializer* s) {}




void CSkirmishAIWrapper::PostLoad() {
	LoadSkirmishAI(teamId, skirmishAIKey, true);
}



void CSkirmishAIWrapper::LoadSkirmishAI(int teamId, const SSAIKey& skirmishAIKey, bool postLoad) {
	
	ai = SAFE_NEW CSkirmishAI(teamId, skirmishAIKey);
	
	const ISkirmishAILibrary* skirmishAILibrary = IAILibraryManager::GetInstance()->FetchSkirmishAILibrary(skirmishAIKey);
	//bool loadSupported = std::string(skirmishAILibrary->GetInfos()[SKIRMISH_AI_PROPERTY_LOAD_SUPPORTED].value) == "yes";
	bool loadSupported = IAILibraryManager::GetInstance()->GetSkirmishAIInfos()->at(skirmishAIKey)->GetInfo(SKIRMISH_AI_PROPERTY_LOAD_SUPPORTED) == "yes";
	IAILibraryManager::GetInstance()->ReleaseSkirmishAILibrary(skirmishAIKey);

	if (postLoad && !loadSupported) {
		// fallback code to help the AI if it
		// doesn't implement load/save support
		for (int a = 0; a < MAX_UNITS; a++) {
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
				if ((uh->units[a]->allyteam == gs->AllyTeam(teamId)) || gs->Ally(gs->AllyTeam(teamId), uh->units[a]->allyteam)) {
					/* do nothing */
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
/*

void CSkirmishAIWrapper::LoadCPPAI(int team, const char* botLibName, bool postLoad, bool loadSupported, bool isJavaAI)
{
	if (isJavaAI) {
		logOutput << botLibName << " is a Java archive\n";
	} else {
		logOutput << botLibName << " has a C++ interface\n";
	}

	_GetGlobalAiVersionFunc = (GETGLOBALAIVERSION) lib->FindAddress("GetGlobalAiVersion");

	if (_GetGlobalAiVersionFunc == 0) {
		char msg[512];
		SNPRINTF(msg, 511, "Incorrect GlobalAI library \"%s\" (no \"GetGlobalAiVersion\" function exported)", botLibName);
		handleerror(NULL, msg, "Error", MBF_OK | MBF_EXCL);
		return;
	}

	const int botInterfaceVersion = _GetGlobalAiVersionFunc();

	if (botInterfaceVersion != GLOBAL_AI_INTERFACE_VERSION) {
		char msg[1024];
		SNPRINTF(msg, 1023,
			"Incorrect GlobalAI library \"%s\"\n"
			"(lib interface version %d, engine interface version %d)",
			botLibName, botInterfaceVersion, GLOBAL_AI_INTERFACE_VERSION);
		handleerror(NULL, msg, "Error", MBF_OK | MBF_EXCL);
		return;
	}


	if (isJavaAI) {
		// we want to load a Java AI inside a jar,
		// pass the name of the actual .jar to the
		// proxy library so it can spawn a JVM for
		// that AI
		_GetNewAIByNameFunc = (GETNEWAIBYNAME) lib->FindAddress("GetNewAIByName");

		if (_GetNewAIByNameFunc == 0) {
			throw std::runtime_error("JAI proxy does not export \"GetNewAIByName\"");
		}

		// note: team parameter is unnecessary
		ai = _GetNewAIByNameFunc(botLibName, team);
	} else {
		_GetNewAIFunc = (GETNEWAI) lib->FindAddress("GetNewAI");

		if (_GetNewAIFunc == 0) {
			char msg[512];
			SNPRINTF(msg, 511, "GlobalAI library \"%s\" does not export \"GetNewAI\"", botLibName);
			throw std::runtime_error(msg);
		}

		ai = _GetNewAIFunc();
	}

	// note: verify that this is really exported too?
	_ReleaseAIFunc = (RELEASEAI) lib->FindAddress("ReleaseAI");
	callback = SAFE_NEW CGlobalAICallback(this);

	if (!postLoad || (postLoad && !loadSupported)) {
		try {
			ai->InitAI(callback, team);
		} HANDLE_EXCEPTION;
	}
}


void CSkirmishAIWrapper::LoadJavaProxyAI()
{
	// TODO: Mac support? non-hardcoded proxy?
	#ifdef WIN32
	const char* javaProxyAI = "AI\\Skirmish\\impls\\JAI\\JAI.dll";
	#else
	const char* javaProxyAI = "AI/Skirmish/impls/JAI/JAI.so";
	#endif

	if (!filesystem.GetFilesize(javaProxyAI)) {
		char msg[512];
		SNPRINTF(msg, 511, "Could not find Java GlobalAI proxy library \"%s\"", javaProxyAI);
		handleerror(NULL, msg, "Error", MBF_OK | MBF_EXCL);
		return;
	}

	lib = SharedLib::Instantiate(javaProxyAI);
}
*/





void CSkirmishAIWrapper::Init() {
	
	callback = SAFE_NEW CGlobalAICallback(this);
	c_callback = initSAICallback(teamId, callback);
	
	SInitEvent evtData = {teamId, c_callback};
	ai->HandleEvent(EVENT_INIT, &evtData);
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

void CSkirmishAIWrapper::UnitDamaged(int unitId, int attackerUnitId, float damage, const float3& dir) {
	
	SUnitDamagedEvent evtData = {unitId, attackerUnitId, damage, dir.toSAIFloat3()};
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

void CSkirmishAIWrapper::EnemyDamaged(int enemyUnitId, int attackerUnitId, float damage, const float3& dir) {
	
	SEnemyDamagedEvent evtData = {enemyUnitId, attackerUnitId, damage, dir.toSAIFloat3()};
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

void CSkirmishAIWrapper::PlayerCommandGiven(const std::vector<int>& selectedUnits, const Command& c, int playerId) {
	
	int numUnits = selectedUnits.size();
	int unitIds[numUnits];
	int i;
	for (i=0; i < numUnits; ++i) {
		unitIds[i] = selectedUnits.at(i);
	}
	int sCommandId;
	void* sCommandData = mallocSUnitCommand(-1, -1, &c, &sCommandId);
	
	SPlayerCommandEvent evtData = {unitIds, numUnits, sCommandId, sCommandData, playerId};
	ai->HandleEvent(EVENT_PLAYER_COMMAND, &evtData);
}

void CSkirmishAIWrapper::SeismicPing(int allyTeam, int unitId, const float3& pos, float strength) {
	
	SSeismicPingEvent evtData = {pos.toSAIFloat3(), strength};
	ai->HandleEvent(EVENT_SEISMIC_PING, &evtData);
}














int CSkirmishAIWrapper::GetTeamId() const {
	return teamId;
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

//IMPLEMENT_PURE_VIRTUAL(CSkirmishAIWrapper::~CSkirmishAIWrapper())
