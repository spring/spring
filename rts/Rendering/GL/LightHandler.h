/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GL_LIGHTHANDLER_H
#define _GL_LIGHTHANDLER_H

#include <array>

#include "Light.h"

namespace Shader {
	struct IProgramObject;
}

namespace GL {
	struct LightHandler {
	public:
		void Init(unsigned int cfgMaxLights);
		void Update();

		unsigned int AddLight(const GL::Light&);
		unsigned int SetLight(unsigned int lgtIndex, const GL::Light&);

		GL::Light* GetLight(unsigned int lgtHandle);

		const float* GetRawLightDataPtr(size_t i = 0) const { return &rawLights[i].worldPos.x; }
		      float* GetRawLightDataPtr(size_t i = 0)       { return &rawLights[i].worldPos.x; }

		static constexpr unsigned int MaxConfigLights()       { return MAX_LIGHTS; }
		                 unsigned int NumConfigLights() const { return  maxLights; }

		static constexpr unsigned int MaxUniformVecs()       { return (MaxConfigLights() * (sizeof(RawLight) / sizeof(float4))); }
		                 unsigned int NumUniformVecs() const { return (NumConfigLights() * (sizeof(RawLight) / sizeof(float4))); }

	private:
		static constexpr unsigned int MAX_LIGHTS = 32;

		struct RawLight {
			float4 worldPos;
			float4 worldDir;
			float4 diffColor;
			float4 specColor;
			float4 ambiColor;
			float4 fovRadius;
		};

		std::array<GL::Light, MAX_LIGHTS> glLights;
		std::array<RawLight, MAX_LIGHTS> rawLights;

		unsigned int maxLights = 0;
		unsigned int lightHandle = 0;
	};
}

#endif // _GL_LIGHTHANDLER_H
