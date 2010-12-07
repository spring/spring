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
#define _AIPLAYERCOMMANDEVENT_H

#include "AIEvent.h"
#include "ExternalAI/Interface/AISCommands.h"
#include "../CommandQueue.h"

class CAIPlayerCommandEvent : public CAIEvent {
public:
	CAIPlayerCommandEvent(const SPlayerCommandEvent& event) : event(event) {}
	~CAIPlayerCommandEvent() {}

	void Run(IGlobalAI& ai, IGlobalAICallback* globalAICallback = NULL) {
		int evtId = AI_EVENT_PLAYER_COMMAND;

		std::vector<int> unitIds;
		int i;
		for (i=0; i < event.unitIds_size; i++) {
			unitIds.push_back(event.unitIds[i]);
		}
		
		// this workaround is a bit ugly, but as only ray use this event anyway,
		// this should suffice
		const CCommandQueue* curCommands = globalAICallback->GetAICallback()
				->GetCurrentUnitCommands(event.unitIds[0]);
		const Command& lastCommand = curCommands->front();
		//Command* c = (Command*) newCommand(event.commandData,
		//		event.commandTopicId);

		IGlobalAI::PlayerCommandEvent evt = {unitIds, lastCommand, event.playerId};
		ai.HandleEvent(evtId, &evt);
		//delete c;
	}

private:
	SPlayerCommandEvent event;
};

#endif // _AIPLAYERCOMMANDEVENT_H
