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
	static float GetApproximateHeight(float x, float z, bool synced = true) const;
	/// Returns the height at the specified position, cropped to a non-negative value
	static float GetHeightAboveWater(float x, float z, bool synced = true) const;
	/// Returns the real height at the specified position, can be below 0
	static float GetHeightReal(float x, float z, bool synced = true) const;
	static float GetOrigHeight(float x, float z) const;

	static float GetSlope(float x, float z, bool synced = true) const;
	static const float3& GetNormal(float x, float z, bool synced = true) const;
	static const float3& GetNormalAboveWater(float x, float z, bool synced = true) const;
	static float3 GetSmoothNormal(float x, float z, bool synced = true) const;

	static float GetApproximateHeight(const float3& p, bool synced = true) const { return (GetApproximateHeight(p.x, p.z, synced)); }
	static float GetHeightAboveWater(const float3& p, bool synced = true) const { return (GetHeightAboveWater(p.x, p.z, synced)); }
	static float GetHeightReal(const float3& p, bool synced = true) const { return (GetHeightReal(p.x, p.z, synced)); }
	static float GetOrigHeight(const float3& p) const { return (GetOrigHeight(p.x, p.z)); }

	static float GetSlope(const float3& p, bool synced = true) const { return (GetSlope(p.x, p.z, synced)); }
	static const float3& GetNormal(const float3& p, bool synced = true) const { return (GetNormal(p.x, p.z, synced)); }
	static const float3& GetNormalAboveWater(const float3& p, bool synced = true) const { return (GetNormalAboveWater(p.x, p.z, synced)); }
	static float3 GetSmoothNormal(const float3& p, bool synced = true) const { return (GetSmoothNormal(p.x, p.z, synced)); }


	static float LineGroundCol(float3 from, float3 to, bool synced = true) const;
	static float TrajectoryGroundCol(float3 from, const float3& flatdir, float length, float linear, float quadratic) const;

	static inline int GetSquare(const float3& pos) const {
		return
			std::max(0, std::min(gs->mapxm1, (int(pos.x) / SQUARE_SIZE))) +
			std::max(0, std::min(gs->mapym1, (int(pos.z) / SQUARE_SIZE))) * gs->mapx;
	};
};

#endif // GROUND_H

