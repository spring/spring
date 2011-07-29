/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _AI_ENEMY_ENTER_LOS_EVENT_H
#define _AI_ENEMY_ENTER_LOS_EVENT_H

#include "AIEvent.h"


namespace springLegacyAI {

class CAIEnemyEnterLOSEvent : public CAIEvent {
public:
	CAIEnemyEnterLOSEvent(const SEnemyEnterLOSEvent& event) : event(event) {}
	~CAIEnemyEnterLOSEvent() {}

	void Run(IGlobalAI& ai, IGlobalAICallback* globalAICallback = NULL) {
		ai.EnemyEnterLOS(event.enemy);
	}

private:
	SEnemyEnterLOSEvent event;
};

} // namespace springLegacyAI

#endif // _AI_ENEMY_ENTER_LOS_EVENT_H
