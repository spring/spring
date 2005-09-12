#ifndef TRACERPROJECTILE_H
#define TRACERPROJECTILE_H
// TracerProjectile.h: interface for the CTracerProjectile class.
//
//////////////////////////////////////////////////////////////////////

#include "Projectile.h"

class CTracerProjectile : public CProjectile  
{
public:
	void Draw();
	void Update();
	CTracerProjectile(const float3 pos,const float3 speed,const float range,CUnit* owner);
	virtual ~CTracerProjectile();
	float speedf;
	float length;
	float drawLength;
	float3 dir;

};

#endif /* TRACERPROJECTILE_H */
