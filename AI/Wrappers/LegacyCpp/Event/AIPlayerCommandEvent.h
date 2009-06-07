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

#ifndef _AIPLAYERCOMMANDEVENT_H
#define	_AIPLAYERCOMMANDEVENT_H

#include "AIEvent.h"
#include "ExternalAI/Interface/AISCommands.h"

class CAIPlayerCommandEvent : public CAIEvent {
public:
	CAIPlayerCommandEvent(const SPlayerCommandEvent& event) : event(event) {}
	~CAIPlayerCommandEvent() {}

	void Run(IGlobalAI& ai, IGlobalAICallback* globalAICallback = NULL) {
		int evtId = AI_EVENT_PLAYER_COMMAND;
		std::vector<int> unitIds;
		int i;
		for (i=0; i < event.numUnitIds; i++) {
			unitIds.push_back(event.unitIds[i]);
		}
		Command* c = (Command*) newCommand(event.commandData,
				event.commandTopic);
		IGlobalAI::PlayerCommandEvent evt = {unitIds, *c, event.playerId};
		ai.HandleEvent(evtId, &evt);
		delete c;
	}

private:
	SPlayerCommandEvent event;
};

#endif // _AIPLAYERCOMMANDEVENT_H
