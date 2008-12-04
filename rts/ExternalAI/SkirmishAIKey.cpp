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

#include "SkirmishAIKey.h"

CR_BIND(SkirmishAIKey,)

CR_REG_METADATA(SkirmishAIKey, (
		CR_MEMBER(shortName),
		CR_MEMBER(version),
		CR_MEMBER(interface)
		));

SkirmishAIKey::SkirmishAIKey(
		const std::string& shortName,
		const std::string& version,
		const AIInterfaceKey& interface)
		: shortName(shortName),
		version(version),
		interface(interface) {}

SkirmishAIKey::SkirmishAIKey(const SkirmishAIKey& base,
		const AIInterfaceKey& interface)
		: shortName(base.shortName),
		version(base.version),
		interface(interface) {}

SkirmishAIKey::SkirmishAIKey(const SkirmishAIKey& toCopy)
		: shortName(toCopy.shortName),
		version(toCopy.version),
		interface(toCopy.interface) {}

SkirmishAIKey::~SkirmishAIKey() {}


bool SkirmishAIKey::isEqual(const SkirmishAIKey& otherKey) const {

	if (shortName == otherKey.shortName) {
		return version == otherKey.version;
	} else {
		return false;
	}
}
bool SkirmishAIKey::isLessThen(const SkirmishAIKey& otherKey) const {

	if (shortName == otherKey.shortName) {
		return version < otherKey.version;
	} else {
		return shortName < otherKey.shortName;
	}
}

const std::string& SkirmishAIKey::GetShortName() const {
	return shortName;
}
const std::string& SkirmishAIKey::GetVersion() const {
	return version;
}
const AIInterfaceKey& SkirmishAIKey::GetInterface() const {
	return interface;
}

bool SkirmishAIKey::IsUnspecified() const {
	return shortName == "";
}
bool SkirmishAIKey::IsFullySpecified() const {
	return shortName != "" || !interface.IsUnspecified();
}

bool SkirmishAIKey::operator==(const SkirmishAIKey& otherKey) const {
	return isEqual(otherKey);
}
bool SkirmishAIKey::operator!=(const SkirmishAIKey& otherKey) const {
	return !isEqual(otherKey);
}
bool SkirmishAIKey::operator<(const SkirmishAIKey& otherKey) const {
	return isLessThen(otherKey);
}
bool SkirmishAIKey::operator>(const SkirmishAIKey& otherKey) const {
	return !isLessThen(otherKey) && !isEqual(otherKey);
}
bool SkirmishAIKey::operator<=(const SkirmishAIKey& otherKey) const {
	return isLessThen(otherKey) || isEqual(otherKey);
}
bool SkirmishAIKey::operator>=(const SkirmishAIKey& otherKey) const {
	return !isLessThen(otherKey);
}
