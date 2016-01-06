/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SKY_LIGHT_H_
#define _SKY_LIGHT_H_

#include "System/float3.h"
#include "System/float4.h"
#include "System/Matrix44f.h"

struct ISkyLight {
public:
	virtual ~ISkyLight() {}
	virtual void Update() {}
	virtual bool SetLightDir(const float4& newDir) { return false; }
	virtual bool IsDynamic() const = 0;

	const float4& GetLightDir() const { return lightDir; }
	const float GetLightIntensity() const { return lightIntensity; }
	const float GetUnitShadowDensity() const { return unitShadowDensity; }
	const float GetGroundShadowDensity() const { return groundShadowDensity; }

protected:
	/**
	 * @brief current sunlight direction
	 */
	float4 lightDir;

	/**
	 * @brief sunlight intensity scale (in [0, 1])
	 */
	float lightIntensity;

	/**
	 * @brief elevation-adjusted unit shadow density
	 */
	float unitShadowDensity;

	/**
	 * @brief elevation-adjusted ground shadow density
	 */
	float groundShadowDensity;
};



struct StaticSkyLight: public ISkyLight {
public:
	StaticSkyLight();
	bool IsDynamic() const { return false; }
};



struct DynamicSkyLight: public ISkyLight {
public:
	DynamicSkyLight();
	bool IsDynamic() const { return true; }

	void Update();
	bool SetLightDir(const float4& newDir);
	void SetLightParams(float4 newDir, float startAngle, float orbitTime);

	void SetLuaControl(bool b) { luaControl = b; }
	bool GetLuaControl() const { return luaControl; }

private:
	inline float4 CalculateSunPos(const float startAngle) const;

	bool luaControl;

	bool updateNeeded;

	/**
	 * @brief the sun normally starts where its peak elevation occurs, unless this offset angle is set
	 */
	float sunStartAngle;

	/**
	 * @brief the initial sun angle (around y-axis)
	 */
	float initialSunAngle;

	/**
	 * @brief the distance of sun orbit center from origin
	 */
	float sunOrbitHeight;

	/**
	 * @brief the radius of the sun orbit
	 */
	float sunOrbitRad;

	/**
	 * @brief the time in seconds for the sun to complete the orbit
	 */
	float sunOrbitTime;

	/**
	 * @brief density factor to provide darker shadows at low sun altitude
	 */
	float shadowDensityFactor;

	/**
	 * @brief the lowest sun elevation for an automatically generated orbit
	 */
	float orbitMinSunHeight;

	/**
	 * @brief matrix that rotates the sun orbit
	 */
	CMatrix44f sunRotation;
};

#endif
