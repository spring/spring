/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GROUND_H
#define GROUND_H

#include "float3.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/GlobalSynced.h"

class CProjectile;


class CGround
{
public:
	CGround();
	virtual ~CGround();

	float GetApproximateHeight(float x,float y) const;
	float GetSlope(float x,float y) const;
	/// Returns the height at the specified position, cropped to a non-negative value
	float GetHeight(float x,float y) const;
	/// Returns the real height at the specified position, can be below 0
	float GetHeight2(float x,float y) const;
	float GetOrigHeight(float x,float y) const;
	float3& GetNormal(float x,float y) const;
	float3 GetSmoothNormal(float x,float y) const;
	float LineGroundCol(float3 from, float3 to) const;
	float TrajectoryGroundCol(float3 from, float3 flatdir, float length, float linear, float quadratic) const;

	inline int GetSquare(const float3& pos) {
		return std::max(0, std::min(gs->mapx - 1, (int(pos.x) / SQUARE_SIZE))) +
			std::max(0, std::min(gs->mapy - 1, (int(pos.z) / SQUARE_SIZE))) * gs->mapx;
	};
private:

	void CheckColSquare(CProjectile* p,int x,int y);
};

extern CGround* ground;

#endif // GROUND_H

