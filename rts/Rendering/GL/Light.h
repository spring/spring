/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GL_LIGHT_H
#define _GL_LIGHT_H

#include "System/Object.h"
#include "System/float3.h"
#include "System/float4.h"

namespace GL {
	struct Light: public CObject {
	public:
		Light()
			: position(0.0f, 0.0f, 1.0f, 1.0f)
			, direction(ZeroVector)

			, ambientColor(0.0f, 0.0f, 0.0f, 1.0f)
			, diffuseColor(0.0f, 0.0f, 0.0f, 1.0f)
			, specularColor(0.0f, 0.0f, 0.0f, 1.0f)
			, intensityWeight(1.0f, 1.0f, 1.0f)
			, ambientDecayRate(1.0f, 1.0f, 1.0f)
			, diffuseDecayRate(1.0f, 1.0f, 1.0f)
			, specularDecayRate(1.0f, 1.0f, 1.0f)
			, decayFunctionType(1.0f, 1.0f, 1.0f)

			, radius(0.0f)
			, fov(180.0f)

			, ignoreLOS(true)

			, id(-1u)
			, uid(-1u)
			, ttl(0)
			, relTime(0)
			, absTime(0)
			, priority(0)

			, trackPosition(nullptr)
			, trackDirection(nullptr)
		{
		}

		void ClearDeathDependencies() {
			for (CObject* obj: GetListening(DEPENDENCE_LIGHT)) {
				DeleteDeathDependence(obj, DEPENDENCE_LIGHT);
			}
		}

		// a light can only depend on one object
		void DependentDied(CObject* o) {
			trackPosition = nullptr;
			trackDirection = nullptr;
		}

		const float4& GetPosition() const { return position; }
		const float3& GetDirection() const { return direction; }
		const float3* GetTrackPosition() const { return trackPosition; }
		const float3* GetTrackDirection() const { return trackDirection; }
		const float4& GetAmbientColor() const { return ambientColor; }
		const float4& GetDiffuseColor() const { return diffuseColor; }
		const float4& GetSpecularColor() const { return specularColor; }
		const float3& GetIntensityWeight() const { return intensityWeight; }
		const float3& GetAttenuation() const { return attenuation; }
		const float3& GetAmbientDecayRate() const { return ambientDecayRate; }
		const float3& GetDiffuseDecayRate() const { return diffuseDecayRate; }
		const float3& GetSpecularDecayRate() const { return specularDecayRate; }
		const float3& GetDecayFunctionType() const { return decayFunctionType; }

		void SetPosition(const float array[3]) { position.fromFloat3(array); }
		void SetDirection(const float array[3]) { direction = array; }
		void SetTrackPosition(const float3* pos) { trackPosition = pos; }
		void SetTrackDirection(const float3* dir) { trackDirection = dir; }
		void SetAmbientColor(const float array[3]) { ambientColor.fromFloat3(array); }
		void SetDiffuseColor(const float array[3]) { diffuseColor.fromFloat3(array); }
		void SetSpecularColor(const float array[3]) { specularColor.fromFloat3(array); }
		void SetIntensityWeight(const float array[3]) { intensityWeight = array; }
		void SetAttenuation(const float array[3]) { attenuation = array; }
		void SetAmbientDecayRate(const float array[3]) { ambientDecayRate = array; }
		void SetDiffuseDecayRate(const float array[3]) { diffuseDecayRate = array; }
		void SetSpecularDecayRate(const float array[3]) { specularDecayRate = array; }
		void SetDecayFunctionType(const float array[3]) { decayFunctionType = array; }

		void SetAmbientColor(const float3& ac) { ambientColor = ac; }
		void SetDiffuseColor(const float3& dc) { diffuseColor = dc; }
		void SetSpecularColor(const float3& sc) { specularColor = sc; }

		float GetRadius() const { return radius; }
		float GetFOV() const { return fov; }
		void SetRadius(float r) { radius = r; }
		void SetFOV(float f) { fov = f; }

		void SetIgnoreLOS(bool b) { ignoreLOS = b; }
		bool GetIgnoreLOS() const { return ignoreLOS; }

		unsigned int GetID() const { return id; }
		unsigned int GetUID() const { return uid; }
		unsigned int GetTTL() const { return ttl; }
		unsigned int GetRelativeTime() const { return relTime; }
		unsigned int GetAbsoluteTime() const { return absTime; }
		unsigned int GetPriority() const { return priority; }

		void SetID(unsigned int n) { id = n; }
		void SetUID(unsigned int n) { uid = n; }
		void SetTTL(unsigned int n) { ttl = n; }
		void SetRelativeTime(unsigned int n) { relTime = n; }
		void SetAbsoluteTime(unsigned int n) { absTime = n; }
		void SetPriority(unsigned int n) { priority = n; }

		void DecayColors() {
			const bool expAmbientDecay  = (decayFunctionType.x != 0.0f);
			const bool expDiffuseDecay  = (decayFunctionType.y != 0.0f);
			const bool expSpecularDecay = (decayFunctionType.z != 0.0f);

			if (expAmbientDecay) {
				ambientColor *= ambientDecayRate;
			} else {
				ambientColor -= ambientDecayRate;
			}

			if (expDiffuseDecay) {
				diffuseColor *= diffuseDecayRate;
			} else {
				diffuseColor -= diffuseDecayRate;
			}

			if (expSpecularDecay) {
				specularColor *= specularDecayRate;
			} else {
				specularColor -= specularDecayRate;
			}
		}

		void ClampColors() {
			ambientColor.x = std::max(0.0f, ambientColor.x);
			ambientColor.y = std::max(0.0f, ambientColor.y);
			ambientColor.z = std::max(0.0f, ambientColor.z);

			diffuseColor.x = std::max(0.0f, diffuseColor.x);
			diffuseColor.y = std::max(0.0f, diffuseColor.y);
			diffuseColor.z = std::max(0.0f, diffuseColor.z);

			specularColor.x = std::max(0.0f, specularColor.x);
			specularColor.y = std::max(0.0f, specularColor.y);
			specularColor.z = std::max(0.0f, specularColor.z);
		}

	private:
		float4  position;         // world-space, w == 1 (non-directional)
		float3  direction;        // world-space
		float4  ambientColor;     // RGB[A]
		float4  diffuseColor;     // RGB[A]
		float4  specularColor;    // RGB[A]
		float3  intensityWeight;  // x=ambientRGB, y=diffuseRGB, z=specularRGB
		float3  attenuation;      // x=constantAtt, y=linearAtt, z=quadraticAtt

		float3 ambientDecayRate;  // x=ambientR,  y=ambientG,  z=ambientB (per-frame decay of ambientColor)
		float3 diffuseDecayRate;  // x=diffuseR,  y=diffuseG,  z=diffuseB (per-frame decay of diffuseColor)
		float3 specularDecayRate; // x=specularR, y=specularG, z=specularB (per-frame decay of specularColor)
		float3 decayFunctionType; // x=ambientDT, y=diffuseDT, z=specularDT (0.0f=linear, 1.0f=exponential)

		float radius;             // elmos
		float fov;                // degrees ([0.0 - 90.0] or 180.0)

		bool ignoreLOS;           // if true, we can be seen out of LOS

		unsigned int id;          // GL_LIGHT[id] we are bound to
		unsigned int uid;         // LightHandler global counter
		unsigned int ttl;         // maximum lifetime in sim-frames
		unsigned int relTime;     // current lifetime in sim-frames
		unsigned int absTime;     // current sim-frame this light is at
		unsigned int priority;

		const float3* trackPosition;
		const float3* trackDirection;
	};
}

#endif // _GL_LIGHT_H
