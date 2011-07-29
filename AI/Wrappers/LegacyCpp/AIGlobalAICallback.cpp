/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "AIGlobalAICallback.h"
#include "AIAICallback.h"
#include "AIAICheats.h"

springLegacyAI::CAIGlobalAICallback::CAIGlobalAICallback()
	: IGlobalAICallback(), sAICallback(NULL), skirmishAIId(-1),
		wrappedAICallback(NULL), wrappedAICheats(NULL) {}

springLegacyAI::CAIGlobalAICallback::CAIGlobalAICallback(const SSkirmishAICallback* sAICallback,
		int skirmishAIId) :
		IGlobalAICallback(), sAICallback(sAICallback), skirmishAIId(skirmishAIId),
		wrappedAICallback(NULL), wrappedAICheats(NULL) {}

springLegacyAI::CAIGlobalAICallback::~CAIGlobalAICallback() {

	delete wrappedAICallback;
	delete wrappedAICheats;
}


springLegacyAI::IAICallback* springLegacyAI::CAIGlobalAICallback::GetAICallback() {

	if (wrappedAICallback == NULL) {
		wrappedAICallback = new CAIAICallback(skirmishAIId, sAICallback);
	}

	return wrappedAICallback;
}

springLegacyAI::IAICheats* springLegacyAI::CAIGlobalAICallback::GetCheatInterface() {

	if (wrappedAICheats == NULL) {
		// to initialize
		this->GetAICallback();
		wrappedAICheats =
				new CAIAICheats(skirmishAIId, sAICallback, wrappedAICallback);
	}

	return wrappedAICheats;
}

const SSkirmishAICallback* springLegacyAI::CAIGlobalAICallback::GetInnerCallback() const {
	return sAICallback;
}
