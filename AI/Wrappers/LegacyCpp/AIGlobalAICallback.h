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
*/

#ifndef _AI_GLOBAL_AI_CALLBACK_H
#define _AI_GLOBAL_AI_CALLBACK_H

#include "IGlobalAICallback.h"

struct SSkirmishAICallback;
class CAIAICallback;
class CAIAICheats;

/**
 * The AI side wrapper over the C AI interface for IGlobalAICallback.
 */
class CAIGlobalAICallback : public IGlobalAICallback {
public:
	CAIGlobalAICallback();
	CAIGlobalAICallback(const SSkirmishAICallback* sAICallback, int skirmishAIId);
	~CAIGlobalAICallback();

	virtual IAICheats* GetCheatInterface();
	virtual IAICallback* GetAICallback();

	const SSkirmishAICallback* GetInnerCallback() const;

private:
	const SSkirmishAICallback* sAICallback;
	int skirmishAIId;
	CAIAICallback* wrappedAICallback;
	CAIAICheats* wrappedAICheats;
};

#endif // _AI_GLOBAL_AI_CALLBACK_H
