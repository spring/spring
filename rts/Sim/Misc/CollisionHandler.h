#ifndef COLLISION_HANDLER_H
#define COLLISION_HANDLER_H

#include "creg/creg.h"

struct CollisionVolume;
class CUnit;
class CFeature;

struct CollisionQuery {
	CollisionQuery() {
		// (0, 0, 0) is volume-space center, so
		// impossible to obtain as actual points
		// except in the special cases
		b0 = false; t0 = 0.0f; p0 = ZeroVector;
		b1 = false; t1 = 0.0f; p1 = ZeroVector;
	}

	bool b0, b1;	// true if ingress (b0) or egress (b1) point on ray segment
	float t0, t1;	// distance parameter for ingress and egress point
	float3 p0, p1;	// ray-volume ingress and egress points
};

// responsible for detecting hits between projectiles
// and world objects (units, features), each WO has a
// collision volume
class CCollisionHandler {
	public:
		CR_DECLARE(CCollisionHandler)

		CCollisionHandler() {}
		~CCollisionHandler() {}

		static bool DetectHit(const CUnit*, const float3&, const float3&, CollisionQuery* q = 0x0);
		static bool DetectHit(const CFeature*, const float3&, const float3&, CollisionQuery* q = 0x0);
		static bool MouseHit(const CUnit*, const float3& p0, const float3& p1, const CollisionVolume*, CollisionQuery* q);

	private:
		static bool Collision(const CUnit*, const float3&);
		static bool Collision(const CFeature*, const float3&);
		static bool Collision(const CollisionVolume*, const CMatrix44f&, const float3&);

	public:
		static bool Intersect(const CUnit*, const float3& p1, const float3& p2, CollisionQuery* q);
		static bool Intersect(const CFeature*, const float3& p1, const float3& p2, CollisionQuery* q);
	private:
		static bool Intersect(const CollisionVolume*, const CMatrix44f&, const float3&, const float3&, CollisionQuery* q);

		static bool IntersectEllipsoid(const CollisionVolume*, const float3&, const float3&, CollisionQuery* q);
		static bool IntersectCylinder(const CollisionVolume*, const float3&, const float3&, CollisionQuery* q);
		static bool IntersectBox(const CollisionVolume*, const float3&, const float3&, CollisionQuery* q);

		static unsigned int numCollisionTests;
		static unsigned int numIntersectionTests;
};

#endif
