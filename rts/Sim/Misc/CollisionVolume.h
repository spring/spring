#ifndef COLLISION_VOLUME_H
#define COLLISION_VOLUME_H

#include "float3.h"
#include "creg/creg.h"
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
	COLVOL_AXIS_X,
	COLVOL_AXIS_Y,
	COLVOL_AXIS_Z,
	COLVOL_NUM_AXES     // number of collision volume axes
};
enum COLVOL_TESTS {
	COLVOL_TEST_DISC,
	COLVOL_TEST_CONT,
	COLVOL_NUM_TESTS    // number of tests
};

struct CollisionVolume
{
	CR_DECLARE_STRUCT(CollisionVolume);

	CollisionVolume(const CollisionVolume* src = NULL);
	CollisionVolume(const std::string& typeStr, const float3& scales, const float3& offsets, int testType);

	void SetDefaultScale(const float s);
	void Init(const float3& scales, const float3& offsets, int vType, int tType, int pAxis);

	int GetVolumeType() const { return volumeType; }
	int GetTestType() const { return testType; }
	void SetTestType(int type) { testType = type; }
	int GetPrimaryAxis() const { return primaryAxis; }
	float GetBoundingRadius() const { return volumeBoundingRadius; }
	float GetBoundingRadiusSq() const { return volumeBoundingRadiusSq; }
	float GetScale(int axis) const { return axisScales[axis]; }
	float GetHScale(int axis) const { return axisHScales[axis]; }
	float GetHScaleSq(int axis) const { return axisHScalesSq[axis]; }
	float GetOffset(int axis) const { return axisOffsets[axis]; }
	const float3& GetOffsets() const { return axisOffsets; }
	bool IsSphere() const { return volumeType == COLVOL_TYPE_SPHERE; }
	bool UseFootprint() const { return volumeType == COLVOL_TYPE_FOOTPRINT; }

private:
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

	friend class CCollisionHandler;     // TODO: refactor
};

#endif
