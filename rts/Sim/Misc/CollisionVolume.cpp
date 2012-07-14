/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "CollisionVolume.h"
#include "Rendering/Models/3DModel.h"
#include "Sim/Units/Unit.h"
#include "Sim/Features/Feature.h"
#include "System/Matrix44f.h"
#include "System/myMath.h"
#include "System/mmgr.h"

#define SUPPORT_COMPLEX_VOLUMES 0

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
	CR_MEMBER(disabled),
	CR_MEMBER(defaultScale)
));

// base ctor (CREG-only)
CollisionVolume::CollisionVolume()
{
	axisScales             = float3(2.0f, 2.0f, 2.0f);
	axisHScales            = float3(1.0f, 1.0f, 1.0f);
	axisHScalesSq          = float3(1.0f, 1.0f, 1.0f);
	axisHIScales           = float3(1.0f, 1.0f, 1.0f);
	axisOffsets            = ZeroVector;
	volumeBoundingRadius   = 1.0f;
	volumeBoundingRadiusSq = 1.0f;
	volumeType             = COLVOL_TYPE_SPHERE;
	testType               = COLVOL_HITTEST_DISC;
	primaryAxis            = COLVOL_AXIS_Z;
	secondaryAxes[0]       = COLVOL_AXIS_X;
	secondaryAxes[1]       = COLVOL_AXIS_Y;
	disabled               = false;
	defaultScale           = true;
}

// copy ctor
CollisionVolume::CollisionVolume(const CollisionVolume* v, float defaultRadius)
{
	axisScales             = v->axisScales;
	axisHScales            = v->axisHScales;
	axisHScalesSq          = v->axisHScalesSq;
	axisHIScales           = v->axisHIScales;
	axisOffsets            = v->axisOffsets;
	volumeBoundingRadius   = v->volumeBoundingRadius;
	volumeBoundingRadiusSq = v->volumeBoundingRadiusSq;
	volumeType             = v->volumeType;
	testType               = v->testType;
	primaryAxis            = v->primaryAxis;
	secondaryAxes[0]       = v->secondaryAxes[0];
	secondaryAxes[1]       = v->secondaryAxes[1];
	disabled               = v->disabled;
	defaultScale           = v->defaultScale;

	if (defaultScale && defaultRadius > 0.0f) {
		// if the volume being copied was not given
		// explicit scales, convert the clone into a
		// sphere if provided with a non-zero radius
		Init(defaultRadius);
	}
}

CollisionVolume::CollisionVolume(
	const std::string& cvTypeString,
	const float3& cvScales,
	const float3& cvOffsets,
	const bool cvContHitTest)
{
	int cvType = COLVOL_TYPE_FOOTPRINT;
	int cvAxis = COLVOL_AXIS_Z;

	if (!cvTypeString.empty()) {
		const std::string& cvTypeStr = StringToLower(cvTypeString);
		const std::string& cvTypePrefix = cvTypeStr.substr(0, 3);

		cvType =
			(cvTypePrefix == "ell")? COLVOL_TYPE_ELLIPSOID:
			(cvTypePrefix == "cyl")? COLVOL_TYPE_CYLINDER:
			(cvTypePrefix == "box")? COLVOL_TYPE_BOX:
			COLVOL_TYPE_SPHERE;

		if (cvType == COLVOL_TYPE_CYLINDER) {
			switch (cvTypeStr[cvTypeStr.size() - 1]) {
				case 'x': { cvAxis = COLVOL_AXIS_X; } break;
				case 'y': { cvAxis = COLVOL_AXIS_Y; } break;
				case 'z': { cvAxis = COLVOL_AXIS_Z; } break;
				default: { assert(false); } break;
			}
		}
	}

	Init(cvScales, cvOffsets, cvType, cvContHitTest, cvAxis);
}



void CollisionVolume::Init(float r)
{
	// <r> is the object's default RADIUS (not its diameter),
	// so we need to double it to get the full-length scales
	Init(float3(r * 2.0f, r * 2.0f, r * 2.0f), ZeroVector, volumeType, testType, primaryAxis);
}

void CollisionVolume::Init(const float3& scales, const float3& offsets, int vType, int tType, int pAxis)
{
	// assign these here, since we can be
	// called from outside the constructor
	primaryAxis = std::max(pAxis, 0) % COLVOL_NUM_AXES;
	volumeType  = std::max(vType, 0) % COLVOL_NUM_SHAPES;
	testType    = std::max(tType, 0) % COLVOL_NUM_HITTESTS;

	#if (SUPPORT_COMPLEX_VOLUMES == 0)
	// NOTE:
	//   ellipses and cylinders are now auto-converted to boxes
	//   we assume that if the volume-type is set to ellipse or
	//   cylinder its shape is largely anisotropic such that the
	//   conversion does not create too much of a difference
	volumeType = Clamp(volumeType, int(COLVOL_TYPE_BOX), int(COLVOL_TYPE_FOOTPRINT));
	#endif

	// allow defining a custom volume without using it for coldet
	disabled    = (scales.x < 0.0f || scales.y < 0.0f || scales.z < 0.0f);
	axisOffsets = offsets;

	// make sure none of the scales are ever negative
	// or zero; if the resulting vector is <1, 1, 1>,
	// then the unit / feature loaders will override
	// the (clone) scales with the model's radius
	const float3 adjScales(std::max(1.0f, scales.x), std::max(1.0f, scales.y), std::max(1.0f, scales.z));

	if (volumeType == COLVOL_TYPE_ELLIPSOID) {
		// if all axes are equal in scale, volume is a sphere (a special-case ellipsoid)
		if ((math::fabsf(adjScales.x - adjScales.y) < EPS) &&
		    (math::fabsf(adjScales.y - adjScales.z) < EPS))
		{
			volumeType = COLVOL_TYPE_SPHERE;
		}
	}

	// secondaryAxes[0] = (primaryAxis + 1) % COLVOL_NUM_AXES;
	// secondaryAxes[1] = (primaryAxis + 2) % COLVOL_NUM_AXES;

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

	SetAxisScales(adjScales.x, adjScales.y, adjScales.z);
}


void CollisionVolume::SetBoundingRadius() {
	// set the radius of the minimum bounding sphere
	// that encompasses this custom collision volume
	// (for early-out testing)
	switch (volumeType) {
		case COLVOL_TYPE_BOX: {
			// would be an over-estimation for cylinders
			volumeBoundingRadiusSq = axisHScalesSq.x + axisHScalesSq.y + axisHScalesSq.z;
			volumeBoundingRadius = math::sqrt(volumeBoundingRadiusSq);
		} break;
		case COLVOL_TYPE_CYLINDER: {
			const float prhs = axisHScales[primaryAxis     ];   // primary axis half-scale
			const float sahs = axisHScales[secondaryAxes[0]];   // 1st secondary axis half-scale
			const float sbhs = axisHScales[secondaryAxes[1]];   // 2nd secondary axis half-scale
			const float mshs = std::max(sahs, sbhs);            // max. secondary axis half-scale

			volumeBoundingRadiusSq = prhs * prhs + mshs * mshs;
			volumeBoundingRadius = math::sqrt(volumeBoundingRadiusSq);
		} break;
		case COLVOL_TYPE_ELLIPSOID: {
			volumeBoundingRadius = std::max(axisHScales.x, std::max(axisHScales.y, axisHScales.z));
			volumeBoundingRadiusSq = volumeBoundingRadius * volumeBoundingRadius;
		} break;
		case COLVOL_TYPE_FOOTPRINT:
			// fall through, this is intersection of footprint-prism
			// and sphere, so it has same bounding radius as sphere.
		case COLVOL_TYPE_SPHERE: {
			// max{x, y, z} would suffice here too (see ellipsoid)
			volumeBoundingRadius = axisHScales.x;
			volumeBoundingRadiusSq = volumeBoundingRadius * volumeBoundingRadius;
		} break;
	}
}

void CollisionVolume::SetAxisScales(const float& xs, const float& ys, const float& zs) {
	axisScales.x = xs;
	axisScales.y = ys;
	axisScales.z = zs;

	axisHScales.x = axisScales.x * 0.5f;
	axisHScales.y = axisScales.y * 0.5f;
	axisHScales.z = axisScales.z * 0.5f;

	axisHScalesSq.x = axisHScales.x * axisHScales.x;
	axisHScalesSq.y = axisHScales.y * axisHScales.y;
	axisHScalesSq.z = axisHScales.z * axisHScales.z;

	axisHIScales.x = 1.0f / axisHScales.x;
	axisHIScales.y = 1.0f / axisHScales.y;
	axisHIScales.z = 1.0f / axisHScales.z;

	// scale was unspecified
	defaultScale = (xs == 1.0f && ys == 1.0f && zs == 1.0f);

	SetBoundingRadius();
}

void CollisionVolume::RescaleAxes(const float& xs, const float& ys, const float& zs) {
	axisScales.x *= xs; axisHScales.x *= xs;
	axisScales.y *= ys; axisHScales.y *= ys;
	axisScales.z *= zs; axisHScales.z *= zs;

	axisHScalesSq.x *= (xs * xs);
	axisHScalesSq.y *= (ys * ys);
	axisHScalesSq.z *= (zs * zs);

	axisHIScales.x *= (1.0f / xs);
	axisHIScales.y *= (1.0f / ys);
	axisHIScales.z *= (1.0f / zs);

	SetBoundingRadius();
}



const CollisionVolume* CollisionVolume::GetVolume(const CUnit* u, float3& pos, bool useLastAttackedPiece) {
	const CollisionVolume* vol = NULL;
	const LocalModelPiece* lap = u->lastAttackedPiece;

	if (useLastAttackedPiece) {
		assert(lap != NULL);

		vol = lap->GetCollisionVolume();
		pos = lap->GetAbsolutePos() + vol->GetOffsets();
		pos = u->pos +
			u->rightdir * pos.x +
			u->updir    * pos.y +
			u->frontdir * pos.z;
	} else {
		vol = u->collisionVolume;
		pos = u->midPos + vol->GetOffsets();
	}

	return vol;
}

const CollisionVolume* CollisionVolume::GetVolume(const CFeature* f, float3& pos) {
	pos = f->midPos + f->collisionVolume->GetOffsets();
	return f->collisionVolume;
}



float CollisionVolume::GetPointDistance(const CUnit* u, const float3& pw) const {
	return (GetPointSurfaceDistance(u, u->GetTransformMatrix(true), pw));
}

float CollisionVolume::GetPointDistance(const CFeature* f, const float3& pw) const {
	return (GetPointSurfaceDistance(f, f->transMatrix, pw));
}

float CollisionVolume::GetPointSurfaceDistance(const CSolidObject* o, const CMatrix44f& m, const float3& pw) const {
	CMatrix44f mv = m;
	mv.Translate(o->relMidPos * WORLD_TO_OBJECT_SPACE);
	mv.Translate(GetOffsets());
	mv.InvertAffineInPlace();

	// transform <pw> to volume-space
	float3 pv = mv.Mul(pw);
	float3 pt;

	float l = 0.0f;
	float d = 0.0f;

	switch (volumeType) {
		case COLVOL_TYPE_BOX: {
			// always clamp <pv> to box (!) surface
			// (so minimum distance is always zero)
			pv.x = int(pv.x >= 0.0f) * std::max(math::fabs(pv.x), axisHScales.x);
			pv.y = int(pv.y >= 0.0f) * std::max(math::fabs(pv.y), axisHScales.y);
			pv.z = int(pv.z >= 0.0f) * std::max(math::fabs(pv.z), axisHScales.z);

			// calculate the closest surface point
			pt.x = Clamp(pv.x, -axisHScales.x, axisHScales.x);
			pt.y = Clamp(pv.y, -axisHScales.y, axisHScales.y);
			pt.z = Clamp(pv.z, -axisHScales.z, axisHScales.z);

			// l = std::min(pv.x - pt.x, std::min(pv.y - pt.y, pv.z - pt.z));
			d = (pv - pt).Length();
		} break;

		case COLVOL_TYPE_SPHERE: {
			l = pv.Length();
			d = std::max(l - volumeBoundingRadius, 0.0f);
			// pt = (pv / std::max(0.01f, l)) * d;
		} break;

		case COLVOL_TYPE_FOOTPRINT: {
			// the default type is a sphere volume combined with
			// a rectangular footprint "box" (of infinite height)
			const float3 mins = float3(-o->xsize * SQUARE_SIZE, -10000.0f, -o->zsize * SQUARE_SIZE);
			const float3 maxs = float3( o->xsize * SQUARE_SIZE,  10000.0f,  o->zsize * SQUARE_SIZE);

			// clamp <pv> to object-space footprint (!) edge
			pv.x = int(pv.x >= 0.0f) * std::max(math::fabs(pv.x), float(o->xsize));
			pv.y = int(pv.y >= 0.0f) * std::max(math::fabs(pv.y),      (10000.0f));
			pv.z = int(pv.z >= 0.0f) * std::max(math::fabs(pv.z), float(o->zsize));

			// calculate closest point on footprint (!) edge
			pt.x = Clamp(pv.x, mins.x, maxs.x);
			pt.y = Clamp(pv.y, mins.y, maxs.y);
			pt.z = Clamp(pv.z, mins.z, maxs.z);

			// now take the minimum of the sphere and box cases
			l = pv.Length();
			d = std::max(l - volumeBoundingRadius, 0.0f);
			d = std::min(d, (pv - pt).Length());
		} break;

		default: {
			// getting the closest (orthogonal) distance to a 3D
			// ellipsoid requires numerically solving a 4th-order
			// polynomial --> too expensive, and because we do not
			// want approximations to prevent invulnerable objects
			// we do not support this primitive (anymore)
			// (cylinders with ellipsoid cross-section are similar)
			#if (SUPPORT_COMPLEX_VOLUMES == 0)
			assert(false);
			#endif
		} break;
	}

	return d;
}

