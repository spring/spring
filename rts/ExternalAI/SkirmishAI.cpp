/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SkirmishAI.h"

#include "IAILibraryManager.h"
#include "SkirmishAILibrary.h"
#include "TimeProfiler.h"
#include "Util.h"

CSkirmishAI::CSkirmishAI(int teamId, const SkirmishAIKey& key,
		const SSkirmishAICallback* c_callback) :
		teamId(teamId),
		key(key),
		timerName("AI t:" + IntToString(teamId) + " " +
		          key.GetShortName() + " " + key.GetVersion()),
		dieing(false)
{
	SCOPED_TIMER(timerName.c_str());
	library = IAILibraryManager::GetInstance()->FetchSkirmishAILibrary(key);
	initOk = library->Init(teamId, c_callback);
}

CSkirmishAI::~CSkirmishAI() {

	SCOPED_TIMER(timerName.c_str());
	if (initOk) {
		library->Release(teamId);
	}
	IAILibraryManager::GetInstance()->ReleaseSkirmishAILibrary(key);
}

void CSkirmishAI::Dieing() {
	dieing = true;
}

int CSkirmishAI::HandleEvent(int topic, const void* data) const {
	SCOPED_TIMER(timerName.c_str());

	// the Release event must always make it through
	const int ret = library->HandleEvent(teamId, topic, data);

	if (!dieing) {
		return ret;
	} else {
		// signal OK to prevent error spam
		return 0;
	}
}
