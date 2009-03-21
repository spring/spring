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

#include "LogOutput.h"
#include "IAILibraryManager.h"
#include "EngineOutHandler.h"
#include "AIInterfaceKey.h"
#include <string>

CSkirmishAILibrary::CSkirmishAILibrary(const SSkirmishAILibrary& ai,
		const SkirmishAIKey& key)
		: sSAI(ai), key(key) {

	if (sSAI.handleEvent == NULL) {
		logOutput.Print(
				"ERROR: Fetched AI library %s-%s has no handleEvent function"
				"available. It is therefore illegal and will not be used."
				"This usually indicates a problem in the used AI Interface"
				"library (%s-%s).",
				key.GetShortName().c_str(), key.GetVersion().c_str(),
				key.GetInterface().GetShortName().c_str(),
				key.GetInterface().GetVersion().c_str());
	}
}

CSkirmishAILibrary::~CSkirmishAILibrary() {}

SkirmishAIKey CSkirmishAILibrary::GetKey() const {
	return key;
}

LevelOfSupport CSkirmishAILibrary::GetLevelOfSupportFor(int teamId,
			const std::string& engineVersionString, int engineVersionNumber,
		const AIInterfaceKey& interfaceKey) const {

	if (sSAI.getLevelOfSupportFor != NULL) {
		return sSAI.getLevelOfSupportFor(teamId, engineVersionString.c_str(),
				engineVersionNumber, interfaceKey.GetShortName().c_str(),
				interfaceKey.GetVersion().c_str());
	} else {
		return LOS_Unknown;
	}
}

void CSkirmishAILibrary::Init(int teamId) const {

	if (sSAI.init != NULL) {
		const SSkirmishAICallback* c_callback = eoh->GetSkirmishAICallback(teamId);
		int error = sSAI.init(teamId, c_callback);
		if (error != 0) {
			// init failed
			logOutput.Print("Failed to initialize an AI for team %d, error: %d",
					teamId, error);
		}
	}
}

void CSkirmishAILibrary::Release(int teamId) const {

	if (sSAI.release != NULL) {
		int error = sSAI.release(teamId);
		if (error != 0) {
			// release failed
			logOutput.Print("Failed to release the AI for team %d, error: %d",
					teamId, error);
		}
	}
}

int CSkirmishAILibrary::HandleEvent(int teamId, int topic, const void* data)
		const {

	int ret = sSAI.handleEvent(teamId, topic, data);

	if (ret != 0) {
		// event handling failed!
		logOutput.Print(
				"Warning: AI for team %i failed handling event with topic %i"
				", error: %i",
				teamId, topic, ret);
	}

	return ret;
}
