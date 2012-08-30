/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef COLLISION_VOLUME_H
#define COLLISION_VOLUME_H

#include "System/float3.h"
#include "System/creg/creg_cond.h"
#include "System/Util.h"

const float COLLISION_VOLUME_EPS = 0.0000000001f;
const float3 WORLD_TO_OBJECT_SPACE = float3(-1.0f, 1.0f, 1.0f);

struct LocalModelPiece;
class CMatrix44f;
class CSolidObject;
class CUnit;
class CFeature;

struct CollisionVolume
{
	CR_DECLARE_STRUCT(CollisionVolume);

public:
	enum COLVOL_SHAPE_TYPES {
		COLVOL_TYPE_ELLIPSOID =  0,
		COLVOL_TYPE_CYLINDER  =  1,
		COLVOL_TYPE_BOX       =  2,
		COLVOL_TYPE_SPHERE    =  3,
		COLVOL_TYPE_FOOTPRINT =  4, // set-intersection of sphere and footprint-prism
		COLVOL_NUM_SHAPES     =  5, // number of non-disabled collision volume types
	};

	enum COLVOL_AXES {
		COLVOL_AXIS_X   = 0,
		COLVOL_AXIS_Y   = 1,
		COLVOL_AXIS_Z   = 2,
		COLVOL_NUM_AXES = 3         // number of collision volume axes
	};
	enum COLVOL_HITTEST_TYPES {
		COLVOL_HITTEST_DISC = 0,
		COLVOL_HITTEST_CONT = 1,
		COLVOL_NUM_HITTESTS = 2     // number of hit-test types
	};

public:
	CollisionVolume();
	CollisionVolume(const CollisionVolume* v, float defaultRadius = 0.0f);
	CollisionVolume(
		const std::string& cvTypeStr,
		const float3& cvScales,
		const float3& cvOffsets,
		const bool cvContHitTest
	);

	/**
	 * Called if a unit or feature does not define a custom volume.
	 * @param r the object's default radius
	 */
	void InitSphere(float r);
	void InitCustom(const float3& scales, const float3& offsets, int vType, int tType, int pAxis);

	void RescaleAxes(const float xs, const float ys, const float zs);
	void SetAxisScales(const float xs, const float ys, const float zs);

	int GetVolumeType() const { return volumeType; }
	void SetVolumeType(int type) { volumeType = type; }

	void Enable() { disabled = false; }
	void Disable() { disabled = true; }
	void SetContHitTest(bool cont) { contHitTest = cont; }

	int GetPrimaryAxis() const { return volumeAxes[0]; }
	int GetSecondaryAxis(int axis) const { return volumeAxes[axis + 1]; }

	float GetBoundingRadius() const { return volumeBoundingRadius; }
	float GetBoundingRadiusSq() const { return volumeBoundingRadiusSq; }

	float GetOffset(int axis) const { return axisOffsets[axis]; }
	const float3& GetOffsets() const { return axisOffsets; }

	float GetScale(int axis) const { return fAxisScales[axis]; }
	float GetHScale(int axis) const { return hAxisScales[axis]; }
	float GetHScaleSq(int axis) const { return hsqAxisScales[axis]; }
	const float3& GetScales() const { return fAxisScales; }
	const float3& GetHScales() const { return hAxisScales; }
	const float3& GetHSqScales() const { return hsqAxisScales; }
	const float3& GetHIScales() const { return hiAxisScales; }

	bool IsDisabled() const { return disabled; }
	bool DefaultScale() const { return defaultScale; }
	bool GetContHitTest() const { return contHitTest; }

	float GetPointDistance(const CUnit* u, const float3& pw) const;
	float GetPointDistance(const CFeature* u, const float3& pw) const;

	static const CollisionVolume* GetVolume(const CUnit* u, float3& pos, bool);
	static const CollisionVolume* GetVolume(const CFeature* f, float3& pos);

private:
	void SetBoundingRadius();
	float GetPointSurfaceDistance(const CSolidObject* o, const CMatrix44f& m, const float3& pw) const;

	float3 fAxisScales;                 ///< full-length axis scales
	float3 hAxisScales;                 ///< half-length axis scales
	float3 hsqAxisScales;               ///< half-length axis scales (squared)
	float3 hiAxisScales;                ///< half-length axis scales (inverted)
	float3 axisOffsets;                 ///< offsets wrt. the model's mid-position (world-space)

	float volumeBoundingRadius;         ///< radius of minimally-bounding sphere around volume
	float volumeBoundingRadiusSq;       ///< squared radius of minimally-bounding sphere

	int volumeType;                     ///< which COLVOL_TYPE_* shape we have
	int volumeAxes[3];                  ///< [0] is prim. axis, [1] and [2] are sec. axes (all COLVOL_AXIS_*)

	bool disabled;
	bool defaultScale;
	bool contHitTest;
};

#endif
