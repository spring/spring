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
	CR_MEMBER(ignoreHits),
	CR_MEMBER(useContHitTest),
	CR_MEMBER(defaultToFootPrint),
	CR_MEMBER(defaultToPieceTree)
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

	ignoreHits             = false;
	useContHitTest         = COLVOL_HITTEST_CONT;
	defaultToFootPrint     = false;
	defaultToPieceTree     = false;
}

CollisionVolume& CollisionVolume::operator = (const CollisionVolume& v) {
	fAxisScales            = v.fAxisScales;
	hAxisScales            = v.hAxisScales;
	hsqAxisScales          = v.hsqAxisScales;
	hiAxisScales           = v.hiAxisScales;
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
			case 'c': { cvType = COLVOL_TYPE_CYLINDER; } break; // "cyl..."
			case 'b': { cvType = COLVOL_TYPE_BOX; } break; // "box"
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
	InitShape(float3(1.0f, 1.0f, 1.0f) * radius * 2.0f, ZeroVector, COLVOL_TYPE_SPHERE, COLVOL_HITTEST_CONT, COLVOL_AXIS_Z);
}

void CollisionVolume::InitBox(const float3& scales)
{
	InitShape(scales, ZeroVector, COLVOL_TYPE_BOX, COLVOL_HITTEST_CONT, COLVOL_AXIS_Z);
}

void CollisionVolume::InitShape(
	const float3& scales,
	const float3& offsets,	
	const int vType,
	const int tType,
	const int pAxis)
{
	float3 s;

	// make sure none of the scales are ever negative or zero
	//
	// if the clamped vector is <1, 1, 1> (ie. all scales were <= 1.0f)
	// then we assume a "default volume" is wanted and the unit/feature
	// instances will be assigned spheres (of size model->radius)
	s.x = std::max(1.0f, scales.x);
	s.y = std::max(1.0f, scales.y);
	s.z = std::max(1.0f, scales.z);

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

	// NOTE:
	//   ellipses are now ALWAYS auto-converted to boxes or
	//   to spheres depending on scale values, cylinders to
	//   base cylinders (ie. with circular cross-section)
	//
	//   we assume that if the volume-type is set to ellipse
	//   then its shape is largely anisotropic such that the
	//   conversion does not create too much of a difference
	//
	if (volumeType == COLVOL_TYPE_ELLIPSOID) {
		if ((math::fabsf(s.x - s.y) < COLLISION_VOLUME_EPS) && (math::fabsf(s.y - s.z) < COLLISION_VOLUME_EPS)) {
			volumeType = COLVOL_TYPE_SPHERE;
		} else {
			volumeType = COLVOL_TYPE_BOX;
		}
	}
	if (volumeType == COLVOL_TYPE_CYLINDER) {
		s[volumeAxes[1]] = std::max(s[volumeAxes[1]], s[volumeAxes[2]]);
		s[volumeAxes[2]] =          s[volumeAxes[1]];
	}

	SetAxisScales(s);
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
		case COLVOL_TYPE_SPHERE: {
			assert(math::fabs(hAxisScales.x - hAxisScales.y) < COLLISION_VOLUME_EPS);
			assert(math::fabs(hAxisScales.y - hAxisScales.z) < COLLISION_VOLUME_EPS);

			volumeBoundingRadius = hAxisScales.x;
			volumeBoundingRadiusSq = volumeBoundingRadius * volumeBoundingRadius;
		} break;
	}
}

void CollisionVolume::SetAxisScales(const float3& scales) {
	fAxisScales.x = scales.x;
	fAxisScales.y = scales.y;
	fAxisScales.z = scales.z;

	hAxisScales.x = fAxisScales.x * 0.5f;
	hAxisScales.y = fAxisScales.y * 0.5f;
	hAxisScales.z = fAxisScales.z * 0.5f;

	hsqAxisScales.x = hAxisScales.x * hAxisScales.x;
	hsqAxisScales.y = hAxisScales.y * hAxisScales.y;
	hsqAxisScales.z = hAxisScales.z * hAxisScales.z;

	hiAxisScales.x = 1.0f / hAxisScales.x;
	hiAxisScales.y = 1.0f / hAxisScales.y;
	hiAxisScales.z = 1.0f / hAxisScales.z;
}

void CollisionVolume::RescaleAxes(const float3& scales) {
	fAxisScales.x *= scales.x; hAxisScales.x *= scales.x;
	fAxisScales.y *= scales.y; hAxisScales.y *= scales.y;
	fAxisScales.z *= scales.z; hAxisScales.z *= scales.z;

	hsqAxisScales.x *= (scales.x * scales.x);
	hsqAxisScales.y *= (scales.y * scales.y);
	hsqAxisScales.z *= (scales.z * scales.z);

	hiAxisScales.x *= (1.0f / scales.x);
	hiAxisScales.y *= (1.0f / scales.y);
	hiAxisScales.z *= (1.0f / scales.z);
}



const CollisionVolume* CollisionVolume::GetVolume(const CUnit* u, float3& pos) {
	const CollisionVolume* vol = u->collisionVolume;
	const LocalModelPiece* lap = u->lastAttackedPiece;

	if (vol->DefaultToPieceTree() && u->HaveLastAttackedPiece(gs->frameNum)) {
		assert(lap != NULL);

		vol = lap->GetCollisionVolume();
		pos = lap->GetAbsolutePos() + vol->GetOffsets();
		pos = u->pos +
			u->rightdir * pos.x +
			u->updir    * pos.y +
			u->frontdir * pos.z;
	} else {
		pos = u->midPos + vol->GetOffsets();
	}

	return vol;
}

const CollisionVolume* CollisionVolume::GetVolume(const CFeature* f, float3& pos) {
	pos = f->midPos + f->collisionVolume->GetOffsets();
	return f->collisionVolume;
}



float CollisionVolume::GetPointDistance(const CUnit* u, const float3& pw) const {
	const CollisionVolume* v = u->collisionVolume;
	const LocalModelPiece* p = u->lastAttackedPiece;

	CMatrix44f m = u->GetTransformMatrix(true);
	float3 o = GetOffsets();

	m.Translate(u->relMidPos * WORLD_TO_OBJECT_SPACE);

	if (v->DefaultToPieceTree() && u->HaveLastAttackedPiece(gs->frameNum)) {
		// NOTE: if we get here, then <this> is the piece-volume
		assert(p != NULL);
		assert(this == p->GetCollisionVolume());

		// need to transform into piece-space
		m = m * p->GetModelSpaceMatrix();
		o = -o;
	}

	m.Translate(o);
	m.InvertAffineInPlace();

	return (GetPointSurfaceDistance(m, pw));
}

float CollisionVolume::GetPointDistance(const CFeature* f, const float3& pw) const {
	CMatrix44f m = f->transMatrix;

	m.Translate(f->relMidPos * WORLD_TO_OBJECT_SPACE);
	m.Translate(GetOffsets());
	m.InvertAffineInPlace();

	return (GetPointSurfaceDistance(m, pw));
}

float CollisionVolume::GetPointSurfaceDistance(const CMatrix44f& mv, const float3& pw) const {
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

