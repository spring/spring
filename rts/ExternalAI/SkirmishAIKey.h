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
	~SkirmishAIKey();

	const std::string& GetShortName() const;
	const std::string& GetVersion() const;
	const AIInterfaceKey& GetInterface() const;

	bool IsUnspecified() const;
	bool IsFullySpecified() const;
	std::string ToString() const;

	bool operator==(const SkirmishAIKey& otherKey) const;
	bool operator!=(const SkirmishAIKey& otherKey) const;
	bool operator<(const SkirmishAIKey& otherKey) const;
	bool operator>(const SkirmishAIKey& otherKey) const;
	bool operator<=(const SkirmishAIKey& otherKey) const;
	bool operator>=(const SkirmishAIKey& otherKey) const;

private:
	bool isEqual(const SkirmishAIKey& otherKey) const;
	bool isLessThen(const SkirmishAIKey& otherKey) const;

	std::string shortName;
	std::string version;
	AIInterfaceKey interface;
};

#endif // SKIRMISH_AI_KEY_H
