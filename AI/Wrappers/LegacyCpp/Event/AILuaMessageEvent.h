/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _AI_LUA_MESSAGE_EVENT_H
#define	_AI_LUA_MESSAGE_EVENT_H

#include "AIEvent.h"


namespace springLegacyAI {

class CAILuaMessageEvent : public CAIEvent {
public:
	CAILuaMessageEvent(const SLuaMessageEvent& event) : event(event) {}
	~CAILuaMessageEvent() {}

	void Run(IGlobalAI& ai, IGlobalAICallback* globalAICallback = NULL) {
		ai.RecvLuaMessage(event.inData, NULL /*event.outData*/);
	}

private:
	SLuaMessageEvent event;
};

} // namespace springLegacyAI

#endif // _AI_CHAT_MESSAGE_EVENT_H
