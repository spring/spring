/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GL_LIGHTHANDLER_H
#define _GL_LIGHTHANDLER_H

#include <list>
#include <map>
#include "Light.h"

namespace Shader {
	struct IProgramObject;
}

namespace GL {
	struct LightHandler {
	public:
		LightHandler(): baseLight(0), maxLights(0), numLights(0), lightHandle(0) {}
		~LightHandler() { Kill(); }

		void Init(unsigned int, unsigned int);
		void Kill() { lights.clear(); }
		void Update(Shader::IProgramObject*);

		unsigned int AddLight(const GL::Light&);
		GL::Light* GetLight(unsigned int);

		unsigned int GetBaseLight() const { return baseLight; }
		unsigned int GetMaxLights() const { return maxLights; }

	private:
		std::map<unsigned int, GL::Light> lights;
		std::list<unsigned int> lightIDs;

		// sum of intensity weights for all active lights
		float3 lightIntensityWeight;

		unsigned int baseLight;
		unsigned int maxLights;
		unsigned int numLights;
		unsigned int lightHandle;
	};
}

#endif // _GL_LIGHTHANDLER_H
