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
		glLightf(lightID, GL_CONSTANT_ATTENUATION,  0.0f);
		glLightf(lightID, GL_LINEAR_ATTENUATION,    0.0f);
		glLightf(lightID, GL_QUADRATIC_ATTENUATION, 0.0f);
		glDisable(lightID);

		// reserve free light ID's
		lightIDs.push_back(lightID);
	}
}


unsigned int GL::LightHandler::AddLight(const GL::Light& light) {
	if (lights.size() >= maxLights || lightIDs.empty()) { return -1U; }
	if (light.GetTTL() == 0 || light.GetRadius() <= 0.0f) { return -1U; }
	if (light.GetColorWeight().SqLength() <= 0.01f) { return -1U; }

	lightWeight += light.GetColorWeight();
	lights[lightHandle] = light;
	lights[lightHandle].SetID(lightIDs.front());
	lights[lightHandle].SetAge(gs->frameNum);

	lightIDs.pop_front();
	return lightHandle++;
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
		const GL::Light& light = it->second;
		const unsigned int lightID = light.GetID();
		const unsigned int lightHndle = it->first;

		const float4  weightedAmbientCol  = (light.GetAmbientColor()  * light.GetColorWeight().x) / lightWeight.x;
		const float4  weightedDiffuseCol  = (light.GetDiffuseColor()  * light.GetColorWeight().y) / lightWeight.y;
		const float4  weightedSpecularCol = (light.GetSpecularColor() * light.GetColorWeight().z) / lightWeight.z;
		const float4  lightRadiusVector   = float4(light.GetRadius(), 0.0f, 0.0f, 0.0f);
		const float3* lightTrackPos       = light.GetTrackPosition();
		const float4  lightPos            = (lightTrackPos != NULL)? float4(*lightTrackPos, 1.0f): light.GetPosition();

		++it;

		if ((gs->frameNum - light.GetAge()) > light.GetTTL()) {
			lightWeight -= light.GetColorWeight();
			lights.erase(lightHndle);
			lightIDs.push_back(lightID);

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
			glLightfv(lightID, GL_SPOT_DIRECTION, &lightRadiusVector.x); //!
			glLightf(lightID, GL_CONSTANT_ATTENUATION,  light.GetAttenuation().x);
			glLightf(lightID, GL_LINEAR_ATTENUATION,    light.GetAttenuation().y);
			glLightf(lightID, GL_QUADRATIC_ATTENUATION, light.GetAttenuation().z);
			glDisable(lightID);
		}
	}
}
