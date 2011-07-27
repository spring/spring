/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _AI_ENEMY_ENTER_RADAR_EVENT_H
#define _AI_ENEMY_ENTER_RADAR_EVENT_H

#include "AIEvent.h"


namespace springLegacyAI {

class CAIEnemyEnterRadarEvent : public CAIEvent {
public:
	CAIEnemyEnterRadarEvent(const SEnemyEnterRadarEvent& event)
			: event(event) {}
	~CAIEnemyEnterRadarEvent() {}

	void Run(IGlobalAI& ai, IGlobalAICallback* globalAICallback = NULL) {
		ai.EnemyEnterRadar(event.enemy);
	}

private:
	SEnemyEnterRadarEvent event;
};

} // namespace springLegacyAI

#endif // _AI_ENEMY_ENTER_RADAR_EVENT_H
