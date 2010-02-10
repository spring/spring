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

#ifndef _SKIRMISH_AI_LIBRARY_H
#define _SKIRMISH_AI_LIBRARY_H

#include "SkirmishAIKey.h"
#include "Interface/SSkirmishAILibrary.h"
#include <vector>

class AIInterfaceKey;
struct SSkirmishAICallback;

/**
 * The engines container for a Skirmish AI library.
 * An instance of this class may represent RAI or KAIK.
 * @see CSkirmishAI
 */
class CSkirmishAILibrary {
public:
	CSkirmishAILibrary(const SSkirmishAILibrary& ai, const SkirmishAIKey& key);
	~CSkirmishAILibrary();

	SkirmishAIKey GetKey() const;

	/**
	 * Level of Support for a specific engine version and ai interface.
	 * @return see enum LevelOfSupport (higher values could be used optionally)
	 */
	LevelOfSupport GetLevelOfSupportFor(int teamId,
			const std::string& engineVersionString, int engineVersionNumber,
			const AIInterfaceKey& interfaceKey) const;

	bool Init(int teamId, const SSkirmishAICallback* c_callback) const;
	bool Release(int teamId) const;
	int HandleEvent(int teamId, int topic, const void* data) const;

private:
	SSkirmishAILibrary sSAI;
	SkirmishAIKey key;
};

#endif // _SKIRMISH_AI_LIBRARY_H
