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

	void Run(IGlobalAI& ai, IGlobalAICallback* globalAICallback = nullptr) {
		int evtId = AI_EVENT_WEAPON_FIRED;

		WeaponDef* weaponDef = nullptr;
		WeaponDef weaponDefTmp;

		if (globalAICallback != nullptr) {
			AIHCGetWeaponDefById fetchCmd = {event.weaponDefId, weaponDef};

			// NOTE: CAIAICallback::HandleCommand does nothing with this event atm
			if ((globalAICallback->GetAICallback())->HandleCommand(AIHCGetWeaponDefByIdId, &fetchCmd) != 1) {
				return;
			}
		}

		weaponDef = &weaponDefTmp;
		weaponDef->id = event.weaponDefId;

		// let the AI itself handle it only if the callback did not
		IGlobalAI::WeaponFireEvent evt = {event.unitId, weaponDef};
		ai.HandleEvent(evtId, &evt);
	}

private:
	SWeaponFiredEvent event;
};

} // namespace springLegacyAI

#endif // _AI_WEAPON_FIRED_EVENT_H
