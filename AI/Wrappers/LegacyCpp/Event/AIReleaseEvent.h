/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _AI_RELEASE_EVENT_H
#define _AI_RELEASE_EVENT_H

#include "AIEvent.h"


namespace springLegacyAI {

class CAIReleaseEvent : public CAIEvent {
public:
	CAIReleaseEvent(const SReleaseEvent& event) /* :event(event)*/ {}
	~CAIReleaseEvent() {}

	void Run(IGlobalAI& ai, IGlobalAICallback* globalAICallback = NULL) {
		ai.ReleaseAI();
	}

//private:
//	SReleaseEvent event;
};

} // namespace springLegacyAI

#endif // _AI_RELEASE_EVENT_H
