/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

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
