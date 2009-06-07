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

#ifndef _AIUNITGIVENEVENT_H
#define	_AIUNITGIVENEVENT_H

#include "AIEvent.h"

class CAIUnitGivenEvent : public CAIEvent {
public:
	CAIUnitGivenEvent(const SUnitGivenEvent& event) : event(event) {}
	~CAIUnitGivenEvent() {}

	void Run(IGlobalAI& ai, IGlobalAICallback* globalAICallback = NULL) {
		int evtId = AI_EVENT_UNITGIVEN;
		IGlobalAI::ChangeTeamEvent evt = {event.unitId, event.newTeamId,
				event.oldTeamId};
		ai.HandleEvent(evtId, &evt);
	}

private:
	SUnitGivenEvent event;
};

#endif // _AIUNITGIVENEVENT_H
