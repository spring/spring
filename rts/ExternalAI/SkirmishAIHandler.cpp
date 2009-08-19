/*
	Copyright (c) 2008 Robin Vobruba <hoijui.quaero@gmail.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

	@author	Robin Vobruba <hoijui.quaero@gmail.com>
*/


#include "SkirmishAIHandler.h"

#include "Game/GameSetup.h"
#include "creg/STL_Map.h"
#include "System/NetProtocol.h"
#include "System/GlobalUnsynced.h"
#include "ExternalAI/SkirmishAIKey.h"
#include "ExternalAI/IAILibraryManager.h"
#include "ExternalAI/EngineOutHandler.h"

#include <assert.h>

CR_BIND(CSkirmishAIHandler,);

CR_REG_METADATA(CSkirmishAIHandler, (
//	CR_MEMBER(nextId),
	CR_MEMBER(id_ai),
	CR_MEMBER(id_dieReason),
	CR_RESERVED(64)
));


CSkirmishAIHandler* CSkirmishAIHandler::mySingleton = NULL;

CSkirmishAIHandler& CSkirmishAIHandler::GetInstance() {

	if (mySingleton == NULL) {
		mySingleton = new CSkirmishAIHandler();
	}

	return *mySingleton;
}

CSkirmishAIHandler::CSkirmishAIHandler()
//	nextId(0)
//	, usedSkirmishAIInfos_initialized(false)
{
}

CSkirmishAIHandler::~CSkirmishAIHandler()
{
}

void CSkirmishAIHandler::LoadFromSetup(const CGameSetup& setup) {

	for (size_t a = 0; a < setup.GetSkirmishAIs().size(); ++a) {
		AddSkirmishAI(setup.GetSkirmishAIs()[a], a);
		// TODO: complete the SkirmishAIData before adding
	}
}

bool CSkirmishAIHandler::IsActiveSkirmishAI(const size_t skirmishAIId) const {

	id_ai_t::const_iterator ai = id_ai.find(skirmishAIId);
	return (ai != id_ai.end());
}

SkirmishAIData* CSkirmishAIHandler::GetSkirmishAI(const size_t skirmishAIId) {

	id_ai_t::iterator ai = id_ai.find(skirmishAIId);
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

	for (id_ai_t::iterator ai = id_ai.begin(); ai != id_ai.end(); ++ai) {
		if ((ai->second.team == teamId) && ((hostPlayerId < 0) || (ai->second.hostPlayer == hostPlayerId))) {
			skirmishAIs.push_back(ai->first);
		}
	}

	return skirmishAIs;
}

CSkirmishAIHandler::ids_t CSkirmishAIHandler::GetSkirmishAIsByPlayer(const int hostPlayerId)
{
	ids_t skirmishAIs;

	for (id_ai_t::iterator ai = id_ai.begin(); ai != id_ai.end(); ++ai) {
		if (ai->second.hostPlayer == hostPlayerId) {
			skirmishAIs.push_back(ai->first);
		}
	}

	return skirmishAIs;
}

CSkirmishAIHandler::ids_t CSkirmishAIHandler::GetAllSkirmishAIs()
{
	ids_t skirmishAIs;

	for (id_ai_t::iterator ai = id_ai.begin(); ai != id_ai.end(); ++ai) {
		skirmishAIs.push_back(ai->first);
	}

	return skirmishAIs;
}

void CSkirmishAIHandler::AddSkirmishAI(const SkirmishAIData& data, const size_t skirmishAIId) {

	// if the ID is already taken, something went very wrong
	assert(id_ai.find(skirmishAIId) == id_ai.end());

	id_ai[skirmishAIId] = data;
	if (CSkirmishAIHandler::IsLocalSkirmishAI(data)) {
		SkirmishAIKey tmpKey(data.shortName, data.version);
		id_libKey[skirmishAIId] = aiLibManager->ResolveSkirmishAIKey(tmpKey);
		team_localAIsInCreation.erase(data.team);
	}
}
/*size_t CSkirmishAIHandler::AddSkirmishAI(const SkirmishAIData& data) {

	const size_t myId = nextId;
	id_ai[myId] = data;
	nextId++;

	return myId;
}*/

bool CSkirmishAIHandler::RemoveSkirmishAI(const size_t skirmishAIId) {

	team_localAIsInCreation.erase(id_ai[skirmishAIId].team);
	id_dieReason.erase(skirmishAIId);
	id_libKey.erase(skirmishAIId);
	return id_ai.erase(skirmishAIId);
}

size_t CSkirmishAIHandler::GetNumSkirmishAIs() const {
	return id_ai.size();
}

void CSkirmishAIHandler::CreateLocalSkirmishAI(const size_t skirmishAIId) {

	SkirmishAIData* aiData = GetSkirmishAI(skirmishAIId);

	// fail if a local AI is already in line for this team
	assert(team_localAIsInCreation.find(aiData->team) == team_localAIsInCreation.end());

	team_localAIsInCreation[aiData->team] = *aiData;

	// create instantly
	eoh->CreateSkirmishAI(skirmishAIId);
}
void CSkirmishAIHandler::CreateLocalSkirmishAI(const SkirmishAIData& aiData) {

	// fail if a local AI is already in line for this team
	assert(team_localAIsInCreation.find(aiData.team) == team_localAIsInCreation.end());

	team_localAIsInCreation[aiData.team] = aiData;

	// this will be ignored; the real one is generated by the server
	static const size_t unspecified_skirmishAIId = 0;
	// send to server, as the AI was not specified in the start script
	net->Send(CBaseNetProtocol::Get().SendAICreated(aiData.hostPlayer, unspecified_skirmishAIId, aiData.team, aiData.name));
}
const SkirmishAIData* CSkirmishAIHandler::GetLocalSkirmishAIInCreation(const int teamId) const {
	return (team_localAIsInCreation.find(teamId) != team_localAIsInCreation.end()) ? &(team_localAIsInCreation.find(teamId)->second) : NULL;
}

void CSkirmishAIHandler::SetLocalSkirmishAIDieing(const size_t skirmishAIId, const int reason) {

	id_ai_t::iterator ai = id_ai.find(skirmishAIId);
	assert(ai != id_ai.end()); // is valid id?
	assert(CSkirmishAIHandler::IsLocalSkirmishAI(ai->second)); // is local AI?

	id_dieReason[skirmishAIId] = reason;

	net->Send(CBaseNetProtocol::Get().SendAIStateChanged(gu->myPlayerNum, skirmishAIId, SKIRMAISTATE_DIEING));
}

int CSkirmishAIHandler::GetLocalSkirmishAIDieReason(const size_t skirmishAIId) const {
	return (id_dieReason.find(skirmishAIId) != id_dieReason.end()) ? id_dieReason.find(skirmishAIId)->second : -1;
}

bool CSkirmishAIHandler::IsLocalSkirmishAIDieing(const size_t skirmishAIId) const {
	return (id_dieReason.find(skirmishAIId) != id_dieReason.end());
}

const SkirmishAIKey* CSkirmishAIHandler::GetLocalSkirmishAILibraryKey(const size_t skirmishAIId) const {

	const SkirmishAIKey* key = NULL;

	id_libKey_t::const_iterator libKey = id_libKey.find(skirmishAIId);
	if (libKey != id_libKey.end()) {
		key = &(libKey->second);
	}

	return key;
}

bool CSkirmishAIHandler::IsLocalSkirmishAI(const size_t skirmishAIId) const {

	bool isLocal = false;

	id_ai_t::const_iterator ai = id_ai.find(skirmishAIId);
	if (ai != id_ai.end()) {
		 CSkirmishAIHandler::IsLocalSkirmishAI(ai->second);
	}

	return isLocal;
}

bool CSkirmishAIHandler::IsLocalSkirmishAI(const SkirmishAIData& aiData) {
	return (aiData.hostPlayer == gu->myPlayerNum);
}

/*
const std::vector<std::string> CSkirmishAIHandler::EMPTY_OPTION_VALUE_KEYS;
const std::vector<std::string>& CSkirmishAIHandler::GetSkirmishAIOptionValueKeys(int teamId) const {

	const SkirmishAIData* aiData = gameSetup->GetSkirmishAIDataForTeam(teamId);
	if (aiData != NULL) {
		return aiData->optionKeys;
	} else {
		return EMPTY_OPTION_VALUE_KEYS;
	}
}
const std::map<std::string, std::string> CSkirmishAIHandler::EMPTY_OPTION_VALUES;
const std::map<std::string, std::string>& CSkirmishAIHandler::GetSkirmishAIOptionValues(int teamId) const {

	const SkirmishAIData* aiData = gameSetup->GetSkirmishAIDataForTeam(teamId);
	if (aiData != NULL) {
		return aiData->options;
	} else {
		return EMPTY_OPTION_VALUES;
	}
}

const CSkirmishAIHandler::T_skirmishAIInfos& CAILibraryManager::GetUsedSkirmishAIInfos() {

	if (!usedSkirmishAIInfos_initialized) {
		const CTeam* team = NULL;
		for (unsigned int t = 0; t < (unsigned int)teamHandler->ActiveTeams(); ++t) {
			team = teamHandler->Team(t);
			if (team != NULL && team->isAI) {
				const std::string& t_sn = team->skirmishAIKey.GetShortName();
				const std::string& t_v = team->skirmishAIKey.GetVersion();

				IAILibraryManager::T_skirmishAIInfos::const_iterator aiInfo;
				for (aiInfo = skirmishAIInfos.begin(); aiInfo != skirmishAIInfos.end(); ++aiInfo) {
					const std::string& ai_sn = aiInfo->second->GetShortName();
					const std::string& ai_v = aiInfo->second->GetVersion();
					// add this AI info if it is used in the current game
					if (ai_sn == t_sn && (t_sn.empty() || ai_v == t_v)) {
						usedSkirmishAIInfos[aiInfo->first] = aiInfo->second;
					}
				}
			}
		}
	}

	return usedSkirmishAIInfos;
}
*/
