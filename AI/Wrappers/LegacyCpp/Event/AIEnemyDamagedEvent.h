/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _AI_ENEMY_DAMAGED_EVENT_H
#define _AI_ENEMY_DAMAGED_EVENT_H

#include "AIEvent.h"
#include "System/float3.h"


namespace springLegacyAI {

class CAIEnemyDamagedEvent : public CAIEvent {
public:
	CAIEnemyDamagedEvent(const SEnemyDamagedEvent& event) : event(event) {}
	~CAIEnemyDamagedEvent() {}

	void Run(IGlobalAI& ai, IGlobalAICallback* globalAICallback = NULL) {
		ai.EnemyDamaged(event.enemy, event.attacker, event.damage, float3(event.dir_posF3));
	}

private:
	SEnemyDamagedEvent event;
};

} // namespace springLegacyAI

#endif // _AI_ENEMY_DAMAGED_EVENT_H
