/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "CollisionHandler.h"
#include "CollisionVolume.h"
#include "Map/ReadMap.h" // mapDims
#include "Rendering/Models/3DModel.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Objects/SolidObject.h"
#include "System/Matrix44f.h"
#include "System/Log/ILog.h"

unsigned int CCollisionHandler::numDiscTests = 0;
unsigned int CCollisionHandler::numContTests = 0;



void CCollisionHandler::PrintStats()
{
	LOG("[CCollisionHandler] dis-/continuous tests: %i/%i", numDiscTests, numContTests);
}



bool CCollisionHandler::DetectHit(
	const CSolidObject* o,
	const CMatrix44f& m,
	const float3 p0,
	const float3 p1,
	CollisionQuery* cq,
	bool forceTrace)
{
	// use the object's own collision volume
	return (DetectHit(o, &o->collisionVolume, m, p0, p1, cq, forceTrace));
}

bool CCollisionHandler::DetectHit(
	const CSolidObject* o,
	const CollisionVolume* v, // can be foreign (not owned by o)
	const CMatrix44f& m,
	const float3 p0,
	const float3 p1,
	CollisionQuery* cq,
	bool forceTrace
) {
	bool hit = false;

	if (cq != nullptr)
		cq->Reset();

	if (o->IsInVoid())
		return hit;

	// test *only* for ray intersections with the piece tree
	// (whether or not the object's regular volume is disabled)
	//
	// overrides forceTrace, which itself overrides testType
	if (v->DefaultToPieceTree())
		return (CCollisionHandler::IntersectPieceTree(o, m, p0, p1, cq));
	if (v->IgnoreHits())
		return hit;

	if (forceTrace || v->UseContHitTest()) {
		hit = CCollisionHandler::Intersect(o, v, m, p0, p1, cq);
	} else {
		// Collision() does not need p1 (no ray, no ray-endpoint)
		hit = CCollisionHandler::Collision(o, v, m, p0,     cq);
	}

	return hit;
}



bool CCollisionHandler::Collision(
	const CSolidObject* o,
	const CollisionVolume* v,
	const CMatrix44f& m,
	const float3 p,
	CollisionQuery* cq
) {
	bool hit = false;

	// if <v> is a sphere, then the bounding radius is just its own radius -->
	// we do not need to test the COLVOL_TYPE_SPHERE case again when this fails
	if ((v->GetWorldSpacePos(o) - p).SqLength() > v->GetBoundingRadiusSq())
		return hit;

	if (v->DefaultToFootPrint()) {
		hit = CCollisionHandler::CollisionFootPrint(o, p);
	} else {
		hit = (v->GetVolumeType() == CollisionVolume::COLVOL_TYPE_SPHERE);

		if (!hit) {
			// transform into midpos-relative space
			CMatrix44f mr = m;
			mr.Translate(o->relMidPos);
			mr.Translate(v->GetOffsets());

			hit = CCollisionHandler::Collision(v, mr, p);
		}
	}

	if (cq != nullptr && hit) {
		// same as the special cases for the continuous tests
		// (but here p is a valid coordinate and safe to use)
		cq->b0 = CQ_POINT_IN_VOL; cq->t0 = 0.0f; cq->p0 = p;
	}

	return hit;
}


bool CCollisionHandler::CollisionFootPrint(const CSolidObject* o, const float3& p)
{
	// If the object isn't marked on blocking map, or if it is flying,
	// effecively only the early-out sphere check  is performed (which
	// we already passed).
	if (!o->IsBlocking())
		return false;
	if (o->IsInAir())
		return false;

	// this is semi-equivalent to testing if <p> is inside the rectangular
	// collision volume in the COLVOL_TYPE_BOX case, but takes non-blocking
	// yardmap squares into account (even though this is a discrete test so
	// projectile might have tunneled across blocking squares to get to <p>)
	// note: if we get here <v> is always a box
	const int hmx = p.x / SQUARE_SIZE;
	const int hmz = p.z / SQUARE_SIZE;
	const int idx = hmx + hmz * mapDims.mapx;

	return (groundBlockingObjectMap.ObjectInCell(idx, o));
}


bool CCollisionHandler::Collision(const CollisionVolume* v, const CMatrix44f& m, const float3& p)
{
	numDiscTests += 1;

	// get the inverse volume transformation matrix and
	// apply it to the projectile's position, then test
	// if the transformed position lies within the axis-
	// aligned collision volume
	const CMatrix44f mInv = m.InvertAffine();
	const float3 pi = mInv.Mul(p);

	bool hit = false;

	switch (v->GetVolumeType()) {
		case CollisionVolume::COLVOL_TYPE_SPHERE: {
			// normally this code is never executed, because the higher
			// level Collision() already optimize via early-out tests
			hit = (pi.dot(pi) <= v->GetHSqScales().x);
		} break;
		case CollisionVolume::COLVOL_TYPE_ELLIPSOID: {
			const float f1 = (pi.x * pi.x) / v->GetHSqScales().x;
			const float f2 = (pi.y * pi.y) / v->GetHSqScales().y;
			const float f3 = (pi.z * pi.z) / v->GetHSqScales().z;
			hit = ((f1 + f2 + f3) <= 1.0f);
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


bool CCollisionHandler::MouseHit(
	const CSolidObject* o,
	const CMatrix44f& m,
	const float3& p0,
	const float3& p1,
	const CollisionVolume* v,
	CollisionQuery* cq
) {
	if (cq != nullptr)
		cq->Reset();

	if (o->IsInVoid())
		return false;

	if (v->DefaultToPieceTree())
		return (CCollisionHandler::IntersectPieceTree(o, m, p0, p1, cq));
	if (v->IgnoreHits())
		return false;

	// note: should mouse-rays care about
	// IgnoreHits if object is not in void?
	return (CCollisionHandler::Intersect(o, v, m, p0, p1, cq));
}


/*
bool CCollisionHandler::IntersectPieceTreeHelper(
	LocalModelPiece* lmp,
	const CMatrix44f& mat,
	const float3& p0,
	const float3& p1,
	std::vector<CollisionQuery>* cqs
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
	const CSolidObject* o,
	const CMatrix44f& m,
	const float3& p0,
	const float3& p1,
	CollisionQuery* cq
) {
	CMatrix44f volMat;

	float minDistSq = std::numeric_limits<float>::max();
	float curDistSq = minDistSq;

	for (unsigned int n = 0; n < o->localModel.pieces.size(); n++) {
		const LocalModelPiece* lmp = o->localModel.GetPiece(n);
		const CollisionVolume* lmpVol = lmp->GetCollisionVolume();

		if (!lmp->scriptSetVisible || lmpVol->IgnoreHits())
			continue;

		volMat = m * lmp->GetModelSpaceMatrix();
		volMat.Translate(lmpVol->GetOffsets());

		CollisionQuery cqn;
		if (!CCollisionHandler::Intersect(lmpVol, volMat, p0, p1, &cqn))
			continue;

		// skip if neither an ingress nor an egress hit
		if (!cqn.AnyHit())
			continue;

		// save the closest intersection (others are not needed)
		if ((curDistSq = (cqn.GetHitPos()).SqDistance(p0)) >= minDistSq)
			continue;

		minDistSq = curDistSq;

		// return early if caller only wants to know a collision exists
		if (cq == nullptr)
			return true;

		*cq = cqn;
		cq->SetHitPiece(lmp);
	}

	// true iff at least one piece was intersected
	// (query must have been reset by calling code)
	return (cq != nullptr && cq->GetHitPiece() != nullptr);
}


bool CCollisionHandler::IntersectPieceTree(
	const CSolidObject* o,
	const CMatrix44f& m,
	const float3& p0,
	const float3& p1,
	CollisionQuery* cq
) {
	const LocalModel& lm = o->localModel;
	const CollisionVolume* bv = lm.GetBoundingVolume();

	// defer to IntersectBox for the early-out test; align OOBB
	// to object's axes (unlike a regular CV this is positioned
	// relative to o->pos, so pass scale=0 to ignore the normal
	// relMidPos translation)
	if (!CCollisionHandler::Intersect(o, bv, m, p0, p1, cq, 0.0f))
		return false;

	return (IntersectPiecesHelper(o, m, p0, p1, cq));
}

inline bool CCollisionHandler::Intersect(
	const CSolidObject* o,
	const CollisionVolume* v,
	const CMatrix44f& m,
	const float3 p0,
	const float3 p1,
	CollisionQuery* cq,
	float s
) {
	// transform into midpos-relative space where the CV is
	// positioned; we have to translate by relMidPos to get
	// to midPos because GetTransformMatrix() only uses pos
	// for all CSolidObject types
	//
	CMatrix44f mr = m;

	mr.Translate(o->relMidPos * s);
	mr.Translate(v->GetOffsets());

	return (CCollisionHandler::Intersect(v, mr, p0, p1, cq));
}

bool CCollisionHandler::Intersect(const CollisionVolume* v, const CMatrix44f& m, const float3& p0, const float3& p1, CollisionQuery* q)
{
	numContTests += 1;

	const CMatrix44f mInv = m.InvertAffine();
	const float3 pi0 = mInv.Mul(p0);
	const float3 pi1 = mInv.Mul(p1);
	bool intersect = false;

	// minimum and maximum (x, y, z) coordinates of transformed ray
	const float3 rmin = float3::min(pi0, pi1);
	const float3 rmax = float3::max(pi0, pi1);
	// minimum and maximum (x, y, z) coordinates of (bounding box around) volume
	const float3 vmin = -v->GetHScales();
	const float3 vmax =  v->GetHScales();

	// check if ray segment misses (bounding box around) volume
	// (if so, then no further intersection tests are necessary)
	if (rmax.x < vmin.x || rmin.x > vmax.x)
		return false;
	if (rmax.y < vmin.y || rmin.y > vmax.y)
		return false;
	if (rmax.z < vmin.z || rmin.z > vmax.z)
		return false;

	switch (v->GetVolumeType()) {
		case CollisionVolume::COLVOL_TYPE_ELLIPSOID:
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

	if (q != nullptr) {
		q->SwapParams();
		q->Transform(m);
	}

	return intersect;
}

bool CCollisionHandler::IntersectEllipsoid(const CollisionVolume* v, const float3& pi0, const float3& pi1, CollisionQuery* q)
{
	// transform the volume-space points into (unit) sphere-space; requires fewer
	// float-ops than solving the surface equation for arbitrary ellipsoid volumes
	const float3 upi0 = pi0 * v->GetHIScales();
	const float3 upi1 = pi1 * v->GetHIScales();
	const float rSq = 1.0f;

	if (upi0.dot(upi0) <= rSq) {
		if (q != nullptr) {
			// terminate early in the special case
			// that ray-segment originated *in* <v>
			// (these points are NOT transformed!)
			q->b0 = CQ_POINT_IN_VOL; q->p0 = ZeroVector;
			q->b1 = CQ_POINT_IN_VOL; q->p1 = ZeroVector;
		}

		return true;
	}


	// get the ray direction in unit-sphere space
	const float3 dir = (upi1 - upi0).SafeNormalize();

	// solves [ x^2 + y^2 + z^2 == r^2 ] for t; closest
	// point on ray is p(t) = p0 + (p1-p0)*t = p0 + d*t
	// (A represents dir.dot(dir), which equals 1 since
	// the ray direction is already normalized)
	// const float A = (upi1 - upi0).dot(upi1 - upi0);
	// const float B = 2.0f * upi0.dot(upi1 - upi0);
	const float A = 1.0f;
	const float B = 2.0f * upi0.dot(dir);
	const float C = upi0.dot(upi0) - rSq;
	const float D = (B * B) - (4.0f * A * C);

	if (D < -COLLISION_VOLUME_EPS)
		return false;

	// get the length of the ray segment in volume-space
	const float segLenSq = (pi1 - pi0).SqLength();

	if (D < COLLISION_VOLUME_EPS) {
		// one solution for t
		const float t0 = -B * 0.5f;
		// const float t0 = -B / (2.0f * A);
		// get the intersection point in sphere-space
		const float3 pTmp = upi0 + (dir * t0);
		// get the intersection point in volume-space
		const float3 p0 = pTmp * v->GetHScales();
		// get the distance from the start of the segment
		// to the intersection point in volume-space
		const float dSq0 = (p0 - pi0).SqLength();
		// if the intersection point is closer to p0 than
		// the end of the ray segment, the hit is valid
		const int b0 = (t0 > 0.0f && dSq0 <= segLenSq) * CQ_POINT_ON_RAY;

		if (q != nullptr) {
			q->b0 = b0; q->b1 = CQ_POINT_NO_INT;
			q->t0 = t0; q->t1 = 0.0f;
			q->p0 = p0; q->p1 = ZeroVector;
		}

		return (b0 == CQ_POINT_ON_RAY);
	}
	{
		// two solutions for t
		const float rD = math::sqrt(D);
		const float t0 = (-B - rD) * 0.5f;
		const float t1 = (-B + rD) * 0.5f;
		// const float t0 = (-B + rD) / (2.0f * A);
		// const float t1 = (-B - rD) / (2.0f * A);
		// get the intersection points in sphere-space
		const float3 pTmp0 = upi0 + (dir * t0);
		const float3 pTmp1 = upi0 + (dir * t1);
		// get the intersection points in volume-space
		const float3 p0 = pTmp0 * v->GetHScales();
		const float3 p1 = pTmp1 * v->GetHScales();
		// get the distances from the start of the ray
		// to the intersection points in volume-space
		const float dSq0 = (p0 - pi0).SqLength();
		const float dSq1 = (p1 - pi0).SqLength();
		// if one of the intersection points is closer to p0
		// than the end of the ray segment, the hit is valid
		const int b0 = (t0 > 0.0f && dSq0 <= segLenSq) * CQ_POINT_ON_RAY;
		const int b1 = (t1 > 0.0f && dSq1 <= segLenSq) * CQ_POINT_ON_RAY;

		if (q != nullptr) {
			q->b0 = b0; q->b1 = b1;
			q->t0 = t0; q->t1 = t1;
			q->p0 = p0; q->p1 = p1;
		}

		return (b0 == CQ_POINT_ON_RAY || b1 == CQ_POINT_ON_RAY);
	}
}

bool CCollisionHandler::IntersectCylinder(const CollisionVolume* v, const float3& pi0, const float3& pi1, CollisionQuery* q)
{
	const int pAx = v->GetPrimaryAxis();
	const int sAx0 = v->GetSecondaryAxis(0);
	const int sAx1 = v->GetSecondaryAxis(1);
	const float3& ahs = v->GetHScales();
	const float3& ahis = v->GetHIScales();
	const float3& ahsq = v->GetHSqScales();
	const float ratio =
		((pi0[sAx0] * pi0[sAx0]) / ahsq[sAx0]) +
		((pi0[sAx1] * pi0[sAx1]) / ahsq[sAx1]);

	if ((math::fabs(pi0[pAx]) < ahs[pAx]) && (ratio <= 1.0f)) {
		if (q != nullptr) {
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
	float3 udir;

	// ray terminals in (unit) cylinder-space
	float3 upi0 = pi0;
	float3 upi1 = pi1;

	// end-cap plane normals in volume-space
	float3 n0;
	float3 n1;

	// (unit) cylinder-space to volume-space transformation
	float3 inv = OnesVector;

	// unit-cylinder surface equation params
	float a = 0.0f;
	float b = 0.0f;
	float c = 0.0f;

	switch (pAx) {
		case CollisionVolume::COLVOL_AXIS_X: {
			upi0.y = pi0.y * ahis.y;
			upi0.z = pi0.z * ahis.z;
			upi1.y = pi1.y * ahis.y;
			upi1.z = pi1.z * ahis.z;

			inv.y = ahs.y;
			inv.z = ahs.z;
			udir = (upi1 - upi0).SafeNormalize();

			n0.x = -1.0f; // left
			n1.x =  1.0f; // right

			// yz-surface equation params
			a =  (udir.y * udir.y) + (udir.z * udir.z);
			b = ((upi0.y * udir.y) + (upi0.z * udir.z)) * 2.0f;
			c =  (upi0.y * upi0.y) + (upi0.z * upi0.z)  - 1.0f;
		} break;
		case CollisionVolume::COLVOL_AXIS_Y: {
			upi0.x = pi0.x * ahis.x;
			upi0.z = pi0.z * ahis.z;
			upi1.x = pi1.x * ahis.x;
			upi1.z = pi1.z * ahis.z;

			inv.x = ahs.x;
			inv.z = ahs.z;
			udir = (upi1 - upi0).SafeNormalize();

			n0.y =  1.0f; // top
			n1.y = -1.0f; // bottom

			// xz-surface equation params
			a =  (udir.x * udir.x) + (udir.z * udir.z);
			b = ((upi0.x * udir.x) + (upi0.z * udir.z)) * 2.0f;
			c =  (upi0.x * upi0.x) + (upi0.z * upi0.z)  - 1.0f;
		} break;
		case CollisionVolume::COLVOL_AXIS_Z: {
			upi0.x = pi0.x * ahis.x;
			upi0.y = pi0.y * ahis.y;
			upi1.x = pi1.x * ahis.x;
			upi1.y = pi1.y * ahis.y;

			inv.x = ahs.x;
			inv.y = ahs.y;
			udir = (upi1 - upi0).SafeNormalize();

			n0.z =  1.0f; // front
			n1.z = -1.0f; // back

			// xy-surface equation params
			a =  (udir.x * udir.x) + (udir.y * udir.y);
			b = ((upi0.x * udir.x) + (upi0.y * udir.y)) * 2.0f;
			c =  (upi0.x * upi0.x) + (upi0.y * upi0.y)  - 1.0f;
		} break;
	}

	// volume-space intersection points
	float3 p0;
	float3 p1;

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
				p0 = (upi0 + (udir * t0)) * inv;
				s0 = (p0 - pi0).SqLength();
				b0 = (s0 < segLenSq  &&  math::fabs(p0[pAx]) < ahs[pAx]) * CQ_POINT_ON_RAY;
			} else {
				rd = math::sqrt(d);
				t0 = (-b - rd) / (2.0f * a);
				t1 = (-b + rd) / (2.0f * a);
				p0 = (upi0 + (udir * t0)) * inv;
				p1 = (upi0 + (udir * t1)) * inv;
				s0 = (p0 - pi0).SqLength();
				s1 = (p1 - pi0).SqLength();
				b0 = (s0 < segLenSq  &&  math::fabs(p0[pAx]) < ahs[pAx]) * CQ_POINT_ON_RAY;
				b1 = (s1 < segLenSq  &&  math::fabs(p1[pAx]) < ahs[pAx]) * CQ_POINT_ON_RAY;
			}
		} else {
			if (b != 0.0f) {
				// linear eq.; one surface intersection
				t0 = -c / b;
				p0 = (upi0 + (udir * t0)) * inv;
				s0 = (p0 - pi0).SqLength();
				b0 = (s0 < segLenSq  &&  math::fabs(p0[pAx]) < ahs[pAx]) * CQ_POINT_ON_RAY;
			}
		}
	}

	if (b0 == CQ_POINT_NO_INT) {
		// p0 does not lie on ray segment, or does not fall
		// between cylinder end-caps: check if segment goes
		// through front cap (plane) in unit-volume space
		// NOTE: normal n0 and dir should not be orthogonal
		dp = n0.dot(udir);
		rd = (dp != 0.0f)? 1.0f / dp: 0.01f;

		t0 = -(n0.dot(upi0) - ahs[pAx]) * rd;
		p0 = (upi0 + (udir * t0)) * inv;
		s0 = (p0 - pi0).SqLength();
		ra =
			(((p0[sAx0] * p0[sAx0]) / ahsq[sAx0]) +
			 ((p0[sAx1] * p0[sAx1]) / ahsq[sAx1]));
		b0 = (t0 >= 0.0f && ra <= 1.0f && s0 <= segLenSq) * CQ_POINT_ON_RAY;
	}
	if (b1 == CQ_POINT_NO_INT) {
		// p1 does not lie on ray segment, or does not fall
		// between cylinder end-caps: check if segment goes
		// through rear cap (plane) in unit-volume space
		// NOTE: normal n1 and dir should not be orthogonal
		dp = n1.dot(udir);
		rd = (dp != 0.0f)? 1.0f / dp: 0.01f;

		t1 = -(n1.dot(upi0) - ahs[pAx]) * rd;
		p1 = (upi0 + (udir * t1)) * inv;
		s1 = (p1 - pi0).SqLength();
		ra =
			(((p1[sAx0] * p1[sAx0]) / ahsq[sAx0]) +
			 ((p1[sAx1] * p1[sAx1]) / ahsq[sAx1]));
		b1 = (t1 >= 0.0f && ra <= 1.0f && s1 <= segLenSq) * CQ_POINT_ON_RAY;
	}

	if (q != nullptr) {
		q->b0 = b0; q->b1 = b1;
		q->t0 = t0; q->t1 = t1;
		q->p0 = p0; q->p1 = p1;
	}

	return (b0 == CQ_POINT_ON_RAY || b1 == CQ_POINT_ON_RAY);
}

bool CCollisionHandler::IntersectBox(const CollisionVolume* v, const float3& pi0, const float3& pi1, CollisionQuery* q)
{
	const float3& ahs = v->GetHScales();

	const bool ba = (math::fabs(pi0.x) < ahs.x);
	const bool bb = (math::fabs(pi0.y) < ahs.y);
	const bool bc = (math::fabs(pi0.z) < ahs.z);

	if (ba && bb && bc) {
		// terminate early in the special case
		// that ray-segment originated within v
		if (q != nullptr) {
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
		if (math::fabs(pi0.x) > ahs.x)
			return false;

	} else {
		if (dir.x > 0.0f) {
			t0 = (-ahs.x - pi0.x) / dir.x;
			t1 = ( ahs.x - pi0.x) / dir.x;
		} else {
			t1 = (-ahs.x - pi0.x) / dir.x;
			t0 = ( ahs.x - pi0.x) / dir.x;
		}

		if (t0 > t1) { t2 = t1; t1 = t0; t0 = t2; }
		if (t0 > tn) { tn = t0; }
		if (t1 < tf) { tf = t1; }
		if (tn > tf) { return false; }
		if (tf < 0.0f) { return false; }
	}

	if (math::fabs(dir.y) < COLLISION_VOLUME_EPS) {
		if (math::fabs(pi0.y) > ahs.y) {
			return false;
		}
	} else {
		if (dir.y > 0.0f) {
			t0 = (-ahs.y - pi0.y) / dir.y;
			t1 = ( ahs.y - pi0.y) / dir.y;
		} else {
			t1 = (-ahs.y - pi0.y) / dir.y;
			t0 = ( ahs.y - pi0.y) / dir.y;
		}

		if (t0 > t1) { t2 = t1; t1 = t0; t0 = t2; }
		if (t0 > tn) { tn = t0; }
		if (t1 < tf) { tf = t1; }
		if (tn > tf) { return false; }
		if (tf < 0.0f) { return false; }
	}

	if (math::fabs(dir.z) < COLLISION_VOLUME_EPS) {
		if (math::fabs(pi0.z) > ahs.z) {
			return false;
		}
	} else {
		if (dir.z > 0.0f) {
			t0 = (-ahs.z - pi0.z) / dir.z;
			t1 = ( ahs.z - pi0.z) / dir.z;
		} else {
			t1 = (-ahs.z - pi0.z) / dir.z;
			t0 = ( ahs.z - pi0.z) / dir.z;
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

	if (q != nullptr) {
		q->b0 = b0; q->b1 = b1;
		q->t0 = tn; q->t1 = tf;
		q->p0 = p0; q->p1 = p1;
	}

	return (b0 == CQ_POINT_ON_RAY || b1 == CQ_POINT_ON_RAY);
}

