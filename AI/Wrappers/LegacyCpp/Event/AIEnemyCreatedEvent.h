/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _AI_ENEMY_CREATED_EVENT_H
#define _AI_ENEMY_CREATED_EVENT_H

#include "AIEvent.h"


namespace springLegacyAI {

class CAIEnemyCreatedEvent : public CAIEvent {
public:
	CAIEnemyCreatedEvent(const SEnemyCreatedEvent& event) : event(event) {}
	~CAIEnemyCreatedEvent() {}

	void Run(IGlobalAI& ai, IGlobalAICallback* globalAICallback = NULL) {
		ai.EnemyCreated(event.enemy);
	}

private:
	SEnemyCreatedEvent event;
};

} // namespace springLegacyAI

#endif // _AI_ENEMY_CREATED_EVENT_H
