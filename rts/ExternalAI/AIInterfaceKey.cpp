/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

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
