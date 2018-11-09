/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef COLLISION_HANDLER_H
#define COLLISION_HANDLER_H

#include "System/creg/creg_cond.h"
#include "System/float3.h"
#include "System/Matrix44f.h"

#include <algorithm>

class CSolidObject;
struct LocalModelPiece;
struct CollisionVolume;

enum {
	CQ_POINT_NO_INT = 0,
	CQ_POINT_ON_RAY = 1,
	CQ_POINT_IN_VOL = 2,
};

struct CollisionQuery {
public:
	bool SwapParams() {
		if (!AllHit() || ValidRay())
			return false;

		std::swap(t1, t0);
		std::swap(p1, p0);
		std::swap(b1, b0);
		return true;
	}
	void Transform(const CMatrix44f& m) {
		// transform intersection points (iff not a special
		// case, otherwise calling code should not use them)
		if (b0 == CQ_POINT_ON_RAY) { p0 = m.Mul(p0); }
		if (b1 == CQ_POINT_ON_RAY) { p1 = m.Mul(p1); }
	}

	void Reset(const CollisionQuery* cq = nullptr) {
		*this = (cq != nullptr) ? *cq : CollisionQuery{};
	}

	// t0 > t1 can happen when intersecting cylinder endcaps
	bool ValidRay() const { return (t0 <= t1); }
	bool InsideHit() const { return (b0 == CQ_POINT_IN_VOL); }
	bool IngressHit() const { return (b0 == CQ_POINT_ON_RAY); }
	bool EgressHit() const { return (b1 == CQ_POINT_ON_RAY); }
	bool AllHit() const { return (b0 != CQ_POINT_NO_INT && b1 != CQ_POINT_NO_INT); }
	bool AnyHit() const { return (b0 != CQ_POINT_NO_INT || b1 != CQ_POINT_NO_INT); }

	const float3& GetIngressPos() const { return p0; }
	const float3& GetEgressPos() const { return p1; }
	const float3& GetHitPos() const {
		if (IngressHit()) return GetIngressPos();
		if (EgressHit()) return GetEgressPos();
		if (InsideHit()) return p0;
		return ZeroVector;
	}

	// if the hit-position equals ZeroVector (i.e. if we have an
	// inside-hit special case), the projected distance could be
	// positive or negative depending on <dir> but we want it to
	// be 0 --> turn <pos> into a ZeroVector if InsideHit()
	float GetHitPosDist(const float3& pos, const float3& dir) const { return (std::max(0.0f, dir.dot(GetHitPos() - pos * (1 - InsideHit())))); }
	float GetIngressPosDist(const float3& pos, const float3& dir) const { return (std::max(0.0f, dir.dot(GetIngressPos() - pos))); }
	float GetEgressPosDist(const float3& pos, const float3& dir) const { return (std::max(0.0f, dir.dot(GetEgressPos() - pos))); }

	const LocalModelPiece* GetHitPiece() const { return lmp; }
	void SetHitPiece(const LocalModelPiece* p) { lmp = p; }

private:
	friend class CCollisionHandler;

	///< true (non-zero) if {in,e}gress (b{0,1}) point on ray segment
	int    b0 = CQ_POINT_NO_INT;
	int    b1 = CQ_POINT_NO_INT;
	///< distance parameter for ingress and egress point
	float  t0 = 0.0f;
	float  t1 = 0.0f;
	///< ray-volume ingress and egress points
	float3 p0;
	float3 p1;

	///< impacted piece
	const LocalModelPiece* lmp = nullptr;
};

/**
 * Responsible for detecting hits between projectiles
 * and solid objects (units, features), each SO has a
 * collision volume.
 */
class CCollisionHandler {
	public:
		static void PrintStats();

		static bool DetectHit(
			const CSolidObject* o,
			const CMatrix44f& m,
			const float3 p0,
			const float3 p1,
			CollisionQuery* cq = nullptr,
			bool forceTrace = false
		);
		static bool DetectHit(
			const CSolidObject* o,
			const CollisionVolume* v,
			const CMatrix44f& m,
			const float3 p0,
			const float3 p1,
			CollisionQuery* cq = nullptr,
			bool forceTrace = false
		);
		static bool MouseHit(
			const CSolidObject* o,
			const CMatrix44f& m,
			const float3& p0,
			const float3& p1,
			const CollisionVolume* v,
			CollisionQuery* cq = nullptr
		);

	private:
		// HITTEST_DISC helpers for DetectHit
		static bool Collision(
			const CSolidObject* o,
			const CollisionVolume* v,
			const CMatrix44f& m,
			const float3 p,
			CollisionQuery* cq
		);
		// HITTEST_CONT helpers for DetectHit
		static bool Intersect(
			const CSolidObject* o,
			const CollisionVolume* v,
			const CMatrix44f& m,
			const float3 p0,
			const float3 p1,
			CollisionQuery* cq,
			float s = 1.0f
		);

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
		static bool Intersect(const CollisionVolume* v, const CMatrix44f& m, const float3& p0, const float3& p1, CollisionQuery* cq);
		static bool IntersectPieceTree(const CSolidObject* o, const CMatrix44f& m, const float3& p0, const float3& p1, CollisionQuery* cq);
		static bool IntersectPiecesHelper(const CSolidObject* o, const CMatrix44f& m, const float3& p0, const float3& p1, CollisionQuery* cqp);

	public:
		static bool IntersectEllipsoid(const CollisionVolume* v, const float3& pi0, const float3& pi1, CollisionQuery* cq);
		static bool IntersectCylinder(const CollisionVolume* v, const float3& pi0, const float3& pi1, CollisionQuery* cq);
		static bool IntersectBox(const CollisionVolume* v, const float3& pi0, const float3& pi1, CollisionQuery* cq);

	private:
		static unsigned int numDiscTests; // number of discrete hit-tests executed
		static unsigned int numContTests; // number of continuous hit-tests executed (inc. unsynced)
};

#endif // COLLISION_HANDLER_H
