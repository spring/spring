/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SkirmishAIWrapper.h"

#include "AICallback.h"
#include "AICheats.h"
#include "EngineOutHandler.h"
#include "IAILibraryManager.h"

#include "SkirmishAIHandler.h"
#include "SkirmishAILibrary.h"
#include "SkirmishAILibraryInfo.h"
#include "SkirmishAIData.h"
#include "SSkirmishAICallbackImpl.h"

#include "Interface/AISEvents.h"
#include "Interface/AISCommands.h"
#include "Interface/SSkirmishAILibrary.h"

#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Misc/TeamHandler.h"

#include "System/FileSystem/DataDirsAccess.h"
#include "System/FileSystem/FileQueryFlags.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Log/ILog.h"
#include "System/TimeProfiler.h"
#include "System/Util.h"

#include <sstream>
#include <iostream>
#include <fstream>

#undef DeleteFile

CR_BIND_DERIVED(CSkirmishAIWrapper, CObject, )
CR_REG_METADATA(CSkirmishAIWrapper, (
	CR_MEMBER(key),

	CR_IGNORED(library),
	CR_IGNORED(sCallback),

	CR_IGNORED(callback),
	CR_IGNORED(cheats),

	CR_MEMBER(timerName),

	CR_MEMBER(skirmishAIId),
	CR_MEMBER(teamId),

	// handled in PostLoad
	CR_IGNORED(initialized),
	CR_IGNORED(released),
	CR_MEMBER(cheatEvents),

	CR_MEMBER(initOk),
	CR_MEMBER(dieing),

	CR_SERIALIZER(Serialize),
	CR_POSTLOAD(PostLoad)
))

/// used only by creg
CSkirmishAIWrapper::CSkirmishAIWrapper():
	library(nullptr),
	sCallback(nullptr),

	skirmishAIId(-1),
	teamId(-1),

	initialized(false),
	released(false),
	cheatEvents(false),

	initOk(false),
	dieing(false)
{
}

CSkirmishAIWrapper::CSkirmishAIWrapper(const int skirmishAIId):
	library(nullptr),
	sCallback(nullptr),

	skirmishAIId(skirmishAIId),
	teamId(-1),

	initialized(false),
	released(false),
	cheatEvents(false),

	initOk(false),
	dieing(false)
{
	const SkirmishAIData* aiData = skirmishAIHandler.GetSkirmishAI(skirmishAIId);

	teamId = aiData->team;
	key    = aiLibManager->ResolveSkirmishAIKey(SkirmishAIKey(aiData->shortName, aiData->version));

	timerName += ("AI t:" + IntToString(teamId));
	timerName += (" id:" + IntToString(skirmishAIId));
	timerName += (" " + key.GetShortName());
	timerName += (" " + key.GetVersion());

	CreateCallback();
}

void CSkirmishAIWrapper::CreateCallback() {
	callback.reset(new CAICallback(teamId));
	cheats.reset(new CAICheats(this));

	sCallback = skirmishAiCallback_getInstanceFor(skirmishAIId, teamId, callback.get(), cheats.get());
}

CSkirmishAIWrapper::~CSkirmishAIWrapper() {
	// send release event
	Release(skirmishAIHandler.GetLocalSkirmishAIDieReason(skirmishAIId));

	{
		SCOPED_TIMER(timerName.c_str());

		if (initOk)
			library->Release(skirmishAIId);

		IAILibraryManager::GetInstance()->ReleaseSkirmishAILibrary(key);
	}

	// NOTE: explicitly ordered so callback is valid in AI's dtor
	cheats.reset();
	callback.reset();

	skirmishAiCallback_release(skirmishAIId);
}

void CSkirmishAIWrapper::PreDestroy() {
	callback->noMessages = true;
}

void CSkirmishAIWrapper::Serialize(creg::ISerializer* s) {
}

void CSkirmishAIWrapper::PostLoad() {
	CreateCallback();
	LoadSkirmishAI(true);
}



bool CSkirmishAIWrapper::LoadSkirmishAI(bool postLoad) {
	{
		SCOPED_TIMER(timerName.c_str());

		library = IAILibraryManager::GetInstance()->FetchSkirmishAILibrary(key);

		if (library == nullptr) {
			dieing = true;
			skirmishAIHandler.SetLocalSkirmishAIDieing(skirmishAIId, 5 /* = AI failed to init */);
		} else {
			initOk = library->Init(skirmishAIId, sCallback);
		}
	}

	// check if initialization went ok
	if (skirmishAIHandler.IsLocalSkirmishAIDieing(skirmishAIId))
		return false;

	IAILibraryManager* libManager = IAILibraryManager::GetInstance();
	libManager->FetchSkirmishAILibrary(key);

	const CSkirmishAILibraryInfo& infos = (libManager->GetSkirmishAIInfos().find(key))->second;
	const bool loadSupported = (infos.GetInfo(SKIRMISH_AI_PROPERTY_LOAD_SUPPORTED) == "yes");

	libManager->ReleaseSkirmishAILibrary(key);

	if (!postLoad || loadSupported)
		return true;

	// fallback code to help the AI if it
	// doesn't implement load/save support
	Init();

	for (size_t a = 0; a < unitHandler->MaxUnits(); a++) {
		const CUnit* unit = unitHandler->units[a];

		if (unit == nullptr)
			continue;

		if (unit->team == teamId) {
			UnitCreated(a, -1);

			if (!unit->beingBuilt) {
				UnitFinished(a);
			}
		} else {
			if (unit->allyteam == teamHandler->AllyTeam(teamId))
				continue;

			if (teamHandler->Ally(teamHandler->AllyTeam(teamId), unit->allyteam))
				continue;

			if (unit->losStatus[teamHandler->AllyTeam(teamId)] & (LOS_INRADAR | LOS_INLOS)) {
				EnemyEnterRadar(a);
			}

			if (unit->losStatus[teamHandler->AllyTeam(teamId)] & LOS_INLOS) {
				EnemyEnterLOS(a);
			}
		}
	}

	return true;
}


void CSkirmishAIWrapper::Init() {
	if (!LoadSkirmishAI(false))
		return;

	const SInitEvent evtData = {skirmishAIId, sCallback};
	const int error = HandleEvent(EVENT_INIT, &evtData);

	if (error == 0) {
		initialized = true;
		return;
	}

	// init failed
	LOG_L(L_ERROR, "Failed to handle init event: AI for team %d, error %d", teamId, error);
	skirmishAIHandler.SetLocalSkirmishAIDieing(skirmishAIId, 5 /* = AI failed to init */);
}

void CSkirmishAIWrapper::Release(int reason) {
	if (!initialized || released)
		return;

	// NOTE: further cleanup is done in the destructor
	const SReleaseEvent evtData = {reason};
	HandleEvent(EVENT_RELEASE, &evtData);

	released = true;
}



static void streamCopy(/*const*/ std::istream* in, std::ostream* out)
{
	std::vector<char> buffer(128);

	in->read(&buffer[0], buffer.size());

	while (in->good()) {
		out->write(&buffer[0], in->gcount());
		in->read(&buffer[0], buffer.size());
	}

	out->write(&buffer[0], in->gcount());
}

static std::string createTempFileName(const char* action, int teamId, int skirmishAIId) {
	char tmpFileName[1024];

	SNPRINTF(tmpFileName, sizeof(tmpFileName), "%s-team%i_id%i.tmp", action, teamId, skirmishAIId);

	return (dataDirsAccess.LocateFile(tmpFileName, FileQueryFlags::WRITE | FileQueryFlags::CREATE_DIRS));
}


void CSkirmishAIWrapper::Load(std::istream* loadStream)
{
	const std::string tmpFile = createTempFileName("load", teamId, skirmishAIId);
	const SLoadEvent evtData = {tmpFile.c_str()};

	{
		std::ofstream tmpFileStream;

		tmpFileStream.open(tmpFile.c_str(), std::ios::binary);
		streamCopy(loadStream, &tmpFileStream);
		tmpFileStream.close();
	}

	HandleEvent(EVENT_LOAD, &evtData);

	FileSystem::DeleteFile(tmpFile);
}

void CSkirmishAIWrapper::Save(std::ostream* saveStream)
{
	const std::string tmpFile = createTempFileName("save", teamId, skirmishAIId);
	const SSaveEvent evtData = {tmpFile.c_str()};

	HandleEvent(EVENT_SAVE, &evtData);

	if (!FileSystem::FileExists(tmpFile))
		return;

	{
		std::ifstream tmpFileStream;

		tmpFileStream.open(tmpFile.c_str(), std::ios::binary);
		streamCopy(&tmpFileStream, saveStream);
		tmpFileStream.close();
	}

	FileSystem::DeleteFile(tmpFile);
}



void CSkirmishAIWrapper::UnitIdle(int unitId) {
	const SUnitIdleEvent evtData = {unitId};
	HandleEvent(EVENT_UNIT_IDLE, &evtData);
}

void CSkirmishAIWrapper::UnitCreated(int unitId, int builderId) {
	const SUnitCreatedEvent evtData = {unitId, builderId};
	HandleEvent(EVENT_UNIT_CREATED, &evtData);
}

void CSkirmishAIWrapper::UnitFinished(int unitId) {
	const SUnitFinishedEvent evtData = {unitId};
	HandleEvent(EVENT_UNIT_FINISHED, &evtData);
}

void CSkirmishAIWrapper::UnitDestroyed(int unitId, int attackerUnitId) {
	const SUnitDestroyedEvent evtData = {unitId, attackerUnitId};
	HandleEvent(EVENT_UNIT_DESTROYED, &evtData);
}

void CSkirmishAIWrapper::UnitDamaged(
	int unitId,
	int attackerUnitId,
	float damage,
	const float3& dir,
	int weaponDefId,
	bool paralyzer
) {
	float3 cpyDir = dir;
	const SUnitDamagedEvent evtData = {unitId, attackerUnitId, damage, &cpyDir[0], weaponDefId, paralyzer};

	HandleEvent(EVENT_UNIT_DAMAGED, &evtData);
}

void CSkirmishAIWrapper::UnitMoveFailed(int unitId) {
	const SUnitMoveFailedEvent evtData = {unitId};
	HandleEvent(EVENT_UNIT_MOVE_FAILED, &evtData);
}

void CSkirmishAIWrapper::UnitGiven(int unitId, int oldTeam, int newTeam) {
	const SUnitGivenEvent evtData = {unitId, oldTeam, newTeam};
	HandleEvent(EVENT_UNIT_GIVEN, &evtData);
}

void CSkirmishAIWrapper::UnitCaptured(int unitId, int oldTeam, int newTeam) {
	const SUnitCapturedEvent evtData = {unitId, oldTeam, newTeam};
	HandleEvent(EVENT_UNIT_CAPTURED, &evtData);
}


void CSkirmishAIWrapper::EnemyCreated(int unitId) {
	const SEnemyCreatedEvent evtData = {unitId};
	HandleEvent(EVENT_ENEMY_CREATED, &evtData);
}

void CSkirmishAIWrapper::EnemyFinished(int unitId) {
	const SEnemyFinishedEvent evtData = {unitId};
	HandleEvent(EVENT_ENEMY_FINISHED, &evtData);
}

void CSkirmishAIWrapper::EnemyEnterLOS(int unitId) {
	const SEnemyEnterLOSEvent evtData = {unitId};
	HandleEvent(EVENT_ENEMY_ENTER_LOS, &evtData);
}

void CSkirmishAIWrapper::EnemyLeaveLOS(int unitId) {
	const SEnemyLeaveLOSEvent evtData = {unitId};
	HandleEvent(EVENT_ENEMY_LEAVE_LOS, &evtData);
}

void CSkirmishAIWrapper::EnemyEnterRadar(int unitId) {
	const SEnemyEnterRadarEvent evtData = {unitId};
	HandleEvent(EVENT_ENEMY_ENTER_RADAR, &evtData);
}

void CSkirmishAIWrapper::EnemyLeaveRadar(int unitId) {
	const SEnemyLeaveRadarEvent evtData = {unitId};
	HandleEvent(EVENT_ENEMY_LEAVE_RADAR, &evtData);
}

void CSkirmishAIWrapper::EnemyDestroyed(int enemyUnitId, int attackerUnitId) {
	const SEnemyDestroyedEvent evtData = {enemyUnitId, attackerUnitId};
	HandleEvent(EVENT_ENEMY_DESTROYED, &evtData);
}

void CSkirmishAIWrapper::EnemyDamaged(
	int enemyUnitId,
	int attackerUnitId,
	float damage,
	const float3& dir,
	int weaponDefId,
	bool paralyzer
) {
	float3 cpyDir = dir;
	const SEnemyDamagedEvent evtData = {enemyUnitId, attackerUnitId, damage, &cpyDir[0], weaponDefId, paralyzer};

	HandleEvent(EVENT_ENEMY_DAMAGED, &evtData);
}

void CSkirmishAIWrapper::Update(int frame) {
	const SUpdateEvent evtData = {frame};
	HandleEvent(EVENT_UPDATE, &evtData);
}

void CSkirmishAIWrapper::SendChatMessage(const char* msg, int fromPlayerId) {
	const SMessageEvent evtData = {fromPlayerId, msg};
	HandleEvent(EVENT_MESSAGE, &evtData);
}

void CSkirmishAIWrapper::SendLuaMessage(const char* inData, const char** outData) {
	const SLuaMessageEvent evtData = {inData /*outData*/};
	HandleEvent(EVENT_LUA_MESSAGE, &evtData);
}

void CSkirmishAIWrapper::WeaponFired(int unitId, int weaponDefId) {
	const SWeaponFiredEvent evtData = {unitId, weaponDefId};
	HandleEvent(EVENT_WEAPON_FIRED, &evtData);
}

void CSkirmishAIWrapper::PlayerCommandGiven(
	const std::vector<int>& playerSelectedUnits,
	const Command& c,
	int playerId
) {
	std::vector<int> unitIds = playerSelectedUnits;

	const int cCommandId = extractAICommandTopic(&c, unitHandler->MaxUnits());
	const SPlayerCommandEvent evtData = {&unitIds[0], static_cast<int>(playerSelectedUnits.size()), cCommandId, playerId};

	HandleEvent(EVENT_PLAYER_COMMAND, &evtData);
}

void CSkirmishAIWrapper::CommandFinished(int unitId, int commandId, int commandTopicId) {
	const SCommandFinishedEvent evtData = {unitId, commandId, commandTopicId};
	HandleEvent(EVENT_COMMAND_FINISHED, &evtData);
}

void CSkirmishAIWrapper::SeismicPing(
	int allyTeam,
	int unitId,
	const float3& pos,
	float strength
) {
	float3 cpyPos = pos;
	const SSeismicPingEvent evtData = {&cpyPos[0], strength};

	HandleEvent(EVENT_SEISMIC_PING, &evtData);
}


int CSkirmishAIWrapper::HandleEvent(int topic, const void* data) const {
	SCOPED_TIMER(timerName.c_str());

	if (!dieing || (topic == EVENT_RELEASE))
		return library->HandleEvent(skirmishAIId, topic, data);

	// to prevent log error spam, signal: OK
	return 0;
}

