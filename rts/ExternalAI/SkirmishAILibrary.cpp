/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SkirmishAILibrary.h"

#include "ExternalAI/IAILibraryManager.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include "ExternalAI/AIInterfaceKey.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/Log/ILog.h"

#include <string>


CSkirmishAILibrary::CSkirmishAILibrary(
	const SSkirmishAILibrary& ai,
	const SkirmishAIKey& key
):
	sSAI(ai),
	key(key)
{
	if (sSAI.handleEvent == nullptr) {
		LOG_L(L_ERROR,
			"Fetched AI library %s-%s has no handleEvent function"
			" available. It is therefore illegal and will not be used."
			" This usually indicates a problem in the used AI Interface"
			" library (%s-%s).",
			key.GetShortName().c_str(), key.GetVersion().c_str(),
			key.GetInterface().GetShortName().c_str(),
			key.GetInterface().GetVersion().c_str());
	}
}



LevelOfSupport CSkirmishAILibrary::GetLevelOfSupportFor(
	const std::string& engineVersionString,
	const int engineVersionNumber,
	const AIInterfaceKey& interfaceKey
) const {
	if (sSAI.getLevelOfSupportFor != nullptr) {
		const char* ksn = key.GetShortName().c_str();
		const char* kv  = key.GetVersion().c_str();
		const char* ev  = engineVersionString.c_str();
		const char* isn = interfaceKey.GetShortName().c_str();
		const char* iv  = interfaceKey.GetVersion().c_str();

		return sSAI.getLevelOfSupportFor(ksn, kv, ev, engineVersionNumber, isn, iv);
	}

	return LOS_Unknown;
}

bool CSkirmishAILibrary::Init(int skirmishAIId, const SSkirmishAICallback* c_callback) const
{
	if (sSAI.init == nullptr)
		return true;

	const int ret = sSAI.init(skirmishAIId, c_callback);

	if (ret == 0)
		return true;

	skirmishAIHandler.SetLocalSkirmishAIDieing(skirmishAIId, 5 /* = AI failed to init */);

	// init failed
	const int teamId = skirmishAIHandler.GetSkirmishAI(skirmishAIId)->team;
	const char* errorStr = "Failed to initialize an AI for team %d (ID: %d), error: %d";

	LOG_L(L_ERROR, errorStr, teamId, skirmishAIId, ret);
	return false;
}

bool CSkirmishAILibrary::Release(int skirmishAIId) const
{
	if (sSAI.release == nullptr)
		return true;

	const int ret = sSAI.release(skirmishAIId);

	if (ret == 0)
		return true;

	// release failed
	const int teamId = skirmishAIHandler.GetSkirmishAI(skirmishAIId)->team;
	const char* errorStr = "Failed to release an AI on team %d (ID: %i), error: %d";

	LOG_L(L_ERROR, errorStr, teamId, skirmishAIId, ret);
	return false;
}

int CSkirmishAILibrary::HandleEvent(int skirmishAIId, int topic, const void* data) const
{
	skirmishAIHandler.SetCurrentAIID(skirmishAIId);
	const int ret = sSAI.handleEvent(skirmishAIId, topic, data);
	skirmishAIHandler.SetCurrentAIID(MAX_AIS);

	if (ret == 0)
		return ret;

	// event handling failed!
	const int teamId = skirmishAIHandler.GetSkirmishAI(skirmishAIId)->team;
	const char* errorStr = "AI for team %d (ID: %d) failed handling event with topic %d, error: %d";

	LOG_L(L_WARNING, errorStr, teamId, skirmishAIId, topic, ret);

	return ret;
}

