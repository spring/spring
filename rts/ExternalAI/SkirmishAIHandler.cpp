/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SkirmishAIHandler.h"

#include "ExternalAI/SkirmishAIKey.h"
#include "ExternalAI/IAILibraryManager.h"
#include "ExternalAI/EngineOutHandler.h"
#include "ExternalAI/LuaAIImplHandler.h"
#include "ExternalAI/Interface/SSkirmishAILibrary.h"
#include "Game/GameSetup.h"
#include "Game/GlobalUnsynced.h"
#include "Net/Protocol/NetProtocol.h"
#include "System/Option.h"

#include "System/creg/STL_Map.h"
#include "System/creg/STL_Set.h"

#include <assert.h>

CR_BIND(CSkirmishAIHandler,)

CR_REG_METADATA(CSkirmishAIHandler, (
	CR_MEMBER(id_ai),
	CR_MEMBER(team_localAIsInCreation),
	CR_MEMBER(id_dieReason),
	CR_MEMBER(id_libKey),
	CR_MEMBER(gameInitialized),
	CR_MEMBER(luaAIShortNames),
	CR_IGNORED(currentAIId)
))


// not extern'ed, so static
static CSkirmishAIHandler* gSkirmishAIHandler = nullptr;

CSkirmishAIHandler* CSkirmishAIHandler::GetInstance()
{
	if (gSkirmishAIHandler == nullptr) {
		gSkirmishAIHandler = new CSkirmishAIHandler();
	}

	return gSkirmishAIHandler;
}

void CSkirmishAIHandler::FreeInstance(CSkirmishAIHandler* handler)
{
	assert(handler == gSkirmishAIHandler);
	delete handler;
	gSkirmishAIHandler = nullptr;
}



void CSkirmishAIHandler::ResetState()
{
	id_ai.clear();
	id_libKey.clear();

	team_localAIsInCreation.clear();
	id_dieReason.clear();

	luaAIShortNames.clear();

	currentAIId = MAX_AIS;
	gameInitialized = false;
}

void CSkirmishAIHandler::LoadFromSetup(const CGameSetup& setup) {

	for (size_t a = 0; a < setup.GetAIStartingDataCont().size(); ++a) {
		AddSkirmishAI(setup.GetAIStartingDataCont()[a], a);
	}
}

void CSkirmishAIHandler::LoadPreGame() {

	if (gameInitialized)
		return;

	// Extract all Lua AI implementations short names
	const std::vector< std::vector<InfoItem> >& luaAIImpls = luaAIImplHandler.LoadInfos();
	for (auto impl = luaAIImpls.cbegin(); impl != luaAIImpls.cend(); ++impl) {
		for (auto info = impl->cbegin(); info != impl->cend(); ++info) {
			if (info->key != SKIRMISH_AI_PROPERTY_SHORT_NAME)
				continue;

			assert(info->valueType == INFO_VALUE_TYPE_STRING);
			luaAIShortNames.insert(info->valueTypeString);
		}
	}

	gameInitialized = true;

	// actualize the already added SkirmishAIData's
	for (auto ai = id_ai.begin(); ai != id_ai.end(); ++ai) {
		CompleteSkirmishAI(ai->first);
	}
}

bool CSkirmishAIHandler::IsActiveSkirmishAI(const size_t skirmishAIId) const {
	return (id_ai.find(skirmishAIId) != id_ai.end());
}

SkirmishAIData* CSkirmishAIHandler::GetSkirmishAI(const size_t skirmishAIId) {
	const auto ai = id_ai.find(skirmishAIId);
	assert(ai != id_ai.end());

	return &(ai->second);
}

size_t CSkirmishAIHandler::GetSkirmishAI(const std::string& name) const
{
	size_t skirmishAIId = 0;

	bool found = false;
	for (id_ai_t::const_iterator ai = id_ai.begin(); ai != id_ai.end() && !found; ++ai) {
		if (ai->second.name == name) {
			skirmishAIId = ai->first;
			found = true;
		}
	}
	assert(found);

	return skirmishAIId;
}

CSkirmishAIHandler::ids_t CSkirmishAIHandler::GetSkirmishAIsInTeam(const int teamId, const int hostPlayerId)
{
	ids_t skirmishAIs;

	for (auto ai = id_ai.cbegin(); ai != id_ai.cend(); ++ai) {
		if ((ai->second.team == teamId) && ((hostPlayerId < 0) || (ai->second.hostPlayer == hostPlayerId))) {
			skirmishAIs.push_back(ai->first);
		}
	}

	return skirmishAIs;
}

CSkirmishAIHandler::ids_t CSkirmishAIHandler::GetSkirmishAIsByPlayer(const int hostPlayerId)
{
	ids_t skirmishAIs;

	for (auto ai = id_ai.cbegin(); ai != id_ai.cend(); ++ai) {
		if (ai->second.hostPlayer == hostPlayerId) {
			skirmishAIs.push_back(ai->first);
		}
	}

	return skirmishAIs;
}


void CSkirmishAIHandler::AddSkirmishAI(const SkirmishAIData& data, const size_t skirmishAIId) {
	// if the ID is already taken, something went very wrong
	assert(id_ai.find(skirmishAIId) == id_ai.end());

	id_ai[skirmishAIId] = data;
	team_localAIsInCreation.erase(data.team);

	CompleteSkirmishAI(skirmishAIId);
}

bool CSkirmishAIHandler::RemoveSkirmishAI(const size_t skirmishAIId) {
	team_localAIsInCreation.erase(id_ai[skirmishAIId].team);

	id_dieReason.erase(skirmishAIId);
	id_libKey.erase(skirmishAIId);

	return id_ai.erase(skirmishAIId);
}


void CSkirmishAIHandler::CreateLocalSkirmishAI(const size_t skirmishAIId) {
	SkirmishAIData* aiData = GetSkirmishAI(skirmishAIId);

	// fail, if a local AI is already in line for this team
	assert(team_localAIsInCreation.find(aiData->team) == team_localAIsInCreation.end());
	// fail, if the specified AI is not a local one
	assert(CSkirmishAIHandler::IsLocalSkirmishAI(*aiData));

	team_localAIsInCreation[aiData->team] = *aiData;
	aiData->isLuaAI = IsLuaAI(*aiData);
	team_localAIsInCreation[aiData->team].isLuaAI = aiData->isLuaAI;

	// create instantly
	eoh->CreateSkirmishAI(skirmishAIId);
}

void CSkirmishAIHandler::CreateLocalSkirmishAI(const SkirmishAIData& aiData) {
	// fail if a local AI is already in line for this team
	assert(team_localAIsInCreation.find(aiData.team) == team_localAIsInCreation.end());
	// fail, if the specified AI is not a local one
	assert(CSkirmishAIHandler::IsLocalSkirmishAI(aiData));

	team_localAIsInCreation[aiData.team] = aiData;
	team_localAIsInCreation[aiData.team].isLuaAI = IsLuaAI(aiData);

	// this will be ignored; the real one is generated by the server
	static const size_t unspecified_skirmishAIId = 0;
	// send to server, as the AI was not specified in the start script
	clientNet->Send(CBaseNetProtocol::Get().SendAICreated(aiData.hostPlayer, unspecified_skirmishAIId, aiData.team, aiData.name));
}

const SkirmishAIData* CSkirmishAIHandler::GetLocalSkirmishAIInCreation(const int teamId) const {
	return (team_localAIsInCreation.find(teamId) != team_localAIsInCreation.end()) ? &(team_localAIsInCreation.find(teamId)->second) : nullptr;
}

void CSkirmishAIHandler::SetLocalSkirmishAIDieing(const size_t skirmishAIId, const int reason) {
	const auto ai = id_ai.find(skirmishAIId);

	assert(ai != id_ai.end()); // is valid id?
	assert(CSkirmishAIHandler::IsLocalSkirmishAI(ai->second)); // is local AI?

	if (!GetSkirmishAI(skirmishAIId)->isLuaAI)
		eoh->SetSkirmishAIDieing(skirmishAIId);

	id_dieReason[skirmishAIId] = reason;

	clientNet->Send(CBaseNetProtocol::Get().SendAIStateChanged(gu->myPlayerNum, skirmishAIId, SKIRMAISTATE_DIEING));
}


const SkirmishAIKey* CSkirmishAIHandler::GetLocalSkirmishAILibraryKey(const size_t skirmishAIId) {
	// fail, if the specified AI is not a local one
	assert(CSkirmishAIHandler::IsLocalSkirmishAI(skirmishAIId));

	const auto libKey = id_libKey.find(skirmishAIId);

	// already resolved
	if (libKey != id_libKey.end())
		return &(libKey->second);

	// resolve it
	const SkirmishAIData* aiData = GetSkirmishAI(skirmishAIId);
	const SkirmishAIKey& resKey = aiLibManager->ResolveSkirmishAIKey(SkirmishAIKey(aiData->shortName, aiData->version));

	assert(!resKey.IsUnspecified());

	id_libKey[skirmishAIId] = resKey;
	return &(id_libKey[skirmishAIId]);
}


bool CSkirmishAIHandler::IsLocalSkirmishAI(const size_t skirmishAIId) const {
	const auto ai = id_ai.find(skirmishAIId);

	if (ai != id_ai.end())
		return CSkirmishAIHandler::IsLocalSkirmishAI(ai->second);

	return false;
}

bool CSkirmishAIHandler::IsLocalSkirmishAI(const SkirmishAIData& aiData) {
	return (aiData.hostPlayer == gu->myPlayerNum);
}


void CSkirmishAIHandler::CompleteWithDefaultOptionValues(const size_t skirmishAIId)
{
	if (!gameInitialized)
		return;

	if (!IsLocalSkirmishAI(skirmishAIId))
		return;

	const IAILibraryManager* aiLibMan = IAILibraryManager::GetInstance();
	const IAILibraryManager::T_skirmishAIInfos& aiInfos = aiLibMan->GetSkirmishAIInfos();
	const SkirmishAIKey* aiKey = GetLocalSkirmishAILibraryKey(skirmishAIId);

	if (aiKey == nullptr)
		return;

	const IAILibraryManager::T_skirmishAIInfos::const_iterator inf = aiInfos.find(*aiKey);

	if (inf == aiInfos.end())
		return;

	const CSkirmishAILibraryInfo& aiInfo = inf->second;
	const std::vector<Option>& options = aiInfo.GetOptions();

	id_ai_t::iterator ai = id_ai.find(skirmishAIId);

	if (ai == id_ai.end())
		return;

	SkirmishAIData& aiData = ai->second;

	for (auto oi = options.cbegin(); oi != options.cend(); ++oi) {
		if (oi->typeCode == opt_error)
			continue;
		if (oi->typeCode == opt_section)
			continue;

		if (aiData.options.find(oi->key) != aiData.options.end())
			continue;

		aiData.optionKeys.push_back(oi->key);
		aiData.options[oi->key] = option_getDefString(*oi);
	}
}

void CSkirmishAIHandler::CompleteSkirmishAI(const size_t skirmishAIId) {
	if (!gameInitialized)
		return;

	auto ai = id_ai.find(skirmishAIId);

	if (ai == id_ai.end())
		return;

	ai->second.isLuaAI = IsLuaAI(ai->second);

	if (!ai->second.isLuaAI) {
		CompleteWithDefaultOptionValues(skirmishAIId);
	}
}

