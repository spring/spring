/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SkirmishAI.h"

#include "Interface/AISEvents.h"
#include "IAILibraryManager.h"
#include "SkirmishAILibrary.h"
#include "TimeProfiler.h"
#include "Util.h"

CSkirmishAI::CSkirmishAI(int skirmishAIId, int teamId, const SkirmishAIKey& key,
		const SSkirmishAICallback* callback) :
		skirmishAIId(skirmishAIId),
		key(key),
		callback(callback),
		timerName("AI t:" + IntToString(teamId) +
		          " id:" + IntToString(skirmishAIId) +
		          " " + key.GetShortName() + " " + key.GetVersion()),
		initOk(false),
		dieing(false)
{
	SCOPED_TIMER(timerName.c_str());
	library = IAILibraryManager::GetInstance()->FetchSkirmishAILibrary(key);
}

CSkirmishAI::~CSkirmishAI() {

	SCOPED_TIMER(timerName.c_str());
	if (initOk) {
		library->Release(skirmishAIId);
	}
	IAILibraryManager::GetInstance()->ReleaseSkirmishAILibrary(key);
}

void CSkirmishAI::Init() {

	if (callback != NULL) {
		initOk  = library->Init(skirmishAIId, callback);
		callback = NULL;
	}
}

void CSkirmishAI::Dieing() {
	dieing = true;
}

int CSkirmishAI::HandleEvent(int topic, const void* data) const {

	SCOPED_TIMER(timerName.c_str());
	if (!dieing || (topic == EVENT_RELEASE)) {
		return library->HandleEvent(skirmishAIId, topic, data);
	} else {
		// to prevent log error spam, signal: OK
		return 0;
	}
}
