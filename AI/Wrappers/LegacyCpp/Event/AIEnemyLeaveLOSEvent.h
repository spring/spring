/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _AI_ENEMY_LEAVE_LOS_EVENT_H
#define _AI_ENEMY_LEAVE_LOS_EVENT_H

#include "AIEvent.h"


namespace springLegacyAI {

class CAIEnemyLeaveLOSEvent : public CAIEvent {
public:
	CAIEnemyLeaveLOSEvent(const SEnemyLeaveLOSEvent& event) : event(event) {}
	~CAIEnemyLeaveLOSEvent() {}

	void Run(IGlobalAI& ai, IGlobalAICallback* globalAICallback = NULL) {
		ai.EnemyLeaveLOS(event.enemy);
	}

private:
	SEnemyLeaveLOSEvent event;
};

} // namespace springLegacyAI

#endif // _AI_ENEMY_LEAVE_LOS_EVENT_H
