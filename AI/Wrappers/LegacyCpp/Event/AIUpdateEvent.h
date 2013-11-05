/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _AI_UPDATE_EVENT_H
#define	_AI_UPDATE_EVENT_H

#include "AIEvent.h"


namespace springLegacyAI {

class CAIUpdateEvent : public CAIEvent {
public:
	CAIUpdateEvent(const SUpdateEvent& event) /*:event(event)*/ {}
	~CAIUpdateEvent() {}

	void Run(IGlobalAI& ai, IGlobalAICallback* globalAICallback = NULL) {
		ai.Update();
	}

//private:
//	SUpdateEvent event;
};

} // namespace springLegacyAI

#endif // _AI_UPDATE_EVENT_H
