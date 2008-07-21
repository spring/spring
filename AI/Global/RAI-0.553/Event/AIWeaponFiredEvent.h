/*
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
*/

#ifndef _AIWEAPONFIREDEVENT_H
#define	_AIWEAPONFIREDEVENT_H

#include "ExternalAI/IGlobalAI.h"

class CAIWeaponFiredEvent : public CAIEvent {
public:
    CAIWeaponFiredEvent(SWeaponFiredEvent* event): event(*event) {}
    ~CAIWeaponFiredEvent() {}
    
    void run(CAI* ai) {
		int evtId = AI_EVENT_WEAPON_FIRED;
		//TODO: maybe: retrieve a WeaponDef that contains all attributes
		// as thisone contians only the correct id
		WeaponDef weaponDef;
		weaponDef.id = event.weaponDefId;
		IGlobalAI::WeaponFireEvent evt = {event.unitId, &weaponDef};
        ((CAIGlobalAI*) ai)->gai->HandleEvent(evtId, &evt);
    }
private:
    SWeaponFiredEvent event;
};

#endif	/* _AIWEAPONFIREDEVENT_H */

