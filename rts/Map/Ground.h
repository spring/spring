/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GROUND_H
#define GROUND_H

#include "System/float3.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/GlobalSynced.h"

class CGround
{
public:
	/// similar to GetHeightReal, but uses nearest filtering instead of interpolating the heightmap
	static float GetApproximateHeight(float x, float z, bool synced = true);
	static float GetApproximateHeightUnsafe(int x, int z, bool synced = true);
	/// Returns the height at the specified position, cropped to a non-negative value
	static float GetHeightAboveWater(float x, float z, bool synced = true);
	/// Returns the real height at the specified position, can be below 0
	static float GetHeightReal(float x, float z, bool synced = true);
	static float GetOrigHeight(float x, float z);

	static float GetSlope(float x, float z, bool synced = true);
	static const float3& GetNormal(float x, float z, bool synced = true);
	static const float3& GetNormalAboveWater(float x, float z, bool synced = true);
	static float3 GetSmoothNormal(float x, float z, bool synced = true);

	static float GetApproximateHeight(const float3& p, bool synced = true) { return (GetApproximateHeight(p.x, p.z, synced)); }
	static float GetHeightAboveWater(const float3& p, bool synced = true) { return (GetHeightAboveWater(p.x, p.z, synced)); }
	static float GetHeightReal(const float3& p, bool synced = true) { return (GetHeightReal(p.x, p.z, synced)); }
	static float GetOrigHeight(const float3& p) { return (GetOrigHeight(p.x, p.z)); }

	static float GetSlope(const float3& p, bool synced = true) { return (GetSlope(p.x, p.z, synced)); }
	static const float3& GetNormal(const float3& p, bool synced = true) { return (GetNormal(p.x, p.z, synced)); }
	static const float3& GetNormalAboveWater(const float3& p, bool synced = true) { return (GetNormalAboveWater(p.x, p.z, synced)); }
	static float3 GetSmoothNormal(const float3& p, bool synced = true) { return (GetSmoothNormal(p.x, p.z, synced)); }


	static float LineGroundCol(float3 from, float3 to, bool synced = true);
	static float TrajectoryGroundCol(float3 from, const float3& flatdir, float length, float linear, float quadratic);

	static int GetSquare(const float3& pos);
};

#endif // GROUND_H

