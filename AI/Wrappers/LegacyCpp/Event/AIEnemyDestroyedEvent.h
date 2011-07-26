/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _AI_ENEMY_DESTROYED_EVENT_H
#define _AI_ENEMY_DESTROYED_EVENT_H

#include "AIEvent.h"


namespace springLegacyAI {

class CAIEnemyDestroyedEvent : public CAIEvent {
public:
	CAIEnemyDestroyedEvent(const SEnemyDestroyedEvent& event) : event(event) {}
	~CAIEnemyDestroyedEvent() {}

	void Run(IGlobalAI& ai, IGlobalAICallback* globalAICallback = NULL) {
		ai.EnemyDestroyed(event.enemy, event.attacker);
	}

private:
	SEnemyDestroyedEvent event;
};

} // namespace springLegacyAI

#endif // _AI_ENEMY_DESTROYED_EVENT_H
