#ifndef GEOSQUAREPROJECTILE_H
#define GEOSQUAREPROJECTILE_H
/*pragma once removed*/
#include "Projectile.h"

class CGeoSquareProjectile : public CProjectile
{
public:
	CGeoSquareProjectile(const float3& p1,const float3& p2,const float3& v1,const float3& v2,float w1,float w2);
	virtual ~CGeoSquareProjectile(void);

	float3 p1,p2,v1,v2;
	float w1,w2;
	float r,g,b,a;
	void Draw(void);
	void Update(void);
};


#endif /* GEOSQUAREPROJECTILE_H */
