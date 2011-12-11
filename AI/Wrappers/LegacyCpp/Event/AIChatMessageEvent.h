/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _AI_CHAT_MESSAGE_EVENT_H
#define	_AI_CHAT_MESSAGE_EVENT_H

#include "AIEvent.h"


namespace springLegacyAI {

class CAIChatMessageEvent : public CAIEvent {
public:
	CAIChatMessageEvent(const SMessageEvent& event) : event(event) {}
	~CAIChatMessageEvent() {}

	void Run(IGlobalAI& ai, IGlobalAICallback* globalAICallback = NULL) {
		ai.RecvChatMessage(event.message, event.player);
	}

private:
	SMessageEvent event;
};

} // namespace springLegacyAI

#endif // _AI_CHAT_MESSAGE_EVENT_H
