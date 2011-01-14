/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __GL_LIGHT_H__
#define __GL_LIGHT_H__

#include "System/float3.h"
#include "System/float4.h"

namespace GL {
	struct Light {
	public:
		Light() {
			position        = float4(0.0f, 0.0f, 1.0f, 1.0f);
			trackPosition   = NULL;
			ambientColor.w  = 1.0f;
			diffuseColor.w  = 1.0f;
			specularColor.w = 1.0f;
			colorWeight     = float3(1.0f, 1.0f, 1.0f);

			radius = 0.0f;

			id = -1U;
			ttl = 0;
			age = 0;
		}

		const float4& GetPosition() const { return position; }
		const float3* GetTrackPosition() const { return trackPosition; }
		const float4& GetAmbientColor() const { return ambientColor; }
		const float4& GetDiffuseColor() const { return diffuseColor; }
		const float4& GetSpecularColor() const { return specularColor; }
		const float3& GetColorWeight() const { return colorWeight; }
		const float3& GetAttenuation() const { return attenuation; }

		void SetPosition(const float array[3]) { position = array; }
		void SetTrackPosition(const float3* pos) { trackPosition = pos; }
		void SetAmbientColor(const float array[3]) { ambientColor = array; }
		void SetDiffuseColor(const float array[3]) { diffuseColor = array; }
		void SetSpecularColor(const float array[3]) { specularColor = array; }
		void SetColorWeight(const float array[3]) { colorWeight = array; }
		void SetAttenuation(const float array[3]) { attenuation = array; }

		float GetRadius() const { return radius; }
		void SetRadius(float r) { radius = r; }

		const unsigned int GetID() const { return id; }
		unsigned int GetTTL() const { return ttl; }
		unsigned int GetAge() const { return age; }
		void SetID(unsigned int n) { id = n; }
		void SetTTL(unsigned int n) { ttl = n; }
		void SetAge(unsigned int n) { age = n; }

	private:
		float4  position;       // world-space, w == 1 (non-directional)
		float4  ambientColor;   // RGBA
		float4  diffuseColor;   // RGBA
		float4  specularColor;  // RGBA
		float3  colorWeight;    // x=ambient, y=diffuse, z=specular
		float3  attenuation;    // x=constant, y=linear, z=quadratic

		float radius;          // elmos

		unsigned int id;       // GL_LIGHT we are bound to
		unsigned int ttl;      // lifetime in sim-frames
		unsigned int age;      // sim-frame this light was created

		const float3* trackPosition;
	};
};

#endif
