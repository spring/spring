#ifndef COLLISION_HANDLER_H
#define COLLISION_HANDLER_H

#include "creg/creg.h"

struct CollisionVolume;
class CUnit;
class CFeature;
struct LocalModelPiece;

#include <list>

struct CollisionQuery {
	CollisionQuery() {
		// (0, 0, 0) is volume-space center, so
		// impossible to obtain as actual points
		// except in the special cases
		b0 = false; t0 = 0.0f; p0 = ZeroVector;
		b1 = false; t1 = 0.0f; p1 = ZeroVector;

		lmp = NULL;
	}

	bool   b0, b1;        // true if ingress (b0) or egress (b1) point on ray segment
	float  t0, t1;        // distance parameter for ingress and egress point
	float3 p0, p1;        // ray-volume ingress and egress points

	LocalModelPiece* lmp; // impacted piece
};

// responsible for detecting hits between projectiles
// and world objects (units, features), each WO has a
// collision volume
class CCollisionHandler {
	public:
		CR_DECLARE(CCollisionHandler)

		CCollisionHandler() {}
		~CCollisionHandler() {}

		static bool DetectHit(const CUnit*, const float3&, const float3&, CollisionQuery* q = NULL);
		static bool DetectHit(const CFeature*, const float3&, const float3&, CollisionQuery* q = NULL);
		static bool MouseHit(const CUnit*, const float3&, const float3&, const CollisionVolume*, CollisionQuery*);

		static bool Intersect(const CUnit*, const float3&, const float3&, CollisionQuery*);
		static bool Intersect(const CFeature*, const float3&, const float3&, CollisionQuery*);

	private:
		static bool Collision(const CUnit*, const float3&);
		static bool Collision(const CFeature*, const float3&);
		static bool Collision(const CollisionVolume*, const CMatrix44f&, const float3&);
		static bool CollisionFootprint(const CSolidObject*, const float3&);

		static bool Intersect(const CollisionVolume*, const CMatrix44f&, const float3&, const float3&, CollisionQuery*);
		static bool IntersectPieceTree(const CUnit*, const float3&, const float3&, CollisionQuery*);
		static void IntersectPieceTreeHelper(LocalModelPiece*, CMatrix44f, const float3&, const float3&, std::list<CollisionQuery>*);

		static bool IntersectEllipsoid(const CollisionVolume*, const float3&, const float3&, CollisionQuery*);
		static bool IntersectCylinder(const CollisionVolume*, const float3&, const float3&, CollisionQuery*);
		static bool IntersectBox(const CollisionVolume*, const float3&, const float3&, CollisionQuery*);

		static unsigned int numCollisionTests;
		static unsigned int numIntersectionTests;
};

#endif
