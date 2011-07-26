/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _AI_AI_H
#define _AI_AI_H

namespace springLegacyAI {

class IGlobalAI;
class IGlobalAICallback;

class CAIAI {
public:
	CAIAI(IGlobalAI* gAI);
	virtual ~CAIAI();

	/**
	 * Through this function, the AI receives events from the engine.
	 * For details about events that may arrive here, see file AISEvents.h.
	 *
	 * @param	topic	unique identifyer of a message
	 *					(see EVENT_* defines in AISEvents.h)
	 * @param	data	an topic specific struct, which contains the data
	 *					associatedwith the event
	 *					(see S*Event structs in AISEvents.h)
	 * @return	ok: 0, error: != 0
	 */
	virtual int handleEvent(int topic, const void* data);

protected:
	IGlobalAI* ai;
	IGlobalAICallback* globalAICallback;
};

} // namespace springLegacyAI

#endif // _AI_AI_H
