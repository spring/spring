#include "StdAfx.h"
#include "mmgr.h"

#include "FastMath.h"
#include "float3.h"
#include "Matrix44f.h"

#include "Sim/Units/Unit.h"
#include "Sim/Features/Feature.h"
#include "GroundBlockingObjectMap.h"

#include "GlobalSynced.h"
#include "GlobalConstants.h"

#include "CollisionHandler.h"
#include "CollisionVolume.h"

#define ZVEC ZeroVector

CR_BIND(CCollisionHandler, );

unsigned int CCollisionHandler::numCollisionTests = 0;
unsigned int CCollisionHandler::numIntersectionTests = 0;



bool CCollisionHandler::DetectHit(const CUnit* u, const float3& p0, const float3& p1, CollisionQuery* q)
{
	bool r = false;

	if (u->unitDef->usePieceCollisionVolumes) {
		// test only for ray intersections with the piece tree
		return (CCollisionHandler::IntersectPieceTree(u, p0, p1, q));
	}

	if (!u->collisionVolume->IsDisabled()) {
		switch (u->collisionVolume->GetTestType()) {
			// Collision(CUnit*) does not need p1 or q
			case COLVOL_TEST_DISC: { r = CCollisionHandler::Collision(u, p0       ); numCollisionTests    += 1; } break;
			case COLVOL_TEST_CONT: { r = CCollisionHandler::Intersect(u, p0, p1, q); numIntersectionTests += 1; } break;
		}
	}

	return r;
}

bool CCollisionHandler::DetectHit(const CFeature* f, const float3& p0, const float3& p1, CollisionQuery* q)
{
	bool r = false;

	if (!f->collisionVolume->IsDisabled()) {
		switch (f->collisionVolume->GetTestType()) {
			// Collision(CFeature*) does not need p1 or q
			case COLVOL_TEST_DISC: { r = CCollisionHandler::Collision(f, p0       ); numCollisionTests    += 1; } break;
			case COLVOL_TEST_CONT: { r = CCollisionHandler::Intersect(f, p0, p1, q); numIntersectionTests += 1; } break;
		}
	}

	return r;
}






bool CCollisionHandler::Collision(const CUnit* u, const float3& p)
{
	const CollisionVolume* v = u->collisionVolume;

	if (((u->midPos + v->GetOffsets()) - p).SqLength() > v->GetBoundingRadiusSq()) {
		return false;
	}

	switch (u->collisionVolume->GetVolumeType()) {
		case COLVOL_TYPE_SPHERE: {
			return true;
		}
		case COLVOL_TYPE_FOOTPRINT: {
			return CCollisionHandler::CollisionFootprint(u, p);
		}
		default: {
			// NOTE: we have to translate by relMidPos to get to midPos
			// (which is where the collision volume gets drawn) because
			// GetTransformMatrix() only uses pos
			CMatrix44f m = u->GetTransformMatrix(true);
			m.Translate(u->relMidPos);
			m.Translate(v->GetOffsets());

			return CCollisionHandler::Collision(v, m, p);
		}
	}
}


bool CCollisionHandler::Collision(const CFeature* f, const float3& p)
{
	const CollisionVolume* v = f->collisionVolume;

	if (((f->midPos + v->GetOffsets()) - p).SqLength() > v->GetBoundingRadiusSq()) {
		return false;
	}

	switch (f->collisionVolume->GetVolumeType()) {
		case COLVOL_TYPE_SPHERE: {
			return true;
		}
		case COLVOL_TYPE_FOOTPRINT: {
			return CCollisionHandler::CollisionFootprint(f, p);
		}
		default: {
			// NOTE: CFeature does not have a relMidPos member so
			// calculate and apply the translation from pos (used
			// by transMatrix) to midPos manually
			const float3 relMidPos(f->midPos - f->pos);

			CMatrix44f m(f->transMatrix);
			m.Translate(relMidPos);
			m.Translate(v->GetOffsets());

			return CCollisionHandler::Collision(v, m, p);
		}
	}
}


bool CCollisionHandler::CollisionFootprint(const CSolidObject* o, const float3& p)
{
	if (o->isMarkedOnBlockingMap && o->physicalState != CSolidObject::Flying) {
		const float invSquareSize = 1.0f / SQUARE_SIZE;
		const int square = int(p.x * invSquareSize) + int(p.z * invSquareSize) * gs->mapx;

		if (square >= 0 && square < gs->mapSquares) {
			const BlockingMapCell& cell = groundBlockingObjectMap->GetCell(square);
			return cell.find(o->GetBlockingMapID()) != cell.end();
		}
	}
	// If the object isn't marked on blocking map, or it is flying,
	// effecively only the sphere check (in Collision(CUnit*) or
	// Collision(CFeature*)) is performed.
	return true;
}


// test if point <p> (in world-coors) lies inside the
// volume whose transformation matrix is given by <m>
bool CCollisionHandler::Collision(const CollisionVolume* v, const CMatrix44f& m, const float3& p)
{
	// get the inverse volume transformation matrix and
	// apply it to the projectile's position, then test
	// if the transformed position lies within the axis-
	// aligned collision volume
	CMatrix44f mInv = m.Invert();
	float3 pi = mInv.Mul(p);
	bool hit = false;

	switch (v->GetVolumeType()) {
		case COLVOL_TYPE_SPHERE: {
			// normally, this code is never executed, because the higher level
			// Collision(CFeature*) and Collision(CUnit*) already optimize
			// for volumeType == COLVOL_TYPE_SPHERE.
			hit = (pi.dot(pi) <= v->GetHScalesSq().x);
		} break;
		case COLVOL_TYPE_ELLIPSOID: {
			const float f1 = (pi.x * pi.x) / v->GetHScalesSq().x;
			const float f2 = (pi.y * pi.y) / v->GetHScalesSq().y;
			const float f3 = (pi.z * pi.z) / v->GetHScalesSq().z;
			hit = ((f1 + f2 + f3) <= 1.0f);
		} break;
		case COLVOL_TYPE_CYLINDER: {
			switch (v->GetPrimaryAxis()) {
				case COLVOL_AXIS_X: {
					const bool xPass = (pi.x > -v->GetHScales().x  &&  pi.x < v->GetHScales().x);
					const float yRat = (pi.y * pi.y) / v->GetHScalesSq().y;
					const float zRat = (pi.z * pi.z) / v->GetHScalesSq().z;
					hit = (xPass && (yRat + zRat <= 1.0f));
				} break;
				case COLVOL_AXIS_Y: {
					const bool yPass = (pi.y > -v->GetHScales().y  &&  pi.y < v->GetHScales().y);
					const float xRat = (pi.x * pi.x) / v->GetHScalesSq().x;
					const float zRat = (pi.z * pi.z) / v->GetHScalesSq().z;
					hit = (yPass && (xRat + zRat <= 1.0f));
				} break;
				case COLVOL_AXIS_Z: {
					const bool zPass = (pi.z > -v->GetHScales().z  &&  pi.z < v->GetHScales().z);
					const float xRat = (pi.x * pi.x) / v->GetHScalesSq().x;
					const float yRat = (pi.y * pi.y) / v->GetHScalesSq().y;
					hit = (zPass && (xRat + yRat <= 1.0f));
				} break;
			}
		} break;
		case COLVOL_TYPE_BOX: {
			const bool b1 = (pi.x > -v->GetHScales().x  &&  pi.x < v->GetHScales().x);
			const bool b2 = (pi.y > -v->GetHScales().y  &&  pi.y < v->GetHScales().y);
			const bool b3 = (pi.z > -v->GetHScales().z  &&  pi.z < v->GetHScales().z);
			hit = (b1 && b2 && b3);
		} break;
	}

	return hit;
}


bool CCollisionHandler::MouseHit(const CUnit* u, const float3& p0, const float3& p1, const CollisionVolume* v, CollisionQuery* q)
{
	// note: use the piece tree?
	CMatrix44f m = u->GetTransformMatrix(false, true);
	m.Translate(u->relMidPos + v->GetOffsets());

	return CCollisionHandler::Intersect(v, m, p0, p1, q);
}


void CCollisionHandler::IntersectPieceTreeHelper(
	LocalModelPiece* lmp,
	CMatrix44f mat,
	const float3& p0,
	const float3& p1,
	std::list<CollisionQuery>* hits)
{
	mat.Translate(lmp->pos);
	mat.RotateY(-lmp->rot[1]);
	mat.RotateX(-lmp->rot[0]);
	mat.RotateZ( lmp->rot[2]);

	const CollisionVolume* vol = lmp->colvol;
	const float3& offset = vol->GetOffsets();

	mat.Translate(offset);

	if (lmp->visible && !vol->IsDisabled()) {
		CollisionQuery q;

		if (CCollisionHandler::Intersect(vol, mat, p0, p1, &q)) {
			q.lmp = lmp;
			hits->push_back(q);
		}
	}

	mat.Translate(-offset);

	for (unsigned int i = 0; i < lmp->childs.size(); i++) {
		IntersectPieceTreeHelper(lmp->childs[i], mat, p0, p1, hits);
	}
}

bool CCollisionHandler::IntersectPieceTree(const CUnit* u, const float3& p0, const float3& p1, CollisionQuery* q)
{
	std::list<CollisionQuery> hits;
	std::list<CollisionQuery>::const_iterator hitsIt;

	// this probably needs an early-out test
	CMatrix44f mat = u->GetTransformMatrix(true);
	IntersectPieceTreeHelper(u->localmodel->pieces[0], mat, p0, p1, &hits);

	float dstNearSq = 1e30f;

	// save the closest intersection
	for (hitsIt = hits.begin(); hitsIt != hits.end(); hitsIt++) {
		const CollisionQuery& qTmp = *hitsIt;
		const float dstSq = (qTmp.p0 - p0).SqLength();

		if (q != NULL && dstSq < dstNearSq) {
			dstNearSq = dstSq;

			q->b0 = qTmp.b0; q->t0 = qTmp.t0; q->p0 = qTmp.p0;
			q->b1 = qTmp.b1; q->t1 = qTmp.t1; q->p1 = qTmp.p1;

			q->lmp = qTmp.lmp;
		}
	}

	return (!hits.empty());
}


bool CCollisionHandler::Intersect(const CUnit* u, const float3& p0, const float3& p1, CollisionQuery* q)
{
	const CollisionVolume* v = u->collisionVolume;

	CMatrix44f m = u->GetTransformMatrix(true);
	m.Translate(u->relMidPos);
	m.Translate(v->GetOffsets());

	return CCollisionHandler::Intersect(v, m, p0, p1, q);
}

bool CCollisionHandler::Intersect(const CFeature* f, const float3& p0, const float3& p1, CollisionQuery* q)
{
	const CollisionVolume* v = f->collisionVolume;
	const float3 relMidPos(f->midPos - f->pos);

	CMatrix44f m(f->transMatrix);
	m.Translate(relMidPos);
	m.Translate(v->GetOffsets());

	return CCollisionHandler::Intersect(v, m, p0, p1, q);
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



// test if ray from <p0> to <p1> (in world-coors) intersects
// the volume whose transformation matrix is given by <m>
bool CCollisionHandler::Intersect(const CollisionVolume* v, const CMatrix44f& m, const float3& p0, const float3& p1, CollisionQuery* q)
{
	if (q) {
		// reset the query
		q->b0 = false; q->t0 = 0.0f; q->p0 = ZVEC;
		q->b1 = false; q->t1 = 0.0f; q->p1 = ZVEC;
	}

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
		case COLVOL_TYPE_FOOTPRINT:
			// fall through, intersection with footprint collision volume
			// is not supported yet, so only test against sphere/ellipsoid
		case COLVOL_TYPE_SPHERE:
			// fall through, sphere is special case of ellipsoid
		case COLVOL_TYPE_ELLIPSOID: {
			intersect = CCollisionHandler::IntersectEllipsoid(v, pi0, pi1, q);
		} break;
		case COLVOL_TYPE_CYLINDER: {
			intersect = CCollisionHandler::IntersectCylinder(v, pi0, pi1, q);
		} break;
		case COLVOL_TYPE_BOX: {
			intersect = CCollisionHandler::IntersectBox(v, pi0, pi1, q);
		} break;
	}

	if (q) {
		// transform the intersection points
		if (q->b0) { q->p0 = m.Mul(q->p0); }
		if (q->b1) { q->p1 = m.Mul(q->p1); }
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

	if (pii0.dot(pii0) <= rSq /* && pii1.dot(pii1) <= rSq */) {
		// terminate early in the special case
		// that shot originated within volume
		q->b0 = true; q->p0 = ZVEC;
		q->b1 = true; q->p1 = ZVEC;
		return true;
	}

	// get the ray direction in unit-sphere space
	const float3 dir = (pii1 - pii0).Normalize();

	// solves [ x^2 + y^2 + z^2 == r^2 ] for t
	// (<A> represents dir.dot(dir), equal to 1
	// since ray direction already normalized)
	const float A = 1.0f;
	const float B = (pii0 * 2.0f).dot(dir);
	const float C = pii0.dot(pii0) - rSq;
	const float D = (B * B) - (4.0f * A * C);

	if (D < -EPS) {
		return false;
	} else {
		// get the length of the ray segment in volume-space
		const float segLenSq = (pi1 - pi0).SqLength();

		if (D < EPS) {
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
			const bool b0 = (t0 > 0.0f && dSq0 <= segLenSq);

			if (q) {
				q->b0 = b0; q->b1 = false;
				q->t0 = t0; q->t1 = 0.0f;
				q->p0 = p0; q->p1 = ZVEC;
			}

			return b0;
		} else {
			// two solutions for t
			const float rD = fastmath::sqrt(D);
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
			const bool b0 = (t0 > 0.0f && dSq0 <= segLenSq);
			const bool b1 = (t1 > 0.0f && dSq1 <= segLenSq);

			if (q) {
				q->b0 = b0; q->b1 = b1;
				q->t0 = t0; q->t1 = t1;
				q->p0 = p0; q->p1 = p1;
			}

			return (b0 || b1);
		}
	}
}

bool CCollisionHandler::IntersectCylinder(const CollisionVolume* v, const float3& pi0, const float3& pi1, CollisionQuery* q)
{
	const int pAx = v->GetPrimaryAxis();
	const int sAx0 = v->GetSecondaryAxis(0);
	const int sAx1 = v->GetSecondaryAxis(1);
	const float3& ahs = v->GetHScales();
	const float3& ahsq = v->GetHScalesSq();
	const float ratio =
		((pi0[sAx0] * pi0[sAx0]) / ahsq[sAx0]) +
		((pi0[sAx1] * pi0[sAx1]) / ahsq[sAx1]);

	if ((pi0[pAx] > -ahs[pAx] && pi0[pAx] < ahs[pAx]) && ratio <= 1.0f) {
		// terminate early in the special case
		// that shot originated within volume
		q->b0 = true; q->p0 = ZVEC;
		q->b1 = true; q->p1 = ZVEC;
		return true;
	}

	// ray direction in volume-space
	const float3 dir = (pi1 - pi0).Normalize();

	// ray direction in (unit) cylinder-space
	float3 diir = ZVEC;

	// ray terminals in (unit) cylinder-space
	float3 pii0 = pi0;
	float3 pii1 = pi1;

	// end-cap plane normals in volume-space
	float3 n0 = ZVEC;
	float3 n1 = ZVEC;

	// (unit) cylinder-space to volume-space transformation
	float3 inv(1.0f, 1.0f, 1.0f);

	// unit-cylinder surface equation params
	float a = 0.0f;
	float b = 0.0f;
	float c = 0.0f;

	switch (pAx) {
		case COLVOL_AXIS_X: {
			pii0.y = pi0.y * v->GetHIScales().y;
			pii0.z = pi0.z * v->GetHIScales().z;
			pii1.y = pi1.y * v->GetHIScales().y;
			pii1.z = pi1.z * v->GetHIScales().z;

			inv.y = v->GetHScales().y;
			inv.z = v->GetHScales().z;
			diir = (pii1 - pii0).Normalize();

			n0.x = -1.0f; // left
			n1.x =  1.0f; // right

			// yz-surface equation params
			a =  (diir.y * diir.y) + (diir.z * diir.z);
			b = ((pii0.y * diir.y) + (pii0.z * diir.z)) * 2.0f;
			c =  (pii0.y * pii0.y) + (pii0.z * pii0.z)  - 1.0f;
		} break;
		case COLVOL_AXIS_Y: {
			pii0.x = pi0.x * v->GetHIScales().x;
			pii0.z = pi0.z * v->GetHIScales().z;
			pii1.x = pi1.x * v->GetHIScales().x;
			pii1.z = pi1.z * v->GetHIScales().z;

			inv.x = v->GetHScales().x;
			inv.z = v->GetHScales().z;
			diir = (pii1 - pii0).Normalize();

			n0.y =  1.0f; // top
			n1.y = -1.0f; // bottom

			// xz-surface equation params
			a =  (diir.x * diir.x) + (diir.z * diir.z);
			b = ((pii0.x * diir.x) + (pii0.z * diir.z)) * 2.0f;
			c =  (pii0.x * pii0.x) + (pii0.z * pii0.z)  - 1.0f;
		} break;
		case COLVOL_AXIS_Z: {
			pii0.x = pi0.x * v->GetHIScales().x;
			pii0.y = pi0.y * v->GetHIScales().y;
			pii1.x = pi1.x * v->GetHIScales().x;
			pii1.y = pi1.y * v->GetHIScales().y;

			inv.x = v->GetHScales().x;
			inv.y = v->GetHScales().y;
			diir = (pii1 - pii0).Normalize();

			n0.z =  1.0f; // front
			n1.z = -1.0f; // back

			// xy-surface equation params
			a =  (diir.x * diir.x) + (diir.y * diir.y);
			b = ((pii0.x * diir.x) + (pii0.y * diir.y)) * 2.0f;
			c =  (pii0.x * pii0.x) + (pii0.y * pii0.y)  - 1.0f;
		} break;
	}

	float d = (b * b) - (4.0f * a * c);
	float rd = 0.0f;
	float dp = 0.0f, rdp = 0.0f;

	// volume-space intersection points
	float3 p0 = ZVEC;
	float3 p1 = ZVEC;

	bool b0 = false;
	bool b1 = false;
	float r0 = 0.0f, s0 = 0.0f, t0 = 0.0f;
	float r1 = 0.0f, s1 = 0.0f, t1 = 0.0f;

	// get the length of the ray segment in volume-space
	const float segLenSq = (pi1 - pi0).SqLength();

	if (d >= -EPS) {
		// one or two surface intersections
		if (d < EPS) {
			t0 = -b / (2.0f * a);
			p0 = (pii0 + (diir * t0)) * inv;
			s0 = (p0 - pi0).SqLength();
			b0 = (s0 < segLenSq && (p0[pAx] > -ahs[pAx] && p0[pAx] < ahs[pAx]));
		} else {
			rd = fastmath::sqrt(d);
			t0 = (-b - rd) / (2.0f * a);
			t1 = (-b + rd) / (2.0f * a);
			p0 = (pii0 + (diir * t0)) * inv;
			p1 = (pii0 + (diir * t1)) * inv;
			s0 = (p0 - pi0).SqLength();
			s1 = (p1 - pi0).SqLength();
			b0 = (s0 < segLenSq && (p0[pAx] > -ahs[pAx] && p0[pAx] < ahs[pAx]));
			b1 = (s1 < segLenSq && (p1[pAx] > -ahs[pAx] && p1[pAx] < ahs[pAx]));
		}
	}

	if (!b0) {
		// p0 does not lie on ray segment, or does not fall
		// between cylinder end-caps: check if segment goes
		// through front cap (plane)
		// NOTE: normal n0 and dir should not be orthogonal
		dp = n0.dot(dir);
		rdp = (dp != 0.0f)? 1.0f / dp: 0.01f;

		t0 = -(n0.dot(pi0) - ahs[pAx]) * rdp;
		p0 = pi0 + (dir * t0);
		s0 = (p0 - pi0).SqLength();
		r0 =
			(((p0[sAx0] * p0[sAx0]) / ahsq[sAx0]) +
			 ((p0[sAx1] * p0[sAx1]) / ahsq[sAx1]));
		b0 = (t0 >= 0.0f && r0 <= 1.0f && s0 <= segLenSq);
	}
	if (!b1) {
		// p1 does not lie on ray segment, or does not fall
		// between cylinder end-caps: check if segment goes
		// through rear cap (plane)
		// NOTE: normal n1 and dir should not be orthogonal
		dp = n1.dot(dir);
		rdp = (dp != 0.0f)? 1.0f / dp: 0.01f;

		t1 = -(n1.dot(pi0) - ahs[pAx]) * rdp;
		p1 = pi0 + (dir * t1);
		s1 = (p1 - pi0).SqLength();
		r1 =
			(((p1[sAx0] * p1[sAx0]) / ahsq[sAx0]) +
			 ((p1[sAx1] * p1[sAx1]) / ahsq[sAx1]));
		b1 = (t1 >= 0.0f && r1 <= 1.0f && s1 <= segLenSq);
	}

	if (q) {
		q->b0 = b0; q->b1 = b1;
		q->t0 = t0; q->t1 = t1;
		q->p0 = p0; q->p1 = p1;
	}

	return (b0 || b1);
}

bool CCollisionHandler::IntersectBox(const CollisionVolume* v, const float3& pi0, const float3& pi1, CollisionQuery* q)
{
	const bool ba = (pi0.x > -v->GetHScales().x  &&  pi0.x < v->GetHScales().x);
	const bool bb = (pi0.y > -v->GetHScales().y  &&  pi0.y < v->GetHScales().y);
	const bool bc = (pi0.z > -v->GetHScales().z  &&  pi0.z < v->GetHScales().z);

	if ((ba && bb && bc) /* && (bd && be && bf) */) {
		// terminate early in the special case
		// that shot originated within volume
		q->b0 = true; q->p0 = ZVEC;
		q->b1 = true; q->p1 = ZVEC;
		return true;
	}

	float tn = -9999999.9f;
	float tf =  9999999.9f;
	float t0 =  0.0f;
	float t1 =  0.0f;
	float t2 =  0.0f;

	const float3 dir = (pi1 - pi0).Normalize();

	if (dir.x > -EPS && dir.x < EPS) {
		if (pi0.x < -v->GetHScales().x  ||  pi0.x > v->GetHScales().x) {
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

	if (dir.y > -EPS && dir.y < EPS) {
		if (pi0.y < -v->GetHScales().y  ||  pi0.y > v->GetHScales().y) {
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

	if (dir.z > -EPS && dir.z < EPS) {
		if (pi0.z < -v->GetHScales().z  ||  pi0.z > v->GetHScales().z) {
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
	const bool b0 = (dSq0 <= segLenSq);
	const bool b1 = (dSq1 <= segLenSq);

	if (q) {
		q->b0 = b0; q->b1 = b1;
		q->t0 = tn; q->t1 = tf;
		q->p0 = p0; q->p1 = p1;
	}

	return (b0 || b1);
}
