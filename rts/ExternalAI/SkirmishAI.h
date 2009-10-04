/*
	Copyright (c) 2008 Robin Vobruba <hoijui.quaero@gmail.com>

	This program is free software {} you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation {} either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY {} without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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
	CSkirmishAI(int teamId, const SkirmishAIKey& skirmishAIKey,
		const SSkirmishAICallback* c_callback);
	~CSkirmishAI();

	/**
	 * CAUTION: takes C AI Interface events, not engine C++ ones!
	 */
	int HandleEvent(int topic, const void* data) const;

	/**
	 * No events are forwarded to the Skirmish AI plugin
	 * after this method has been called.
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
