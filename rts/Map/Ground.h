#ifndef GROUND_H
#define GROUND_H
// Ground.h: interface for the CGround class.
//
//////////////////////////////////////////////////////////////////////

#include "float3.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/GlobalSynced.h"

class CProjectile;


class CGround
{
public:
	CGround();
	virtual ~CGround();

	float GetApproximateHeight(float x,float y);
	float GetSlope(float x,float y);
	float GetHeight(float x,float y);
	float GetHeight2(float x,float y);
	float GetOrigHeight(float x,float y);
	float3& GetNormal(float x,float y);
	float3 GetSmoothNormal(float x,float y);
	float LineGroundCol(float3 from, float3 to);
	float TrajectoryGroundCol(float3 from, float3 flatdir, float length, float linear, float quadratic);

	inline int GetSquare(const float3& pos) {
		return std::max(0, std::min(gs->mapx - 1, (int(pos.x) / SQUARE_SIZE))) +
			std::max(0, std::min(gs->mapy - 1, (int(pos.z) / SQUARE_SIZE))) * gs->mapx;
	};
private:

	void CheckColSquare(CProjectile* p,int x,int y);
};

extern CGround* ground;

#endif /* GROUND_H */
