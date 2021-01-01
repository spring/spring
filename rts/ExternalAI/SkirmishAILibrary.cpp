/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SkirmishAILibrary.h"

#include "ExternalAI/SkirmishAIHandler.h"
#include "ExternalAI/AIInterfaceKey.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/Log/ILog.h"

#include <string>


CSkirmishAILibrary::CSkirmishAILibrary(
	const SSkirmishAILibrary& ai,
	const SkirmishAIKey& key
):
	aiLib(ai),
	aiKey(key)
{
	if (aiLib.handleEvent != nullptr)
		return;

	const char* fmt = "AI-library %s-%s (using interface %s-%s) has no handleEvent function";
	const char* ksn = (aiKey.GetShortName()).c_str();
	const char* kv  = (aiKey.GetVersion()).c_str();
	const char* isn = (aiKey.GetInterface().GetShortName()).c_str();
	const char* iv  = (aiKey.GetInterface().GetVersion()).c_str();

	LOG_L(L_ERROR, fmt, ksn, kv, isn, iv);
}



LevelOfSupport CSkirmishAILibrary::GetLevelOfSupportFor(
	const std::string& engineVersionString,
	const int engineVersionNumber,
	const AIInterfaceKey& interfaceKey
) const {
	if (aiLib.getLevelOfSupportFor != nullptr) {
		const char* ksn = aiKey.GetShortName().c_str();
		const char* kv  = aiKey.GetVersion().c_str();
		const char* ev  = engineVersionString.c_str();
		const char* isn = interfaceKey.GetShortName().c_str();
		const char* iv  = interfaceKey.GetVersion().c_str();

		return aiLib.getLevelOfSupportFor(ksn, kv, ev, engineVersionNumber, isn, iv);
	}

	return LOS_Unknown;
}

bool CSkirmishAILibrary::Init(int skirmishAIId, const SSkirmishAICallback* c_callback) const
{
	if (aiLib.init == nullptr)
		return true;

	const int ret = aiLib.init(skirmishAIId, c_callback);

	if (ret == 0)
		return true;

	// init failed
	const int teamId = skirmishAIHandler.GetSkirmishAI(skirmishAIId)->team;
	const char* errorStr = "Failed to initialize an AI for team %d (ID: %d), error: %d";

	LOG_L(L_ERROR, errorStr, teamId, skirmishAIId, ret);
	return false;
}

bool CSkirmishAILibrary::Release(int skirmishAIId) const
{
	if (aiLib.release == nullptr)
		return true;

	const int ret = aiLib.release(skirmishAIId);

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
	const int ret = aiLib.handleEvent(skirmishAIId, topic, data);
	skirmishAIHandler.SetCurrentAIID(MAX_AIS);

	if (ret == 0)
		return ret;

	// event handling failed!
	const int teamId = skirmishAIHandler.GetSkirmishAI(skirmishAIId)->team;
	const char* errorStr = "AI for team %d (ID: %d) failed handling event with topic %d, error: %d";

	LOG_L(L_WARNING, errorStr, teamId, skirmishAIId, topic, ret);

	return ret;
}

