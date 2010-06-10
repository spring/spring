/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SkirmishAI.h"

#include "Interface/AISEvents.h"
#include "IAILibraryManager.h"
#include "SkirmishAILibrary.h"
#include "SkirmishAIHandler.h"
#include "TimeProfiler.h"
#include "Util.h"

CSkirmishAI::CSkirmishAI(int teamId, int skirmishAIId, const SkirmishAIKey& key,
		const SSkirmishAICallback* c_callback) :
		teamId(teamId),
		key(key),
		timerName("AI t:" + IntToString(teamId) + " " +
		          key.GetShortName() + " " + key.GetVersion()),
		initOk(false),
		dieing(false)
{
	SCOPED_TIMER(timerName.c_str());
	library = IAILibraryManager::GetInstance()->FetchSkirmishAILibrary(key);
	if (library != NULL) {
		initOk = library->Init(teamId, c_callback);
	} else {
		dieing = true;
		skirmishAIHandler.SetLocalSkirmishAIDieing(skirmishAIId, 5);
	}
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
	if (!dieing || (topic == EVENT_RELEASE)) {
		return library->HandleEvent(teamId, topic, data);
	} else {
		// to prevent log error spam, signal: OK
		return 0;
	}
}
