/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Rendering/Shaders/ShaderStates.h"
#include "System/Sync/HsiehHash.h"
#include "System/UnorderedSet.hpp"


static spring::unordered_set<int> samplerTypes{
#ifndef HEADLESS
	GL_SAMPLER_1D,
	GL_SAMPLER_2D,
	GL_SAMPLER_3D,
	GL_SAMPLER_CUBE,
	GL_SAMPLER_1D_SHADOW,
	GL_SAMPLER_2D_SHADOW,
	GL_SAMPLER_1D_ARRAY,
	GL_SAMPLER_2D_ARRAY,
	GL_SAMPLER_1D_ARRAY_SHADOW,
	GL_SAMPLER_2D_ARRAY_SHADOW,
	GL_SAMPLER_2D_MULTISAMPLE,
	GL_SAMPLER_2D_MULTISAMPLE_ARRAY,
	GL_SAMPLER_CUBE_SHADOW,
	GL_SAMPLER_BUFFER,
	GL_SAMPLER_2D_RECT,
	GL_SAMPLER_2D_RECT_SHADOW,

	GL_INT_SAMPLER_1D,
	GL_INT_SAMPLER_2D,
	GL_INT_SAMPLER_3D,
	GL_INT_SAMPLER_CUBE,
	GL_INT_SAMPLER_1D_ARRAY,
	GL_INT_SAMPLER_2D_ARRAY,
	GL_INT_SAMPLER_2D_MULTISAMPLE,
	GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY,
	GL_INT_SAMPLER_BUFFER,
	GL_INT_SAMPLER_2D_RECT,

	GL_UNSIGNED_INT_SAMPLER_1D,
	GL_UNSIGNED_INT_SAMPLER_2D,
	GL_UNSIGNED_INT_SAMPLER_3D,
	GL_UNSIGNED_INT_SAMPLER_CUBE,
	GL_UNSIGNED_INT_SAMPLER_1D_ARRAY,
	GL_UNSIGNED_INT_SAMPLER_2D_ARRAY,
	GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE,
	GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY,
	GL_UNSIGNED_INT_SAMPLER_BUFFER,
	GL_UNSIGNED_INT_SAMPLER_2D_RECT,
#endif
};


namespace Shader {
	int UniformState::Hash(const int v0, const int v1, const int v2, const int v3) const
	{
		int hash = ~0;
		hash += v0 ^ (hash * 33);
		hash += v1 ^ (hash * 33);
		hash += v2 ^ (hash * 33);
		hash += v3 ^ (hash * 33);
		return hash;
	}


	int UniformState::Hash(const int* v, int count) const
	{
		int hash = ~0;
		for (int n = 0; n < count; ++n) {
			hash += v[n] ^ (hash * 33);
		}
		return hash;
	}


	bool UniformState::IsLocationValid() const
	{
	#ifdef HEADLESS
		// our stub headers are outdated and are missing GL_INVALID_INDEX
		return false;
	#else
		return (location != GL_INVALID_INDEX);
	#endif
	}


#ifdef DEBUG
	void UniformState::AssertType(int type) const
	{
		int utype = this->type;
		if (samplerTypes.find(utype) != samplerTypes.end())
			utype = GL_INT;
		assert(type == utype || utype == -1);
	}
#endif


	unsigned int ShaderFlags::CalcHash() const {
		unsigned int hash = 997;

		for (const auto& p: bitFlags) {
			hash = HsiehHash(p.first, (p.second).first, hash);
			hash = HsiehHash(reinterpret_cast<const uint8_t*>(&((p.second).second)), sizeof((p.second).second), hash);
		}
		for (const auto& p: intFlags) {
			hash = HsiehHash(p.first, (p.second).first, hash);
			hash = HsiehHash(reinterpret_cast<const uint8_t*>(&((p.second).second)), sizeof((p.second).second), hash);
		}
		for (const auto& p: fltFlags) {
			hash = HsiehHash(p.first, (p.second).first, hash);
			hash = HsiehHash(reinterpret_cast<const uint8_t*>(&((p.second).second)), sizeof((p.second).second), hash);
		}

		assert(hash != 0);
		return hash;
	}
}

