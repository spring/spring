/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef COLLISION_VOLUME_H
#define COLLISION_VOLUME_H

#include "System/float3.h"
#include "System/creg/creg_cond.h"

#include <string>

// the positive x-axis points to the "left" in object-space and to the "right" in world-space
// converting between them means flipping the sign of x-components of positions and directions
const float3 WORLD_TO_OBJECT_SPACE = float3(-1.0f, 1.0f, 1.0f);
const float COLLISION_VOLUME_EPS = 0.0000000001f;

struct LocalModelPiece;
class CMatrix44f;
class CSolidObject;
class CUnit;
class CFeature;

struct CollisionVolume
{
	CR_DECLARE_STRUCT(CollisionVolume)

public:
	enum {
		COLVOL_TYPE_ELLIPSOID =  0, // dummy, these become spheres or boxes
		COLVOL_TYPE_CYLINDER  =  1,
		COLVOL_TYPE_BOX       =  2,
		COLVOL_TYPE_SPHERE    =  3,
	};
	enum {
		COLVOL_AXIS_X = 0,
		COLVOL_AXIS_Y = 1,
		COLVOL_AXIS_Z = 2,
	};
	enum {
		COLVOL_HITTEST_DISC = 0,
		COLVOL_HITTEST_CONT = 1,
	};

public:
	CollisionVolume();
	CollisionVolume(const CollisionVolume* v) { *this = *v; }
	CollisionVolume(
		const std::string& cvTypeStr,
		const float3& cvScales,
		const float3& cvOffsets
	);

	void PostLoad();

	CollisionVolume& operator = (const CollisionVolume&);

	/**
	 * Called if a unit or feature does not define a custom volume.
	 * @param radius the object's default radius
	 */
	void InitSphere(float radius);
	void InitBox(const float3& scales, const float3& offsets = ZeroVector);
	void InitShape(
		const float3& scales,
		const float3& offsets,
		const int vType,
		const int tType,
		const int pAxis
	);

	void RescaleAxes(const float3& scales);
	void SetAxisScales(const float3& scales);
	void FixTypeAndScale(float3& scales);
	void SetBoundingRadius();
	void SetOffsets(const float3& offsets) { axisOffsets = offsets; }

	int GetVolumeType() const { return volumeType; }
	void SetVolumeType(int type) { volumeType = type; }

	void SetIgnoreHits(bool b) { ignoreHits = b; }
	void SetUseContHitTest(bool b) { useContHitTest = b; }
	void SetDefaultToPieceTree(bool b) { defaultToPieceTree = b; }
	void SetDefaultToFootPrint(bool b) { defaultToFootPrint = b; }

	int GetPrimaryAxis() const { return volumeAxes[0]; }
	int GetSecondaryAxis(int axis) const { return volumeAxes[axis + 1]; }

	float GetBoundingRadius() const { return volumeBoundingRadius; }
	float GetBoundingRadiusSq() const { return volumeBoundingRadiusSq; }

	float GetOffset(int axis) const { return axisOffsets[axis]; }
	const float3& GetOffsets() const { return axisOffsets; }

	float GetScale(int axis) const { return fullAxisScales[axis]; }
	float GetHScale(int axis) const { return halfAxisScales[axis]; }
	float GetHScaleSq(int axis) const { return halfAxisScalesSqr[axis]; }

	const float3& GetScales() const { return fullAxisScales; }
	const float3& GetHScales() const { return halfAxisScales; }
	const float3& GetHSqScales() const { return halfAxisScalesSqr; }
	const float3& GetHIScales() const { return halfAxisScalesInv; }

	bool IgnoreHits() const { return ignoreHits; }
	bool UseContHitTest() const { return useContHitTest; }
	bool DefaultToSphere() const { return (fullAxisScales.equals(OnesVector, ZeroVector)); }
	bool DefaultToFootPrint() const { return defaultToFootPrint; }
	bool DefaultToPieceTree() const { return defaultToPieceTree; }

	float3 GetWorldSpacePos(const CSolidObject* o, const float3& extOffsets = ZeroVector) const;

	float GetPointSurfaceDistance(const CUnit* u, const LocalModelPiece* lmp, const float3& pos) const;
	float GetPointSurfaceDistance(const CFeature* u, const LocalModelPiece* lmp, const float3& pos) const;

private:
	float GetPointSurfaceDistance(const CSolidObject* obj, const LocalModelPiece* lmp, const CMatrix44f& mat, const float3& pos) const;
	float GetPointSurfaceDistance(const CMatrix44f& mv, const float3& p) const;

	float GetCylinderDistance(const float3& pv, size_t axisA = 0, size_t axisB = 1, size_t axisC = 2) const;
	float GetEllipsoidDistance(const float3& pv) const;

private:
	float3 fullAxisScales;              ///< full-length axis scales
	float3 halfAxisScales;              ///< half-length axis scales
	float3 halfAxisScalesSqr;           ///< half-length axis scales (squared)
	float3 halfAxisScalesInv;           ///< half-length axis scales (inverted)
	float3 axisOffsets;                 ///< offsets wrt. the model's mid-position (world-space)

	float volumeBoundingRadius;         ///< radius of minimally-bounding sphere around volume
	float volumeBoundingRadiusSq;       ///< squared radius of minimally-bounding sphere

	int volumeType;                     ///< which COLVOL_TYPE_* shape we have
	int volumeAxes[3];                  ///< [0] is prim. axis, [1] and [2] are sec. axes (all COLVOL_AXIS_*)

	bool ignoreHits;                    /// if true, CollisionHandler does not check for hits against us
	bool useContHitTest;                /// if true, CollisionHandler does continuous instead of discrete hit-testing
	bool defaultToFootPrint;            /// if true, volume becomes a box with dimensions equal to a SolidObject's {x,z}size
	bool defaultToPieceTree;            /// if true, volume is owned by a unit but defaults to piece-volume tree for hit-testing
};

#endif

