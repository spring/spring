/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _AI_UNIT_FINISHED_EVENT_H
#define	_AI_UNIT_FINISHED_EVENT_H

#include "AIEvent.h"


namespace springLegacyAI {

class CAIUnitFinishedEvent : public CAIEvent {
public:
	CAIUnitFinishedEvent(const SUnitFinishedEvent& event) : event(event) {}
	~CAIUnitFinishedEvent() {}

	void Run(IGlobalAI& ai, IGlobalAICallback* globalAICallback = NULL) {
		ai.UnitFinished(event.unit);
	}

private:
	SUnitFinishedEvent event;
};

} // namespace springLegacyAI

#endif // _AI_UNIT_FINISHED_EVENT_H
