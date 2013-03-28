/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "CollisionHandler.h"
#include "CollisionVolume.h"
#include "Rendering/Models/3DModel.h"
#include "Sim/Units/Unit.h"
#include "Sim/Features/Feature.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/FastMath.h"
#include "System/Matrix44f.h"
#include "System/Log/ILog.h"


unsigned int CCollisionHandler::numDiscTests = 0;
unsigned int CCollisionHandler::numContTests = 0;


void CCollisionHandler::PrintStats()
{
	LOG("[CCollisionHandler] dis-/continuous tests: %i/%i", numDiscTests, numContTests);
}


bool CCollisionHandler::DetectHit(const CUnit* u, const float3 p0, const float3 p1, CollisionQuery* cq, bool forceTrace)
{
	return DetectHit(u->collisionVolume, u, p0, p1, cq, forceTrace);
}


bool CCollisionHandler::DetectHit(const CSolidObject* o, const float3 p0, const float3 p1, CollisionQuery* cq, bool forceTrace)
{
	return DetectHit(o->collisionVolume, o, p0, p1, cq, forceTrace);
}


bool CCollisionHandler::DetectHit(const CollisionVolume* v, const CUnit* u, const float3 p0, const float3 p1, CollisionQuery* cq, bool forceTrace)
{
	if (cq != NULL) {
		cq->Reset();
	}

	// test *only* for ray intersections with the piece tree
	// (whether or not the unit's regular volume is disabled)
	//
	// overrides forceTrace, which itself overrides testType
	// FIXME make this available to SolidObjects too! (not worth it?)
	if (v->DefaultToPieceTree())
		return (CCollisionHandler::IntersectPieceTree(u, p0, p1, cq));

	return DetectHit(v, (const CSolidObject*)u, p0, p1, cq, forceTrace);
}


bool CCollisionHandler::DetectHit(const CollisionVolume* v, const CSolidObject* o, const float3 p0, const float3 p1, CollisionQuery* cq, bool forceTrace)
{
	bool hit = false;

	if (cq != NULL) {
		cq->Reset();
	}

	if (v->IgnoreHits())
		return false;

	if (forceTrace)
		return (CCollisionHandler::Intersect(v, o, p0, p1, cq));

	switch (int(v->UseContHitTest())) {
		// Collision() does not need p1
		case CollisionVolume::COLVOL_HITTEST_DISC: { hit = CCollisionHandler::Collision(v, o, p0    , cq); } break;
		case CollisionVolume::COLVOL_HITTEST_CONT: { hit = CCollisionHandler::Intersect(v, o, p0, p1, cq); } break;
		default: assert(false);
	}

	return hit;
}


bool CCollisionHandler::Collision(const CollisionVolume* v, const CSolidObject* o, const float3 p, CollisionQuery* cq)
{
	// if <v> is a sphere, then the bounding radius is just its own radius -->
	// we do not need to test the COLVOL_TYPE_SPHERE case again when this fails
	if ((v->GetWorldSpacePos(o) - p).SqLength() > v->GetBoundingRadiusSq()) {
		return false;
	}

	bool hit = false;

	if (v->DefaultToFootPrint()) {
		hit = CCollisionHandler::CollisionFootPrint(o, p);
	} else {
		switch (v->GetVolumeType()) {
			case CollisionVolume::COLVOL_TYPE_SPHERE: {
				hit = true;
			} break;
			default: {
				// NOTE: we have to translate by relMidPos to get to midPos
				// (which is where the collision volume gets drawn) because
				// GetTransformMatrix() only uses pos
				CMatrix44f m = o->GetTransformMatrix(true);
				m.Translate(o->relMidPos * WORLD_TO_OBJECT_SPACE);
				m.Translate(v->GetOffsets());

				hit = CCollisionHandler::Collision(v, m, p);
			}
		}
	}

	if (cq != NULL && hit) {
		// same as the special cases for the continuous tests
		// (but here p is a valid coordinate and safe to use)
		cq->b0 = CQ_POINT_IN_VOL; cq->t0 = 0.0f; cq->p0 = p;
	}

	return hit;
}


bool CCollisionHandler::CollisionFootPrint(const CSolidObject* o, const float3& p)
{
	// If the object isn't marked on blocking map, or if it is flying,
	// effecively only the early-out sphere check (in Collision(CUnit*)
	// or Collision(CFeature*)) is performed (which we already passed).
	if (!o->isMarkedOnBlockingMap)
		return false;
	if (o->physicalState == CSolidObject::Flying)
		return false;

	// this is semi-equivalent to testing if <p> is inside the rectangular
	// collision volume in the COLVOL_TYPE_BOX case, but takes non-blocking
	// yardmap squares into account (even though this is a discrete test so
	// projectile might have tunneled across blocking squares to get to <p>)
	// note: if we get here <v> is always a box
	const float invSquareSize = 1.0f / SQUARE_SIZE;
	const int squareIdx = int(p.x * invSquareSize) + int(p.z * invSquareSize) * gs->mapx;

	if (squareIdx < 0 || squareIdx >= gs->mapSquares)
		return false;

	const BlockingMapCell& cell = groundBlockingObjectMap->GetCell(squareIdx);

	return (cell.find(o->GetBlockingMapID()) != cell.end());
}


bool CCollisionHandler::Collision(const CollisionVolume* v, const CMatrix44f& m, const float3& p)
{
	numDiscTests += 1;

	// get the inverse volume transformation matrix and
	// apply it to the projectile's position, then test
	// if the transformed position lies within the axis-
	// aligned collision volume
	CMatrix44f mInv = m.Invert();
	float3 pi = mInv.Mul(p);

	bool hit = false;

	switch (v->GetVolumeType()) {
		case CollisionVolume::COLVOL_TYPE_SPHERE: {
			// normally, this code is never executed, because the higher level
			// Collision(CFeature*) and Collision(CUnit*) already optimize via
			// early-out tests
			hit = (pi.dot(pi) <= v->GetHSqScales().x);

			// test for arbitrary ellipsoids (no longer supported)
			// const float f1 = (pi.x * pi.x) / v->GetHSqScales().x;
			// const float f2 = (pi.y * pi.y) / v->GetHSqScales().y;
			// const float f3 = (pi.z * pi.z) / v->GetHSqScales().z;
			// hit = ((f1 + f2 + f3) <= 1.0f);
		} break;
		case CollisionVolume::COLVOL_TYPE_CYLINDER: {
			switch (v->GetPrimaryAxis()) {
				case CollisionVolume::COLVOL_AXIS_X: {
					const bool xPass = (math::fabs(pi.x) < v->GetHScales().x);
					const float yRat = (pi.y * pi.y) / v->GetHSqScales().y;
					const float zRat = (pi.z * pi.z) / v->GetHSqScales().z;
					hit = (xPass && (yRat + zRat <= 1.0f));
				} break;
				case CollisionVolume::COLVOL_AXIS_Y: {
					const bool yPass = (math::fabs(pi.y) < v->GetHScales().y);
					const float xRat = (pi.x * pi.x) / v->GetHSqScales().x;
					const float zRat = (pi.z * pi.z) / v->GetHSqScales().z;
					hit = (yPass && (xRat + zRat <= 1.0f));
				} break;
				case CollisionVolume::COLVOL_AXIS_Z: {
					const bool zPass = (math::fabs(pi.z) < v->GetHScales().z);
					const float xRat = (pi.x * pi.x) / v->GetHSqScales().x;
					const float yRat = (pi.y * pi.y) / v->GetHSqScales().y;
					hit = (zPass && (xRat + yRat <= 1.0f));
				} break;
			}
		} break;
		case CollisionVolume::COLVOL_TYPE_BOX: {
			const bool b1 = (math::fabs(pi.x) < v->GetHScales().x);
			const bool b2 = (math::fabs(pi.y) < v->GetHScales().y);
			const bool b3 = (math::fabs(pi.z) < v->GetHScales().z);
			hit = (b1 && b2 && b3);
		} break;
	}

	return hit;
}


bool CCollisionHandler::MouseHit(const CUnit* u, const float3& p0, const float3& p1, const CollisionVolume* v, CollisionQuery* cq)
{
	bool hit = false;

	if (v->DefaultToPieceTree()) {
		hit = CCollisionHandler::IntersectPieceTree(u, p0, p1, cq);
	} else {
		CMatrix44f m = u->GetTransformMatrix(false, true);
		m.Translate(u->relMidPos * WORLD_TO_OBJECT_SPACE);
		m.Translate(v->GetOffsets());

		hit = CCollisionHandler::Intersect(v, m, p0, p1, cq);
	}

	return hit;
}


/*
bool CCollisionHandler::IntersectPieceTreeHelper(
	LocalModelPiece* lmp,
	const CMatrix44f& mat,
	const float3& p0,
	const float3& p1,
	std::list<CollisionQuery>* cqs
) {
	bool ret = false;

	CollisionVolume* lmpVol = lmp->GetCollisionVolume();
	CMatrix44f volMat = lmp->GetModelSpaceMatrix() * mat;

	if (lmp->scriptSetVisible && !lmpVol->IgnoreHits()) {
		volMat.Translate(lmpVol->GetOffsets());

		CollisionQuery cq;

		if ((ret = CCollisionHandler::Intersect(lmpVol, volMat, p0, p1, &cq))) {
			cq.SetHitPiece(lmp); cqs->push_back(cq);
		}

		volMat.Translate(-lmpVol->GetOffsets());
	}

	for (unsigned int i = 0; i < lmp->children.size(); i++) {
		ret |= IntersectPieceTreeHelper(lmp->children[i], mat, p0, p1, cqs);
	}

	return ret;
}
*/

bool CCollisionHandler::IntersectPiecesHelper(
	const CUnit* u,
	const float3& p0,
	const float3& p1,
	std::list<CollisionQuery>* cqs
) {
	CMatrix44f unitMat = u->GetTransformMatrix(true);
	CMatrix44f volMat;
	CollisionQuery cq;

	for (unsigned int n = 0; n < u->localModel->pieces.size(); n++) {
		const LocalModelPiece* lmp = u->localModel->GetPiece(n);
		const CollisionVolume* lmpVol = lmp->GetCollisionVolume();

		if (!lmp->scriptSetVisible || lmpVol->IgnoreHits())
			continue;

		volMat = lmp->GetModelSpaceMatrix() * unitMat;
		volMat.Translate(lmpVol->GetOffsets());

		if (!CCollisionHandler::Intersect(lmpVol, volMat, p0, p1, &cq))
			continue;
		// skip if neither an ingress nor an egress hit
		if (cq.GetHitPos() == ZeroVector)
			continue;

		cq.SetHitPiece(const_cast<LocalModelPiece*>(lmp));
		cqs->push_back(cq);
	}

	// true iff at least one piece was intersected
	return (cq.GetHitPiece() != NULL);
}


bool CCollisionHandler::IntersectPieceTree(const CUnit* u, const float3& p0, const float3& p1, CollisionQuery* cq)
{
	std::list<CollisionQuery> cqs;
	std::list<CollisionQuery>::const_iterator cqsIt;

	// TODO:
	//   needs an early-out test, but gets complicated because
	//   pieces can move --> no clearly defined bounding volume
	if (!IntersectPiecesHelper(u, p0, p1, &cqs))
		return false;

	assert(!cqs.empty());

	// not interested in the details
	if (cq == NULL)
		return true;

	float minDstSq = std::numeric_limits<float>::max();
	float curDstSq = 0.0f;

	// save the closest intersection
	for (cqsIt = cqs.begin(); cqsIt != cqs.end(); ++cqsIt) {
		if ((curDstSq = (cqsIt->GetHitPos() - p0).SqLength()) >= minDstSq)
			continue;

		minDstSq = curDstSq;
		*cq = *cqsIt;
	}

	return true;
}

inline bool CCollisionHandler::Intersect(const CollisionVolume* v, const CSolidObject* o, const float3 p0, const float3 p1, CollisionQuery* cq)
{
	CMatrix44f m = o->GetTransformMatrix(true);
	m.Translate(o->relMidPos * WORLD_TO_OBJECT_SPACE);
	m.Translate(v->GetOffsets());

	return CCollisionHandler::Intersect(v, m, p0, p1, cq);
}

/*
bool CCollisionHandler::IntersectAlt(const collisionVolume* d, const CMatrix44f& m, const float3& p0, const float3& p1, CollisionQuery*)
{
	// alternative numerical integration method (unused)
	const float delta = 1.0f;
	const float length = (p1 - p0).Length();
	const float3 dir = (p1 - p0).Normalize();

	for (float t = 0.0f; t <= length; t += delta) {
		if (::Collision(d, m, p0 + dir * t)) return true;
	}

	return false;
}
*/


bool CCollisionHandler::Intersect(const CollisionVolume* v, const CMatrix44f& m, const float3& p0, const float3& p1, CollisionQuery* q)
{
	numContTests += 1;

	CMatrix44f mInv = m.Invert();
	const float3 pi0 = mInv.Mul(p0);
	const float3 pi1 = mInv.Mul(p1);
	bool intersect = false;

	// minimum and maximum (x, y, z) coordinates of transformed ray
	const float rminx = std::min(pi0.x, pi1.x), rminy = std::min(pi0.y, pi1.y), rminz = std::min(pi0.z, pi1.z);
	const float rmaxx = std::max(pi0.x, pi1.x), rmaxy = std::max(pi0.y, pi1.y), rmaxz = std::max(pi0.z, pi1.z);

	// minimum and maximum (x, y, z) coordinates of (bounding box around) volume
	const float vminx = -v->GetHScales().x, vminy = -v->GetHScales().y, vminz = -v->GetHScales().z;
	const float vmaxx =  v->GetHScales().x, vmaxy =  v->GetHScales().y, vmaxz =  v->GetHScales().z;

	// check if ray segment misses (bounding box around) volume
	// (if so, then no further intersection tests are necessary)
	if (rmaxx < vminx || rminx > vmaxx) { return false; }
	if (rmaxy < vminy || rminy > vmaxy) { return false; }
	if (rmaxz < vminz || rminz > vmaxz) { return false; }

	switch (v->GetVolumeType()) {
		case CollisionVolume::COLVOL_TYPE_SPHERE: {
			// sphere is special case of ellipsoid, reuse code
			intersect = CCollisionHandler::IntersectEllipsoid(v, pi0, pi1, q);
		} break;
		case CollisionVolume::COLVOL_TYPE_CYLINDER: {
			intersect = CCollisionHandler::IntersectCylinder(v, pi0, pi1, q);
		} break;
		case CollisionVolume::COLVOL_TYPE_BOX: {
			// also covers footprints, but without taking the blocking-map into account
			// TODO: this would require stepping ray across non-blocking yardmap squares?
			//
			// intersect = CCollisionHandler::IntersectFootPrint(v, pi0, pi1, q);
			intersect = CCollisionHandler::IntersectBox(v, pi0, pi1, q);
		} break;
	}

	if (q != NULL) {
		// transform intersection points (iff not a special
		// case, otherwise calling code should not use them)
		if (q->b0 == CQ_POINT_ON_RAY) { q->p0 = m.Mul(q->p0); }
		if (q->b1 == CQ_POINT_ON_RAY) { q->p1 = m.Mul(q->p1); }
	}

	return intersect;
}

bool CCollisionHandler::IntersectEllipsoid(const CollisionVolume* v, const float3& pi0, const float3& pi1, CollisionQuery* q)
{
	// transform the volume-space points into (unit) sphere-space (requires fewer
	// float-ops than solving the surface equation for arbitrary ellipsoid volumes)
	const float3 pii0 = float3(pi0.x * v->GetHIScales().x, pi0.y * v->GetHIScales().y, pi0.z * v->GetHIScales().z);
	const float3 pii1 = float3(pi1.x * v->GetHIScales().x, pi1.y * v->GetHIScales().y, pi1.z * v->GetHIScales().z);
	const float rSq = 1.0f;

	if (pii0.dot(pii0) <= rSq) {
		if (q != NULL) {
			// terminate early in the special case
			// that ray-segment originated *in* <v>
			// (these points are NOT transformed!)
			q->b0 = CQ_POINT_IN_VOL; q->p0 = ZeroVector;
			q->b1 = CQ_POINT_IN_VOL; q->p1 = ZeroVector;
		}
		return true;
	}

	// get the ray direction in unit-sphere space
	const float3 dir = (pii1 - pii0).SafeNormalize();

	// solves [ x^2 + y^2 + z^2 == r^2 ] for t
	// (<A> represents dir.dot(dir), equal to 1
	// since ray direction already normalized)
	const float A = 1.0f;
	const float B = 2.0f * (pii0).dot(dir);
	const float C = pii0.dot(pii0) - rSq;
	const float D = (B * B) - (4.0f * A * C);

	if (D < -COLLISION_VOLUME_EPS) {
		return false;
	} else {
		// get the length of the ray segment in volume-space
		const float segLenSq = (pi1 - pi0).SqLength();

		if (D < COLLISION_VOLUME_EPS) {
			// one solution for t
			const float t0 = -B * 0.5f;
			// const float t0 = -B / (2.0f * A);
			// get the intersection point in sphere-space
			const float3 pTmp = pii0 + (dir * t0);
			// get the intersection point in volume-space
			const float3 p0(pTmp.x * v->GetHScales().x, pTmp.y * v->GetHScales().y, pTmp.z * v->GetHScales().z);
			// get the distance from the start of the segment
			// to the intersection point in volume-space
			const float dSq0 = (p0 - pi0).SqLength();
			// if the intersection point is closer to p0 than
			// the end of the ray segment, the hit is valid
			const int b0 = (t0 > 0.0f && dSq0 <= segLenSq) * CQ_POINT_ON_RAY;

			if (q != NULL) {
				q->b0 = b0; q->b1 = CQ_POINT_NO_INT;
				q->t0 = t0; q->t1 = 0.0f;
				q->p0 = p0; q->p1 = ZeroVector;
			}

			return (b0 == CQ_POINT_ON_RAY);
		} else {
			// two solutions for t
			const float rD = fastmath::apxsqrt(D);
			const float t0 = (-B - rD) * 0.5f;
			const float t1 = (-B + rD) * 0.5f;
			// const float t0 = (-B + rD) / (2.0f * A);
			// const float t1 = (-B - rD) / (2.0f * A);
			// get the intersection points in sphere-space
			const float3 pTmp0 = pii0 + (dir * t0);
			const float3 pTmp1 = pii0 + (dir * t1);
			// get the intersection points in volume-space
			const float3 p0(pTmp0.x * v->GetHScales().x, pTmp0.y * v->GetHScales().y, pTmp0.z * v->GetHScales().z);
			const float3 p1(pTmp1.x * v->GetHScales().x, pTmp1.y * v->GetHScales().y, pTmp1.z * v->GetHScales().z);
			// get the distances from the start of the ray
			// to the intersection points in volume-space
			const float dSq0 = (p0 - pi0).SqLength();
			const float dSq1 = (p1 - pi0).SqLength();
			// if one of the intersection points is closer to p0
			// than the end of the ray segment, the hit is valid
			const int b0 = (t0 > 0.0f && dSq0 <= segLenSq) * CQ_POINT_ON_RAY;
			const int b1 = (t1 > 0.0f && dSq1 <= segLenSq) * CQ_POINT_ON_RAY;

			if (q != NULL) {
				q->b0 = b0; q->b1 = b1;
				q->t0 = t0; q->t1 = t1;
				q->p0 = p0; q->p1 = p1;
			}

			return (b0 == CQ_POINT_ON_RAY || b1 == CQ_POINT_ON_RAY);
		}
	}

	return false;
}

#if defined(USE_GML) && defined(__GNUC__) && (__GNUC__ == 4)
// This is supposed to fix some GCC crashbug related to threading
// The MOVAPS SSE instruction is otherwise getting misaligned data
__attribute__ ((force_align_arg_pointer))
#endif
bool CCollisionHandler::IntersectCylinder(const CollisionVolume* v, const float3& pi0, const float3& pi1, CollisionQuery* q)
{
	const int pAx = v->GetPrimaryAxis();
	const int sAx0 = v->GetSecondaryAxis(0);
	const int sAx1 = v->GetSecondaryAxis(1);
	const float3& ahs = v->GetHScales();
	const float3& ahsq = v->GetHSqScales();
	const float ratio =
		((pi0[sAx0] * pi0[sAx0]) / ahsq[sAx0]) +
		((pi0[sAx1] * pi0[sAx1]) / ahsq[sAx1]);

	if ((math::fabs(pi0[pAx]) < ahs[pAx]) && (ratio <= 1.0f)) {
		if (q != NULL) {
			// terminate early in the special case
			// that ray-segment originated within v
			q->b0 = CQ_POINT_IN_VOL; q->p0 = ZeroVector;
			q->b1 = CQ_POINT_IN_VOL; q->p1 = ZeroVector;
		}
		return true;
	}

	// ray direction in volume-space
	const float3 dir = (pi1 - pi0).SafeNormalize();

	// ray direction in (unit) cylinder-space
	float3 diir = ZeroVector;

	// ray terminals in (unit) cylinder-space
	float3 pii0 = pi0;
	float3 pii1 = pi1;

	// end-cap plane normals in volume-space
	float3 n0 = ZeroVector;
	float3 n1 = ZeroVector;

	// (unit) cylinder-space to volume-space transformation
	float3 inv(1.0f, 1.0f, 1.0f);

	// unit-cylinder surface equation params
	float a = 0.0f;
	float b = 0.0f;
	float c = 0.0f;

	switch (pAx) {
		case CollisionVolume::COLVOL_AXIS_X: {
			pii0.y = pi0.y * v->GetHIScales().y;
			pii0.z = pi0.z * v->GetHIScales().z;
			pii1.y = pi1.y * v->GetHIScales().y;
			pii1.z = pi1.z * v->GetHIScales().z;

			inv.y = v->GetHScales().y;
			inv.z = v->GetHScales().z;
			diir = (pii1 - pii0).SafeNormalize();

			n0.x = -1.0f; // left
			n1.x =  1.0f; // right

			// yz-surface equation params
			a =  (diir.y * diir.y) + (diir.z * diir.z);
			b = ((pii0.y * diir.y) + (pii0.z * diir.z)) * 2.0f;
			c =  (pii0.y * pii0.y) + (pii0.z * pii0.z)  - 1.0f;
		} break;
		case CollisionVolume::COLVOL_AXIS_Y: {
			pii0.x = pi0.x * v->GetHIScales().x;
			pii0.z = pi0.z * v->GetHIScales().z;
			pii1.x = pi1.x * v->GetHIScales().x;
			pii1.z = pi1.z * v->GetHIScales().z;

			inv.x = v->GetHScales().x;
			inv.z = v->GetHScales().z;
			diir = (pii1 - pii0).SafeNormalize();

			n0.y =  1.0f; // top
			n1.y = -1.0f; // bottom

			// xz-surface equation params
			a =  (diir.x * diir.x) + (diir.z * diir.z);
			b = ((pii0.x * diir.x) + (pii0.z * diir.z)) * 2.0f;
			c =  (pii0.x * pii0.x) + (pii0.z * pii0.z)  - 1.0f;
		} break;
		case CollisionVolume::COLVOL_AXIS_Z: {
			pii0.x = pi0.x * v->GetHIScales().x;
			pii0.y = pi0.y * v->GetHIScales().y;
			pii1.x = pi1.x * v->GetHIScales().x;
			pii1.y = pi1.y * v->GetHIScales().y;

			inv.x = v->GetHScales().x;
			inv.y = v->GetHScales().y;
			diir = (pii1 - pii0).SafeNormalize();

			n0.z =  1.0f; // front
			n1.z = -1.0f; // back

			// xy-surface equation params
			a =  (diir.x * diir.x) + (diir.y * diir.y);
			b = ((pii0.x * diir.x) + (pii0.y * diir.y)) * 2.0f;
			c =  (pii0.x * pii0.x) + (pii0.y * pii0.y)  - 1.0f;
		} break;
	}

	// volume-space intersection points
	float3 p0 = ZeroVector;
	float3 p1 = ZeroVector;

	int b0 = CQ_POINT_NO_INT;
	int b1 = CQ_POINT_NO_INT;
	float
		d = (b * b) - (4.0f * a * c),
		rd = 0.0f, // math::sqrt(d) or 1/dp
		dp = 0.0f, // dot(n{0, 1}, dir)
		ra = 0.0f; // ellipsoid ratio of p{0, 1}
	float s0 = 0.0f, t0 = 0.0f;
	float s1 = 0.0f, t1 = 0.0f;

	// get the length of the ray segment in volume-space
	const float segLenSq = (pi1 - pi0).SqLength();

	if (d >= -COLLISION_VOLUME_EPS) {
		if (a != 0.0f) {
			// quadratic eq.; one or two surface intersections
			if (d < COLLISION_VOLUME_EPS) {
				t0 = -b / (2.0f * a);
				p0 = (pii0 + (diir * t0)) * inv;
				s0 = (p0 - pi0).SqLength();
				b0 = (s0 < segLenSq  &&  math::fabs(p0[pAx]) < ahs[pAx]) * CQ_POINT_ON_RAY;
			} else {
				rd = fastmath::apxsqrt(d);
				t0 = (-b - rd) / (2.0f * a);
				t1 = (-b + rd) / (2.0f * a);
				p0 = (pii0 + (diir * t0)) * inv;
				p1 = (pii0 + (diir * t1)) * inv;
				s0 = (p0 - pi0).SqLength();
				s1 = (p1 - pi0).SqLength();
				b0 = (s0 < segLenSq  &&  math::fabs(p0[pAx]) < ahs[pAx]) * CQ_POINT_ON_RAY;
				b1 = (s1 < segLenSq  &&  math::fabs(p1[pAx]) < ahs[pAx]) * CQ_POINT_ON_RAY;
			}
		} else {
			if (b != 0.0f) {
				// linear eq.; one surface intersection
				t0 = -c / b;
				p0 = (pii0 + (diir * t0)) * inv;
				s0 = (p0 - pi0).SqLength();
				b0 = (s0 < segLenSq  &&  math::fabs(p0[pAx]) < ahs[pAx]) * CQ_POINT_ON_RAY;
			}
		}
	}

	if (b0 == CQ_POINT_NO_INT) {
		// p0 does not lie on ray segment, or does not fall
		// between cylinder end-caps: check if segment goes
		// through front cap (plane)
		// NOTE: normal n0 and dir should not be orthogonal
		dp = n0.dot(dir);
		rd = (dp != 0.0f)? 1.0f / dp: 0.01f;

		t0 = -(n0.dot(pi0) - ahs[pAx]) * rd;
		p0 = pi0 + (dir * t0);
		s0 = (p0 - pi0).SqLength();
		ra =
			(((p0[sAx0] * p0[sAx0]) / ahsq[sAx0]) +
			 ((p0[sAx1] * p0[sAx1]) / ahsq[sAx1]));
		b0 = (t0 >= 0.0f && ra <= 1.0f && s0 <= segLenSq) * CQ_POINT_ON_RAY;
	}
	if (b1 == CQ_POINT_NO_INT) {
		// p1 does not lie on ray segment, or does not fall
		// between cylinder end-caps: check if segment goes
		// through rear cap (plane)
		// NOTE: normal n1 and dir should not be orthogonal
		dp = n1.dot(dir);
		rd = (dp != 0.0f)? 1.0f / dp: 0.01f;

		t1 = -(n1.dot(pi0) - ahs[pAx]) * rd;
		p1 = pi0 + (dir * t1);
		s1 = (p1 - pi0).SqLength();
		ra =
			(((p1[sAx0] * p1[sAx0]) / ahsq[sAx0]) +
			 ((p1[sAx1] * p1[sAx1]) / ahsq[sAx1]));
		b1 = (t1 >= 0.0f && ra <= 1.0f && s1 <= segLenSq) * CQ_POINT_ON_RAY;
	}

	if (q != NULL) {
		q->b0 = b0; q->b1 = b1;
		q->t0 = t0; q->t1 = t1;
		q->p0 = p0; q->p1 = p1;
	}

	return (b0 == CQ_POINT_ON_RAY || b1 == CQ_POINT_ON_RAY);
}

bool CCollisionHandler::IntersectBox(const CollisionVolume* v, const float3& pi0, const float3& pi1, CollisionQuery* q)
{
	const bool ba = (math::fabs(pi0.x) < v->GetHScales().x);
	const bool bb = (math::fabs(pi0.y) < v->GetHScales().y);
	const bool bc = (math::fabs(pi0.z) < v->GetHScales().z);

	if (ba && bb && bc) {
		// terminate early in the special case
		// that ray-segment originated within v
		if (q != NULL) {
			q->b0 = CQ_POINT_IN_VOL; q->p0 = ZeroVector;
			q->b1 = CQ_POINT_IN_VOL; q->p1 = ZeroVector;
		}

		return true;
	}

	float tn = -9999999.9f;
	float tf =  9999999.9f;
	float t0 =  0.0f;
	float t1 =  0.0f;
	float t2 =  0.0f;

	const float3 dir = (pi1 - pi0).SafeNormalize();

	if (math::fabs(dir.x) < COLLISION_VOLUME_EPS) {
		if (math::fabs(pi0.x) > v->GetHScales().x) {
			return false;
		}
	} else {
		if (dir.x > 0.0f) {
			t0 = (-v->GetHScales().x - pi0.x) / dir.x;
			t1 = ( v->GetHScales().x - pi0.x) / dir.x;
		} else {
			t1 = (-v->GetHScales().x - pi0.x) / dir.x;
			t0 = ( v->GetHScales().x - pi0.x) / dir.x;
		}

		if (t0 > t1) { t2 = t1; t1 = t0; t0 = t2; }
		if (t0 > tn) { tn = t0; }
		if (t1 < tf) { tf = t1; }
		if (tn > tf) { return false; }
		if (tf < 0.0f) { return false; }
	}

	if (math::fabs(dir.y) < COLLISION_VOLUME_EPS) {
		if (math::fabs(pi0.y) > v->GetHScales().y) {
			return false;
		}
	} else {
		if (dir.y > 0.0f) {
			t0 = (-v->GetHScales().y - pi0.y) / dir.y;
			t1 = ( v->GetHScales().y - pi0.y) / dir.y;
		} else {
			t1 = (-v->GetHScales().y - pi0.y) / dir.y;
			t0 = ( v->GetHScales().y - pi0.y) / dir.y;
		}

		if (t0 > t1) { t2 = t1; t1 = t0; t0 = t2; }
		if (t0 > tn) { tn = t0; }
		if (t1 < tf) { tf = t1; }
		if (tn > tf) { return false; }
		if (tf < 0.0f) { return false; }
	}

	if (math::fabs(dir.z) < COLLISION_VOLUME_EPS) {
		if (math::fabs(pi0.z) > v->GetHScales().z) {
			return false;
		}
	} else {
		if (dir.z > 0.0f) {
			t0 = (-v->GetHScales().z - pi0.z) / dir.z;
			t1 = ( v->GetHScales().z - pi0.z) / dir.z;
		} else {
			t1 = (-v->GetHScales().z - pi0.z) / dir.z;
			t0 = ( v->GetHScales().z - pi0.z) / dir.z;
		}

		if (t0 > t1) { t2 = t1; t1 = t0; t0 = t2; }
		if (t0 > tn) { tn = t0; }
		if (t1 < tf) { tf = t1; }
		if (tn > tf) { return false; }
		if (tf < 0.0f) { return false; }
	}

	// get the intersection points in volume-space
	const float3 p0 = pi0 + (dir * tn);
	const float3 p1 = pi0 + (dir * tf);
	// get the length of the ray segment in volume-space
	const float segLenSq = (pi1 - pi0).SqLength();
	// get the distances from the start of the ray
	// to the intersection points in volume-space
	const float dSq0 = (p0 - pi0).SqLength();
	const float dSq1 = (p1 - pi0).SqLength();
	// if one of the intersection points is closer to p0
	// than the end of the ray segment, the hit is valid
	const int b0 = (dSq0 <= segLenSq) * CQ_POINT_ON_RAY;
	const int b1 = (dSq1 <= segLenSq) * CQ_POINT_ON_RAY;

	if (q != NULL) {
		q->b0 = b0; q->b1 = b1;
		q->t0 = tn; q->t1 = tf;
		q->p0 = p0; q->p1 = p1;
	}

	return (b0 == CQ_POINT_ON_RAY || b1 == CQ_POINT_ON_RAY);
}

