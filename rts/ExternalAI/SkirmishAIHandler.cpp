/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SkirmishAIHandler.h"

#include "ExternalAI/SkirmishAIKey.h"
#include "ExternalAI/AILibraryManager.h"
#include "ExternalAI/EngineOutHandler.h"
#include "ExternalAI/LuaAIImplHandler.h"
#include "ExternalAI/Interface/SSkirmishAILibrary.h"
#include "Game/GameSetup.h"
#include "Game/GlobalUnsynced.h"
#include "Net/Protocol/NetProtocol.h"
#include "System/Option.h"

#include "System/creg/STL_Map.h"
#include "System/creg/STL_Set.h"

#include <algorithm>
#include <cassert>

CR_BIND(CSkirmishAIHandler,)

CR_REG_METADATA(CSkirmishAIHandler, (
	CR_MEMBER(aiInstanceData),
	CR_MEMBER(localTeamAIs),
	CR_MEMBER(aiKillFlags),
	CR_MEMBER(aiLibraryKeys),

	CR_MEMBER(skirmishAIDataMap),
	CR_MEMBER(luaAIShortNames),

	CR_IGNORED(currentAIId),
	CR_IGNORED(numSkirmishAIs),

	CR_MEMBER(gameInitialized)
))


CSkirmishAIHandler skirmishAIHandler;


void CSkirmishAIHandler::ResetState()
{
	aiInstanceData.fill({});
	aiLibraryKeys.fill({});
	localTeamAIs.fill({});
	aiKillFlags.fill(-1);

	skirmishAIDataMap.clear();
	luaAIShortNames.clear();

	numSkirmishAIs = 0;
	currentAIId = MAX_AIS;

	gameInitialized = false;
}

void CSkirmishAIHandler::LoadFromSetup(const CGameSetup& setup) {
	const auto& aiDataCont = setup.GetAIStartingDataCont();

	for (const SkirmishAIData& aiData: aiDataCont) {
		AddSkirmishAI(aiData, &aiData - &aiDataCont[0]);
	}
}

void CSkirmishAIHandler::LoadPreGame() {

	if (gameInitialized)
		return;

	// extract all Lua AI implementation short names
	const CLuaAIImplHandler::InfoItemVector& luaAIImpls = luaAIImplHandler.LoadInfoItems();

	for (const auto& impl: luaAIImpls) {
		for (const auto& info: impl) {
			if (info.key != SKIRMISH_AI_PROPERTY_SHORT_NAME)
				continue;

			assert(info.valueType == INFO_VALUE_TYPE_STRING);
			luaAIShortNames.insert(info.valueTypeString);
		}
	}

	gameInitialized = true;

	// finalize already added SkirmishAIData's
	for (size_t i = 0; i < aiInstanceData.size(); i++) {
		CompleteSkirmishAI(i);
	}
}

bool CSkirmishAIHandler::IsActiveSkirmishAI(const size_t skirmishAIId) const {
	return IsValidSkirmishAI(aiInstanceData[skirmishAIId]);
}

SkirmishAIData* CSkirmishAIHandler::GetSkirmishAI(const size_t skirmishAIId) {
	if (!IsActiveSkirmishAI(skirmishAIId))
		return nullptr;

	return &aiInstanceData[skirmishAIId];
}

size_t CSkirmishAIHandler::GetSkirmishAI(const std::string& name) const
{
	const auto pred = [&name](const SkirmishAIData& aiData) { return (name == aiData.name); };
	const auto iter = std::find_if(aiInstanceData.begin(), aiInstanceData.end(), pred);

	if (iter == aiInstanceData.end())
		return -1;

	return (iter - aiInstanceData.begin());
}

std::vector<uint8_t> CSkirmishAIHandler::GetSkirmishAIsInTeam(const int teamId, const int hostPlayerId) const
{
	std::vector<uint8_t> ids;

	if (!skirmishAIDataMap.empty()) {
		ids.reserve(skirmishAIDataMap.size());

		for (const auto& p: skirmishAIDataMap) {
			const SkirmishAIData& aiData = *(p.second);

			if (aiData.team != teamId)
				continue;
			if ((hostPlayerId >= 0) && (aiData.hostPlayer != hostPlayerId))
				continue;

			ids.push_back(p.first);
		}

		std::sort(ids.begin(), ids.end());
	}

	return ids;
}

std::vector<uint8_t> CSkirmishAIHandler::GetSkirmishAIsByPlayer(const int hostPlayerId) const
{
	std::vector<uint8_t> skirmishAIs;

	for (const auto& p: skirmishAIDataMap) {
		const SkirmishAIData& aiData = *(p.second);

		if (aiData.hostPlayer != hostPlayerId)
			continue;

		skirmishAIs.push_back(p.first);
	}

	return skirmishAIs;
}


bool CSkirmishAIHandler::AddSkirmishAI(const SkirmishAIData& data, const size_t skirmishAIId) {
	// if the ID is already taken, something went very wrong
	if (IsValidSkirmishAI(aiInstanceData[skirmishAIId]))
		return false;

	assert(IsValidSkirmishAI(data));

	aiInstanceData[skirmishAIId] = data;
	localTeamAIs[data.team] = {};

	skirmishAIDataMap.emplace(skirmishAIId, &aiInstanceData[skirmishAIId]);
	CompleteSkirmishAI(skirmishAIId);

	numSkirmishAIs += 1;
	return true;
}

bool CSkirmishAIHandler::RemoveSkirmishAI(const size_t skirmishAIId) {
	if (!IsValidSkirmishAI(aiInstanceData[skirmishAIId]))
		return false;

	localTeamAIs[ aiInstanceData[skirmishAIId].team ] = {};

	aiKillFlags[skirmishAIId] = -1;
	aiLibraryKeys[skirmishAIId] = {};
	aiInstanceData[skirmishAIId] = {};

	skirmishAIDataMap.erase(skirmishAIId);

	numSkirmishAIs -= 1;
	return true;
}


void CSkirmishAIHandler::CreateLocalSkirmishAI(const size_t skirmishAIId) {
	SkirmishAIData* aiData = GetSkirmishAI(skirmishAIId);

	// fail, if a local AI is already in line for this team
	assert(!IsValidSkirmishAI(localTeamAIs[aiData->team]));
	// fail, if the specified AI is not a local one
	assert(IsLocalSkirmishAI(*aiData));

	localTeamAIs[aiData->team] = *aiData;
	localTeamAIs[aiData->team].isLuaAI = (aiData->isLuaAI = IsLuaAI(*aiData));

	// create instantly
	eoh->CreateSkirmishAI(skirmishAIId);
}

void CSkirmishAIHandler::CreateLocalSkirmishAI(const SkirmishAIData& aiData) {
	// fail if a local AI is already in line for this team
	assert(!IsValidSkirmishAI(localTeamAIs[aiData.team]));
	// fail, if the specified AI is not a local one
	assert(IsLocalSkirmishAI(aiData));

	localTeamAIs[aiData.team] = aiData;
	localTeamAIs[aiData.team].isLuaAI = IsLuaAI(aiData);

	// send to server, as the AI was not specified in the start script
	// (0 is bogus but will be ignored, server generates AI's real ID)
	clientNet->Send(CBaseNetProtocol::Get().SendAICreated(aiData.hostPlayer, 0, aiData.team, aiData.name));
}

const SkirmishAIData* CSkirmishAIHandler::GetLocalSkirmishAIInCreation(const int teamId) const {
	if (IsValidSkirmishAI(localTeamAIs[teamId]))
		return &localTeamAIs[teamId];
	return nullptr;
}

void CSkirmishAIHandler::SetLocalKillFlag(const size_t skirmishAIId, const int reason) {
	const SkirmishAIData& aiData = aiInstanceData[skirmishAIId];

	assert(IsValidSkirmishAI(aiData)); // is valid id?
	assert(IsLocalSkirmishAI(aiData)); // is local AI?

	aiKillFlags[skirmishAIId] = reason;

	if (!aiData.isLuaAI)
		eoh->BlockSkirmishAIEvents(skirmishAIId);

	clientNet->Send(CBaseNetProtocol::Get().SendAIStateChanged(gu->myPlayerNum, skirmishAIId, SKIRMAISTATE_DIEING));
}


const SkirmishAIKey* CSkirmishAIHandler::GetLocalSkirmishAILibraryKey(const size_t skirmishAIId) {
	// fail if the specified AI is not local
	assert(IsLocalSkirmishAI(skirmishAIId));

	const SkirmishAIKey& libKey = aiLibraryKeys[skirmishAIId];

	// already resolved
	if (!libKey.IsUnspecified())
		return &libKey;

	// resolve it
	const SkirmishAIData* aiData = GetSkirmishAI(skirmishAIId);
	const SkirmishAIKey& resKey = aiLibManager->ResolveSkirmishAIKey(SkirmishAIKey(aiData->shortName, aiData->version));

	assert(!resKey.IsUnspecified());

	aiLibraryKeys[skirmishAIId] = resKey;
	return &aiLibraryKeys[skirmishAIId];
}


bool CSkirmishAIHandler::IsLocalSkirmishAI(const size_t skirmishAIId) const {
	return (IsValidSkirmishAI(aiInstanceData[skirmishAIId]) && IsLocalSkirmishAI(aiInstanceData[skirmishAIId]));
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

	const AILibraryManager* aiLibMan = AILibraryManager::GetInstance();
	const AILibraryManager::T_skirmishAIInfos& aiInfos = aiLibMan->GetSkirmishAIInfos();
	const SkirmishAIKey* aiKey = GetLocalSkirmishAILibraryKey(skirmishAIId);

	if (aiKey == nullptr)
		return;

	const auto infoIt = aiInfos.find(*aiKey);

	if (infoIt == aiInfos.end())
		return;

	const CSkirmishAILibraryInfo& aiInfo = infoIt->second;
	const std::vector<Option>& options = aiInfo.GetOptions();

	SkirmishAIData& aiData = aiInstanceData[skirmishAIId];

	if (!IsValidSkirmishAI(aiData))
		return;

	for (const auto& option: options) {
		if (option.typeCode == opt_error)
			continue;
		if (option.typeCode == opt_section)
			continue;

		if (aiData.options.find(option.key) != aiData.options.end())
			continue;

		aiData.optionKeys.push_back(option.key);
		aiData.options[option.key] = option_getDefString(option);
	}
}

void CSkirmishAIHandler::CompleteSkirmishAI(const size_t skirmishAIId) {
	if (!gameInitialized)
		return;

	SkirmishAIData& aiData = aiInstanceData[skirmishAIId];

	if (!IsValidSkirmishAI(aiData))
		return;

	if ((aiData.isLuaAI = IsLuaAI(aiData)))
		return;

	CompleteWithDefaultOptionValues(skirmishAIId);
}

