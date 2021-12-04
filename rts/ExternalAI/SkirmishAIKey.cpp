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
	const AIInterfaceKey& interface
)
	: shortName(shortName)
	, version(version)
	, interface(interface)
{}

SkirmishAIKey::SkirmishAIKey(
	const SkirmishAIKey& base,
	const AIInterfaceKey& interface
)
	: shortName(base.shortName)
	, version(base.version)
	, interface(interface)
{}

SkirmishAIKey::SkirmishAIKey(const SkirmishAIKey& toCopy) = default;

