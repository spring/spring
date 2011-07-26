/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _AI_UNIT_CREATED_EVENT_H
#define	_AI_UNIT_CREATED_EVENT_H

#include "AIEvent.h"


namespace springLegacyAI {

class CAIUnitCreatedEvent : public CAIEvent {
public:
	CAIUnitCreatedEvent(const SUnitCreatedEvent& event) : event(event) {}
	~CAIUnitCreatedEvent() {}

	void Run(IGlobalAI& ai, IGlobalAICallback* globalAICallback = NULL) {
		ai.UnitCreated(event.unit, event.builder);
	}

private:
	SUnitCreatedEvent event;
};

} // namespace springLegacyAI

#endif // _AI_UNIT_CREATED_EVENT_H
