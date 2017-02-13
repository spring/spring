/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _AI_INIT_EVENT_H
#define _AI_INIT_EVENT_H

#include "AIEvent.h"
#include "../IGlobalAICallback.h"
#include "../AIGlobalAICallback.h"


namespace springLegacyAI {

class CAIInitEvent : public CAIEvent {
public:
	CAIInitEvent(const SInitEvent& event): event(event) {}

	#if 0
	void Run(IGlobalAI& ai, IGlobalAICallback* globalAICallback = NULL) {
		ai.InitAI(wrappedClb, wrappedClb->GetInnerCallback()->SkirmishAI_getTeamId(event.skirmishAIId));
	}
	#else
	// done in AIAI::handleEvent (cleaner)
	void Run(IGlobalAI& ai, IGlobalAICallback* globalAICallback = NULL) {}
	#endif

private:
	SInitEvent event;
};

}; // namespace springLegacyAI

#endif // _AI_INIT_EVENT_H
