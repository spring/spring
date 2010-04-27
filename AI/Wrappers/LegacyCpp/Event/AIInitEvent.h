/*
	Copyright 2008  Nicolas Wu

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

	@author Nicolas Wu
	@author Robin Vobruba <hoijui.quaero@gmail.com>
*/

#ifndef _AIINITEVENT_H
#define _AIINITEVENT_H

#include "AIEvent.h"
#include "../IGlobalAICallback.h"
#include "../AIGlobalAICallback.h"

class CAIInitEvent : public CAIEvent {
public:
	CAIInitEvent(const SInitEvent& event)
		: event(event)
		, wrappedClb(new CAIGlobalAICallback(event.callback, event.skirmishAIId))
		{}

	~CAIInitEvent() {
		// do not delete wrappedClb here.
		// it should be deleted in AIAI.cpp
	}

	void Run(IGlobalAI& ai, IGlobalAICallback* globalAICallback = NULL) {

		const int teamId = wrappedClb->GetInnerCallback()->SkirmishAI_getTeamId(event.skirmishAIId);
		ai.InitAI(wrappedClb, teamId);
	}

	IGlobalAICallback* GetWrappedGlobalAICallback() {
		return wrappedClb;
	}

private:
	SInitEvent event;
	CAIGlobalAICallback* wrappedClb;
};

#endif // _AIINITEVENT_H
