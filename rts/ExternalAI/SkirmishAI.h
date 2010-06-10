/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SKIRMISHAI_H
#define _SKIRMISHAI_H

#include "SkirmishAIKey.h"

#include <string>

class CSkirmishAILibrary;
struct SSkirmishAICallback;

/**
 * The most basic representation of a Skirmish AI instance from the engines POV.
 * A Skirmish AI instance can be seen as an instance of the AI,
 * that is dedicated to control a specific team (eg team 1)).
 * @see CSkirmishAILibrary
 */
class CSkirmishAI {
public:
	CSkirmishAI(int teamId, int skirmishAIId,
		const SkirmishAIKey& skirmishAIKey,
		const SSkirmishAICallback* c_callback);
	~CSkirmishAI();

	/**
	 * CAUTION: takes C AI Interface events, not engine C++ ones!
	 */
	int HandleEvent(int topic, const void* data) const;

	/**
	 * No events are forwarded to the Skirmish AI plugin
	 * after this method has been called.
	 * Do not call this if you want to kill a local AI, but use
	 * the Skirmish AI Handler instead.
	 * @see CSkirmishAIHandler::SetLocalSkirmishAIDieing()
	 */
	void Dieing();

private:
	int teamId;
	const SkirmishAIKey key;
	const CSkirmishAILibrary* library;
	const std::string timerName;
	bool initOk;
	bool dieing;
};

#endif // _SKIRMISHAI_H
