/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "CollisionVolume.h"
#include "Rendering/Models/3DModel.h"
#include "Sim/Units/Unit.h"
#include "Sim/Features/Feature.h"
#include "System/Matrix44f.h"
#include "System/myMath.h"
#include "System/mmgr.h"

CR_BIND(CollisionVolume, );
CR_REG_METADATA(CollisionVolume, (
	CR_MEMBER(fAxisScales),
	CR_MEMBER(hAxisScales),
	CR_MEMBER(hsqAxisScales),
	CR_MEMBER(hiAxisScales),
	CR_MEMBER(axisOffsets),
	CR_MEMBER(volumeBoundingRadius),
	CR_MEMBER(volumeBoundingRadiusSq),
	CR_MEMBER(volumeType),
	CR_MEMBER(volumeAxes),
	CR_MEMBER(disabled),
	CR_MEMBER(defaultScale),
	CR_MEMBER(contHitTest)
));

// base ctor (CREG-only)
CollisionVolume::CollisionVolume()
{
	fAxisScales            = float3(2.0f, 2.0f, 2.0f);
	hAxisScales            = float3(1.0f, 1.0f, 1.0f);
	hsqAxisScales          = float3(1.0f, 1.0f, 1.0f);
	hiAxisScales           = float3(1.0f, 1.0f, 1.0f);
	axisOffsets            = ZeroVector;
	volumeBoundingRadius   = 1.0f;
	volumeBoundingRadiusSq = 1.0f;
	volumeType             = COLVOL_TYPE_SPHERE;
	volumeAxes[0]          = COLVOL_AXIS_Z;
	volumeAxes[1]          = COLVOL_AXIS_X;
	volumeAxes[2]          = COLVOL_AXIS_Y;
	disabled               = false;
	defaultScale           = true;
	contHitTest            = COLVOL_HITTEST_DISC;
}

// copy ctor
CollisionVolume::CollisionVolume(const CollisionVolume* v, float defaultRadius)
{
	fAxisScales            = v->fAxisScales;
	hAxisScales            = v->hAxisScales;
	hsqAxisScales          = v->hsqAxisScales;
	hiAxisScales           = v->hiAxisScales;
	axisOffsets            = v->axisOffsets;
	volumeBoundingRadius   = v->volumeBoundingRadius;
	volumeBoundingRadiusSq = v->volumeBoundingRadiusSq;
	volumeType             = v->volumeType;
	volumeAxes[0]          = v->volumeAxes[0];
	volumeAxes[1]          = v->volumeAxes[1];
	volumeAxes[2]          = v->volumeAxes[2];
	disabled               = v->disabled;
	defaultScale           = v->defaultScale;
	contHitTest            = v->contHitTest;

	if (defaultScale && defaultRadius > 0.0f) {
		// if the volume being copied was not given
		// explicit scales, convert the clone into a
		// sphere if provided with a non-zero radius
		InitSphere(defaultRadius);
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

	InitCustom(cvScales, cvOffsets, cvType, cvContHitTest, cvAxis);
}



void CollisionVolume::InitSphere(float r)
{
	// <r> is the object's default RADIUS (not its diameter),
	// so we need to double it to get the full-length scales
	InitCustom(float3(r * 2.0f, r * 2.0f, r * 2.0f), ZeroVector, volumeType, contHitTest, volumeAxes[0]);
}

void CollisionVolume::InitCustom(const float3& scales, const float3& offsets, int vType, int tType, int pAxis)
{
	// assign these here, since we can be
	// called from outside the constructor
	volumeType    = std::max(vType, 0) % COLVOL_NUM_SHAPES;
	volumeAxes[0] = std::max(pAxis, 0) % COLVOL_NUM_AXES;

	axisOffsets = offsets;

	// allow defining a custom volume without using it for coldet
	disabled    = (scales.x < 0.0f || scales.y < 0.0f || scales.z < 0.0f);
	contHitTest = std::max(tType, 0) % COLVOL_NUM_HITTESTS;

	// make sure none of the scales are ever negative or zero
	//
	// if the clamped vector is <1, 1, 1> (ie. all scales were <= 1.0f)
	// then we assume a "default scale" is wanted and the unit / feature
	// loaders will override the (unit/feature instance) scales with the
	// model's radius
	float3 cScales(std::max(1.0f, scales.x), std::max(1.0f, scales.y), std::max(1.0f, scales.z));

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

	// NOTE:
	//   ellipses are now auto-converted to boxes, cylinders
	//   to base cylinders (ie. with circular cross-section)
	//
	//   we assume that if the volume-type is set to ellipse
	//   then its shape is largely anisotropic such that the
	//   conversion does not create too much of a difference
	//
	//   if all axes are equal in scale, ellipsoid volume is
	//   a sphere (a special-case ellipsoid), otherwise turn
	//   it into a box
	if (volumeType == COLVOL_TYPE_ELLIPSOID) {
		if ((math::fabsf(cScales.x - cScales.y) < COLLISION_VOLUME_EPS) && (math::fabsf(cScales.y - cScales.z) < COLLISION_VOLUME_EPS)) {
			volumeType = COLVOL_TYPE_SPHERE;
		} else {
			volumeType = COLVOL_TYPE_BOX;
		}
	}

	if (volumeType == COLVOL_TYPE_CYLINDER) {
		cScales[volumeAxes[1]] = std::max(cScales[volumeAxes[1]], cScales[volumeAxes[2]]);
		cScales[volumeAxes[2]] =          cScales[volumeAxes[1]];
	}

	SetAxisScales(cScales.x, cScales.y, cScales.z);
}


void CollisionVolume::SetBoundingRadius() {
	// set the radius of the minimum bounding sphere
	// that encompasses this custom collision volume
	// (for early-out testing)
	switch (volumeType) {
		case COLVOL_TYPE_BOX: {
			// would be an over-estimation for cylinders
			volumeBoundingRadiusSq = hsqAxisScales.x + hsqAxisScales.y + hsqAxisScales.z;
			volumeBoundingRadius = math::sqrt(volumeBoundingRadiusSq);
		} break;
		case COLVOL_TYPE_CYLINDER: {
			const float prhs = hAxisScales[volumeAxes[0]];   // primary axis half-scale
			const float sahs = hAxisScales[volumeAxes[1]];   // 1st secondary axis half-scale
			const float sbhs = hAxisScales[volumeAxes[2]];   // 2nd secondary axis half-scale
			const float mshs = std::max(sahs, sbhs);         // max. secondary axis half-scale

			volumeBoundingRadiusSq = prhs * prhs + mshs * mshs;
			volumeBoundingRadius = math::sqrt(volumeBoundingRadiusSq);
		} break;
		case COLVOL_TYPE_ELLIPSOID: {
			volumeBoundingRadius = std::max(hAxisScales.x, std::max(hAxisScales.y, hAxisScales.z));
			volumeBoundingRadiusSq = volumeBoundingRadius * volumeBoundingRadius;
		} break;
		case COLVOL_TYPE_FOOTPRINT:
			// fall through, this is intersection of footprint-prism
			// and sphere, so it has same bounding radius as sphere.
		case COLVOL_TYPE_SPHERE: {
			// max{x, y, z} would suffice here too (see ellipsoid)
			volumeBoundingRadius = hAxisScales.x;
			volumeBoundingRadiusSq = volumeBoundingRadius * volumeBoundingRadius;
		} break;
	}
}

void CollisionVolume::SetAxisScales(const float xs, const float ys, const float zs) {
	fAxisScales.x = xs;
	fAxisScales.y = ys;
	fAxisScales.z = zs;

	hAxisScales.x = fAxisScales.x * 0.5f;
	hAxisScales.y = fAxisScales.y * 0.5f;
	hAxisScales.z = fAxisScales.z * 0.5f;

	hsqAxisScales.x = hAxisScales.x * hAxisScales.x;
	hsqAxisScales.y = hAxisScales.y * hAxisScales.y;
	hsqAxisScales.z = hAxisScales.z * hAxisScales.z;

	hiAxisScales.x = 1.0f / hAxisScales.x;
	hiAxisScales.y = 1.0f / hAxisScales.y;
	hiAxisScales.z = 1.0f / hAxisScales.z;

	// scale was unspecified
	defaultScale = (xs == 1.0f && ys == 1.0f && zs == 1.0f);

	SetBoundingRadius();
}

void CollisionVolume::RescaleAxes(const float xs, const float ys, const float zs) {
	fAxisScales.x *= xs; hAxisScales.x *= xs;
	fAxisScales.y *= ys; hAxisScales.y *= ys;
	fAxisScales.z *= zs; hAxisScales.z *= zs;

	hsqAxisScales.x *= (xs * xs);
	hsqAxisScales.y *= (ys * ys);
	hsqAxisScales.z *= (zs * zs);

	hiAxisScales.x *= (1.0f / xs);
	hiAxisScales.y *= (1.0f / ys);
	hiAxisScales.z *= (1.0f / zs);

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
			pv.x = ((int(pv.x >= 0.0f) * 2) - 1) * std::max(math::fabs(pv.x), hAxisScales.x);
			pv.y = ((int(pv.y >= 0.0f) * 2) - 1) * std::max(math::fabs(pv.y), hAxisScales.y);
			pv.z = ((int(pv.z >= 0.0f) * 2) - 1) * std::max(math::fabs(pv.z), hAxisScales.z);

			// calculate the closest surface point
			pt.x = Clamp(pv.x, -hAxisScales.x, hAxisScales.x);
			pt.y = Clamp(pv.y, -hAxisScales.y, hAxisScales.y);
			pt.z = Clamp(pv.z, -hAxisScales.z, hAxisScales.z);

			// l = std::min(pv.x - pt.x, std::min(pv.y - pt.y, pv.z - pt.z));
			d = (pv - pt).Length();
		} break;

		case COLVOL_TYPE_SPHERE: {
			l = pv.Length();
			d = std::max(l - volumeBoundingRadius, 0.0f);
			// pt = (pv / std::max(0.01f, l)) * d;
		} break;

		case COLVOL_TYPE_FOOTPRINT: {
			#if 0
			#define INSIDE_RECTANGLE(p, mins, maxs) \
				(p.x >= mins.x && p.x <= maxs.x) && \
				(p.y >= mins.y && p.y <= maxs.y) && \
				(p.z >= mins.z && p.z <= maxs.z)

			const float3 mins = float3(-(o->xsize >> 1) * SQUARE_SIZE, -10000.0f, -(o->zsize >> 1) * SQUARE_SIZE);
			const float3 maxs = float3( (o->xsize >> 1) * SQUARE_SIZE,  10000.0f,  (o->zsize >> 1) * SQUARE_SIZE);

			// if <pv> lies outside the sphere, project it onto
			// its surface (we might even be done at this point)
			if (pv.SqLength() > volumeBoundingRadiusSq) {
				pt = pv.Normalize() * volumeBoundingRadius;
			} else {
				pt = pv;
			}

			// if <pv>'s surface projection also lies inside the
			// footprint, use it to determine orthogonal distance
			// to the combined shape and bail early
			if (INSIDE_RECTANGLE(pt, mins, maxs)) {
				d = (pv - pt).Length(); break;
			}

			// otherwise project <pv> inward onto the footprint edge
			// (which we now know must be encompassed by the sphere)
			pt.x = Clamp(pv.x, mins.x, maxs.x);
			pt.y = Clamp(pv.y, mins.y, maxs.y);
			pt.z = Clamp(pv.z, mins.z, maxs.z);

			// if point on footprint edge also lies within the sphere,
			// use it to determine distance to the sphere/fprint shape
			if (pt.SqLength() <= volumeBoundingRadiusSq) {
				d = (pv - pt).Length();
			} else {
				// general case, intersection of sphere and footprint
				// prism forms complex shape and distance calculation
				// gets messy
			}

			#undef INSIDE_RECTANGLE
			#endif

			// FIXME:
			//   collision point always lies on the sphere surface
			//   because CCollisionHandler does not intersect rays
			//   with footprint prisms --> the above code will not
			//   work correctly in case sphere encompasses box
			l = pv.Length();
			d = std::max(l - volumeBoundingRadius, 0.0f);
		} break;

		case COLVOL_TYPE_CYLINDER: {
			// code below is only valid for non-ellipsoidal cylinders
			assert(hAxisScales[volumeAxes[1]] == hAxisScales[volumeAxes[2]]);

			float pSq = 0.0f;
			float rSq = 0.0f;

			#define CYLINDER_CASE(a, b, c)                                                  \
			{                                                                               \
				pSq = pv.b * pv.b + pv.c * pv.c;                                            \
				rSq = hsqAxisScales.b + hsqAxisScales.c;                                    \
                                                                                            \
				if (pv.a >= -hAxisScales.a && pv.a <= hAxisScales.a) {                      \
					/* case 1: point is between end-cap bounds along primary axis */        \
					d = std::max(math::sqrt(pSq) - math::sqrt(rSq), 0.0f);                  \
				} else {                                                                    \
					if (pSq <= rSq) {                                                       \
						/* case 2: point is outside end-cap bounds but inside inf-tube */   \
						d = std::max(math::fabs(pv.a) - hAxisScales.a, 0.0f);               \
					} else {                                                                \
						/* general case: compute distance to end-cap edge */                \
						l = math::fabs(pv.a) - hAxisScales.a;                               \
						d = 1.0f / (math::sqrt(pSq) / math::sqrt(rSq));                     \
						pt.b = pv.b * d;                                                    \
						pt.c = pv.c * d;                                                    \
						d = math::sqrt((l * l) + (pv - pt).SqLength());                     \
					}                                                                       \
				}                                                                           \
			}

			switch (volumeAxes[0]) {
				case COLVOL_AXIS_X: {
					CYLINDER_CASE(x, y, z)
				} break;

				case COLVOL_AXIS_Y: {
					CYLINDER_CASE(y, x, z)
				} break;

				case COLVOL_AXIS_Z: {
					CYLINDER_CASE(z, x, y)
				} break;
			}

			#undef CYLINDER_CASE
		} break;

		default: {
			// getting the closest (orthogonal) distance to a 3D
			// ellipsoid requires numerically solving a 4th-order
			// polynomial --> too expensive, and because we do not
			// want approximations to prevent invulnerable objects
			// we do not support this primitive (anymore)
			assert(false);
		} break;
	}

	return d;
}

