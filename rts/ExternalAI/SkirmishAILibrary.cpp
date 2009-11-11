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

#include "ExternalAI/IAILibraryManager.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include "ExternalAI/AIInterfaceKey.h"
#include "System/LogOutput.h"
#include "System/GlobalUnsynced.h"

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

LevelOfSupport CSkirmishAILibrary::GetLevelOfSupportFor(
			const std::string& engineVersionString, int engineVersionNumber,
		const AIInterfaceKey& interfaceKey) const {

	if (sSAI.getLevelOfSupportFor != NULL) {
		return sSAI.getLevelOfSupportFor(
				key.GetShortName().c_str(), key.GetVersion().c_str(),
				engineVersionString.c_str(), engineVersionNumber,
				interfaceKey.GetShortName().c_str(), interfaceKey.GetVersion().c_str()
				);
	} else {
		return LOS_Unknown;
	}
}

bool CSkirmishAILibrary::Init(int skirmishAIId, const SSkirmishAICallback* c_callback) const
{
	bool ok = true;

	if (sSAI.init != NULL) {
		const int error = sSAI.init(skirmishAIId, c_callback);
		ok = (error == 0);

		if (!ok) {
			// init failed
			const int teamId = skirmishAIHandler.GetSkirmishAI(skirmishAIId)->team;
			logOutput.Print("Failed to initialize an AI for team %d (ID: %i), error: %d", teamId, skirmishAIId, error);
			skirmishAIHandler.SetLocalSkirmishAIDieing(skirmishAIId, 5 /* = AI failed to init */);
		}
	}

	return ok;
}

bool CSkirmishAILibrary::Release(int skirmishAIId) const
{
	bool ok = true;

	if (sSAI.release != NULL) {
		int error = sSAI.release(skirmishAIId);
		ok = (error == 0);

		if (!ok) {
			// release failed
			const int teamId = skirmishAIHandler.GetSkirmishAI(skirmishAIId)->team;
			logOutput.Print("Failed to release an AI on team %d (ID: %i), error: %d", teamId, skirmishAIId, error);
		}
	}

	return ok;
}

int CSkirmishAILibrary::HandleEvent(int skirmishAIId, int topic, const void* data) const
{
	int ret = sSAI.handleEvent(skirmishAIId, topic, data);

	if (ret != 0) {
		// event handling failed!
		const int teamId = skirmishAIHandler.GetSkirmishAI(skirmishAIId)->team;
		logOutput.Print(
			"Warning: AI for team %i (ID: %i) failed handling event with topic %i, error: %i",
			teamId, skirmishAIId, topic, ret
		);
	}

	return ret;
}
