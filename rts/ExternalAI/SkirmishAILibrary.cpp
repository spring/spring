/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

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
