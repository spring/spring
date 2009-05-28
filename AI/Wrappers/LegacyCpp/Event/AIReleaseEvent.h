/*
	Copyright 2008  Robin Vobruba <hoijui.quaero@gmail.com>

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

#ifndef _AIRELEASEEVENT_H
#define _AIRELEASEEVENT_H

#include "AIEvent.h"

class CAIReleaseEvent : public CAIEvent {
public:
	CAIReleaseEvent(const SReleaseEvent& event) : event(event) {}
	~CAIReleaseEvent() {}

	void Run(IGlobalAI& ai, IGlobalAICallback* globalAICallback = NULL) {
		ai.ReleaseAI();
	}

private:
	SReleaseEvent event;
};

#endif // _AIRELEASEEVENT_H
