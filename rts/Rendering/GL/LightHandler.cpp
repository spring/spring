#include "myGL.h"
#include "LightHandler.h"
#include "Rendering/Shaders/Shader.hpp"
#include "Sim/Misc/GlobalSynced.h"

static const float4 ZeroVector4 = float4(0.0f, 0.0f, 0.0f, 0.0f);

void GL::LightHandler::Init(unsigned int cfgBaseLight, unsigned int cfgMaxLights) {
	glGetIntegerv(GL_MAX_LIGHTS, reinterpret_cast<int*>(&maxLights));

	baseLight = cfgBaseLight;
	maxLights -= baseLight;
	maxLights = std::min(maxLights, cfgMaxLights);

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

		// reserve free light ID's
		lightIDs.push_back(lightID);
	}
}


unsigned int GL::LightHandler::AddLight(const GL::Light& light) {
	if (light.GetTTL() == 0 || light.GetRadius() <= 0.0f) { return -1U; }
	if (light.GetIntensityWeight().SqLength() <= 0.01f) { return -1U; }

	if (lights.size() >= maxLights || lightIDs.empty()) {
		unsigned int minPriorityValue = light.GetPriority();
		unsigned int minPriorityHandle = -1U;

		for (std::map<unsigned int, GL::Light>::const_iterator it = lights.begin(); it != lights.end(); ++it) {
			const GL::Light& lgt = it->second;

			if (lgt.GetPriority() < minPriorityValue) {
				minPriorityValue = lgt.GetPriority();
				minPriorityHandle = it->first;
			}
		}

		if (minPriorityHandle != -1U) {
			lightIntensityWeight -= lights[minPriorityHandle].GetIntensityWeight();
			lightIDs.push_back(lights[minPriorityHandle].GetID());
			lights.erase(minPriorityHandle);
		} else {
			// no available light to replace
			return -1U;
		}
	}

	lights[lightHandle] = light;
	lights[lightHandle].SetID(lightIDs.front());
	lights[lightHandle].SetRelativeTime(0);
	lights[lightHandle].SetAbsoluteTime(gs->frameNum);

	lightIntensityWeight += light.GetIntensityWeight();
	lightIDs.pop_front();

	return (lightHandle++);
}

GL::Light* GL::LightHandler::GetLight(unsigned int _lightHandle) {
	const std::map<unsigned int, GL::Light>::iterator it = lights.find(_lightHandle);

	if (it != lights.end()) {
		return &(it->second);
	}

	return NULL;
}


void GL::LightHandler::Update(Shader::IProgramObject* shader) {
	if (lights.size() != numLights) {
		numLights = lights.size();

		// update the active light-count (note: unused, number of lights
		// to iterate over needs to be known at shader compilation-time)
		// shader->SetUniform1i(uniformIndex, numLights);
	}

	if (numLights == 0) {
		return;
	}

	for (std::map<unsigned int, GL::Light>::iterator it = lights.begin(); it != lights.end(); ) {
		GL::Light& light = it->second;

		if (light.GetAbsoluteTime() != gs->frameNum) {
			light.SetRelativeTime(light.GetRelativeTime() + 1);
			light.SetAbsoluteTime(gs->frameNum);
			light.DecayColors();
			light.ClampColors();
		}

		const unsigned int lightID = light.GetID();
		const unsigned int lightHndle = it->first;

		const float4  weightedAmbientCol  = (light.GetAmbientColor()  * light.GetIntensityWeight().x) / lightIntensityWeight.x;
		const float4  weightedDiffuseCol  = (light.GetDiffuseColor()  * light.GetIntensityWeight().y) / lightIntensityWeight.y;
		const float4  weightedSpecularCol = (light.GetSpecularColor() * light.GetIntensityWeight().z) / lightIntensityWeight.z;
		const float3* lightTrackPos       = light.GetTrackPosition();
		const float3* lightTrackDir       = light.GetTrackDirection();
		const float4& lightPos            = (lightTrackPos != NULL)? float4(*lightTrackPos, 1.0f): light.GetPosition();
		const float3& lightDir            = (lightTrackDir != NULL)? float3(*lightTrackDir      ): light.GetDirection();

		++it;

		if (light.GetRelativeTime() > light.GetTTL()) {
			lightIntensityWeight -= light.GetIntensityWeight();
			lightIDs.push_back(lightID);
			lights.erase(lightHndle);

			// kill the contribution from this light
			glEnable(lightID);
			glLightfv(lightID, GL_AMBIENT,  &ZeroVector4.x);
			glLightfv(lightID, GL_DIFFUSE,  &ZeroVector4.x);
			glLightfv(lightID, GL_SPECULAR, &ZeroVector4.x);
			glDisable(lightID);
		} else {
			// communicate properties via the FFP to save uniforms
			// note: we want MV to be identity here
			glEnable(lightID);
			glLightfv(lightID, GL_POSITION, &lightPos.x);
			glLightfv(lightID, GL_AMBIENT,  &weightedAmbientCol.x);
			glLightfv(lightID, GL_DIFFUSE,  &weightedDiffuseCol.x);
			glLightfv(lightID, GL_SPECULAR, &weightedSpecularCol.x);
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
