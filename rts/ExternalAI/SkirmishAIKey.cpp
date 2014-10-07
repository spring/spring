/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SkirmishAIKey.h"

#include "System/creg/creg_cond.h"
#include <string>


CR_BIND(SkirmishAIKey, )

CR_REG_METADATA(SkirmishAIKey, (
		CR_MEMBER(shortName),
		CR_MEMBER(version),
		CR_MEMBER(interface)
		))

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

std::string SkirmishAIKey::ToString() const {
	return GetShortName() + " " + GetVersion();
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
