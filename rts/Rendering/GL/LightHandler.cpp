/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "myGL.h"
#include "LightHandler.h"
#include "Game/GlobalUnsynced.h"
#include "Rendering/Shaders/Shader.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/LosHandler.h"

static const float4 ZeroVector4 = float4(0.0f, 0.0f, 0.0f, 0.0f);

void GL::LightHandler::Init(unsigned int cfgBaseLight, unsigned int cfgMaxLights) {
	glGetIntegerv(GL_MAX_LIGHTS, reinterpret_cast<int*>(&maxLights));

	baseLight = cfgBaseLight;
	maxLights -= baseLight;
	maxLights = std::min(maxLights, cfgMaxLights);

	lights.resize(maxLights);

	for (unsigned int i = 0; i < maxLights; i++) {
		const unsigned int lightID = GL_LIGHT0 + baseLight + i;

		glEnable(lightID);
		glLightfv(lightID, GL_POSITION, &ZeroVector4.x);
		glLightfv(lightID, GL_AMBIENT,  &ZeroVector4.x);
		glLightfv(lightID, GL_DIFFUSE,  &ZeroVector4.x);
		glLightfv(lightID, GL_SPECULAR, &ZeroVector4.x);
		glLightfv(lightID, GL_SPOT_DIRECTION, &ZeroVector4.x);
		glLightf(lightID, GL_SPOT_CUTOFF, 180.0f);
		glLightf(lightID, GL_CONSTANT_ATTENUATION,  1.0f);
		glLightf(lightID, GL_LINEAR_ATTENUATION,    0.0f);
		glLightf(lightID, GL_QUADRATIC_ATTENUATION, 0.0f);
		glDisable(lightID);

		lights[i].SetID(lightID);
	}
}


unsigned int GL::LightHandler::AddLight(const GL::Light& light) {
	if (light.GetTTL() == 0 || light.GetRadius() <= 0.0f)
		return -1u;
	if ((light.GetIntensityWeight()).SqLength() <= 0.01f)
		return -1u;

	const auto it = std::find_if(lights.begin(), lights.end(), [&](const GL::Light& lgt) { return (lgt.GetTTL() == 0); });

	if (it == lights.end()) {
		// all are claimed; find the lowest-priority light we can evict
		unsigned int minPriorityValue = light.GetPriority();
		unsigned int minPriorityIndex = -1u;

		for (unsigned int n = 0; n < lights.size(); n++) {
			const GL::Light& lgt = lights[n];

			if (lgt.GetPriority() < minPriorityValue) {
				minPriorityValue = lgt.GetPriority();
				minPriorityIndex = n;
			}
		}

		if (minPriorityIndex != -1u)
			return (SetLight(minPriorityIndex, light));

		// no available light to replace
		return -1u;
	}

	return (SetLight(it - lights.begin(), light));
}

unsigned int GL::LightHandler::SetLight(unsigned int lgtIndex, const GL::Light& light) {
	const unsigned int lightID = lights[lgtIndex].GetID();

	// clear any previous dependence this light might have
	lights[lgtIndex].ClearDeathDependencies();

	lights[lgtIndex] = light;
	lights[lgtIndex].SetID(lightID);
	lights[lgtIndex].SetUID(lightHandle++);
	lights[lgtIndex].SetRelativeTime(0);
	lights[lgtIndex].SetAbsoluteTime(gs->frameNum);

	return (lights[lgtIndex].GetUID());
}

GL::Light* GL::LightHandler::GetLight(unsigned int lgtHandle) {
	const auto it = std::find_if(lights.begin(), lights.end(), [&](const GL::Light& lgt) { return (lgt.GetUID() == lgtHandle); });

	if (it != lights.end())
		return &(*it);

	return nullptr;
}


void GL::LightHandler::Update(Shader::IProgramObject* shader) {
	if (lights.size() != numLights) {
		numLights = lights.size();

		// update the active light-count (note: unused, number of lights
		// to iterate over needs to be known at shader compilation-time)
		// shader->SetUniform1i(uniformIndex, numLights);
	}

	if (numLights == 0)
		return;

	// float3 sumWeight;
	float3 maxWeight = OnesVector * 0.01f;

	for (const GL::Light& light: lights) {
		if (light.GetTTL() == 0)
			continue;

		// sumWeight += light.GetIntensityWeight();
		maxWeight = float3::max(maxWeight, light.GetIntensityWeight());
	}

	for (GL::Light& light: lights) {
		const unsigned int lightID = light.GetID();

		// dead light, ignore (but kill its contribution)
		if (light.GetTTL() == 0) {
			glEnable(lightID);
			glLightfv(lightID, GL_AMBIENT,  &ZeroVector4.x);
			glLightfv(lightID, GL_DIFFUSE,  &ZeroVector4.x);
			glLightfv(lightID, GL_SPECULAR, &ZeroVector4.x);
			glDisable(lightID);
			continue;
		}

		if (light.GetAbsoluteTime() != gs->frameNum) {
			light.SetRelativeTime(light.GetRelativeTime() + 1);
			light.SetAbsoluteTime(gs->frameNum);
			light.DecayColors();
			light.ClampColors();
		}

		// rescale by max (not sum!), otherwise 1) the intensity would
		// change if any light is added or removed when all have equal
		// weight and 2) a non-uniform set would cause a reduction for
		// all
		const float3 weight              = light.GetIntensityWeight() / maxWeight;
		const float4 weightedAmbientCol  = light.GetAmbientColor()  * weight.x;
		const float4 weightedDiffuseCol  = light.GetDiffuseColor()  * weight.y;
		const float4 weightedSpecularCol = light.GetSpecularColor() * weight.z;

		const float3* lightTrackPos      = light.GetTrackPosition();
		const float3* lightTrackDir      = light.GetTrackDirection();
		const float4& lightPos           = (lightTrackPos != nullptr)? float4(*lightTrackPos, 1.0f): light.GetPosition();
		const float3& lightDir           = (lightTrackDir != nullptr)? float3(*lightTrackDir      ): light.GetDirection();
		const bool    lightVisible       = (gu->spectatingFullView || light.GetIgnoreLOS() || losHandler->InLos(lightPos, gu->myAllyTeam));

		if (light.GetRelativeTime() > light.GetTTL()) {
			// mark light as dead
			light.SetTTL(0);
		} else {
			// communicate properties via the FFP to save uniforms
			// note: we want MV to be identity here
			glEnable(lightID);
			glLightfv(lightID, GL_POSITION, &lightPos.x);

			if (lightVisible) {
				glLightfv(lightID, GL_AMBIENT,  &weightedAmbientCol.x);
				glLightfv(lightID, GL_DIFFUSE,  &weightedDiffuseCol.x);
				glLightfv(lightID, GL_SPECULAR, &weightedSpecularCol.x);
			} else {
				// zero contribution from this light if not in LOS
				// (whether or not camera can see it is irrelevant
				// since the light always takes up a slot anyway)
				glLightfv(lightID, GL_AMBIENT,  &ZeroVector4.x);
				glLightfv(lightID, GL_DIFFUSE,  &ZeroVector4.x);
				glLightfv(lightID, GL_SPECULAR, &ZeroVector4.x);
			}

			glLightfv(lightID, GL_SPOT_DIRECTION, &lightDir.x);
			glLightf(lightID, GL_SPOT_CUTOFF, light.GetFOV());
			glLightf(lightID, GL_CONSTANT_ATTENUATION, light.GetRadius()); //!
			#if (OGL_SPEC_ATTENUATION == 1)
			glLightf(lightID, GL_CONSTANT_ATTENUATION,  light.GetAttenuation().x);
			glLightf(lightID, GL_LINEAR_ATTENUATION,    light.GetAttenuation().y);
			glLightf(lightID, GL_QUADRATIC_ATTENUATION, light.GetAttenuation().z);
			#endif
			glDisable(lightID);
		}
	}
}

