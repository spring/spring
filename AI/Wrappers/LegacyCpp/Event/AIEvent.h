/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _AI_EVENT_H
#define _AI_EVENT_H

#include "../IGlobalAI.h"
#include "../IGlobalAICallback.h"


namespace springLegacyAI {

class CAIEvent {
public:

	virtual void Run(IGlobalAI& ai,
			IGlobalAICallback* globalAICallback = NULL) = 0;
	virtual ~CAIEvent(){}
};

} // namespace springLegacyAI

#endif // _AI_EVENT_H
