/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SKIRMISH_AI_KEY_H
#define SKIRMISH_AI_KEY_H

// this is needed when compiling with MinGW,
// cause it has interface defined somewhere in its base files
// it is not needed on GCC eg.
#ifdef interface
#undef interface
#endif

#include "AIInterfaceKey.h"

#include "System/creg/creg_cond.h"
#include <string>


/**
 * Used to uniquely identify a Skirmish AI within the engine.
 */
class SkirmishAIKey {
	CR_DECLARE_STRUCT(SkirmishAIKey)

public:
	SkirmishAIKey(
			const std::string& shortName = "",
			const std::string& version = "",
			const AIInterfaceKey& interface = AIInterfaceKey());
	SkirmishAIKey(const SkirmishAIKey& base, const AIInterfaceKey& interface);
	SkirmishAIKey(const SkirmishAIKey& toCopy);

	const std::string& GetShortName() const { return shortName; }
	const std::string& GetVersion() const { return version; }
	const AIInterfaceKey& GetInterface() const { return interface; }

	bool IsUnspecified() const { return (shortName.empty()); }
	bool IsFullySpecified() const { return (!shortName.empty() || !interface.IsUnspecified()); }
	std::string ToString() const { return (GetShortName() + " " + GetVersion()); }

	bool operator==(const SkirmishAIKey& otherKey) const { return (isEqual(otherKey)); }
	bool operator!=(const SkirmishAIKey& otherKey) const { return (!isEqual(otherKey)); }
	bool operator<(const SkirmishAIKey& otherKey) const { return (isLessThen(otherKey)); }
	bool operator>(const SkirmishAIKey& otherKey) const { return (!isLessThen(otherKey) && !isEqual(otherKey)); }
	bool operator<=(const SkirmishAIKey& otherKey) const { return (isLessThen(otherKey) || isEqual(otherKey)); }
	bool operator>=(const SkirmishAIKey& otherKey) const { return (!isLessThen(otherKey)); }

private:
	bool isEqual(const SkirmishAIKey& otherKey) const { return (shortName == otherKey.shortName && version == otherKey.version); }
	bool isLessThen(const SkirmishAIKey& otherKey) const {
		if (shortName == otherKey.shortName)
			return (version < otherKey.version);

		return (shortName < otherKey.shortName);
	}

	std::string shortName;
	std::string version;
	AIInterfaceKey interface;
};

#endif // SKIRMISH_AI_KEY_H
