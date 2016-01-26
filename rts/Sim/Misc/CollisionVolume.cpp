/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "CollisionVolume.h"
#include "Rendering/Models/3DModel.h"
#include "Sim/Units/Unit.h"
#include "Sim/Features/Feature.h"
#include "System/Matrix44f.h"
#include "System/myMath.h"
#include "System/Util.h"

CR_BIND(CollisionVolume, )
CR_REG_METADATA(CollisionVolume, (
	CR_MEMBER(fullAxisScales),
	CR_MEMBER(halfAxisScales),
	CR_MEMBER(halfAxisScalesSqr),
	CR_MEMBER(halfAxisScalesInv),
	CR_MEMBER(axisOffsets),
	CR_MEMBER(volumeBoundingRadius),
	CR_MEMBER(volumeBoundingRadiusSq),
	CR_MEMBER(volumeType),
	CR_MEMBER(volumeAxes),
	CR_MEMBER(ignoreHits),
	CR_MEMBER(useContHitTest),
	CR_MEMBER(defaultToFootPrint),
	CR_MEMBER(defaultToPieceTree)
))

// base ctor (CREG-only)
CollisionVolume::CollisionVolume():
	fullAxisScales(OnesVector * 2.0f),
	halfAxisScales(OnesVector),
	halfAxisScalesSqr(OnesVector),
	halfAxisScalesInv(OnesVector),
	axisOffsets(ZeroVector),
	volumeBoundingRadius(1.0f),
	volumeBoundingRadiusSq(1.0f),
	volumeType(COLVOL_TYPE_SPHERE),
	ignoreHits(false),
	useContHitTest(COLVOL_HITTEST_CONT),
	defaultToFootPrint(false),
	defaultToPieceTree(false)
{
	// set the axes s.t. cylinders start z-aligned
	volumeAxes[0] = COLVOL_AXIS_Z;
	volumeAxes[1] = COLVOL_AXIS_X;
	volumeAxes[2] = COLVOL_AXIS_Y;
}

CollisionVolume& CollisionVolume::operator = (const CollisionVolume& v) {
	fullAxisScales         = v.fullAxisScales;
	halfAxisScales         = v.halfAxisScales;
	halfAxisScalesSqr      = v.halfAxisScalesSqr;
	halfAxisScalesInv      = v.halfAxisScalesInv;
	axisOffsets            = v.axisOffsets;

	volumeBoundingRadius   = v.volumeBoundingRadius;
	volumeBoundingRadiusSq = v.volumeBoundingRadiusSq;

	volumeType             = v.volumeType;
	volumeAxes[0]          = v.volumeAxes[0];
	volumeAxes[1]          = v.volumeAxes[1];
	volumeAxes[2]          = v.volumeAxes[2];

	ignoreHits             = v.ignoreHits;
	useContHitTest         = v.useContHitTest;
	defaultToFootPrint     = v.defaultToFootPrint;
	defaultToPieceTree     = v.defaultToPieceTree;

	return *this;
}

CollisionVolume::CollisionVolume(
	const std::string& cvTypeString,
	const float3& cvScales,
	const float3& cvOffsets
) {
	// default-initialize
	*this = CollisionVolume();

	int cvType = COLVOL_TYPE_SPHERE;
	int cvAxis = COLVOL_AXIS_Z;

	if (!cvTypeString.empty()) {
		const std::string& cvTypeStr = StringToLower(cvTypeString);
		const std::string& cvTypePrefix = cvTypeStr.substr(0, 3);

		switch (cvTypePrefix[0]) {
			case 'e': { cvType = COLVOL_TYPE_ELLIPSOID; } break; // "ell..."
			case 'c': { cvType = COLVOL_TYPE_CYLINDER;  } break; // "cyl..."
			case 'b': { cvType = COLVOL_TYPE_BOX;       } break; // "box"
			case 's': { cvType = COLVOL_TYPE_SPHERE;    } break; // "sph..."
		}

		if (cvType == COLVOL_TYPE_CYLINDER) {
			switch (cvTypeStr[cvTypeStr.size() - 1]) {
				case 'x': { cvAxis = COLVOL_AXIS_X; } break;
				case 'y': { cvAxis = COLVOL_AXIS_Y; } break;
				case 'z': { cvAxis = COLVOL_AXIS_Z; } break;
				default: {} break; // just use the z-axis
			}
		}
	}

	InitShape(cvScales, cvOffsets, cvType, COLVOL_HITTEST_CONT, cvAxis);
}



void CollisionVolume::InitSphere(float radius)
{
	// <r> is the object's default RADIUS (not its diameter),
	// so we need to double it to get the full-length scales
	InitShape(OnesVector * radius * 2.0f, ZeroVector, COLVOL_TYPE_SPHERE, COLVOL_HITTEST_CONT, COLVOL_AXIS_Z);
}

void CollisionVolume::InitBox(const float3& scales, const float3& offsets)
{
	InitShape(scales, offsets, COLVOL_TYPE_BOX, COLVOL_HITTEST_CONT, COLVOL_AXIS_Z);
}

void CollisionVolume::InitShape(
	const float3& scales,
	const float3& offsets,
	const int vType,
	const int tType,
	const int pAxis)
{
	// make sure none of the scales are ever negative or zero
	//
	// if the clamped vector is <1, 1, 1> (ie. all scales were <= 1.0f)
	// then we assume a "default volume" is wanted and the unit/feature
	// instances will be assigned spheres (of size model->radius)
	//
	float3 clampedScales = float3::max(scales, OnesVector);

	// assign these here, since we can be
	// called from outside the constructor
	volumeType    = std::max(vType, 0) % (COLVOL_TYPE_SPHERE + 1);
	volumeAxes[0] = std::max(pAxis, 0) % (COLVOL_AXIS_Z + 1);

	axisOffsets = offsets;
	useContHitTest = (tType == COLVOL_HITTEST_CONT);

	switch (volumeAxes[0]) {
		case COLVOL_AXIS_X: {
			volumeAxes[1] = COLVOL_AXIS_Y;
			volumeAxes[2] = COLVOL_AXIS_Z;
		} break;
		case COLVOL_AXIS_Y: {
			volumeAxes[1] = COLVOL_AXIS_X;
			volumeAxes[2] = COLVOL_AXIS_Z;
		} break;
		case COLVOL_AXIS_Z: {
			volumeAxes[1] = COLVOL_AXIS_X;
			volumeAxes[2] = COLVOL_AXIS_Y;
		} break;
	}

	FixTypeAndScale(clampedScales);
	SetAxisScales(clampedScales);
	SetBoundingRadius();
}


void CollisionVolume::SetBoundingRadius() {
	// set the radius of the minimum bounding sphere
	// that encompasses this custom collision volume
	// (for early-out testing)
	// NOTE:
	//   this must be called manually after either
	//   a call to SetAxisScales or to RescaleAxes
	switch (volumeType) {
		case COLVOL_TYPE_BOX: {
			// would be an over-estimation for cylinders
			volumeBoundingRadiusSq = halfAxisScalesSqr.x + halfAxisScalesSqr.y + halfAxisScalesSqr.z;
			volumeBoundingRadius = math::sqrt(volumeBoundingRadiusSq);
		} break;
		case COLVOL_TYPE_CYLINDER: {
			const float prhs = halfAxisScales[volumeAxes[0]];   // primary axis half-scale
			const float sahs = halfAxisScales[volumeAxes[1]];   // 1st secondary axis half-scale
			const float sbhs = halfAxisScales[volumeAxes[2]];   // 2nd secondary axis half-scale
			const float mshs = std::max(sahs, sbhs);            // max. secondary axis half-scale

			volumeBoundingRadiusSq = prhs * prhs + mshs * mshs;
			volumeBoundingRadius = math::sqrt(volumeBoundingRadiusSq);
		} break;
		case COLVOL_TYPE_SPHERE: {
			volumeBoundingRadius = halfAxisScales.x;
			volumeBoundingRadiusSq = volumeBoundingRadius * volumeBoundingRadius;
		} break;
		case COLVOL_TYPE_ELLIPSOID: {
			volumeBoundingRadius = std::max(halfAxisScales.x, std::max(halfAxisScales.y, halfAxisScales.z));
			volumeBoundingRadiusSq = volumeBoundingRadius * volumeBoundingRadius;
		} break;
	}
}

void CollisionVolume::SetAxisScales(const float3& scales) {
	fullAxisScales = scales;
	halfAxisScales = fullAxisScales * 0.5f;

	halfAxisScalesSqr = halfAxisScales * halfAxisScales;
	halfAxisScalesInv = OnesVector / halfAxisScales;
}

void CollisionVolume::RescaleAxes(const float3& scales) {
	fullAxisScales *= scales;
	halfAxisScales *= scales;

	halfAxisScalesSqr *= (scales * scales);
	halfAxisScalesInv = OnesVector / scales;
}

void CollisionVolume::FixTypeAndScale(float3& scales) {
	// NOTE:
	//   prevent Lua (which calls InitShape directly) from
	//   creating non-uniform spheres to emulate ellipsoids
	if (volumeType == COLVOL_TYPE_SPHERE) {
		scales.x = std::max(scales.x, std::max(scales.y, scales.z));
		scales.y = scales.x;
		scales.z = scales.x;
	} else if (volumeType == COLVOL_TYPE_ELLIPSOID) {
		if (scales.x == scales.y && scales.y == scales.z)
			volumeType = COLVOL_TYPE_SPHERE;

	} else if (volumeType == COLVOL_TYPE_CYLINDER) {
		scales[volumeAxes[1]] = std::max(scales[volumeAxes[1]], scales[volumeAxes[2]]);
		scales[volumeAxes[2]] =          scales[volumeAxes[1]];
	}
}



float3 CollisionVolume::GetWorldSpacePos(const CSolidObject* o, const float3& extOffsets) const {
	// collision-volumes are always centered on midPos
	return (o->midPos + o->GetObjectSpaceVec(axisOffsets + extOffsets));
}



float CollisionVolume::GetPointSurfaceDistance(const CUnit* u, const LocalModelPiece* lmp, const float3& pos) const {
	return (GetPointSurfaceDistance(u, lmp, u->GetTransformMatrix(true), pos));
}

float CollisionVolume::GetPointSurfaceDistance(const CFeature* f, const LocalModelPiece* lmp, const float3& pos) const {
	return (GetPointSurfaceDistance(f, lmp, f->GetTransformMatrixRef(), pos));
}

float CollisionVolume::GetPointSurfaceDistance(
	const CSolidObject* obj,
	const LocalModelPiece* lmp,
	const CMatrix44f& mat,
	const float3& pos
) const {
	CMatrix44f vm = mat;

	if ((obj->collisionVolume).DefaultToPieceTree() && lmp != NULL) {
		// NOTE: if we get here, <this> is the piece-volume
		assert(this == lmp->GetCollisionVolume());

		// transform into piece-space relative to pos
		vm <<= lmp->GetModelSpaceMatrix();
	} else {
		// SObj::GetTransformMatrix does not include this
		// (its translation component is pos, not midPos)
		vm.Translate(obj->relMidPos * WORLD_TO_OBJECT_SPACE);
	}

	vm.Translate(GetOffsets());
	vm.InvertAffineInPlace();

	return (GetPointSurfaceDistance(vm, pos));
}



float CollisionVolume::GetPointSurfaceDistance(const CMatrix44f& mv, const float3& p) const {
	// transform <p> from world- to volume-space
	float3 pv = mv.Mul(p);

	float d = 0.0f;

	switch (volumeType) {
		case COLVOL_TYPE_BOX: {
			// always clamp <pv> to box (!) surface
			// (so minimum distance is always zero)
			pv.x = ((int(pv.x >= 0.0f) * 2) - 1) * std::max(math::fabs(pv.x), halfAxisScales.x);
			pv.y = ((int(pv.y >= 0.0f) * 2) - 1) * std::max(math::fabs(pv.y), halfAxisScales.y);
			pv.z = ((int(pv.z >= 0.0f) * 2) - 1) * std::max(math::fabs(pv.z), halfAxisScales.z);

			// calculate the closest surface point
			float3 pt;
			pt.x = Clamp(pv.x, -halfAxisScales.x, halfAxisScales.x);
			pt.y = Clamp(pv.y, -halfAxisScales.y, halfAxisScales.y);
			pt.z = Clamp(pv.z, -halfAxisScales.z, halfAxisScales.z);

			// float l = std::min(pv.x - pt.x, std::min(pv.y - pt.y, pv.z - pt.z));
			d = pv.distance(pt);
		} break;

		case COLVOL_TYPE_SPHERE: {
			float l = pv.Length();
			d = std::max(l - volumeBoundingRadius, 0.0f);
			//float3 pt = (pv / std::max(0.01f, l)) * d;
		} break;

		case COLVOL_TYPE_CYLINDER: {
			// code below is only valid for non-ellipsoidal cylinders
			assert(halfAxisScales   [volumeAxes[1]] == halfAxisScales   [volumeAxes[2]]);
			assert(halfAxisScalesSqr[volumeAxes[1]] == halfAxisScalesSqr[volumeAxes[2]]);

			switch (volumeAxes[0]) {
				case COLVOL_AXIS_X: { d = GetCylinderDistance(pv, 0, 1, 2); } break;
				case COLVOL_AXIS_Y: { d = GetCylinderDistance(pv, 1, 0, 2); } break;
				case COLVOL_AXIS_Z: { d = GetCylinderDistance(pv, 2, 0, 1); } break;
			}
		} break;

		case COLVOL_TYPE_ELLIPSOID: {
			d = GetEllipsoidDistance(pv);
		} break;

		default: {
			assert(false);
		} break;
	}

	return d;
}



float CollisionVolume::GetCylinderDistance(const float3& pv, size_t axisA, size_t axisB, size_t axisC) const
{
	const float pSq = (pv[axisB] * pv[axisB]) + (pv[axisC] * pv[axisC]);
	const float rSq = (halfAxisScalesSqr[axisB] + halfAxisScalesSqr[axisC]) * 0.5f;

	float d = 0.0f;

	if (pv[axisA] >= -halfAxisScales[axisA] && pv[axisA] <= halfAxisScales[axisA]) {
		/* case 1: point is between end-cap bounds along primary axis */
		d = std::max(math::sqrt(pSq) - math::sqrt(rSq), 0.0f);
	} else {
		if (pSq <= rSq) {
			/* case 2: point is outside end-cap bounds but inside inf-tube */
			d = std::max(math::fabs(pv[axisA]) - halfAxisScales[axisA], 0.0f);
		} else {
			/* case 3: compute orthogonal distance to end-cap edge (rim) */
			const float l = Square(math::fabs(pv[axisA]) - halfAxisScales[axisA]);
			d = Square(std::max(math::sqrt(pSq) - math::sqrt(rSq), 0.0f));
			d = math::sqrt(l + d);
		}
	}

	return d;
}

#define MAX_ITERATIONS 10
#define THRESHOLD 0.001

//Newton's method according to http://wwwf.imperial.ac.uk/~rn/distance2ellipse.pdf
float CollisionVolume::GetEllipsoidDistance(const float3& pv) const
{
	const float& a = halfAxisScales.x;
	const float& b = halfAxisScales.y;
	const float& c = halfAxisScales.z;
	const float x = math::fabsf(pv.x);
	const float y = math::fabsf(pv.y);
	const float z = math::fabsf(pv.z);

	const float a2 = a * a;
	const float b2 = b * b;
	const float c2 = c * c;
	const float x2 = x * x;
	const float y2 = y * y;
	const float z2 = z * z;
	const float x2_a2 = x2 / a2;
	const float y2_b2 = y2 / b2;
	const float z2_c2 = z2 / c2;


	//bail if inside the ellipsoid
	assert(a > 0.0f && b > 0.0f && c > 0.0f);
	if (x2_a2 + y2_b2 + z2_c2 <= 1.0f)
		return 0.0f;

	const float a2b2 = a2 - b2;
	const float xa = x * a;
	const float yb = y * b;
	const float zc = z * c;

	//Initial guess
	float theta = math::atan2(a * y, b * x);
	float phi = math::atan2(z, c * math::sqrt(x2_a2 + y2_b2));

	float dist = 0.0f;
	float lastDist = 0.0f;

	//Iterations
	for (int i = 0; i < MAX_ITERATIONS; i++) {
		const float cost = math::cos(theta);
		const float sint = math::sin(theta);
		const float sinp = math::sin(phi);
		const float cosp = math::cos(phi);

		const float sin2t = sint * sint;
		const float xacost_ybsint = xa * cost + yb * sint;
		const float xasint_ybcost = xa * sint - yb * cost;
		const float a2b2costsint = a2b2 * cost * sint;
		const float a2cos2t_b2sin2t_c2 = a2 * cost * cost + b2 * sin2t - c2;

		const float d1 = a2b2costsint * cosp - xasint_ybcost;
		const float d2 = a2cos2t_b2sin2t_c2 * sinp * cosp - sinp * xacost_ybsint + zc * cosp;
		{
			const float fx = a * cosp * cost - x;
			const float fy = b * cosp * sint - y;
			const float fz = c * sinp - z;
			lastDist = dist;
			dist = sqrt(fx * fx + fy * fy + fz * fz);

			if (math::fabsf(dist - lastDist) < THRESHOLD * dist)
				break;
		}

		//Derivative matrix
		const float a11 = a2b2 * (1 - 2 * sin2t) * cosp - xacost_ybsint;
		const float a12 = -a2b2costsint * sinp;
		const float a21 = 2 * a12 * cosp + sinp * xasint_ybcost;
		const float a22 = a2cos2t_b2sin2t_c2 * (1 - 2 * sinp * sinp) - cosp * xacost_ybsint - zc;

		const float invDet = 1.0f / (a11 * a22 - a21 * a12);

		theta += (a12 * d2 - a22 * d1) * invDet;
		theta = Clamp(theta, 0.0f, HALFPI);
		phi += (a21 * d1 - a11 * d2) * invDet;
		phi = Clamp(phi, 0.0f, HALFPI);
	}

	return dist;
}

