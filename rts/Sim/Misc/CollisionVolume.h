#ifndef COLLISION_VOLUME_H
#define COLLISION_VOLUME_H

#include "StdAfx.h"
#include "creg/creg.h"

enum COLVOL_TYPES {COLVOL_TYPE_ELLIPSOID, COLVOL_TYPE_CYLINDER, COLVOL_TYPE_BOX};
enum COLVOL_AXES {COLVOL_AXIS_X, COLVOL_AXIS_Y, COLVOL_AXIS_Z};

class CUnit;
class CFeature;

struct CollisionQuery {
	CollisionQuery() {}
	void Reset() {
		t0 = 0.0f; p0.x = -1.0f; p0.y = -1.0f; p0.z = -1.0f;
		t1 = 0.0f; p1.x = -1.0f; p1.y = -1.0f; p1.z = -1.0f;
	}

	float t0; float3 p0;
	float t1; float3 p1;
};

class CCollisionVolume {
	public:
		CR_DECLARE(CCollisionVolume)

		CCollisionVolume() {}
		CCollisionVolume(const std::string&, const float3&, const float3&);
		~CCollisionVolume() {}

		void SetDefaultScale(const float);

		bool Collision(const CUnit*, const float3&) const;
		bool Collision(const CFeature*, const float3&) const;
		bool Intersect(const CUnit*, const float3& p1, const float3& p2, CollisionQuery* q = 0x0) const;
		bool Intersect(const CFeature*, const float3& p1, const float3& p2, CollisionQuery* q = 0x0) const;

		int GetType() const { return volumeType; }
		int GetPrimaryAxis() const { return primaryAxis; }
		float GetBoundingRadius() const { return volumeBoundingRadius; }
		float GetBoundingRadiusSq() const { return volumeBoundingRadiusSq; }
		float GetScale(int axis) const { return axisScales[axis]; }
		float GetHScale(int axis) const { return axisHScales[axis]; }
		float GetHScaleSq(int axis) const { return axisHScalesSq[axis]; }
		float GetOffset(int axis) const { return axisOffsets[axis]; }
		bool IsSphere() const { return spherical; }

	private:
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

		float volumeBoundingRadius;
		float volumeBoundingRadiusSq;
		int volumeType;
		int primaryAxis;
		int secondaryAxes[2];
		bool spherical;
};

#endif
