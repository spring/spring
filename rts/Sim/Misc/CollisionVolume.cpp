#include <iostream>

#include "System/float3.h"
#include "System/Matrix44f.h"

#include "Sim/Units/Unit.h"
#include "Sim/Features/Feature.h"

#include "CollisionVolume.h"

#define MIN(a, b) std::min((a), (b))
#define MAX(a, b) std::max((a), (b))
// #define MIN (((a) < (b))? (a): (b))
// #define MAX (((a) > (b))? (a): (b))
#define EPS 0.001f


CR_BIND(CCollisionVolume, );
CR_REG_METADATA(CCollisionVolume, (
	CR_MEMBER(axisScales),
	CR_MEMBER(axisHScales),
	CR_MEMBER(axisHScalesSq),
	CR_MEMBER(axisHIScales),
	CR_MEMBER(axisOffsets),
	CR_MEMBER(volumeBoundingRadius),
	CR_MEMBER(volumeBoundingRadiusSq),
	CR_MEMBER(volumeType),
	CR_MEMBER(primaryAxis),
	CR_MEMBER(secondaryAxes),
	CR_MEMBER(spherical)
));


CCollisionVolume::CCollisionVolume(const std::string& volTypeStr, const float3& volScales, const float3& volOffsets)
{
	// note: primaryAxis is only relevant for cylinders
	primaryAxis = COLVOL_AXIS_Z;
	volumeType = COLVOL_TYPE_ELLIPSOID;

	if (volTypeStr.size() > 0) {
		// note: case-sensitivity?
		if (volTypeStr.find("Ell") != std::string::npos) {
			volumeType = COLVOL_TYPE_ELLIPSOID;
		}

		if (volTypeStr.find("Cyl") != std::string::npos) {
			volumeType = COLVOL_TYPE_CYLINDER;

			if (volTypeStr.size() == 4) {
				if (volTypeStr[3] == 'X') { primaryAxis = COLVOL_AXIS_X; }
				if (volTypeStr[3] == 'Y') { primaryAxis = COLVOL_AXIS_Y; }
				if (volTypeStr[3] == 'Z') { primaryAxis = COLVOL_AXIS_Z; }
			}
		}

		if (volTypeStr.find("Box") != std::string::npos) {
			volumeType = COLVOL_TYPE_BOX;
		}
	}

	// set the full-axis lengths
	axisScales.x = volScales.x;
	axisScales.y = volScales.y;
	axisScales.z = volScales.z;

	// set the half-axis lengths and squared lengths
	axisHScales.x = axisScales.x * 0.5f; axisHScalesSq.x = axisHScales.x * axisHScales.x;
	axisHScales.y = axisScales.y * 0.5f; axisHScalesSq.y = axisHScales.y * axisHScales.y;
	axisHScales.z = axisScales.z * 0.5f; axisHScalesSq.z = axisHScales.z * axisHScales.z;

	// set the inverted half-axis lengths
	axisHIScales.x = 1.0f / axisHScales.x;
	axisHIScales.y = 1.0f / axisHScales.y;
	axisHIScales.z = 1.0f / axisHScales.z;

	// set the axis offsets
	axisOffsets.x = volOffsets.x;
	axisOffsets.y = volOffsets.y;
	axisOffsets.z = volOffsets.z;

	// if all axes (or half-axes) are equal in scale, volume is a sphere
	spherical = ((volumeType == COLVOL_TYPE_ELLIPSOID) &&
				 (fabsf(axisHScales.x - axisHScales.y) < EPS) &&
				 (fabsf(axisHScales.y - axisHScales.z) < EPS));

	switch (primaryAxis) {
		case COLVOL_AXIS_X: {
			secondaryAxes[0] = COLVOL_AXIS_Y;
			secondaryAxes[1] = COLVOL_AXIS_Z;
		} break;
		case COLVOL_AXIS_Y: {
			secondaryAxes[0] = COLVOL_AXIS_X;
			secondaryAxes[1] = COLVOL_AXIS_Z;
		} break;
		case COLVOL_AXIS_Z: {
			secondaryAxes[0] = COLVOL_AXIS_X;
			secondaryAxes[1] = COLVOL_AXIS_Y;
		} break;
	}

	// set the radius of the minimum bounding sphere
	// that encompasses this custom collision volume
	// (for early-out testing)
	switch (volumeType) {
		case COLVOL_TYPE_BOX: {
			// would be an over-estimation for cylinders
			volumeBoundingRadiusSq = axisHScalesSq.x + axisHScalesSq.y + axisHScalesSq.z;
			volumeBoundingRadius = sqrt(volumeBoundingRadiusSq);
		} break;
		case COLVOL_TYPE_CYLINDER: {
			const float prhs = axisHScales[primaryAxis     ];	// primary axis half-scale
			const float sahs = axisHScales[secondaryAxes[0]];	// 1st secondary axis half-scale
			const float sbhs = axisHScales[secondaryAxes[1]];	// 2nd secondary axis half-scale
			const float mshs = MAX(sahs, sbhs);					// max. secondary axis half-scale

			volumeBoundingRadiusSq = prhs * prhs + mshs * mshs;
			volumeBoundingRadius = sqrt(volumeBoundingRadiusSq);
		} break;
		case COLVOL_TYPE_ELLIPSOID: {
			if (spherical) {
				// MAX(x, y, z) would suffice here too
				volumeBoundingRadius = axisHScales.x;
			} else {
				volumeBoundingRadius = MAX(axisHScales.x, MAX(axisHScales.y, axisHScales.z));
			}

			volumeBoundingRadiusSq = volumeBoundingRadius * volumeBoundingRadius;
		} break;
	}
}

// called iif unit or feature defines no custom volume
void CCollisionVolume::SetDefaultScale(const float s)
{
	axisScales.x = s;
	axisScales.y = s;
	axisScales.z = s;

	axisHScales.x = s * 0.5f; axisHScalesSq.x = axisHScales.x * axisHScales.x;
	axisHScales.y = s * 0.5f; axisHScalesSq.y = axisHScales.y * axisHScales.y;
	axisHScales.z = s * 0.5f; axisHScalesSq.z = axisHScales.z * axisHScales.z;

	axisHIScales.x = 1.0f / axisHScales.x;
	axisHIScales.y = 1.0f / axisHScales.y;
	axisHIScales.z = 1.0f / axisHScales.z;

	spherical = true;
	volumeBoundingRadius = s;
	volumeBoundingRadiusSq = s * s;
}



bool CCollisionVolume::Collision(const CUnit* u, const float3& p) const
{
	CMatrix44f m;
	u->GetTransformMatrix(m);

	// if the volume is offset along any of its axes, add that
	// translation so the projectile pos is transformed properly
	m.Translate(axisOffsets.x, axisOffsets.y, axisOffsets.z);

	return Collision(m, p);
}

bool CCollisionVolume::Collision(const CFeature* f, const float3& p) const
{
	CMatrix44f m(f->transMatrix);
	m.Translate(axisOffsets.x, axisOffsets.y, axisOffsets.z);

	return Collision(m, p);
}

// test if point <p> (in world-coors) lies inside the
// volume whose transformation matrix is given by <m>
bool CCollisionVolume::Collision(const CMatrix44f& m, const float3& p) const
{
	// get the inverse volume transformation matrix and
	// apply it to the projectile's position, then test
	// if the transformed position lies within the axis-
	// aligned collision volume
	CMatrix44f mInv = m.Invert();
	float3 pi = mInv.Mul(p);
	bool hit = false;

	// NOTE: should be 1.0f for mathematical correctness of the
	// tests, but suffers too much loss of precision at Spring's
	// framerate (if projectile speeds are high enough) leading
	// to undetected collisions
	const float maxRatio = 8.0f;

	switch (volumeType) {
		case COLVOL_TYPE_ELLIPSOID: {
			const float f1 = (pi.x * pi.x) / axisHScalesSq.x;
			const float f2 = (pi.y * pi.y) / axisHScalesSq.y;
			const float f3 = (pi.z * pi.z) / axisHScalesSq.z;
			hit = ((f1 + f2 + f3) <= maxRatio);
		} break;
		case COLVOL_TYPE_CYLINDER: {
			switch (primaryAxis) {
				case COLVOL_AXIS_X: {
					const bool xPass = (pi.x > -axisHScales.x && pi.x < axisHScales.x);
					const float yRat = (pi.y * pi.y) / axisHScalesSq.y;
					const float zRat = (pi.z * pi.z) / axisHScalesSq.z;
					hit = (xPass && (yRat + zRat <= maxRatio));
				} break;
				case COLVOL_AXIS_Y: {
					const bool yPass = (pi.y > -axisHScales.y && pi.y < axisHScales.y);
					const float xRat = (pi.x * pi.x) / axisHScalesSq.x;
					const float zRat = (pi.z * pi.z) / axisHScalesSq.z;
					hit = (yPass && (xRat + zRat <= maxRatio));
				} break;
				case COLVOL_AXIS_Z: {
					const bool zPass = (pi.z > -axisHScales.z && pi.z < axisHScales.z);
					const float xRat = (pi.x * pi.x) / axisHScalesSq.x;
					const float yRat = (pi.y * pi.y) / axisHScalesSq.y;
					hit = (zPass && (xRat + yRat <= maxRatio));
				} break;
			}
		} break;
		case COLVOL_TYPE_BOX: {
			const bool b1 = (pi.x > -axisHScales.x && pi.x < axisHScales.x);
			const bool b2 = (pi.y > -axisHScales.y && pi.y < axisHScales.y);
			const bool b3 = (pi.z > -axisHScales.z && pi.z < axisHScales.z);
			hit = (b1 && b2 && b3);
		} break;
	}

	return hit;
}



bool CCollisionVolume::Intersect(const CUnit* u, const float3& p1, const float3& p2) const
{
	CMatrix44f m;
	u->GetTransformMatrix(m);
	m.Translate(axisOffsets.x, axisOffsets.y, axisOffsets.z);

	return Intersect(m, p1, p2);
}
bool CCollisionVolume::Intersect(const CFeature* f, const float3& p1, const float3& p2) const
{
	CMatrix44f m(f->transMatrix);
	m.Translate(axisOffsets.x, axisOffsets.y, axisOffsets.z);

	return Intersect(m, p1, p2);
}

// test if ray from <p1> to <p2> (in world-coors) intersects
// the volume whose transformation matrix is given by <m>
bool CCollisionVolume::Intersect(const CMatrix44f& m, const float3& p1, const float3& p2) const
{
	CMatrix44f mInv = m.Invert();
	const float3 pi1 = mInv.Mul(p1);
	const float3 pi2 = mInv.Mul(p2);
	bool intersect = false;


	// minimum and maximum (x, y, z) coordinates of transformed ray
	const float rminx = MIN(pi1.x, pi2.x), rminy = MIN(pi1.y, pi2.y), rminz = MIN(pi1.z, pi2.z);
	const float rmaxx = MAX(pi1.x, pi2.x), rmaxy = MAX(pi1.y, pi2.y), rmaxz = MAX(pi1.z, pi2.z);

	// minimum and maximum (x, y, z) coordinates of (bounding box around) volume
	const float vminx = -axisHScales.x, vminy = -axisHScales.y, vminz = -axisHScales.z;
	const float vmaxx =  axisHScales.x, vmaxy =  axisHScales.y, vmaxz =  axisHScales.z;

	// check if ray misses (bounding box around) volume completely
	if (rmaxx < vminx || rminx > vmaxx) { return false; }
	if (rmaxy < vminy || rminy > vmaxy) { return false; }
	if (rmaxz < vminz || rminz > vmaxz) { return false; }


	switch (volumeType) {
		case COLVOL_TYPE_ELLIPSOID: {
			intersect = IntersectEllipsoid(pi1, pi2);
		} break;
		case COLVOL_TYPE_CYLINDER: {
			intersect = IntersectCylinder(pi1, pi2);
		} break;
		case COLVOL_TYPE_BOX: {
			intersect = IntersectBox(pi1, pi2);
		} break;
	}

	return intersect;
}



bool CCollisionVolume::IntersectEllipsoid(const float3& pi1, const float3& pi2) const
{
	// NOTE: numerically unreliable for large volumes
	//
	// surface = [ (x / a)^2 + (y / b)^2 + (z / c)^2 = 1 ]
	// line(t) = [ p + dir * t ]
	//
	// solves the surface equation for t
	const float3 dir = (pi2 - pi1).Normalize();

	const float mul1 = axisHScalesSq.y * axisHScalesSq.z;
	const float mul2 = axisHScalesSq.x * axisHScalesSq.z;
	const float mul3 = axisHScalesSq.x * axisHScalesSq.y;
	const float div  = axisHScalesSq.x * axisHScalesSq.y * axisHScalesSq.z;

	const float a = (pi1.x * pi1.x),        d = (pi1.y * pi1.y),        g = (pi1.z * pi1.z);
	const float b = (pi1.x * dir.x) * 2.0f, e = (pi1.y * dir.y) * 2.0f, h = (pi1.z * dir.z) * 2.0f;
	const float c = (dir.x * dir.x),        f = (dir.y * dir.y),        i = (dir.z * dir.z);

	const float aa = a * mul1, dd = d * mul2, gg = g * mul3;
	const float bb = b * mul1, ee = e * mul2, hh = h * mul3;
	const float cc = c * mul1, ff = f * mul2, ii = i * mul3;

	// const float aaa = (aa + dd + gg) - (div);
	// const float bbb = (bb + ee + hh);
	// const float ccc = (cc + ff + ii);
	const float aaa = ((aa + dd + gg) / div) - 1.0f;
	const float bbb = ((bb + ee + hh) / div);
	const float ccc = ((cc + ff + ii) / div);
	const float dis = (bbb * bbb) - (4.0f * aaa * ccc);

	if (dis < -EPS) {
		// no solution
		return false;
	} else {
		if (dis < EPS) {
			// one solution, ray touches volume surface
			// float t = -bbb / (2.0f * aaa);
			return true;
		} else {
			// two solutions, ray intersects volume
			// float t0 = (-bbb + sqrt(dis)) / (2.0f * aaa);
			// float t1 = (-bbb - sqrt(dis)) / (2.0f * aaa);
			return true;
		}
	}
}

bool CCollisionVolume::IntersectCylinder(const float3& pi1, const float3& pi2) const
{
	// TODO
	switch (primaryAxis) {
		case COLVOL_AXIS_X: {} break;
		case COLVOL_AXIS_Y: {} break;
		case COLVOL_AXIS_Z: {} break;
	}

	return true;
}

bool CCollisionVolume::IntersectBox(const float3& pi1, const float3& pi2) const
{
	// get the ray's axis delta's
	float dx = (pi1.x - pi2.x);
	float dy = (pi1.y - pi2.y);
	float dz = (pi1.z - pi2.z);

	// lazily avoid any DIV0's
	if (dx > -EPS && dx < EPS) { dx = EPS; }
	if (dy > -EPS && dy < EPS) { dy = EPS; }
	if (dz > -EPS && dz < EPS) { dz = EPS; }

	const float ddx = 1.0 / dx;
	const float ddy = 1.0 / dy;
	const float ddz = 1.0 / dz;

	float txmin = 0.0f, txmax = 0.0f;
	float tymin = 0.0f, tymax = 0.0f;
	float tzmin = 0.0f, tzmax = 0.0f;

	if (ddx >= 0.0f) {
		txmin = (-axisHScales.x - pi1.x) * ddx;
		txmax = ( axisHScales.x - pi1.x) * ddx;
	} else {
		txmin = ( axisHScales.x - pi1.x) * ddx;
		txmax = (-axisHScales.x - pi1.x) * ddx;
	}

	if (ddy >= 0.0f) {
		tymin = (-axisHScales.y - pi1.y) * ddy;
		tymax = ( axisHScales.y - pi1.y) * ddy;
	} else {
		tymin = ( axisHScales.y - pi1.y) * ddy;
		tymax = (-axisHScales.y - pi1.y) * ddy;
	}

	if ((txmin > tymax) || (tymin > txmax)) {
		return false;
	}

	if (tymin > txmin) { txmin = tymin; }
	if (tymax < txmax) { txmax = tymax; }

	if (ddz >= 0.0f) {
		tzmin = (-axisHScales.z - pi1.z) * ddz;
		tzmax = ( axisHScales.z - pi1.z) * ddz;
	} else {
		tzmin = ( axisHScales.z - pi1.z) * ddz;
		tzmax = (-axisHScales.z - pi1.z) * ddz;
	}

	if ((txmin > tzmax) || (tzmin > txmax)) {
		return false;
	}

	return true;
}
