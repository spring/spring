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

#include <assert.h>

CR_BIND(CSkirmishAIHandler,);

CR_REG_METADATA(CSkirmishAIHandler, (
	CR_MEMBER(nextId),
	CR_MEMBER(id_ai),
	CR_RESERVED(64)
));


CSkirmishAIHandler* CSkirmishAIHandler::mySingleton = NULL;

CSkirmishAIHandler& CSkirmishAIHandler::GetInstance() {

	if (mySingleton == NULL) {
		mySingleton = new CSkirmishAIHandler();
	}

	return *mySingleton;
}

CSkirmishAIHandler::CSkirmishAIHandler():
	nextId(0)
	//, usedSkirmishAIInfos_initialized(false)
{
}

CSkirmishAIHandler::~CSkirmishAIHandler()
{
}

void CSkirmishAIHandler::LoadFromSetup(const CGameSetup& setup) {

	/*for (size_t a = 0; a < setup.GetSkirmishAIs().size(); ++a) {
		AddSkirmishAI(setup.GetSkirmishAIs()[a]);
		// TODO: complete the SkirmishAIData before adding
	}*/
}

SkirmishAIData* CSkirmishAIHandler::GetSkirmishAI(const size_t skirmishAiId) {

	id_ai_t::iterator ai = id_ai.find(skirmishAiId);
	assert(ai != id_ai.end());

	return &(ai->second);
}

size_t CSkirmishAIHandler::GetSkirmishAI(const std::string& name) const
{
	size_t skirmishAiId = 0;

	bool found = false;
	for (id_ai_t::const_iterator ai = id_ai.begin(); ai != id_ai.end() && !found; ++ai) {
		if (ai->second.name == name) {
			skirmishAiId = ai->first;
			found = true;
		}
	}
	assert(found);

	return skirmishAiId;
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

size_t CSkirmishAIHandler::AddSkirmishAI(const SkirmishAIData& data) {

	const size_t myId = nextId;
	id_ai[myId] = data;
	nextId++;

	return myId;
}

bool CSkirmishAIHandler::RemoveSkirmishAI(const size_t skirmishAiId) {
	return id_ai.erase(skirmishAiId);
}

size_t CSkirmishAIHandler::GetNumSkirmishAIs() const {
	return id_ai.size();
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
