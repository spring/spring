/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _AI_UNIT_DESTROYED_EVENT_H
#define	_AI_UNIT_DESTROYED_EVENT_H

#include "AIEvent.h"


namespace springLegacyAI {

class CAIUnitDestroyedEvent : public CAIEvent {
public:
	CAIUnitDestroyedEvent(const SUnitDestroyedEvent& event) : event(event) {}
	~CAIUnitDestroyedEvent() {}

	void Run(IGlobalAI& ai, IGlobalAICallback* globalAICallback = NULL) {
		ai.UnitDestroyed(event.unit, event.attacker);
	}

private:
	SUnitDestroyedEvent event;
};

} // namespace springLegacyAI

#endif // _AI_UNIT_DESTROYED_EVENT_H
