/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

//#include <boost/functional/hash.hpp>
#include "Rendering/Shaders/ShaderStates.h"
#include "Rendering/GL/myGL.h"
#include "System/Sync/HsiehHash.h"


namespace Shader {
	bool UniformState::IsLocationValid() const
	{
	#ifdef HEADLESS
		// our stub headers are outdated and are missing GL_INVALID_INDEX
		return false;
	#else
		return (location != GL_INVALID_INDEX);
	#endif
	}


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
