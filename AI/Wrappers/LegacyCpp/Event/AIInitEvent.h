/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _AI_INIT_EVENT_H
#define _AI_INIT_EVENT_H

#include "AIEvent.h"
#include "../IGlobalAICallback.h"
#include "../AIGlobalAICallback.h"


namespace springLegacyAI {

class CAIInitEvent : public CAIEvent {
public:
	CAIInitEvent(const SInitEvent& event)
		: event(event)
		, wrappedClb(new CAIGlobalAICallback(event.callback, event.skirmishAIId))
		{}

	~CAIInitEvent() {
		// do not delete wrappedClb here.
		// it should be deleted in AIAI.cpp
	}

	void Run(IGlobalAI& ai, IGlobalAICallback* globalAICallback = NULL) {

		const int teamId = wrappedClb->GetInnerCallback()->SkirmishAI_getTeamId(event.skirmishAIId);
		ai.InitAI(wrappedClb, teamId);
	}

	IGlobalAICallback* GetWrappedGlobalAICallback() {
		return wrappedClb;
	}

private:
	SInitEvent event;
	CAIGlobalAICallback* wrappedClb;
};

} // namespace springLegacyAI

#endif // _AI_INIT_EVENT_H
