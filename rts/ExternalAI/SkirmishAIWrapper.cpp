/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SkirmishAIWrapper.h"

#include "AILibraryManager.h"
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
#include "System/Platform/SharedLib.h"
#include "System/TimeProfiler.h"
#include "System/StringUtil.h"

#include <string>
#include <sstream>
#include <iostream>
#include <fstream>

#undef DeleteFile

CR_BIND(CSkirmishAIWrapper, )
CR_REG_METADATA(CSkirmishAIWrapper, (
	CR_MEMBER(key),

	CR_IGNORED(library),
	CR_IGNORED(callback),

	CR_MEMBER(timerName),

	CR_MEMBER(skirmishAIId),
	CR_MEMBER(teamId),

	// handled in PostLoad
	CR_IGNORED(initialized),
	CR_IGNORED(released),
	CR_MEMBER(libraryInit),

	CR_MEMBER(cheatEvents),
	CR_MEMBER(blockEvents),

	CR_SERIALIZER(Serialize),
	CR_POSTLOAD(PostLoad)
))

void CSkirmishAIWrapper::PreInit(int aiID)
{
	const SkirmishAIData* aiData = skirmishAIHandler.GetSkirmishAI(aiID);

	{
		key = aiLibManager->ResolveSkirmishAIKey(SkirmishAIKey(aiData->shortName, aiData->version));
	}
	{
		skirmishAIId = aiID;
		teamId       = aiData->team;
	}
	{
		initialized = false;
		released = false;
		libraryInit = false;

		cheatEvents = false;
		blockEvents = false;
	}
	{
		const std::string& kn = key.GetShortName();
		const std::string& kv = key.GetVersion();

		CTimeProfiler::UnRegisterTimer(GetTimerName());

		memset(&timerName[0], 0, sizeof(timerName));
		snprintf(GetTimerName(), sizeof(timerName) - sizeof(uint32_t) - 1, "AI::{id=%d team=%d name=%s version=%s}", teamId, skirmishAIId, kn.c_str(), kv.c_str());

		const uint32_t hash = hashString(GetTimerName());
		memcpy(&timerName[0], &hash, sizeof(hash));

		CTimeProfiler::RegisterTimer(GetTimerName());
	}

	LOG_L(L_INFO, "[AIWrapper::%s][AI=%d team=%d] creating callbacks", __func__, skirmishAIId, teamId);
	CreateCallback();
}

void CSkirmishAIWrapper::PreDestroy() {
	skirmishAiCallback_BlockOrders(this);
}


void CSkirmishAIWrapper::CreateCallback() {
	library = nullptr;
	callback = skirmishAiCallback_GetInstance(this);
}



bool CSkirmishAIWrapper::InitLibrary(bool postLoad) {
	AILibraryManager* libManager = AILibraryManager::GetInstance();

	{
		ScopedTimer timer(GetTimerNameHash());

		if ((library = libManager->FetchSkirmishAILibrary(key)) == nullptr || !(libraryInit = library->Init(skirmishAIId, callback)))
			skirmishAIHandler.SetLocalKillFlag(skirmishAIId, 5 /* = AI failed to init */);
	}

	// check if initialization went ok
	if (skirmishAIHandler.HasLocalKillFlag(skirmishAIId)) {
		SetBlockEvents(true);
		return false;
	}

	#if 0
	libManager->FetchSkirmishAILibrary(key);

	const auto& libInfoMap = libManager->GetSkirmishAIInfos();
	const auto& libInfoIt  = libInfoMap.find(key);

	const bool loadSupported = (libInfoIt != libInfoMap.end() && libInfoIt->second.GetInfo(SKIRMISH_AI_PROPERTY_LOAD_SUPPORTED) == "yes");

	libManager->ReleaseSkirmishAILibrary(key);

	LOG_L(L_INFO, "[AIWrapper::%s][AI=%d team=%d] postLoad=%d loadSupported=%d numUnits=%u", __func__, skirmishAIId, teamId, postLoad, loadSupported, unitHandler.NumUnitsByTeam(teamId));

	if (!postLoad || loadSupported)
		return true;

	SendInitEvent();
	SendUnitEvents();
	#else
	assert(!postLoad);
	#endif

	return true;
}


void CSkirmishAIWrapper::Init() {
	if (!InitLibrary(false))
		return;

	SendInitEvent();
}

void CSkirmishAIWrapper::Kill()
{
	assert(Active());
	// send release event
	Release(skirmishAIHandler.GetLocalKillFlag(skirmishAIId));

	{
		ScopedTimer timer(GetTimerNameHash());

		if (libraryInit)
			library->Release(skirmishAIId);

		AILibraryManager::GetInstance()->ReleaseSkirmishAILibrary(key);
	}

	{
		skirmishAiCallback_Release(this);
	}
	{
		library = nullptr;
		callback = nullptr;
	}
	{
		// mark as inactive for EngineOutHandler::{Load,Save}; AI data
		// remains present in SkirmishAIHandler until RemoveSkirmishAI
		skirmishAIId = -1;
		teamId = -1;
	}
}

void CSkirmishAIWrapper::Release(int reason)
{
	if (!initialized || released)
		return;

	// NOTE: further cleanup is done in the destructor
	const SReleaseEvent evtData = {reason};
	HandleEvent(EVENT_RELEASE, &evtData);

	released = true;
}


void CSkirmishAIWrapper::SendInitEvent()
{
	const SInitEvent evtData = {skirmishAIId, callback};
	const int error = HandleEvent(EVENT_INIT, &evtData);

	if (error == 0) {
		initialized = true;
		return;
	}

	// init failed
	LOG_L(L_ERROR, "[AIWrapper::%s][AI=%d team=%d] error %d handling EVENT_INIT", __func__, skirmishAIId, teamId, error);
	skirmishAIHandler.SetLocalKillFlag(skirmishAIId, 5 /* = AI failed to init */);
}

void CSkirmishAIWrapper::SendUnitEvents()
{
	LOG_L(L_INFO, "[AIWrapper::%s][AI=%d team=%d] numUnits=%u", __func__, skirmishAIId, teamId, unitHandler.NumUnitsByTeam(teamId));

	// fallback code to help the AI if it
	// doesn't implement load/save support
	for (const CUnit* unit: unitHandler.GetActiveUnits()) {
		if (unit->team == teamId) {
			UnitCreated(unit->id, -1);

			if (!unit->beingBuilt)
				UnitFinished(unit->id);

			continue;
		}

		if (unit->allyteam == teamHandler.AllyTeam(teamId))
			continue;

		if (teamHandler.Ally(teamHandler.AllyTeam(teamId), unit->allyteam))
			continue;

		if (unit->losStatus[teamHandler.AllyTeam(teamId)] & (LOS_INRADAR | LOS_INLOS))
			EnemyEnterRadar(unit->id);

		if (unit->losStatus[teamHandler.AllyTeam(teamId)] & LOS_INLOS)
			EnemyEnterLOS(unit->id);
	}
}



static void streamCopy(/*const*/ std::istream* in, std::ostream* out)
{
	std::array<char, 128> buffer;
	std::streampos start = in->tellg();

	in->read(buffer.data(), buffer.size());

	while (in->good()) {
		out->write(buffer.data(), in->gcount());
		in->read(buffer.data(), buffer.size());
	}

	out->write(buffer.data(), in->gcount());

	in->clear();  // clear fail and eof bits
	in->seekg(start, std::ios::beg);  // back to the start!
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

	assert(Active());
	HandleEvent(EVENT_LOAD, &evtData);

	FileSystem::DeleteFile(tmpFile);
}

void CSkirmishAIWrapper::Save(std::ostream* saveStream)
{
	const std::string tmpFile = createTempFileName("save", teamId, skirmishAIId);
	const SSaveEvent evtData = {tmpFile.c_str()};

	assert(Active());
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

	const int cCommandId = extractAICommandTopic(&c, unitHandler.MaxUnits());
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
	/*const*/ float3 cpyPos = pos;
	const SSeismicPingEvent evtData = {&cpyPos[0], strength};

	HandleEvent(EVENT_SEISMIC_PING, &evtData);
}


int CSkirmishAIWrapper::HandleEvent(int topic, const void* data) const {
	ScopedTimer timer(GetTimerNameHash());

	if (!blockEvents || (topic == EVENT_RELEASE))
		return library->HandleEvent(skirmishAIId, topic, data);

	// to prevent log error spam, signal: OK
	return 0;
}

