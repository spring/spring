/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "CollisionVolume.h"
#include "System/Log/ILog.h"
#include "System/mmgr.h"


#define LOG_SECTION_COLVOL "CollisionVolume"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_COLVOL)

// use the specific section for all LOG*() calls in this source file
#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_COLVOL


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
	volumeType             = COLVOL_TYPE_ELLIPSOID;
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

CollisionVolume::CollisionVolume(const std::string& volTypeString, const float3& scales, const float3& offsets, int hitTestType)
{
	int volType = COLVOL_TYPE_FOOTPRINT;
	int volAxis = COLVOL_AXIS_Z;

	if (!volTypeString.empty()) {
		const std::string& volTypeStr = StringToLower(volTypeString);
		const std::string& volTypePrefix = volTypeStr.substr(0, 3);

		if (volTypePrefix == "ell") {
			volType = COLVOL_TYPE_ELLIPSOID;
		} else if (volTypePrefix == "cyl") {
			volType = COLVOL_TYPE_CYLINDER;

			switch (volTypeStr[volTypeStr.size() - 1]) {
				case 'x': { volAxis = COLVOL_AXIS_X; } break;
				case 'y': { volAxis = COLVOL_AXIS_Y; } break;
				case 'z': { volAxis = COLVOL_AXIS_Z; } break;
				default: {} break;
			}
		} else if (volTypePrefix == "box") {
			volType = COLVOL_TYPE_BOX;
		}
	}

	const char* typeStr = NULL;
	switch (volType) {
		case COLVOL_TYPE_ELLIPSOID: {
			typeStr = "ellipsoid";
		} break;
		case COLVOL_TYPE_CYLINDER: {
			typeStr = "cylinder";
		} break;
		case COLVOL_TYPE_BOX: {
			typeStr = "cuboid";
		} break;
		case COLVOL_TYPE_FOOTPRINT: {
			typeStr = "footprint";
		} break;
		default: {} break;
	}
	if (typeStr != NULL) {
		LOG_L(L_DEBUG,
				"%s (scale: <%.2f, %.2f, %.2f>, "
				"offsets: <%.2f, %.2f, %.2f>, "
				"test-type: %d, axis: %d)",
				typeStr,
				scales.x, scales.y, scales.z,
				offsets.x, offsets.y, offsets.z,
				hitTestType, volAxis);
	}

	Init(scales, offsets, volType, hitTestType, volAxis);
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
			LOG_L(L_DEBUG, "auto-converting spherical COLVOL_TYPE_ELLIPSOID to COLVOL_TYPE_SPHERE");
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
