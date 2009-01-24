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

#ifndef _ISKIRMISHAILIBRARY_H
#define _ISKIRMISHAILIBRARY_H

#include "Interface/ELevelOfSupport.h"
#include <string>
#include <vector>
#include <map>

class SkirmishAIKey;
class AIInterfaceKey;

/**
 * The interface for a Skirmish AI library.
 * @see ISkirmishAI
 */
class ISkirmishAILibrary {
public:
	virtual ~ISkirmishAILibrary() {}

	virtual SkirmishAIKey GetKey() const = 0;
	/**
	 * Level of Support for a specific engine version and AI interface.
	 * @return see enum LevelOfSupport (higher values could be used optionally)
	 */
	virtual LevelOfSupport GetLevelOfSupportFor(int teamId,
			const std::string& engineVersionString, int engineVersionNumber,
			const AIInterfaceKey& interfaceKey) const = 0;

    virtual void Init(int teamId) const = 0;
    virtual void Release(int teamId) const = 0;
    virtual int HandleEvent(int teamId, int topic, const void* data) const = 0;
};

#endif // _ISKIRMISHAILIBRARY_H
