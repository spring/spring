#ifndef GROUND_H
#define GROUND_H
// Ground.h: interface for the CGround class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GROUND_H__EB512761_1B68_11D4_AD55_0080ADA84DE3__INCLUDED_)
#define AFX_GROUND_H__EB512761_1B68_11D4_AD55_0080ADA84DE3__INCLUDED_

#if _MSC_VER > 1000
/*pragma once removed*/
#endif // _MSC_VER > 1000

#pragma warning(disable:4786)

class CGround;


#include <vector>
	// Added by ClassView
#include "ReadMap.h"
class CProjectileHandler;
class CProjectile;

using namespace std;
class CGround  
{
public:
	CGround();
	virtual ~CGround();

	float GetApproximateHeight(float x,float y);
	float GetSlope(float x,float y);
	float GetHeight(float x,float y);
	float GetHeight2(float x,float y);
	float3& GetNormal(float x,float y);
	float3 GetSmoothNormal(float x,float y);
	float LineGroundCol(float3 from,float3 to);
	void CheckCol(CProjectileHandler* ph);

	inline int GetSquare(const float3& pos){return max(0,min(gs->mapx-1,(int(pos.x)/SQUARE_SIZE)))+max(0,min(gs->mapy-1,(int(pos.z)/SQUARE_SIZE)))*gs->mapx;};
private:

	void CheckColSquare(CProjectile* p,int x,int y);

	float LineGroundSquareCol(const float3 &from,const float3 &to,int xs,int ys);
};
extern CGround* ground;

#endif // !defined(AFX_GROUND_H__EB512761_1B68_11D4_AD55_0080ADA84DE3__INCLUDED_)


#endif /* GROUND_H */
