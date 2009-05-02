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

#ifndef _SKIRMISHAIKEY_H
#define _SKIRMISHAIKEY_H

// this is needed when compiling with MinGW,
// cause it has interface defined somewhere in its base files
// it is not needed on GCC eg.
#ifdef interface
#undef interface
#endif

#include <string>
#include "creg/creg_cond.h"
#include "AIInterfaceKey.h"

/**
 * Used to uniquely identify a Skirmish AI within the engine.
 */
class SkirmishAIKey {
	CR_DECLARE_STRUCT(SkirmishAIKey);

private:
	std::string shortName;
	std::string version;
	AIInterfaceKey interface;

	bool isEqual(const SkirmishAIKey& otherKey) const;
	bool isLessThen(const SkirmishAIKey& otherKey) const;

public:
	SkirmishAIKey(
			const std::string& shortName = "",
			const std::string& version = "",
			const AIInterfaceKey& interface = AIInterfaceKey());
	SkirmishAIKey(const SkirmishAIKey& base, const AIInterfaceKey& interface);
	SkirmishAIKey(const SkirmishAIKey& toCopy);
	~SkirmishAIKey();

	const std::string& GetShortName() const;
	const std::string& GetVersion() const;
	const AIInterfaceKey& GetInterface() const;

	bool IsUnspecified() const;
	bool IsFullySpecified() const;

	bool operator==(const SkirmishAIKey& otherKey) const;
	bool operator!=(const SkirmishAIKey& otherKey) const;
	bool operator<(const SkirmishAIKey& otherKey) const;
	bool operator>(const SkirmishAIKey& otherKey) const;
	bool operator<=(const SkirmishAIKey& otherKey) const;
	bool operator>=(const SkirmishAIKey& otherKey) const;
};

#endif // _SKIRMISHAIKEY_H
