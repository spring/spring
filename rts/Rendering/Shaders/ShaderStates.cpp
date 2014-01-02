/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

//#include <boost/functional/hash.hpp>
#include "Rendering/Shaders/ShaderStates.h"
#include "System/Sync/HsiehHash.h"


namespace Shader {
	unsigned int SShaderFlagState::GetHash()
	{
		if (updates != lastUpdates) {
			const std::string defs = GetString();

			lastUpdates = updates;
			lastHash = HsiehHash(&defs[0], defs.length(), 0);
		}
		return lastHash;
	}
}
