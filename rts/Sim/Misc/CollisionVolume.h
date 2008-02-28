#ifndef COLLISION_VOLUME_H
#define COLLISION_VOLUME_H

#include "StdAfx.h"
#include "creg/creg.h"

#define ZVec ZeroVector

enum COLVOL_TYPES {COLVOL_TYPE_ELLIPSOID, COLVOL_TYPE_CYLINDER, COLVOL_TYPE_BOX};
enum COLVOL_AXES {COLVOL_AXIS_X, COLVOL_AXIS_Y, COLVOL_AXIS_Z};
enum COLVOL_TESTS {COLVOL_TEST_DISC, COLVOL_TEST_CONT};

class CUnit;
class CFeature;

struct CollisionQuery {
	CollisionQuery() {
		// (0, 0, 0) is volume-space center, so
		// impossible to obtain as actual points
		// except in the special cases
		b0 = false; t0 = 0.0f; p0 = ZVec;
		b1 = false; t1 = 0.0f; p1 = ZVec;
	}

	bool b0, b1;
	float t0, t1;
	float3 p0, p1;
};

class CCollisionVolume {
	public:
		CR_DECLARE(CCollisionVolume)

		CCollisionVolume() {}
		CCollisionVolume(const std::string&, const float3&, const float3&, int);
		~CCollisionVolume() {}

		void SetDefaultScale(const float);

		bool DetectHit(const CUnit*, const float3&, const float3&, CollisionQuery* q = 0x0) const;
		bool DetectHit(const CFeature*, const float3&, const float3&, CollisionQuery* q = 0x0) const;

		int GetVolumeType() const { return volumeType; }
		int GetTestType() const { return testType; }
		int GetPrimaryAxis() const { return primaryAxis; }
		float GetBoundingRadius() const { return volumeBoundingRadius; }
		float GetBoundingRadiusSq() const { return volumeBoundingRadiusSq; }
		float GetScale(int axis) const { return axisScales[axis]; }
		float GetHScale(int axis) const { return axisHScales[axis]; }
		float GetHScaleSq(int axis) const { return axisHScalesSq[axis]; }
		float GetOffset(int axis) const { return axisOffsets[axis]; }
		bool IsSphere() const { return spherical; }

	private:
		bool Collision(const CUnit*, const float3&) const;
		bool Collision(const CFeature*, const float3&) const;
		bool Intersect(const CUnit*, const float3& p1, const float3& p2, CollisionQuery* q) const;
		bool Intersect(const CFeature*, const float3& p1, const float3& p2, CollisionQuery* q) const;

		bool Collision(const CMatrix44f&, const float3&) const;
		bool Intersect(const CMatrix44f&, const float3&, const float3&, CollisionQuery* q) const;
		bool IntersectAlt(const CMatrix44f&, const float3&, const float3&, CollisionQuery* q) const;

		bool IntersectEllipsoid(const float3&, const float3&, CollisionQuery* q) const;
		bool IntersectCylinder(const float3&, const float3&, CollisionQuery* q) const;
		bool IntersectBox(const float3&, const float3&, CollisionQuery* q) const;

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
		bool spherical;
};

#endif
