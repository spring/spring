/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef COLLISION_HANDLER_H
#define COLLISION_HANDLER_H

#include "System/creg/creg_cond.h"
#include "System/float3.h"
#include <vector>

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
	CollisionQuery(                        ): lmp(NULL) { Reset(NULL); }
	CollisionQuery(const CollisionQuery& cq): lmp(NULL) { Reset( &cq); }

	void Reset(const CollisionQuery* cq = NULL) {
		// (0, 0, 0) is volume-space center, so impossible
		// to obtain as actual points except in the special
		// cases (which all calling code should check for!)
		// when continuous hit-detection is enabled
		if (cq == NULL) {
			b0 = CQ_POINT_NO_INT; t0 = 0.0f; p0 = ZeroVector;
			b1 = CQ_POINT_NO_INT; t1 = 0.0f; p1 = ZeroVector;
		} else {
			b0 = cq->b0; t0 = cq->t0; p0 = cq->p0;
			b1 = cq->b1; t1 = cq->t1; p1 = cq->p1;

			lmp = cq->lmp;
		}
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
	// inside-hit special case), the projected distance could be
	// positive or negative depending on <dir> but we want it to
	// be 0 --> turn <pos> into a ZeroVector if InsideHit()
	float GetHitPosDist(const float3& pos, const float3& dir) const { return (std::max(0.0f, ((GetHitPos() - pos * (1 - InsideHit())).dot(dir)))); }
	float GetIngressPosDist(const float3& pos, const float3& dir) const { return (std::max(0.0f, ((GetIngressPos() - pos).dot(dir)))); }
	float GetEgressPosDist(const float3& pos, const float3& dir) const { return (std::max(0.0f, ((GetEgressPos() - pos).dot(dir)))); }

	LocalModelPiece* GetHitPiece() const { return lmp; }
	void SetHitPiece(LocalModelPiece* p) { lmp = p; }

private:
	friend class CCollisionHandler;

	int    b0, b1;        ///< true (non-zero) if ingress (b0) or egress (b1) point on ray segment
	float  t0, t1;        ///< distance parameter for ingress and egress point
	float3 p0, p1;        ///< ray-volume ingress and egress points

	LocalModelPiece* lmp; ///< impacted piece
};

/**
 * Responsible for detecting hits between projectiles
 * and solid objects (units, features), each SO has a
 * collision volume.
 */
class CCollisionHandler {
	public:
		static void PrintStats();

		static bool DetectHit(const CSolidObject* o, const float3 p0, const float3 p1, CollisionQuery* cq = NULL, bool forceTrace = false);
		static bool DetectHit(const CollisionVolume* v, const CSolidObject* o, const float3 p0, const float3 p1, CollisionQuery* cq, bool forceTrace = false);
		static bool MouseHit(const CUnit* u, const float3& p0, const float3& p1, const CollisionVolume* v, CollisionQuery* cq);

	private:
		// HITTEST_DISC helpers for DetectHit
		static bool Collision(const CollisionVolume* v, const CSolidObject* u, const float3 p, CollisionQuery* cq);
		// HITTEST_CONT helpers for DetectHit
		static bool Intersect(const CollisionVolume* v, const CSolidObject* o, const float3 p0, const float3 p1, CollisionQuery* cq);

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
		static bool IntersectPieceTree(const CUnit* u, const float3& p0, const float3& p1, CollisionQuery* cq);

		static bool IntersectPieceTreeHelper(LocalModelPiece* lmp, const CMatrix44f& mat, const float3& p0, const float3& p1, std::vector<CollisionQuery>* cqs);
		static bool IntersectPiecesHelper(const CUnit* u, const float3& p0, const float3& p1, std::vector<CollisionQuery>* cqs);

	public:
		static bool IntersectEllipsoid(const CollisionVolume* v, const float3& pi0, const float3& pi1, CollisionQuery* cq);
		static bool IntersectCylinder(const CollisionVolume* v, const float3& pi0, const float3& pi1, CollisionQuery* cq);
		static bool IntersectBox(const CollisionVolume* v, const float3& pi0, const float3& pi1, CollisionQuery* cq);

	private:
		static unsigned int numDiscTests; // number of discrete hit-tests executed
		static unsigned int numContTests; // number of continuous hit-tests executed (inc. unsynced)
};

#endif // COLLISION_HANDLER_H
