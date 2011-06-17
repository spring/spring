/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _AI_ENEMY_FINISHED_EVENT_H
#define _AI_ENEMY_FINISHED_EVENT_H

#include "AIEvent.h"


namespace springLegacyAI {

class CAIEnemyFinishedEvent : public CAIEvent {
public:
	CAIEnemyFinishedEvent(const SEnemyFinishedEvent& event) : event(event) {}
	~CAIEnemyFinishedEvent() {}

	void Run(IGlobalAI& ai, IGlobalAICallback* globalAICallback = NULL) {
		ai.EnemyFinished(event.enemy);
	}

private:
	SEnemyFinishedEvent event;
};

} // namespace springLegacyAI

#endif // _AI_ENEMY_FINISHED_EVENT_H
