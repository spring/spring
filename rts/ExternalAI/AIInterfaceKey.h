/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef AI_INTERFACE_KEY_H
#define AI_INTERFACE_KEY_H

#include "System/creg/creg_cond.h"
#include <string>


/**
 * Used to uniquely identify an AI Interface within the engine.
 */
class AIInterfaceKey {
	CR_DECLARE_STRUCT(AIInterfaceKey)

public:
	AIInterfaceKey(
			const std::string& shortName = "",
			const std::string& version = "");
	AIInterfaceKey(const AIInterfaceKey& toCopy);
	~AIInterfaceKey();

	const std::string& GetShortName() const;
	const std::string& GetVersion() const;

	bool IsUnspecified() const;
	std::string ToString() const;

	bool operator==(const AIInterfaceKey& otherKey) const;
	bool operator!=(const AIInterfaceKey& otherKey) const;
	bool operator<(const AIInterfaceKey& otherKey) const;
	bool operator>(const AIInterfaceKey& otherKey) const;
	bool operator<=(const AIInterfaceKey& otherKey) const;
	bool operator>=(const AIInterfaceKey& otherKey) const;

private:
	bool isEqual(const AIInterfaceKey& otherKey) const;
	bool isLessThen(const AIInterfaceKey& otherKey) const;

	std::string shortName;
	std::string version;
};

#endif // AI_INTERFACE_KEY_H
