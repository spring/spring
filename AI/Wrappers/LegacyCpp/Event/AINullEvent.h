/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _AI_NULL_EVENT_H
#define	_AI_NULL_EVENT_H

#include "AIEvent.h"


namespace springLegacyAI {

class CAINullEvent : public CAIEvent {
public:
	CAINullEvent() {}
	~CAINullEvent() {}

	void Run(IGlobalAI& ai, IGlobalAICallback* globalAICallback = NULL) {}
};

} // namespace springLegacyAI

#endif // _AI_NULL_EVENT_H
