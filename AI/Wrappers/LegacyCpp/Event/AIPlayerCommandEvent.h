/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _AI_PLAYER_COMMAND_EVENT_H
#define _AI_PLAYER_COMMAND_EVENT_H

#include "AIEvent.h"
#include "ExternalAI/Interface/AISCommands.h"
#include "../CommandQueue.h"


namespace springLegacyAI {

class CAIPlayerCommandEvent : public CAIEvent {
public:
	CAIPlayerCommandEvent(const SPlayerCommandEvent& event) : event(event) {}
	~CAIPlayerCommandEvent() {}

	void Run(IGlobalAI& ai, IGlobalAICallback* globalAICallback = NULL) {
		int evtId = AI_EVENT_PLAYER_COMMAND;

		std::vector<int> unitIds;

		for (int i = 0; i < event.unitIds_size; i++) {
			unitIds.push_back(event.unitIds[i]);
		}
		
		// this workaround is a bit ugly, but as only ray use this event anyway,
		// this should suffice
		const CCommandQueue* curCommands = (globalAICallback->GetAICallback())->GetCurrentUnitCommands(event.unitIds[0]);
		const Command& lastCommand = curCommands->front();
		// Command* c = (Command*) newCommand(event.commandData, event.commandTopicId);

		IGlobalAI::PlayerCommandEvent evt = {unitIds, lastCommand, event.playerId};
		ai.HandleEvent(evtId, &evt);
	}

private:
	SPlayerCommandEvent event;
};

} // namespace springLegacyAI

#endif // _AI_PLAYER_COMMAND_EVENT_H
