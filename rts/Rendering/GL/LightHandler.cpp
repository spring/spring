/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "myGL.h"
#include "LightHandler.h"
#include "Game/GlobalUnsynced.h"
#include "Rendering/Shaders/Shader.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Projectiles/Projectile.h"

void GL::LightHandler::Init(unsigned int cfgMaxLights) {
	maxLights = std::min(MaxConfigLights(), cfgMaxLights);

	for (unsigned int i = 0; i < maxLights; i++) {
		glLights[i].SetID(GL_LIGHT0 + i);
	}
}


unsigned int GL::LightHandler::AddLight(const GL::Light& light) {
	if (light.GetTTL() == 0 || light.GetRadius() <= 0.0f)
		return -1u;
	if ((light.GetIntensityWeight()).SqLength() <= 0.01f)
		return -1u;

	const auto it = std::find_if(glLights.begin(), glLights.end(), [&](const GL::Light& lgt) { return (lgt.GetTTL() == 0); });

	if (it == glLights.end()) {
		// if all are claimed, find the lowest-priority light we can evict
		unsigned int minPriorityValue = light.GetPriority();
		unsigned int minPriorityIndex = -1u;

		for (unsigned int n = 0; n < glLights.size(); n++) {
			const GL::Light& lgt = glLights[n];

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

	return (SetLight(it - glLights.begin(), light));
}

unsigned int GL::LightHandler::SetLight(unsigned int lgtIndex, const GL::Light& light) {
	const unsigned int lightID = glLights[lgtIndex].GetID();

	// clear any previous dependence this light might have
	glLights[lgtIndex].ClearDeathDependencies();

	glLights[lgtIndex] = light;
	glLights[lgtIndex].SetID(lightID);
	glLights[lgtIndex].SetUID(lightHandle++);
	glLights[lgtIndex].SetRelativeTime(0);
	glLights[lgtIndex].SetAbsoluteTime(gs->frameNum);

	memset(GetRawLightDataPtr(lgtIndex), 0, sizeof(RawLight));

	return (glLights[lgtIndex].GetUID());
}

GL::Light* GL::LightHandler::GetLight(unsigned int lgtHandle) {
	const auto pred = [&](const GL::Light& lgt) { return (lgt.GetUID() == lgtHandle); };
	const auto iter = std::find_if(glLights.begin(), glLights.end(), pred);

	if (iter != glLights.end())
		return &(*iter);

	return nullptr;
}


void GL::LightHandler::Update() {
	assert(maxLights != 0);

	// float3 sumWeight;
	float3 maxWeight = OnesVector * 0.01f;

	for (const GL::Light& light: glLights) {
		if (light.GetTTL() == 0)
			continue;

		// sumWeight += light.GetIntensityWeight();
		maxWeight = float3::max(maxWeight, light.GetIntensityWeight());
	}

	for (unsigned int i = 0; i < maxLights; i++) {
		GL::Light& light = glLights[i];

		// dead light, ignore (but kill its contribution)
		if (light.GetTTL() == 0) {
			memset(GetRawLightDataPtr(i), 0, sizeof(RawLight));
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

		float4 lightPos = light.GetPosition();
		float4 lightDir = light.GetDirection(); // w=0, make sure to pick mat::oper*(float4)

		if (light.GetTrackObject() != nullptr) {
			switch (light.GetTrackType()) {
				case GL::Light::TRACK_TYPE_UNIT: {
					const CSolidObject* so = static_cast<const CSolidObject*>(light.GetTrackObject());

					if (light.LocalSpace()) {
						lightPos = so->GetObjectSpaceDrawPos(lightPos);
						lightDir = so->GetObjectSpaceVec(lightDir);
					} else {
						lightPos = so->drawPos;
						lightDir = so->frontdir;
					}
				} break;
				case GL::Light::TRACK_TYPE_PROJ: {
					const CProjectile* po = static_cast<const CProjectile*>(light.GetTrackObject());

					if (light.LocalSpace()) {
						const CMatrix44f m = po->GetTransformMatrix();

						lightPos = m * lightPos;
						lightDir = m * lightDir;
					} else {
						lightPos = po->drawPos;
						lightDir = po->dir;
					}
				} break;
				default: {} break;
			}
		}

		if (light.GetRelativeTime() > light.GetTTL()) {
			// mark light as dead
			light.SetTTL(0);
			continue;
		}

		rawLights[i].worldPos = lightPos;
		rawLights[i].worldDir = lightDir;

		if (gu->spectatingFullView || light.IgnoreLOS() || losHandler->InLos(lightPos, gu->myAllyTeam)) {
			// light is visible
			rawLights[i].diffColor = weightedDiffuseCol;
			rawLights[i].specColor = weightedSpecularCol;
			rawLights[i].ambiColor = weightedAmbientCol;
		} else {
			// zero contribution from this light if not in LOS
			// (whether or not camera can see it is irrelevant
			// since the light always takes up a slot anyway)
			memset(GetRawLightDataPtr(i), 0, sizeof(RawLight));
		}

		#if (OGL_SPEC_ATTENUATION == 1)
		rawLights[i].fovRadius = {light.GetFOV(), light.GetAttenuation()};
		#else
		rawLights[i].fovRadius = {light.GetFOV(), light.GetRadius(), light.GetRadius(), light.GetRadius()};
		#endif
	}
}

