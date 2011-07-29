/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _AI_ENEMY_LEAVE_RADAR_EVENT_H
#define _AI_ENEMY_LEAVE_RADAR_EVENT_H

#include "AIEvent.h"


namespace springLegacyAI {

class CAIEnemyLeaveRadarEvent : public CAIEvent {
public:
	CAIEnemyLeaveRadarEvent(const SEnemyLeaveRadarEvent& event)
			: event(event) {}
	~CAIEnemyLeaveRadarEvent() {}

	void Run(IGlobalAI& ai, IGlobalAICallback* globalAICallback = NULL) {
		ai.EnemyLeaveRadar(event.enemy);
	}

private:
	SEnemyLeaveRadarEvent event;
};

} // namespace springLegacyAI

#endif // _AI_ENEMY_LEAVE_RADAR_EVENT_H
