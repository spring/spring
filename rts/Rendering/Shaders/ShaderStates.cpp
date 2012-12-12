/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

//#include <boost/functional/hash.hpp>
#include "Rendering/Shaders/ShaderStates.h"
#include "System/Sync/HsiehHash.h"


namespace Shader {
	int SShaderFlagState::GetHash()
	{
		if (updates != lastUpdates) {
			lastUpdates = updates;
			const std::string defs = GetString();
			lastHash = HsiehHash(&defs[0], defs.length(), 0);
		}
		return lastHash;
	}
}
