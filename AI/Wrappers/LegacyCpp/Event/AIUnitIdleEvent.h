/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _AI_UNIT_IDLE_EVENT_H
#define	_AI_UNIT_IDLE_EVENT_H

#include "AIEvent.h"


namespace springLegacyAI {

class CAIUnitIdleEvent : public CAIEvent {
public:
	CAIUnitIdleEvent(const SUnitIdleEvent& event) : event(event) {}
	~CAIUnitIdleEvent() {}

	void Run(IGlobalAI& ai, IGlobalAICallback* globalAICallback = NULL) {
		ai.UnitIdle(event.unit);
	}

private:
	SUnitIdleEvent event;
};

} // namespace springLegacyAI

#endif // _AI_UNIT_IDLE_EVENT_H
