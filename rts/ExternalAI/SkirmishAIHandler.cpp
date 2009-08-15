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
{
}

CSkirmishAIHandler::~CSkirmishAIHandler()
{
}

void CSkirmishAIHandler::LoadFromSetup(const CGameSetup& setup) {

	for (size_t a = 0; a < setup.GetSkirmishAIs().size(); ++a) {
		AddSkirmishAI(setup.GetSkirmishAIs()[a]);
	}
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

std::set<SkirmishAIData*> CSkirmishAIHandler::GetSkirmishAIsInTeam(const int teamId)
{
	std::set<SkirmishAIData*> skirmishAIsInTeam;

	for (id_ai_t::iterator ai = id_ai.begin(); ai != id_ai.end(); ++ai) {
		if (ai->second.team == teamId) {
			skirmishAIsInTeam.insert(&(ai->second));
		}
	}

	return skirmishAIsInTeam;
}
