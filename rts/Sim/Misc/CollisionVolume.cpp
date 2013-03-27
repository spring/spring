/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "CollisionVolume.h"
#include "Rendering/Models/3DModel.h"
#include "Sim/Units/Unit.h"
#include "Sim/Features/Feature.h"
#include "System/Matrix44f.h"
#include "System/myMath.h"

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
	float3 clampedScales;

	// make sure none of the scales are ever negative or zero
	//
	// if the clamped vector is <1, 1, 1> (ie. all scales were <= 1.0f)
	// then we assume a "default volume" is wanted and the unit/feature
	// instances will be assigned spheres (of size model->radius)
	clampedScales.x = std::max(1.0f, scales.x);
	clampedScales.y = std::max(1.0f, scales.y);
	clampedScales.z = std::max(1.0f, scales.z);

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
			volumeBoundingRadius = hAxisScales.x;
			volumeBoundingRadiusSq = volumeBoundingRadius * volumeBoundingRadius;
		} break;
	}
}

void CollisionVolume::SetAxisScales(const float3& scales) {
	fAxisScales = scales;
	hAxisScales = fAxisScales * 0.5f;

	hsqAxisScales = hAxisScales * hAxisScales;

	hiAxisScales.x = 1.0f / hAxisScales.x;
	hiAxisScales.y = 1.0f / hAxisScales.y;
	hiAxisScales.z = 1.0f / hAxisScales.z;
}

void CollisionVolume::RescaleAxes(const float3& scales) {
	fAxisScales *= scales;
	hAxisScales *= scales;

	hsqAxisScales *= (scales * scales);

	hiAxisScales.x *= (1.0f / scales.x);
	hiAxisScales.y *= (1.0f / scales.y);
	hiAxisScales.z *= (1.0f / scales.z);
}

void CollisionVolume::FixTypeAndScale(float3& scales) {
	// NOTE:
	//   ellipses are now ALWAYS auto-converted to boxes or
	//   to spheres depending on scale values, cylinders to
	//   base cylinders (ie. with circular cross-section)
	//
	//   we assume that if the volume-type is set to ellipse
	//   then its shape is largely anisotropic such that the
	//   conversion does not create too much of a difference
	//
	//   prevent Lua (which calls InitShape directly) from
	//   creating non-uniform spheres to emulate ellipsoids
	if (volumeType == COLVOL_TYPE_SPHERE) {
		scales.x = std::max(scales.x, std::max(scales.y, scales.z));
		scales.y = scales.x;
		scales.z = scales.x;
	}

	if (volumeType == COLVOL_TYPE_ELLIPSOID) {
		const float dxyAbs = math::fabsf(scales.x - scales.y);
		const float dyzAbs = math::fabsf(scales.y - scales.z);
		const float d12Abs = math::fabsf(scales[volumeAxes[1]] - scales[volumeAxes[2]]);

		if (dxyAbs < COLLISION_VOLUME_EPS && dyzAbs < COLLISION_VOLUME_EPS) {
			volumeType = COLVOL_TYPE_SPHERE;
		} else {
			if (d12Abs < COLLISION_VOLUME_EPS) {
				volumeType = COLVOL_TYPE_CYLINDER;
			} else {
				volumeType = COLVOL_TYPE_BOX;
			}
		}
	}

	if (volumeType == COLVOL_TYPE_CYLINDER) {
		scales[volumeAxes[1]] = std::max(scales[volumeAxes[1]], scales[volumeAxes[2]]);
		scales[volumeAxes[2]] =          scales[volumeAxes[1]];
	}
}



float3 CollisionVolume::GetWorldSpacePos(const CSolidObject* o, const float3& extOffsets) const {
	float3 pos = o->midPos;
	pos += (o->rightdir * (axisOffsets.x + extOffsets.x));
	pos += (o->updir    * (axisOffsets.y + extOffsets.y));
	pos += (o->frontdir * (axisOffsets.z + extOffsets.z));
	return pos;
}



const CollisionVolume* CollisionVolume::GetVolume(const CSolidObject* o, const LocalModelPiece* lmp) {
	const CollisionVolume* vol = o->collisionVolume;

	if (vol->DefaultToPieceTree() && lmp != NULL) {
		vol = lmp->GetCollisionVolume();
	}

	return vol;
}



float CollisionVolume::GetPointSurfaceDistance(const CUnit* u, const LocalModelPiece* lmp, const float3& p) const {
	const CollisionVolume* vol = u->collisionVolume;

	CMatrix44f mat = u->GetTransformMatrix(true);

	if (vol->DefaultToPieceTree() && lmp != NULL) {
		// NOTE: if we get here, <this> is the piece-volume
		assert(this == lmp->GetCollisionVolume());

		// transform into piece-space relative to pos
		mat = lmp->GetModelSpaceMatrix() * mat;
	} else {
		// Unit::GetTransformMatrix does not include this
		// (its translation component is pos, not midPos)
		mat.Translate(u->relMidPos * WORLD_TO_OBJECT_SPACE);
	}

	mat.Translate(GetOffsets());
	mat.InvertAffineInPlace();

	return (GetPointSurfaceDistance(mat, p));
}

float CollisionVolume::GetPointSurfaceDistance(const CFeature* f, const LocalModelPiece* /*lmp*/, const float3& p) const {
	CMatrix44f mat = f->GetTransformMatrixRef();

	mat.Translate(f->relMidPos * WORLD_TO_OBJECT_SPACE);
	mat.Translate(GetOffsets());
	mat.InvertAffineInPlace();

	return (GetPointSurfaceDistance(mat, p));
}

float CollisionVolume::GetPointSurfaceDistance(const CMatrix44f& mv, const float3& p) const {
	// transform <p> from world- to volume-space
	float3 pv = mv.Mul(p);
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
			d = pv.distance(pt);
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

