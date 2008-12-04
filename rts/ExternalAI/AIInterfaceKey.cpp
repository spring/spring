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

#include "AIInterfaceKey.h"

CR_BIND(AIInterfaceKey, );

CR_REG_METADATA(AIInterfaceKey, (
		CR_MEMBER(shortName),
		CR_MEMBER(version)
		));

AIInterfaceKey::AIInterfaceKey(
		const std::string& shortName,
		const std::string& version)
		: shortName(shortName),
		version(version) {}


AIInterfaceKey::AIInterfaceKey(const AIInterfaceKey& toCopy)
		: shortName(toCopy.shortName),
		version(toCopy.version) {}

AIInterfaceKey::~AIInterfaceKey() {}

bool AIInterfaceKey::isEqual(const AIInterfaceKey& otherKey) const {

	if (shortName == otherKey.shortName) {
		return version == otherKey.version;
	} else {
		return false;
	}
}
bool AIInterfaceKey::isLessThen(const AIInterfaceKey& otherKey) const {

	if (shortName == otherKey.shortName) {
		return version < otherKey.version;
	} else {
		return shortName < otherKey.shortName;
	}
}

const std::string& AIInterfaceKey::GetShortName() const {
	return shortName;
}
const std::string& AIInterfaceKey::GetVersion() const {
	return version;
}

bool AIInterfaceKey::IsUnspecified() const {
	return shortName == "";
}

bool AIInterfaceKey::operator==(const AIInterfaceKey& otherKey) const {
	return isEqual(otherKey);
}
bool AIInterfaceKey::operator!=(const AIInterfaceKey& otherKey) const {
	return !isEqual(otherKey);
}
bool AIInterfaceKey::operator<(const AIInterfaceKey& otherKey) const {
	return isLessThen(otherKey);
}
bool AIInterfaceKey::operator>(const AIInterfaceKey& otherKey) const {
	return !isLessThen(otherKey) && !isEqual(otherKey);
}
bool AIInterfaceKey::operator<=(const AIInterfaceKey& otherKey) const {
	return isLessThen(otherKey) || isEqual(otherKey);
}
bool AIInterfaceKey::operator>=(const AIInterfaceKey& otherKey) const {
	return !isLessThen(otherKey);
}
