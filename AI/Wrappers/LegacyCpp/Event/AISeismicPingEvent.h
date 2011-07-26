/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _AI_SEISMIC_PING_EVENT_H
#define _AI_SEISMIC_PING_EVENT_H

#include "AIEvent.h"
#include "System/float3.h"


namespace springLegacyAI {

class CAISeismicPingEvent : public CAIEvent {
public:
	CAISeismicPingEvent(const SSeismicPingEvent& event) : event(event) {}
	~CAISeismicPingEvent() {}

	void Run(IGlobalAI& ai, IGlobalAICallback* globalAICallback = NULL) {
		int evtId = AI_EVENT_SEISMIC_PING;
		IGlobalAI::SeismicPingEvent evt = {float3(event.pos_posF3), event.strength};
		ai.HandleEvent(evtId, &evt);
	}

private:
	SSeismicPingEvent event;
};

} // namespace springLegacyAI

#endif // _AI_SEISMIC_PING_EVENT_H
