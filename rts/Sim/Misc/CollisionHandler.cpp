#include <iostream>

#include "System/FastMath.h"
#include "System/float3.h"
#include "System/Matrix44f.h"

#include "Sim/Units/Unit.h"
#include "Sim/Features/Feature.h"

#include "CollisionHandler.h"
#include "CollisionVolume.h"

#define ZVec ZeroVector

CR_BIND(CCollisionHandler, );
CR_BIND(CollisionVolume, );
	CR_REG_METADATA(CollisionVolume, (
		CR_MEMBER(axisScales),
		CR_MEMBER(axisHScales),
		CR_MEMBER(axisHScalesSq),
		CR_MEMBER(axisHIScales),
		CR_MEMBER(axisOffsets),
		CR_MEMBER(volumeBoundingRadius),
		CR_MEMBER(volumeBoundingRadiusSq),
		CR_MEMBER(volumeType),
		CR_MEMBER(testType),
		CR_MEMBER(primaryAxis),
		CR_MEMBER(secondaryAxes),
		CR_MEMBER(spherical)
	));

unsigned int CCollisionHandler::numCollisionTests = 0;
unsigned int CCollisionHandler::numIntersectionTests = 0;



bool CCollisionHandler::DetectHit(const CUnit* u, const float3& p0, const float3& p1, CollisionQuery* q)
{
	bool r = false;

	switch (u->collisionVolume->testType) {
		// Collision(CUnit*) does not need p1 or q
		case COLVOL_TEST_DISC: { r = CCollisionHandler::Collision(u, p0       ); numCollisionTests    += 1; } break;
		case COLVOL_TEST_CONT: { r = CCollisionHandler::Intersect(u, p0, p1, q); numIntersectionTests += 1; } break;
	}

	return r;
}

bool CCollisionHandler::DetectHit(const CFeature* f, const float3& p0, const float3& p1, CollisionQuery* q)
{
	bool r = false;

	switch (f->collisionVolume->testType) {
		// Collision(CFeature*) does not need p1 or q
		case COLVOL_TEST_DISC: { r = CCollisionHandler::Collision(f, p0       ); numCollisionTests    += 1; } break;
		case COLVOL_TEST_CONT: { r = CCollisionHandler::Intersect(f, p0, p1, q); numIntersectionTests += 1; } break;
	}

	return r;
}






bool CCollisionHandler::Collision(const CUnit* u, const float3& p)
{
	const CollisionVolume* v = u->collisionVolume;

	if ((u->midPos - p).SqLength() > v->volumeBoundingRadiusSq) {
		return false;
	} else {
		if (v->spherical) {
			return true;
		} else {
			// NOTE: we have to translate by relMidPos (which is where
			// collision volume gets drawn) since GetTransformMatrix()
			// does not
			CMatrix44f m;
			u->GetTransformMatrix(m);
			m.Translate(u->relMidPos.x, u->relMidPos.y, u->relMidPos.z);
			m.Translate(v->axisOffsets.x, v->axisOffsets.y, v->axisOffsets.z);

			return CCollisionHandler::Collision(v, m, p);
		}
	}
}

bool CCollisionHandler::Collision(const CFeature* f, const float3& p)
{
	const CollisionVolume* v = f->collisionVolume;

	if ((f->midPos - p).SqLength() > v->volumeBoundingRadiusSq) {
		return false;
	} else {
		if (v->spherical) {
			return true;
		} else {
			// NOTE: CFeature does not have a relMidPos member
			CMatrix44f m(f->transMatrix);
			m.Translate(v->axisOffsets.x, v->axisOffsets.y, v->axisOffsets.z);

			return CCollisionHandler::Collision(v, m, p);
		}
	}
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

	switch (v->volumeType) {
		case COLVOL_TYPE_ELLIPSOID: {
			if (v->spherical) {
				hit = (pi.dot(pi) <= v->axisHScalesSq.x);
			} else {
				const float f1 = (pi.x * pi.x) / v->axisHScalesSq.x;
				const float f2 = (pi.y * pi.y) / v->axisHScalesSq.y;
				const float f3 = (pi.z * pi.z) / v->axisHScalesSq.z;
				hit = ((f1 + f2 + f3) <= 1.0f);
			}
		} break;
		case COLVOL_TYPE_CYLINDER: {
			switch (v->primaryAxis) {
				case COLVOL_AXIS_X: {
					const bool xPass = (pi.x > -v->axisHScales.x  &&  pi.x < v->axisHScales.x);
					const float yRat = (pi.y * pi.y) / v->axisHScalesSq.y;
					const float zRat = (pi.z * pi.z) / v->axisHScalesSq.z;
					hit = (xPass && (yRat + zRat <= 1.0f));
				} break;
				case COLVOL_AXIS_Y: {
					const bool yPass = (pi.y > -v->axisHScales.y  &&  pi.y < v->axisHScales.y);
					const float xRat = (pi.x * pi.x) / v->axisHScalesSq.x;
					const float zRat = (pi.z * pi.z) / v->axisHScalesSq.z;
					hit = (yPass && (xRat + zRat <= 1.0f));
				} break;
				case COLVOL_AXIS_Z: {
					const bool zPass = (pi.z > -v->axisHScales.z  &&  pi.z < v->axisHScales.z);
					const float xRat = (pi.x * pi.x) / v->axisHScalesSq.x;
					const float yRat = (pi.y * pi.y) / v->axisHScalesSq.y;
					hit = (zPass && (xRat + yRat <= 1.0f));
				} break;
			}
		} break;
		case COLVOL_TYPE_BOX: {
			const bool b1 = (pi.x > -v->axisHScales.x  &&  pi.x < v->axisHScales.x);
			const bool b2 = (pi.y > -v->axisHScales.y  &&  pi.y < v->axisHScales.y);
			const bool b3 = (pi.z > -v->axisHScales.z  &&  pi.z < v->axisHScales.z);
			hit = (b1 && b2 && b3);
		} break;
	}

	return hit;
}






bool CCollisionHandler::Intersect(const CUnit* u, const float3& p0, const float3& p1, CollisionQuery* q)
{
	const CollisionVolume* v = u->collisionVolume;

	CMatrix44f m;
	u->GetTransformMatrix(m);
	m.Translate(u->relMidPos.x, u->relMidPos.y, u->relMidPos.z);
	m.Translate(v->axisOffsets.x, v->axisOffsets.y, v->axisOffsets.z);

	return CCollisionHandler::Intersect(v, m, p0, p1, q);
}

bool CCollisionHandler::Intersect(const CFeature* f, const float3& p0, const float3& p1, CollisionQuery* q)
{
	const CollisionVolume* v = f->collisionVolume;

	CMatrix44f m(f->transMatrix);
	m.Translate(v->axisOffsets.x, v->axisOffsets.y, v->axisOffsets.z);

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
	CMatrix44f mInv = m.Invert();
	const float3 pi0 = mInv.Mul(p0);
	const float3 pi1 = mInv.Mul(p1);
	bool intersect = false;

	// minimum and maximum (x, y, z) coordinates of transformed ray
	const float rminx = MIN(pi0.x, pi1.x), rminy = MIN(pi0.y, pi1.y), rminz = MIN(pi0.z, pi1.z);
	const float rmaxx = MAX(pi0.x, pi1.x), rmaxy = MAX(pi0.y, pi1.y), rmaxz = MAX(pi0.z, pi1.z);

	// minimum and maximum (x, y, z) coordinates of (bounding box around) volume
	const float vminx = -v->axisHScales.x, vminy = -v->axisHScales.y, vminz = -v->axisHScales.z;
	const float vmaxx =  v->axisHScales.x, vmaxy =  v->axisHScales.y, vmaxz =  v->axisHScales.z;

	// check if ray segment misses (bounding box around) volume
	// (if so, then no further intersection tests are necessary)
	if (rmaxx < vminx || rminx > vmaxx) { return false; }
	if (rmaxy < vminy || rminy > vmaxy) { return false; }
	if (rmaxz < vminz || rminz > vmaxz) { return false; }

	switch (v->volumeType) {
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

	return intersect;
}

bool CCollisionHandler::IntersectEllipsoid(const CollisionVolume* v, const float3& pi0, const float3& pi1, CollisionQuery* q)
{
	// transform the volume-space points into (unit) sphere-space (requires fewer
	// float-ops than solving the surface equation for arbitrary ellipsoid volumes)
	const float3 pii0 = float3(pi0.x * v->axisHIScales.x, pi0.y * v->axisHIScales.y, pi0.z * v->axisHIScales.z);
	const float3 pii1 = float3(pi1.x * v->axisHIScales.x, pi1.y * v->axisHIScales.y, pi1.z * v->axisHIScales.z);
	const float rSq = 1.0f;

	if (pii0.dot(pii0) <= rSq /* && pii1.dot(pii1) <= rSq */) {
		// terminate early in the special case
		// that shot originated within volume
		if (q) {
			q->b0 = true; q->b1 = true;
			q->t0 = 0.0f; q->t1 = 0.0f;
			q->p0 = ZVec; q->p1 = ZVec;
		}
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
			const float3 p0(pTmp.x * v->axisHScales.x, pTmp.y * v->axisHScales.y, pTmp.z * v->axisHScales.z);
			// get the distance from the start of the segment
			// to the intersection point in volume-space
			const float dSq0 = (p0 - pi0).SqLength();
			// if the intersection point is closer to p0 than
			// the end of the ray segment, the hit is valid
			const bool b0 = (t0 > 0.0f && dSq0 <= segLenSq);

			if (q) {
				q->b0 = b0; q->b1 = false;
				q->t0 = t0; q->t1 = 0.0f;
				q->p0 = p0; q->p1 = ZVec;
			}

			return b0;
		} else {
			// two solutions for t
			const float rD = fastmath::sqrt(D);
			const float t0 = (-B + rD) * 0.5f;
			const float t1 = (-B - rD) * 0.5f;
			// const float t0 = (-B + rD) / (2.0f * A);
			// const float t1 = (-B - rD) / (2.0f * A);
			// get the intersection points in sphere-space
			const float3 pTmp0 = pii0 + (dir * t0);
			const float3 pTmp1 = pii0 + (dir * t1);
			// get the intersection points in volume-space
			const float3 p0(pTmp0.x * v->axisHScales.x, pTmp0.y * v->axisHScales.y, pTmp0.z * v->axisHScales.z);
			const float3 p1(pTmp1.x * v->axisHScales.x, pTmp1.y * v->axisHScales.y, pTmp1.z * v->axisHScales.z);
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
	// get the ray direction in volume space
	const float3 dir = (pi1 - pi0).Normalize();

	// end-cap plane normals
	float3 n0;
	float3 n1;

	// pi0 transformed to unit-cylinder space
	float3 pii0;
	bool pass = false;

	// pi0.dot(pi0), pi0.dot(dir), dir.dot(dir)
	const float pxSq = pi0.x * pi0.x, pxdx = pi0.x * dir.x, dxSq = dir.x * dir.x;
	const float pySq = pi0.y * pi0.y, pydy = pi0.y * dir.y, dySq = dir.y * dir.y;
	const float pzSq = pi0.z * pi0.z, pzdz = pi0.z * dir.z, dzSq = dir.z * dir.z;
	const float saSq = v->axisHScalesSq.x;
	const float sbSq = v->axisHScalesSq.y;
	const float scSq = v->axisHScalesSq.z;

	float A = 0.0f, B = 0.0f, C = 0.0f;

	switch (v->primaryAxis) {
		case COLVOL_AXIS_X: {
			// see if start of ray lies between end-caps
			pass = (pi0.x > -v->axisHScales.x  &&  pi0.x < v->axisHScales.x);
			pii0 = float3(pi0.x, pi0.y * v->axisHIScales.y, pi0.z * v->axisHIScales.z);

			n0 = float3( 1.0f, 0.0f, 0.0f);
			n1 = float3(-1.0f, 0.0f, 0.0f);

			// get the parameters for the (2D)
			// yz-plane ellipse surface equation
			A = (dySq / sbSq) + (dzSq / scSq);
			B = ((2.0f * pydy) / sbSq) + ((2.0f * pzdz) / scSq);
			C = (pySq / sbSq) + (pzSq / scSq) - 1.0f;
		} break;
		case COLVOL_AXIS_Y: {
			// see if start of ray lies between end-caps
			pass = (pi0.y > -v->axisHScales.y  &&  pi0.y < v->axisHScales.y);
			pii0 = float3(pi0.x * v->axisHIScales.x, pi0.y, pi0.z * v->axisHIScales.z);

			n0 = float3(0.0f,  1.0f, 0.0f);
			n1 = float3(0.0f, -1.0f, 0.0f);

			// get the parameters for the (2D)
			// xz-plane ellipse surface equation
			A = (dxSq / saSq) + (dzSq / scSq);
			B = ((2.0f * pxdx) / saSq) + ((2.0f * pzdz) / scSq);
			C = (pxSq / saSq) + (pzSq / scSq) - 1.0f;
		} break;
		case COLVOL_AXIS_Z: {
			// see if start of ray lies between end-caps
			pass = (pi0.z > -v->axisHScales.z  &&  pi0.z < v->axisHScales.z);
			pii0 = float3(pi0.x * v->axisHIScales.x, pi0.y * v->axisHIScales.y, pi0.z);

			n0 = float3(0.0f, 0.0f,  1.0f);
			n1 = float3(0.0f, 0.0f, -1.0f);

			// get the parameters for the (2D)
			// xy-plane ellipse surface equation
			A = (dxSq / saSq) + (dySq / sbSq);
			B = ((2.0f * pxdx) / saSq) + ((2.0f * pydy) / sbSq);
			C = (pxSq / saSq) + (pySq / sbSq) - 1.0f;
		} break;
	}

	if (pass && pii0.dot(pii0) <= 1.0f) {
		// terminate early in the special case
		// that shot originated within volume
		if (q) {
			q->b0 = true; q->b1 = true;
			q->t0 = 0.0f; q->t1 = 0.0f;
			q->p0 = ZVec; q->p1 = ZVec;
		}
		return true;
	}

	const int pAx = v->primaryAxis;
	const int sAx0 = v->secondaryAxes[0];
	const int sAx1 = v->secondaryAxes[1];
	const float D = (B * B) - (4.0f * A * C);

	if (D < -EPS) {
		return false;
	} else {
		// get the length of the ray segment in volume-space
		const float segLenSq = (pi1 - pi0).SqLength();

		float3 p0; float t0 = 0.0f, r0 = 0.0f, dSq0 = 0.0f; bool b0 = false;
		float3 p1; float t1 = 0.0f, r1 = 0.0f, dSq1 = 0.0f; bool b1 = false;

		if (D < EPS) {
			// one solution for t
			t0 = -D / (2.0f * A); p0 = pi0 + (dir * t0);

			if (p0[pAx] > -v->axisHScales[pAx]  &&  p0[pAx] < v->axisHScales[pAx]) {
				// intersection point <p0> falls between cylinder
				// caps, check if it also lies on our ray segment
				dSq0 = (p0 - pi0).SqLength();
				b0 = (/* t0 > 0.0f && */ dSq0 <= segLenSq);
			} else {
				// <p> does not fall between end-caps but ray
				// segment might still intersect one, so test
				// for intersection against the cap planes
				t0 = -(n0.dot(pi0) + v->axisHScales[pAx]) / n0.dot(dir); p0 = pi0 + (dir * t0);
				t1 = -(n1.dot(pi0) - v->axisHScales[pAx]) / n1.dot(dir); p1 = pi0 + (dir * t1);
				r0 = (((p0[sAx0] * p0[sAx0]) / v->axisHScalesSq[sAx0]) + ((p0[sAx1] * p0[sAx1]) / v->axisHScalesSq[sAx1]));
				r1 = (((p1[sAx0] * p1[sAx0]) / v->axisHScalesSq[sAx0]) + ((p1[sAx1] * p1[sAx1]) / v->axisHScalesSq[sAx1]));
				b0 = (t0 > 0.0f && r0 <= 1.0f);
				b1 = (t1 > 0.0f && r1 <= 1.0f);
			}
		} else {
			// two solutions for t
			const float rD = fastmath::sqrt(D);
			t0 = (-B + rD) / (2.0f * A); p0 = pi0 + (dir * t0);
			t1 = (-B - rD) / (2.0f * A); p1 = pi0 + (dir * t1);

			// test the 1st intersection point
			// along the cylinder's major axis
			if (p0[pAx] > -v->axisHScales[pAx]  &&  p0[pAx] < v->axisHScales[pAx]) {
				// intersection point <p0> falls between cylinder
				// caps, check if it also lies on our ray segment
				dSq0 = (p0 - pi0).SqLength();
				b0 = (/* t0 > 0.0f && */ dSq0 <= segLenSq);
			}

			// test the 2nd intersection point
			// along the cylinder's major axis
			if (p1[pAx] > -v->axisHScales[pAx]  &&  p1[pAx] < v->axisHScales[pAx]) {
				// intersection point <p1> falls between cylinder
				// caps, check if it also lies on our ray segment
				dSq1 = (p1 - pi0).SqLength();
				b1 = (/* t1 > 0.0f && */ dSq1 <= segLenSq);
			}
		}

		if (!b0 && !b1) {
			// neither p0 nor p1 lies on ray segment (or falls between
			// the cylinder end-caps) but segment might still intersect
			// a cap, so do extra test for intersection against the cap
			// planes
			// NOTE: DIV0 if normal and dir are orthogonal?
			t0 = -(n0.dot(pi0) + v->axisHScales[pAx]) / n0.dot(dir);
			t1 = -(n1.dot(pi0) - v->axisHScales[pAx]) / n1.dot(dir);
			p0 = pi0 + (dir * t0); dSq0 = (p0 - pi0).SqLength();
			p1 = pi0 + (dir * t1); dSq1 = (p1 - pi0).SqLength();
			r0 = (((p0[sAx0] * p0[sAx0]) / v->axisHScalesSq[sAx0]) + ((p0[sAx1] * p0[sAx1]) / v->axisHScalesSq[sAx1]));
			r1 = (((p1[sAx0] * p1[sAx0]) / v->axisHScalesSq[sAx0]) + ((p1[sAx1] * p1[sAx1]) / v->axisHScalesSq[sAx1]));
			b0 = (t0 > 0.0f && r0 <= 1.0f && dSq0 <= segLenSq);
			b1 = (t1 > 0.0f && r1 <= 1.0f && dSq1 <= segLenSq);
		}

		if (q) {
			q->b0 = b0; q->b1 = b1;
			q->t0 = t0; q->t1 = t1;
			q->p0 = p0; q->p1 = p1;
		}

		return (b0 || b1);
	}
}

bool CCollisionHandler::IntersectBox(const CollisionVolume* v, const float3& pi0, const float3& pi1, CollisionQuery* q)
{
	const bool ba = (pi0.x > -v->axisHScales.x  &&  pi0.x < v->axisHScales.x);
	const bool bb = (pi0.y > -v->axisHScales.y  &&  pi0.y < v->axisHScales.y);
	const bool bc = (pi0.z > -v->axisHScales.z  &&  pi0.z < v->axisHScales.z);

	if ((ba && bb && bc) /* && (bd && be && bf) */) {
		// terminate early in the special case
		// that shot originated within volume
		if (q) {
			q->b0 = true; q->b1 = true;
			q->t0 = 0.0f; q->t1 = 0.0f;
			q->p0 = ZVec; q->p1 = ZVec;
		}
		return true;
	}

	float tn = -9999999.9f;
	float tf =  9999999.9f;
	float t0 =  0.0f;
	float t1 =  0.0f;
	float t2 =  0.0f;

	const float3 dir = (pi1 - pi0).Normalize();

	if (dir.x > -EPS && dir.x < EPS) {
		if (pi0.x < -v->axisHScales.x  ||  pi0.x > v->axisHScales.x) {
			return false;
		}
	} else {
		if (dir.x > 0.0f) {
			t0 = (-v->axisHScales.x - pi0.x) / dir.x;
			t1 = ( v->axisHScales.x - pi0.x) / dir.x;
		} else {
			t1 = (-v->axisHScales.x - pi0.x) / dir.x;
			t0 = ( v->axisHScales.x - pi0.x) / dir.x;
		}

		if (t0 > t1) { t2 = t1; t1 = t0; t0 = t2; }
		if (t0 > tn) { tn = t0; }
		if (t1 < tf) { tf = t1; }
		if (tn > tf) { return false; }
		if (tf < 0.0f) { return false; }
	}

	if (dir.y > -EPS && dir.y < EPS) {
		if (pi0.y < -v->axisHScales.y  ||  pi0.y > v->axisHScales.y) {
			return false;
		}
	} else {
		if (dir.y > 0.0f) {
			t0 = (-v->axisHScales.y - pi0.y) / dir.y;
			t1 = ( v->axisHScales.y - pi0.y) / dir.y;
		} else {
			t1 = (-v->axisHScales.y - pi0.y) / dir.y;
			t0 = ( v->axisHScales.y - pi0.y) / dir.y;
		}

		if (t0 > t1) { t2 = t1; t1 = t0; t0 = t2; }
		if (t0 > tn) { tn = t0; }
		if (t1 < tf) { tf = t1; }
		if (tn > tf) { return false; }
		if (tf < 0.0f) { return false; }
	}

	if (dir.z > -EPS && dir.z < EPS) {
		if (pi0.z < -v->axisHScales.z  ||  pi0.z > v->axisHScales.z) {
			return false;
		}
	} else {
		if (dir.z > 0.0f) {
			t0 = (-v->axisHScales.z - pi0.z) / dir.z;
			t1 = ( v->axisHScales.z - pi0.z) / dir.z;
		} else {
			t1 = (-v->axisHScales.z - pi0.z) / dir.z;
			t0 = ( v->axisHScales.z - pi0.z) / dir.z;
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
