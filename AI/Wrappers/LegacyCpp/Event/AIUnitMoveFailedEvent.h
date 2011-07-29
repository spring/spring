/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _AI_UNIT_MOVE_FAILED_EVENT_H
#define	_AI_UNIT_MOVE_FAILED_EVENT_H

#include "AIEvent.h"


namespace springLegacyAI {

class CAIUnitMoveFailedEvent : public CAIEvent {
public:
	CAIUnitMoveFailedEvent(const SUnitMoveFailedEvent& event) : event(event) {}
	~CAIUnitMoveFailedEvent() {}

	void Run(IGlobalAI& ai, IGlobalAICallback* globalAICallback = NULL) {
		ai.UnitMoveFailed(event.unit);
	}

private:
	SUnitMoveFailedEvent event;
};

} // namespace springLegacyAI

#endif // _AI_UNIT_MOVE_FAILED_EVENT_H
