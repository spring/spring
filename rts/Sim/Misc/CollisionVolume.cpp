#include <iostream> // DBG

#include "Sim/Units/Unit.h"
#include "System/float3.h"
#include "System/Matrix44f.h"

#include "CollisionVolume.h"

CCollisionVolume::CCollisionVolume(const float3& volScales, const float3& volOffsets, int primAxis, int volType)
{
	// set the full-axis lengths
	axisScales[COLVOL_AXIS_X] = volScales.x;
	axisScales[COLVOL_AXIS_Y] = volScales.y;
	axisScales[COLVOL_AXIS_Z] = volScales.z;

	// set the axis offsets (these adjust the
	// position of the volume so that it does
	// not have to be equal to unit center)
	axisOffsets[COLVOL_AXIS_X] = volOffsets.x;
	axisOffsets[COLVOL_AXIS_Y] = volOffsets.y;
	axisOffsets[COLVOL_AXIS_Z] = volOffsets.z;

	// get the half-axis lengths (measured from volume center)
	const float xScale = axisScales[COLVOL_AXIS_X] * 0.5f;
	const float yScale = axisScales[COLVOL_AXIS_Y] * 0.5f;
	const float zScale = axisScales[COLVOL_AXIS_Z] * 0.5f;

	// note: primaryAxis is only relevant for cylinders
	volumeType = volType;
	primaryAxis = primAxis;
	spherical = ((volType == COLVOL_TYPE_ELLIPSOID) &&
				 (fabsf(xScale - yScale) < 0.01f) &&
				 (fabsf(yScale - zScale) < 0.01f));

	switch (primAxis) {
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
	switch (volType) {
		case COLVOL_TYPE_BOX: {
			// would be an over-estimation for cylinders
			volumeBoundingRadius = xScale * xScale + yScale * yScale + zScale * zScale;
		} break;
		case COLVOL_TYPE_CYLINDER: {
			const float primAxisScale = axisScales[primAxis        ] * 0.5f;
			const float secAxisScaleA = axisScales[secondaryAxes[0]] * 0.5f;
			const float secAxisScaleB = axisScales[secondaryAxes[1]] * 0.5f;
			const float maxSecAxisScale = std::max(secAxisScaleA, secAxisScaleB);
			volumeBoundingRadius = primAxisScale * primAxisScale +
								   maxSecAxisScale * maxSecAxisScale;
		} break;
		case COLVOL_TYPE_ELLIPSOID: {
			if (spherical) {
				// max(x, y, z) would suffice here too
				volumeBoundingRadius = xScale;
			} else {
				volumeBoundingRadius = std::max(xScale, std::max(yScale, zScale));
			}
		} break;
	}
}

CCollisionVolume::~CCollisionVolume()
{
}



bool CCollisionVolume::Collision(const CMatrix44f& m, const float3& pos) const
{
	// get the inverse unit transformation matrix and
	// apply it to the projectile's position (actually
	// the inverse volume transformation since it can
	// be offset from the unit's center), then test if
	// the transformed pos lies within the axis-aligned
	// collision volume
	CMatrix44f mInv = m.Invert();
	float3 iPos = mInv.Mul(pos);
	bool hit = false;

	// get the half-axis lengths (measured from volume center)
	const float xScale = (axisScales[COLVOL_AXIS_X] * 0.5f);
	const float yScale = (axisScales[COLVOL_AXIS_Y] * 0.5f);
	const float zScale = (axisScales[COLVOL_AXIS_Z] * 0.5f);

	// NOTE: should be 1.0f for mathematical correctness of the
	// tests, but suffers too much loss of precision at Spring's
	// framerate (if projectile speeds are high enough) leading
	// to undetected collisions
	const float maxRatio = 8.0f;

	switch (volumeType) {
		case COLVOL_TYPE_ELLIPSOID: {
			const float f1 = (iPos.x * iPos.x) / (xScale * xScale);
			const float f2 = (iPos.y * iPos.y) / (yScale * yScale);
			const float f3 = (iPos.z * iPos.z) / (zScale * zScale);
			hit = ((f1 + f2 + f3) <= maxRatio);
		} break;
		case COLVOL_TYPE_CYLINDER: {
			switch (primaryAxis) {
				case COLVOL_AXIS_X: {
					const bool xPass = (iPos.x > -xScale && iPos.x < xScale);
					const float yRat = (iPos.y * iPos.y) / (yScale * yScale);
					const float zRat = (iPos.z * iPos.z) / (zScale * zScale);
					hit = (xPass && (yRat + zRat <= maxRatio));
				} break;
				case COLVOL_AXIS_Y: {
					const bool yPass = (iPos.y > -yScale && iPos.y < yScale);
					const float xRat = (iPos.x * iPos.x) / (xScale * xScale);
					const float zRat = (iPos.z * iPos.z) / (zScale * zScale);
					hit = (yPass && (xRat + zRat <= maxRatio));
				} break;
				case COLVOL_AXIS_Z: {
					const bool zPass = (iPos.z > -zScale && iPos.z < zScale);
					const float xRat = (iPos.x * iPos.x) / (xScale * xScale);
					const float yRat = (iPos.y * iPos.y) / (yScale * yScale);
					hit = (zPass && (xRat + yRat <= maxRatio));
				} break;
			}
		} break;
		case COLVOL_TYPE_BOX: {
			const bool b1 = (iPos.x > -xScale && iPos.x < xScale);
			const bool b2 = (iPos.y > -yScale && iPos.y < yScale);
			const bool b3 = (iPos.z > -zScale && iPos.z < zScale);
			hit = (b1 && b2 && b3);
		} break;
	}

	return hit;
}
