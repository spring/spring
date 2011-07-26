/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _AI_UNIT_DAMAGED_EVENT_H
#define _AI_UNIT_DAMAGED_EVENT_H

#include "AIEvent.h"
#include "System/float3.h"


namespace springLegacyAI {

class CAIUnitDamagedEvent : public CAIEvent {
public:
	CAIUnitDamagedEvent(const SUnitDamagedEvent& event) : event(event) {}
	~CAIUnitDamagedEvent() {}

	void Run(IGlobalAI& ai, IGlobalAICallback* globalAICallback = NULL) {
		ai.UnitDamaged(event.unit, event.attacker, event.damage, float3(event.dir_posF3));
	}

private:
	SUnitDamagedEvent event;
};

} // namespace springLegacyAI

#endif // _AI_UNIT_DAMAGED_EVENT_H
