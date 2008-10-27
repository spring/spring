#ifndef COLLISION_VOLUME_H
#define COLLISION_VOLUME_H

#include "creg/creg.h"
#include "Util.h"

const float EPS = 0.0000000001f;

enum COLVOL_TYPES {COLVOL_TYPE_ELLIPSOID, COLVOL_TYPE_CYLINDER, COLVOL_TYPE_BOX};
enum COLVOL_AXES {COLVOL_AXIS_X, COLVOL_AXIS_Y, COLVOL_AXIS_Z};
enum COLVOL_TESTS {COLVOL_TEST_DISC, COLVOL_TEST_CONT};

struct CollisionVolume {
	CR_DECLARE_STRUCT(CollisionVolume);

	CollisionVolume(const CollisionVolume* src = 0x0) {
		axisScales             = src? src->axisScales:             float3(2.0f, 2.0f, 2.0f);
		axisHScales            = src? src->axisHScales:            float3(1.0f, 1.0f, 1.0f);
		axisHScalesSq          = src? src->axisHScalesSq:          float3(1.0f, 1.0f, 1.0f);
		axisHIScales           = src? src->axisHIScales:           float3(1.0f, 1.0f, 1.0f);
		axisOffsets            = src? src->axisOffsets:            ZeroVector;
		volumeBoundingRadius   = src? src->volumeBoundingRadius:   1.0f;
		volumeBoundingRadiusSq = src? src->volumeBoundingRadiusSq: 1.0f;
		volumeType             = src? src->volumeType:             COLVOL_TYPE_ELLIPSOID;
		testType               = src? src->testType:               COLVOL_TEST_DISC;
		primaryAxis            = src? src->primaryAxis:            COLVOL_AXIS_Z;
		secondaryAxes[0]       = src? src->secondaryAxes[0]:       COLVOL_AXIS_X;
		secondaryAxes[1]       = src? src->secondaryAxes[1]:       COLVOL_AXIS_Y;
		spherical              = src? src->spherical:              true;
		disabled               = src? src->disabled:               false;
	}

	CollisionVolume(const std::string& typeStr, const float3& scales, const float3& offsets, int testType) {
		if (typeStr.size() > 0) {
			std::string typeStrLC(StringToLower(typeStr));

			if (typeStrLC.find("ell") != std::string::npos) {
				volumeType = COLVOL_TYPE_ELLIPSOID;
			}

			if (typeStrLC.find("cyl") != std::string::npos) {
				volumeType = COLVOL_TYPE_CYLINDER;

				if (typeStrLC.size() == 4) {
					if (typeStrLC[3] == 'x') { primaryAxis = COLVOL_AXIS_X; }
					if (typeStrLC[3] == 'y') { primaryAxis = COLVOL_AXIS_Y; }
					if (typeStrLC[3] == 'z') { primaryAxis = COLVOL_AXIS_Z; }
				}
			}

			if (typeStrLC.find("box") != std::string::npos) {
				volumeType = COLVOL_TYPE_BOX;
			}
		} else {
			volumeType = COLVOL_TYPE_ELLIPSOID;
			primaryAxis = COLVOL_AXIS_Z;
		}

		Init(scales, offsets, volumeType, testType, primaryAxis);
	}


	void SetDefaultScale(const float s) {
		// called iif unit or feature defines no custom volume,
		// <s> is the object's default RADIUS (not its diameter)
		// so we need to double it to get the full-length scales
		const float3 scales(s * 2.0f, s * 2.0f, s * 2.0f);

		Init(scales, ZeroVector, volumeType, testType, primaryAxis);
	}


	void Init(const float3& scales, const float3& offsets, int vType, int tType, int pAxis) {
		disabled = (tType < 0);
		tType = std::max(tType, 0);

		// assign these here, since we can be
		// called from outside the constructor
		primaryAxis = pAxis % 3;
		volumeType = vType % 3;
		testType = tType % 2;

		axisScales.x = (scales.x < 1.0f)? 1.0f: scales.x;
		axisScales.y = (scales.y < 1.0f)? 1.0f: scales.y;
		axisScales.z = (scales.z < 1.0f)? 1.0f: scales.z;

		axisHScales.x = axisScales.x * 0.5f;  axisHScalesSq.x = axisHScales.x * axisHScales.x;
		axisHScales.y = axisScales.y * 0.5f;  axisHScalesSq.y = axisHScales.y * axisHScales.y;
		axisHScales.z = axisScales.z * 0.5f;  axisHScalesSq.z = axisHScales.z * axisHScales.z;

		axisHIScales.x = 1.0f / axisHScales.x;  axisOffsets.x = offsets.x;
		axisHIScales.y = 1.0f / axisHScales.y;  axisOffsets.y = offsets.y;
		axisHIScales.z = 1.0f / axisHScales.z;  axisOffsets.z = offsets.z;

		// if all axes (or half-axes) are equal in scale, volume is a sphere
		spherical =
			((volumeType == COLVOL_TYPE_ELLIPSOID) &&
			(streflop::fabsf(axisHScales.x - axisHScales.y) < EPS) &&
			(streflop::fabsf(axisHScales.y - axisHScales.z) < EPS));

		// secondaryAxes[0] = (primaryAxis + 1) % 3;
		// secondaryAxes[1] = (primaryAxis + 2) % 3;

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
				volumeBoundingRadius = streflop::sqrt(volumeBoundingRadiusSq);
			} break;
			case COLVOL_TYPE_CYLINDER: {
				const float prhs = axisHScales[primaryAxis     ];   // primary axis half-scale
				const float sahs = axisHScales[secondaryAxes[0]];   // 1st secondary axis half-scale
				const float sbhs = axisHScales[secondaryAxes[1]];   // 2nd secondary axis half-scale
				const float mshs = std::max(sahs, sbhs);            // max. secondary axis half-scale

				volumeBoundingRadiusSq = prhs * prhs + mshs * mshs;
				volumeBoundingRadius = streflop::sqrtf(volumeBoundingRadiusSq);
			} break;
			case COLVOL_TYPE_ELLIPSOID: {
				if (spherical) {
					// max{x, y, z} would suffice here too
					volumeBoundingRadius = axisHScales.x;
				} else {
					volumeBoundingRadius = std::max(axisHScales.x, std::max(axisHScales.y, axisHScales.z));
				}

				volumeBoundingRadiusSq = volumeBoundingRadius * volumeBoundingRadius;
			} break;
		}
	}

	int GetVolumeType() const { return volumeType; }
	int GetTestType() const { return testType; }
	int GetPrimaryAxis() const { return primaryAxis; }
	float GetBoundingRadius() const { return volumeBoundingRadius; }
	float GetBoundingRadiusSq() const { return volumeBoundingRadiusSq; }
	float GetScale(int axis) const { return axisScales[axis]; }
	float GetHScale(int axis) const { return axisHScales[axis]; }
	float GetHScaleSq(int axis) const { return axisHScalesSq[axis]; }
	float GetOffset(int axis) const { return axisOffsets[axis]; }
	bool IsSphere() const { return spherical; }


	float3 axisScales;					// full-length axis scales
	float3 axisHScales;					// half-length axis scales
	float3 axisHScalesSq;				// half-length axis scales (squared)
	float3 axisHIScales;				// half-length axis scales (inverted)
	float3 axisOffsets;

	float volumeBoundingRadius;			// radius of minimally-bounding sphere around volume
	float volumeBoundingRadiusSq;		// squared radius of minimally-bounding sphere
	int volumeType;
	int testType;
	int primaryAxis;
	int secondaryAxes[2];
	bool spherical;
	bool disabled;
};

#endif
