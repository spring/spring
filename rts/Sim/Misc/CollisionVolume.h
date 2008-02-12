#ifndef COLLISION_VOLUME_H
#define COLLISION_VOLUME_H

#include "StdAfx.h"
#include "creg/creg.h"

enum COLVOL_TYPES {COLVOL_TYPE_ELLIPSOID, COLVOL_TYPE_CYLINDER, COLVOL_TYPE_BOX};
enum COLVOL_AXES {COLVOL_AXIS_X, COLVOL_AXIS_Y, COLVOL_AXIS_Z};

class CCollisionVolume {
	public:
		CR_DECLARE(CCollisionVolume)

		CCollisionVolume() {}
		CCollisionVolume(const float3&, const float3&, int, int);
		~CCollisionVolume();

		bool Collision(const CMatrix44f&, const float3&) const;

		int GetType() const { return volumeType; }
		int GetPrimaryAxis() const { return primaryAxis; }
		float GetBoundingRadiusSq() const { return (volumeBoundingRadius * volumeBoundingRadius); }
		float GetScale(int axis) const { return axisScales[axis]; }
		float GetOffset(int axis) const { return axisOffsets[axis]; }
		bool IsSphere() const { return spherical; }

	private:
		float axisScales[3];
		float axisOffsets[3];
		float volumeBoundingRadius;
		int volumeType;
		int primaryAxis;
		int secondaryAxes[2];
		bool spherical;
};

#endif
