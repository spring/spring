/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef COLLISION_HANDLER_H
#define COLLISION_HANDLER_H

#include "System/creg/creg_cond.h"
#include "System/float3.h"
#include <list>

struct CollisionVolume;
class CMatrix44f;
class CSolidObject;
class CUnit;
struct LocalModelPiece;

enum {
	CQ_POINT_NO_INT = 0,
	CQ_POINT_ON_RAY = 1,
	CQ_POINT_IN_VOL = 2,
};

struct CollisionQuery {
public:
	CollisionQuery(): lmp(NULL) {
		Reset();
	}

	void Reset() {
		// (0, 0, 0) is volume-space center, so impossible
		// to obtain as actual points except in the special
		// cases (which all calling code should check for!)
		// when continuous hit-detection is enabled
		b0 = CQ_POINT_NO_INT; t0 = 0.0f; p0 = ZeroVector;
		b1 = CQ_POINT_NO_INT; t1 = 0.0f; p1 = ZeroVector;
	}

	bool InsideHit() const { return (b0 == CQ_POINT_IN_VOL); }
	bool IngressHit() const { return (b0 == CQ_POINT_ON_RAY); }
	bool EgressHit() const { return (b1 == CQ_POINT_ON_RAY); }

	const float3& GetIngressPos() const { return p0; }
	const float3& GetEgressPos() const { return p1; }
	const float3& GetHitPos() const {
		if (IngressHit()) return (GetIngressPos());
		if (EgressHit()) return (GetEgressPos());
		return ZeroVector;
	}

	// if the hit-position equals ZeroVector (i.e. if we have an
	// inside-hit special case), the projected distance would be
	// negative --> clamp it
	float GetHitPosDist(const float3& pos, const float3& dir) const { return (std::max(0.0f, ((GetHitPos() - pos).dot(dir)))); }
	float GetIngressPosDist(const float3& pos, const float3& dir) const { return (std::max(0.0f, ((GetIngressPos() - pos).dot(dir)))); }
	float GetEgressPosDist(const float3& pos, const float3& dir) const { return (std::max(0.0f, ((GetEgressPos() - pos).dot(dir)))); }

	LocalModelPiece* GetHitPiece() const { return lmp; }

private:
	friend class CCollisionHandler;

	int    b0, b1;        ///< true (non-zero) if ingress (b0) or egress (b1) point on ray segment
	float  t0, t1;        ///< distance parameter for ingress and egress point
	float3 p0, p1;        ///< ray-volume ingress and egress points

	LocalModelPiece* lmp; ///< impacted piece
};

/**
 * Responsible for detecting hits between projectiles
 * and world objects (units, features), each WO has a
 * collision volume.
 */
class CCollisionHandler {
	public:
		CCollisionHandler() {}
		~CCollisionHandler() {}

		static void PrintStats();

		static bool DetectHit(const CUnit* u, const float3 p0, const float3 p1, CollisionQuery* q = NULL, bool forceTrace = false);
		static bool DetectHit(const CSolidObject* f, const float3 p0, const float3 p1, CollisionQuery* q = NULL, bool forceTrace = false);
		static bool DetectHit(const CollisionVolume* v, const CUnit* u, const float3 p0, const float3 p1, CollisionQuery* q, bool forceTrace = false);
		static bool DetectHit(const CollisionVolume* v, const CSolidObject* f, const float3 p0, const float3 p1, CollisionQuery* q, bool forceTrace = false);
		static bool MouseHit(const CUnit* u, const float3& p0, const float3& p1, const CollisionVolume* v, CollisionQuery* q);

	private:
		// HITTEST_DISC helpers for DetectHit
		static bool Collision(const CollisionVolume* v, const CSolidObject* u, const float3 p, CollisionQuery* q);
		// HITTEST_CONT helpers for DetectHit
		static bool Intersect(const CollisionVolume* v, const CSolidObject* o, const float3 p0, const float3 p1, CollisionQuery* q);

	private:
		/**
		 * Test if a point lies inside a volume.
		 * @param v volume
		 * @param m volumes transformation matrix
		 * @param p point in world-coordinates
		 */
		static bool Collision(const CollisionVolume* v, const CMatrix44f& m, const float3& p);
		static bool CollisionFootPrint(const CSolidObject* o, const float3& p);

		/**
		 * Test if a ray intersects a volume.
		 * @param v volume
		 * @param m volumes transformation matrix
		 * @param p0 start of ray (in world-coordinates)
		 * @param p1 end of ray (in world-coordinates)
		 */
		static bool Intersect(const CollisionVolume* v, const CMatrix44f& m, const float3& p0, const float3& p1, CollisionQuery* q);
		static bool IntersectPieceTree(const CUnit* u, const float3& p0, const float3& p1, CollisionQuery* q);
		static void IntersectPieceTreeHelper(LocalModelPiece* lmp, CMatrix44f mat, const float3& p0, const float3& p1, std::list<CollisionQuery>* hits);

	public:
		static bool IntersectEllipsoid(const CollisionVolume* v, const float3& pi0, const float3& pi1, CollisionQuery* q);
		static bool IntersectCylinder(const CollisionVolume* v, const float3& pi0, const float3& pi1, CollisionQuery* q);
		static bool IntersectBox(const CollisionVolume* v, const float3& pi0, const float3& pi1, CollisionQuery* q);

	private:
		static unsigned int numDiscTests; // number of discrete hit-tests executed
		static unsigned int numContTests; // number of continuous hit-tests executed (inc. unsynced)
};

#endif // COLLISION_HANDLER_H
