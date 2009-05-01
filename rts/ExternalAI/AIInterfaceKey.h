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

#ifndef _AIINTERFACEKEY_H
#define _AIINTERFACEKEY_H

#include <string>
#include "creg/creg_cond.h"

/**
 * Used to uniquely identify an AI Interface within the engine.
 */
class AIInterfaceKey {
	CR_DECLARE_STRUCT(AIInterfaceKey);

private:
	std::string shortName;
	std::string version;

	bool isEqual(const AIInterfaceKey& otherKey) const;
	bool isLessThen(const AIInterfaceKey& otherKey) const;

public:
	AIInterfaceKey(
			const std::string& shortName = "",
			const std::string& version = "");
	AIInterfaceKey(const AIInterfaceKey& toCopy);
	~AIInterfaceKey();

	const std::string& GetShortName() const;
	const std::string& GetVersion() const;

	bool IsUnspecified() const;

	bool operator==(const AIInterfaceKey& otherKey) const;
	bool operator!=(const AIInterfaceKey& otherKey) const;
	bool operator<(const AIInterfaceKey& otherKey) const;
	bool operator>(const AIInterfaceKey& otherKey) const;
	bool operator<=(const AIInterfaceKey& otherKey) const;
	bool operator>=(const AIInterfaceKey& otherKey) const;
};

#endif // _AIINTERFACEKEY_H
