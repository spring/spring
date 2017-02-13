/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "AIGlobalAICallback.h"
#include "AIAICallback.h"
#include "AIAICheats.h"

springLegacyAI::CAIGlobalAICallback::CAIGlobalAICallback():
	IGlobalAICallback(),
	sAICallback(nullptr),
	skirmishAIId(-1)
{
}

springLegacyAI::CAIGlobalAICallback::CAIGlobalAICallback(const SSkirmishAICallback* _sAICallback, int _skirmishAIId):
	IGlobalAICallback(),
	sAICallback(_sAICallback),
	skirmishAIId(_skirmishAIId)
{
}



springLegacyAI::IAICallback* springLegacyAI::CAIGlobalAICallback::GetAICallback() {
	if (wrappedAICallback.get() == nullptr) {
		wrappedAICallback.reset(new CAIAICallback(skirmishAIId, sAICallback));
	}

	return (wrappedAICallback.get());
}

springLegacyAI::IAICheats* springLegacyAI::CAIGlobalAICallback::GetCheatInterface() {
	if (wrappedAICheats.get() == nullptr) {
		// to initialize
		GetAICallback();

		wrappedAICheats.reset(new CAIAICheats(skirmishAIId, sAICallback, wrappedAICallback.get()));
	}

	return (wrappedAICheats.get());
}

