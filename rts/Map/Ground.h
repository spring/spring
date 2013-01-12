/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GROUND_H
#define GROUND_H

#include "System/float3.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/GlobalSynced.h"

class CProjectile;


class CGround
{
public:
	CGround() {}
	~CGround();

	/// similar to GetHeightReal, but uses nearest filtering instead of interpolating the heightmap
	float GetApproximateHeight(float x, float y, bool synced = true) const;
	/// Returns the height at the specified position, cropped to a non-negative value
	float GetHeightAboveWater(float x, float y, bool synced = true) const;
	/// Returns the real height at the specified position, can be below 0
	float GetHeightReal(float x, float y, bool synced = true) const;
	float GetOrigHeight(float x, float y) const;

	float GetSlope(float x, float y, bool synced = true) const;
	const float3& GetNormal(float x, float y, bool synced = true) const;
	const float3& GetNormalAboveWater(const float3& p, bool synced = true) const;
	float3 GetSmoothNormal(float x, float y, bool synced = true) const;

	float LineGroundCol(float3 from, float3 to, bool synced = true) const;
	float TrajectoryGroundCol(float3 from, const float3& flatdir, float length, float linear, float quadratic) const;

	inline int GetSquare(const float3& pos) const {
		return std::max(0, std::min(gs->mapxm1, (int(pos.x) / SQUARE_SIZE))) +
			std::max(0, std::min(gs->mapym1, (int(pos.z) / SQUARE_SIZE))) * gs->mapx;
	};
private:

	void CheckColSquare(CProjectile* p, int x, int y);
};

extern CGround* ground;

#endif // GROUND_H

