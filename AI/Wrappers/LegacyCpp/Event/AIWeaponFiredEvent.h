/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _AI_WEAPON_FIRED_EVENT_H
#define	_AI_WEAPON_FIRED_EVENT_H

#include "AIEvent.h"
#include "../IAICallback.h"


namespace springLegacyAI {

class CAIWeaponFiredEvent : public CAIEvent {
public:
	CAIWeaponFiredEvent(const SWeaponFiredEvent& event) : event(event) {}
	~CAIWeaponFiredEvent() {}

	void Run(IGlobalAI& ai, IGlobalAICallback* globalAICallback = NULL) {
		int evtId = AI_EVENT_WEAPON_FIRED;

		WeaponDef* weaponDef = NULL;
		if (globalAICallback) {
			AIHCGetWeaponDefById fetchCmd = {event.weaponDefId, weaponDef};
			int ret = globalAICallback->GetAICallback()
					->HandleCommand(AIHCGetWeaponDefByIdId, &fetchCmd);
			if (ret != 1) {
				weaponDef = NULL;
			}
		}
		if (weaponDef == NULL) {
			weaponDef = new WeaponDef();
			weaponDef->id = event.weaponDefId;
		}

		IGlobalAI::WeaponFireEvent evt = {event.unitId, weaponDef};
		ai.HandleEvent(evtId, &evt);

		delete weaponDef;
		weaponDef = NULL;
	}

private:
	SWeaponFiredEvent event;
};

} // namespace springLegacyAI

#endif // _AI_WEAPON_FIRED_EVENT_H
