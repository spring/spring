/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _AI_UNIT_GIVEN_EVENT_H
#define	_AI_UNIT_GIVEN_EVENT_H

#include "AIEvent.h"


namespace springLegacyAI {

class CAIUnitGivenEvent : public CAIEvent {
public:
	CAIUnitGivenEvent(const SUnitGivenEvent& event) : event(event) {}
	~CAIUnitGivenEvent() {}

	void Run(IGlobalAI& ai, IGlobalAICallback* globalAICallback = NULL) {
		int evtId = AI_EVENT_UNITGIVEN;
		IGlobalAI::ChangeTeamEvent evt = {event.unitId, event.newTeamId,
				event.oldTeamId};
		ai.HandleEvent(evtId, &evt);
	}

private:
	SUnitGivenEvent event;
};

} // namespace springLegacyAI

#endif // _AI_UNIT_GIVEN_EVENT_H
