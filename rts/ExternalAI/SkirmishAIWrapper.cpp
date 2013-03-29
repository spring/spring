/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SkirmishAIWrapper.h"

#include "System/FileSystem/DataDirsAccess.h"
#include "System/FileSystem/FileQueryFlags.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Log/ILog.h"
#include "System/Util.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "ExternalAI/AICallback.h"
#include "ExternalAI/AICheats.h"
#include "ExternalAI/SkirmishAI.h"
#include "ExternalAI/EngineOutHandler.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include "ExternalAI/SkirmishAILibraryInfo.h"
#include "ExternalAI/SkirmishAIData.h"
#include "ExternalAI/SSkirmishAICallbackImpl.h"
#include "ExternalAI/IAILibraryManager.h"
#include "ExternalAI/Interface/AISEvents.h"
#include "ExternalAI/Interface/AISCommands.h"
#include "ExternalAI/Interface/SSkirmishAILibrary.h"

#include <sstream>
#include <iostream>
#include <fstream>

#undef DeleteFile

CR_BIND_DERIVED(CSkirmishAIWrapper, CObject, )
CR_REG_METADATA(CSkirmishAIWrapper, (
	CR_MEMBER(skirmishAIId),
	CR_MEMBER(teamId),
	CR_MEMBER(cheatEvents),
	CR_MEMBER(key),

	// handled in PostLoad
	CR_IGNORED(initialized),
	CR_IGNORED(released),
	CR_IGNORED(ai),
	CR_IGNORED(callback),
	CR_IGNORED(cheats),
	CR_IGNORED(c_callback),
	CR_IGNORED(info),

	CR_SERIALIZER(Serialize),
	CR_POSTLOAD(PostLoad)
));

/// used only by creg
CSkirmishAIWrapper::CSkirmishAIWrapper():
		skirmishAIId(-1),
		teamId(-1),
		cheatEvents(false),
		ai(NULL),
		initialized(false),
		released(false),
		callback(NULL),
		cheats(NULL),
		c_callback(NULL),
		info(NULL)
{
}

CSkirmishAIWrapper::CSkirmishAIWrapper(const int skirmishAIId):
		skirmishAIId(skirmishAIId),
		teamId(-1),
		cheatEvents(false),
		ai(NULL),
		initialized(false),
		released(false),
		callback(NULL),
		cheats(NULL),
		c_callback(NULL),
		info(NULL)
{
	const SkirmishAIData* aiData = skirmishAIHandler.GetSkirmishAI(skirmishAIId);

	teamId = aiData->team;
	SkirmishAIKey keyTmp(aiData->shortName, aiData->version);
	key    = aiLibManager->ResolveSkirmishAIKey(keyTmp);

	CreateCallback();
}

void CSkirmishAIWrapper::CreateCallback() {

	if (c_callback == NULL) {
		callback = new CAICallback(teamId);
		cheats = new CAICheats(this);
		c_callback = skirmishAiCallback_getInstanceFor(skirmishAIId, teamId, callback, cheats);
	}
}

void CSkirmishAIWrapper::PreDestroy() {
	callback->noMessages = true;
}

CSkirmishAIWrapper::~CSkirmishAIWrapper() {
	if (ai) {
		if (initialized && !released) {
			Release();
		}

		delete ai;
		ai = NULL;

		skirmishAiCallback_release(skirmishAIId);
		c_callback = NULL;

		delete callback;
		callback = NULL;

		delete cheats;
		cheats = NULL;
	}
}

void CSkirmishAIWrapper::Serialize(creg::ISerializer* s) {
}

void CSkirmishAIWrapper::PostLoad() {
	//CreateCallback();
	LoadSkirmishAI(true);
}



bool CSkirmishAIWrapper::LoadSkirmishAI(bool postLoad) {

	ai = new CSkirmishAI(skirmishAIId, teamId, key, GetCallback());
	ai->Init();

	// check if initialization went ok
	if (skirmishAIHandler.IsLocalSkirmishAIDieing(skirmishAIId)) {
		return false;
	}

	IAILibraryManager* libManager = IAILibraryManager::GetInstance();
	libManager->FetchSkirmishAILibrary(key);

	const CSkirmishAILibraryInfo* infos = &*libManager->GetSkirmishAIInfos().find(key)->second;
	bool loadSupported = (infos->GetInfo(SKIRMISH_AI_PROPERTY_LOAD_SUPPORTED) == "yes");

	libManager->ReleaseSkirmishAILibrary(key);


	if (postLoad && !loadSupported) {
		// fallback code to help the AI if it
		// doesn't implement load/save support
		Init();
		for (size_t a = 0; a < unitHandler->MaxUnits(); a++) {
			if (!unitHandler->units[a])
				continue;

			if (unitHandler->units[a]->team == teamId) {
				try {
					UnitCreated(a, -1);
				} CATCH_AI_EXCEPTION;
				if (!unitHandler->units[a]->beingBuilt)
					try {
						UnitFinished(a);
					} CATCH_AI_EXCEPTION;
			} else {
				if ((unitHandler->units[a]->allyteam == teamHandler->AllyTeam(teamId))
						|| teamHandler->Ally(teamHandler->AllyTeam(teamId), unitHandler->units[a]->allyteam)) {
					// do nothing
				} else {
					if (unitHandler->units[a]->losStatus[teamHandler->AllyTeam(teamId)] & (LOS_INRADAR | LOS_INLOS)) {
						try {
							EnemyEnterRadar(a);
						} CATCH_AI_EXCEPTION;
					}
					if (unitHandler->units[a]->losStatus[teamHandler->AllyTeam(teamId)] & LOS_INLOS) {
						try {
							EnemyEnterLOS(a);
						} CATCH_AI_EXCEPTION;
					}
				}
			}
		}
	}

	return true;
}


void CSkirmishAIWrapper::Init() {

	if (ai == NULL) {
		bool loadOk = LoadSkirmishAI(false);
		if (!loadOk) {
			return;
		}
	}

	SInitEvent evtData = {skirmishAIId, GetCallback()};
	int error = ai->HandleEvent(EVENT_INIT, &evtData);
	if (error != 0) {
		// init failed
		LOG_L(L_ERROR, "Failed to handle init event: AI for team %d, error %d",
				teamId, error);
		skirmishAIHandler.SetLocalSkirmishAIDieing(skirmishAIId, 5 /* = AI failed to init */);
	} else {
		initialized = true;
	}
}

void CSkirmishAIWrapper::Dieing() {

	if (ai != NULL) {
		ai->Dieing();
	}
}

void CSkirmishAIWrapper::Release(int reason) {

	if (initialized && !released) {
		SReleaseEvent evtData = {reason};
		ai->HandleEvent(EVENT_RELEASE, &evtData);

		released = true;

		// further cleanup is done in the destructor
	}
}

static void streamCopy(std::istream* in, std::ostream* out)
{
	static const size_t buffer_size = 128;

	// have to use _new_ here for VS compatibility (no C99 support)
	char* buffer = new char[buffer_size];

	in->read(buffer, buffer_size);
	while (in->good()) {
		out->write(buffer, in->gcount());
		in->read(buffer, buffer_size);
	}
	out->write(buffer, in->gcount());

	delete[] buffer;
}

static std::string createTempFileName(const char* action, int teamId,
		int skirmishAIId) {

	static const size_t tmpFileName_size = 1024;

	// have to use _new_ here for VS compatibility (no C99 support)
	char* tmpFileName = new char[tmpFileName_size];
	SNPRINTF(tmpFileName, tmpFileName_size, "%s-team%i_id%i.tmp", action,
			teamId, skirmishAIId);
	std::string tmpFile = dataDirsAccess.LocateFile(tmpFileName,
			FileQueryFlags::WRITE | FileQueryFlags::CREATE_DIRS);
	delete[] tmpFileName;
	return tmpFile;
}

void CSkirmishAIWrapper::Load(std::istream* load_s)
{
	const std::string tmpFile = createTempFileName("load", teamId, skirmishAIId);

	std::ofstream tmpFile_s;
	tmpFile_s.open(tmpFile.c_str(), std::ios::binary);
	streamCopy(load_s, &tmpFile_s);
	tmpFile_s.close();

	SLoadEvent evtData = {tmpFile.c_str()};
	ai->HandleEvent(EVENT_LOAD, &evtData);

	FileSystem::DeleteFile(tmpFile);
}

void CSkirmishAIWrapper::Save(std::ostream* save_s)
{
	const std::string tmpFile = createTempFileName("save", teamId, skirmishAIId);

	SSaveEvent evtData = {tmpFile.c_str()};
	ai->HandleEvent(EVENT_SAVE, &evtData);

	if (FileSystem::FileExists(tmpFile)) {
		std::ifstream tmpFile_s;
		tmpFile_s.open(tmpFile.c_str(), std::ios::binary);
		streamCopy(&tmpFile_s, save_s);
		tmpFile_s.close();
		FileSystem::DeleteFile(tmpFile);
	}
}

void CSkirmishAIWrapper::UnitIdle(int unitId) {
	SUnitIdleEvent evtData = {unitId};
	ai->HandleEvent(EVENT_UNIT_IDLE, &evtData);
}

void CSkirmishAIWrapper::UnitCreated(int unitId, int builderId) {
	SUnitCreatedEvent evtData = {unitId, builderId};
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
		float damage, const float3& dir, int weaponDefId, bool paralyzer) {

	SUnitDamagedEvent evtData = {unitId, attackerUnitId, damage,
			new float[3], weaponDefId, paralyzer};
	dir.copyInto(evtData.dir_posF3);
	ai->HandleEvent(EVENT_UNIT_DAMAGED, &evtData);
	delete [] evtData.dir_posF3;
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


void CSkirmishAIWrapper::EnemyCreated(int unitId) {
	SEnemyCreatedEvent evtData = {unitId};
	ai->HandleEvent(EVENT_ENEMY_CREATED, &evtData);
}

void CSkirmishAIWrapper::EnemyFinished(int unitId) {
	SEnemyFinishedEvent evtData = {unitId};
	ai->HandleEvent(EVENT_ENEMY_FINISHED, &evtData);
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
		float damage, const float3& dir, int weaponDefId, bool paralyzer) {

	SEnemyDamagedEvent evtData = {enemyUnitId, attackerUnitId, damage,
			new float[3], weaponDefId, paralyzer};
	dir.copyInto(evtData.dir_posF3);
	ai->HandleEvent(EVENT_ENEMY_DAMAGED, &evtData);
	delete [] evtData.dir_posF3;
}

void CSkirmishAIWrapper::Update(int frame) {
	SUpdateEvent evtData = {frame};
	ai->HandleEvent(EVENT_UPDATE, &evtData);
}

void CSkirmishAIWrapper::SendChatMessage(const char* msg, int fromPlayerId) {
	SMessageEvent evtData = {fromPlayerId, msg};
	ai->HandleEvent(EVENT_MESSAGE, &evtData);
}

void CSkirmishAIWrapper::SendLuaMessage(const char* inData, const char** outData) {
	SLuaMessageEvent evtData = {inData /*outData*/};
	ai->HandleEvent(EVENT_LUA_MESSAGE, &evtData);
}

void CSkirmishAIWrapper::WeaponFired(int unitId, int weaponDefId) {
	SWeaponFiredEvent evtData = {unitId, weaponDefId};
	ai->HandleEvent(EVENT_WEAPON_FIRED, &evtData);
}

void CSkirmishAIWrapper::PlayerCommandGiven(
	const std::vector<int>& playerSelectedUnits,
	const Command& c,
	int playerId
) {
	const int cCommandId = extractAICommandTopic(&c, unitHandler->MaxUnits());
	int* unitIds = new int[playerSelectedUnits.size()];

	for (unsigned int i = 0; i < playerSelectedUnits.size(); ++i) {
		unitIds[i] = playerSelectedUnits[i];
	}

	SPlayerCommandEvent evtData = {unitIds, playerSelectedUnits.size(), cCommandId, playerId};
	ai->HandleEvent(EVENT_PLAYER_COMMAND, &evtData);
	delete[] unitIds;
}

void CSkirmishAIWrapper::CommandFinished(int unitId, int commandId, int commandTopicId) {
	SCommandFinishedEvent evtData = {unitId, commandId, commandTopicId};
	ai->HandleEvent(EVENT_COMMAND_FINISHED, &evtData);
}

void CSkirmishAIWrapper::SeismicPing(int allyTeam, int unitId,
		const float3& pos, float strength) {

	SSeismicPingEvent evtData = {new float[3], strength};
	pos.copyInto(evtData.pos_posF3);
	ai->HandleEvent(EVENT_SEISMIC_PING, &evtData);
	delete [] evtData.pos_posF3;
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
