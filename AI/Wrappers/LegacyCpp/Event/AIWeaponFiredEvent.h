/*
	Copyright (c) 2008 Robin Vobruba <hoijui.quaero@gmail.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

	@author Robin Vobruba <hoijui.quaero@gmail.com>
*/

#ifndef _AIWEAPONFIREDEVENT_H
#define	_AIWEAPONFIREDEVENT_H

#include "AIEvent.h"
#include "ExternalAI/IAICallback.h"

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

#endif // _AIWEAPONFIREDEVENT_H
