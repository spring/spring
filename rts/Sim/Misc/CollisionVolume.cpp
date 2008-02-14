#include <iostream>

#include "System/float3.h"
#include "System/Matrix44f.h"

#include "CollisionVolume.h"


CR_BIND(CCollisionVolume, );
CR_REG_METADATA(CCollisionVolume, (
	CR_MEMBER(axisScales),
	CR_MEMBER(axisOffsets),
	CR_MEMBER(volumeBoundingRadius),
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
	axisScales[COLVOL_AXIS_X] = volScales.x;
	axisScales[COLVOL_AXIS_Y] = volScales.y;
	axisScales[COLVOL_AXIS_Z] = volScales.z;

	// set the axis offsets
	axisOffsets[COLVOL_AXIS_X] = volOffsets.x;
	axisOffsets[COLVOL_AXIS_Y] = volOffsets.y;
	axisOffsets[COLVOL_AXIS_Z] = volOffsets.z;

	// get the half-axis lengths (measured from volume center)
	const float xHScale = axisScales[COLVOL_AXIS_X] * 0.5f;
	const float yHScale = axisScales[COLVOL_AXIS_Y] * 0.5f;
	const float zHScale = axisScales[COLVOL_AXIS_Z] * 0.5f;

	spherical = ((volumeType == COLVOL_TYPE_ELLIPSOID) &&
				 (fabsf(xHScale - yHScale) < 0.01f) &&
				 (fabsf(yHScale - zHScale) < 0.01f));

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
			volumeBoundingRadius = xHScale * xHScale + yHScale * yHScale + zHScale * zHScale;
		} break;
		case COLVOL_TYPE_CYLINDER: {
			const float primAxisScale = axisScales[primaryAxis     ] * 0.5f;
			const float secAxisScaleA = axisScales[secondaryAxes[0]] * 0.5f;
			const float secAxisScaleB = axisScales[secondaryAxes[1]] * 0.5f;
			const float maxSecAxisScale = std::max(secAxisScaleA, secAxisScaleB);
			volumeBoundingRadius = primAxisScale * primAxisScale +
								   maxSecAxisScale * maxSecAxisScale;
		} break;
		case COLVOL_TYPE_ELLIPSOID: {
			if (spherical) {
				// max(x, y, z) would suffice here too
				volumeBoundingRadius = xHScale;
			} else {
				volumeBoundingRadius = std::max(xHScale, std::max(yHScale, zHScale));
			}
		} break;
	}
}

void CCollisionVolume::SetDefaultScale(const float s)
{
	axisScales[COLVOL_AXIS_X] = s;
	axisScales[COLVOL_AXIS_Y] = s;
	axisScales[COLVOL_AXIS_Z] = s;

	spherical = true;
	volumeBoundingRadius = s;
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
	const float xHScale = (axisScales[COLVOL_AXIS_X] * 0.5f);
	const float yHScale = (axisScales[COLVOL_AXIS_Y] * 0.5f);
	const float zHScale = (axisScales[COLVOL_AXIS_Z] * 0.5f);

	// NOTE: should be 1.0f for mathematical correctness of the
	// tests, but suffers too much loss of precision at Spring's
	// framerate (if projectile speeds are high enough) leading
	// to undetected collisions
	const float maxRatio = 8.0f;

	switch (volumeType) {
		case COLVOL_TYPE_ELLIPSOID: {
			const float f1 = (iPos.x * iPos.x) / (xHScale * xHScale);
			const float f2 = (iPos.y * iPos.y) / (yHScale * yHScale);
			const float f3 = (iPos.z * iPos.z) / (zHScale * zHScale);
			hit = ((f1 + f2 + f3) <= maxRatio);
		} break;
		case COLVOL_TYPE_CYLINDER: {
			switch (primaryAxis) {
				case COLVOL_AXIS_X: {
					const bool xPass = (iPos.x > -xHScale && iPos.x < xHScale);
					const float yRat = (iPos.y * iPos.y) / (yHScale * yHScale);
					const float zRat = (iPos.z * iPos.z) / (zHScale * zHScale);
					hit = (xPass && (yRat + zRat <= maxRatio));
				} break;
				case COLVOL_AXIS_Y: {
					const bool yPass = (iPos.y > -yHScale && iPos.y < yHScale);
					const float xRat = (iPos.x * iPos.x) / (xHScale * xHScale);
					const float zRat = (iPos.z * iPos.z) / (zHScale * zHScale);
					hit = (yPass && (xRat + zRat <= maxRatio));
				} break;
				case COLVOL_AXIS_Z: {
					const bool zPass = (iPos.z > -zHScale && iPos.z < zHScale);
					const float xRat = (iPos.x * iPos.x) / (xHScale * xHScale);
					const float yRat = (iPos.y * iPos.y) / (yHScale * yHScale);
					hit = (zPass && (xRat + yRat <= maxRatio));
				} break;
			}
		} break;
		case COLVOL_TYPE_BOX: {
			const bool b1 = (iPos.x > -xHScale && iPos.x < xHScale);
			const bool b2 = (iPos.y > -yHScale && iPos.y < yHScale);
			const bool b3 = (iPos.z > -zHScale && iPos.z < zHScale);
			hit = (b1 && b2 && b3);
		} break;
	}

	return hit;
}
