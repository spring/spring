/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _AI_UNIT_CAPTURED_EVENT_H
#define	_AI_UNIT_CAPTURED_EVENT_H

#include "AIEvent.h"


namespace springLegacyAI {

class CAIUnitCapturedEvent : public CAIEvent {
public:
	CAIUnitCapturedEvent(const SUnitCapturedEvent& event) : event(event) {}
	~CAIUnitCapturedEvent() {}

	void Run(IGlobalAI& ai, IGlobalAICallback* globalAICallback = NULL) {
		int evtId = AI_EVENT_UNITCAPTURED;
		IGlobalAI::ChangeTeamEvent evt = {event.unitId, event.newTeamId,
				event.oldTeamId};
		ai.HandleEvent(evtId, &evt);
	}

private:
	SUnitCapturedEvent event;
};

} // namespace springLegacyAI

#endif // _AI_UNIT_CAPTURED_EVENT_H
