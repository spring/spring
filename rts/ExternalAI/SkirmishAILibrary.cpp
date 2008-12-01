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
*/

#include "SkirmishAILibrary.h"

//#include "Interface/SAIInterfaceLibrary.h"
#include "LogOutput.h"
#include "IAILibraryManager.h"
#include <string>

CSkirmishAILibrary::CSkirmishAILibrary(const SSAILibrary& ai,
		const SSAIKey& key)
		: sSAI(ai), key(key) {}

CSkirmishAILibrary::~CSkirmishAILibrary() {}

SSAISpecifier CSkirmishAILibrary::GetSpecifier() const {
	return key.ai;
}
SSAIKey CSkirmishAILibrary::GetKey() const {
	return key;
}

LevelOfSupport CSkirmishAILibrary::GetLevelOfSupportFor(int teamId,
			const std::string& engineVersionString, int engineVersionNumber,
		const SAIInterfaceSpecifier& interfaceSpecifier) const {

	if (sSAI.getLevelOfSupportFor != NULL) {
		return sSAI.getLevelOfSupportFor(teamId, engineVersionString.c_str(),
				engineVersionNumber, interfaceSpecifier.shortName,
				interfaceSpecifier.version);
	} else {
		return LOS_Unknown;
	}
}

void CSkirmishAILibrary::Init(int teamId) const {

	if (sSAI.init != NULL) {
		const IAILibraryManager* libMan = IAILibraryManager::GetInstance();
		const CSkirmishAILibraryInfo* skiInf = libMan->GetSkirmishAIInfos().find(key)->second;

		unsigned int infSize = skiInf->GetInfo().size();
		const char** infKeys = skiInf->GetCInfoKeys();
		const char** infValues = skiInf->GetCInfoValues();

		unsigned int optSize = libMan->GetSkirmishAICOptionSize(teamId);
		const char** optKeys = libMan->GetSkirmishAICOptionKeys(teamId);
		const char** optValues = libMan->GetSkirmishAICOptionValues(teamId);

		int error = sSAI.init(teamId,
				infSize, infKeys, infValues,
				optSize, optKeys, optValues);
		if (error != 0) {
			// init failed
			logOutput.Print("Failed to initialize an AI for team %d, error: %d", teamId, error);
		}
	}
}

void CSkirmishAILibrary::Release(int teamId) const {

	if (sSAI.release != NULL) {
		int error = sSAI.release(teamId);
		if (error != 0) {
			// release failed
			logOutput.Print("Failed to release the AI for team %d, error: %d", teamId, error);
		}
	}
}

int CSkirmishAILibrary::HandleEvent(int teamId, int topic, const void* data) const {
	return sSAI.handleEvent(teamId, topic, data);
}
