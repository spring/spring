#ifndef COLLISION_VOLUME_H
#define COLLISION_VOLUME_H

#include "float3.h"
#include "creg/creg_cond.h"
#include "Util.h"

const float EPS = 0.0000000001f;

enum COLVOL_TYPES {
	COLVOL_TYPE_DISABLED = -1,
	COLVOL_TYPE_ELLIPSOID = 0,
	COLVOL_TYPE_CYLINDER,
	COLVOL_TYPE_BOX,
	COLVOL_TYPE_SPHERE,
	COLVOL_TYPE_FOOTPRINT, // intersection of sphere and footprint-prism
	COLVOL_NUM_TYPES       // number of non-disabled collision volume types
};

enum COLVOL_AXES {
	COLVOL_AXIS_X   = 0,
	COLVOL_AXIS_Y   = 1,
	COLVOL_AXIS_Z   = 2,
	COLVOL_NUM_AXES = 3    // number of collision volume axes
};
enum COLVOL_TESTS {
	COLVOL_TEST_DISC = 0,
	COLVOL_TEST_CONT = 1,
	COLVOL_NUM_TESTS = 2   // number of tests
};

struct CollisionVolume
{
	CR_DECLARE_STRUCT(CollisionVolume);

	CollisionVolume();
	CollisionVolume(const CollisionVolume* v, float defRadius = 0.0f);
	CollisionVolume(const std::string&, const float3&, const float3&, int);

	static std::pair<int, int> GetVolumeTypeForString(const std::string&);

	void SetDefaultScale(const float);
	void Init(const float3&, const float3&, int, int, int);

	int GetVolumeType() const { return volumeType; }
	int GetTestType() const { return testType; }
	void SetVolumeType(int type) { volumeType = type; }
	void SetTestType(int type) { testType = type; }
	void Enable() { disabled = false; }
	void Disable() { disabled = true; }

	int GetPrimaryAxis() const { return primaryAxis; }
	int GetSecondaryAxis(int axis) const { return secondaryAxes[axis]; }

	float GetBoundingRadius() const { return volumeBoundingRadius; }
	float GetBoundingRadiusSq() const { return volumeBoundingRadiusSq; }

	float GetOffset(int axis) const { return axisOffsets[axis]; }
	const float3& GetOffsets() const { return axisOffsets; }

	float GetScale(int axis) const { return axisScales[axis]; }
	float GetHScale(int axis) const { return axisHScales[axis]; }
	float GetHScaleSq(int axis) const { return axisHScalesSq[axis]; }
	const float3& GetScales() const { return axisScales; }
	const float3& GetHScales() const { return axisHScales; }
	const float3& GetHScalesSq() const { return axisHScalesSq; }
	const float3& GetHIScales() const { return axisHIScales; }

	void RescaleAxes(float, float, float);

	bool IsDisabled() const { return disabled; }
	bool IsSphere() const { return volumeType == COLVOL_TYPE_SPHERE; }
	bool UseFootprint() const { return volumeType == COLVOL_TYPE_FOOTPRINT; }

private:
	void SetAxisScales(float, float, float);
	void SetBoundingRadius();

	float3 axisScales;					// full-length axis scales
	float3 axisHScales;					// half-length axis scales
	float3 axisHScalesSq;				// half-length axis scales (squared)
	float3 axisHIScales;				// half-length axis scales (inverted)
	float3 axisOffsets;

	float volumeBoundingRadius;			// radius of minimally-bounding sphere around volume
	float volumeBoundingRadiusSq;		// squared radius of minimally-bounding sphere
	int volumeType;
	int testType;
	int primaryAxis;
	int secondaryAxes[2];
	bool disabled;
};

#endif
