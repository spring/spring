#ifndef TRACERPROJECTILE_H
#define TRACERPROJECTILE_H
// TracerProjectile.h: interface for the CTracerProjectile class.
//
//////////////////////////////////////////////////////////////////////

#include "Sim/Projectiles/Projectile.h"

class CTracerProjectile : public CProjectile
{
public:
	CR_DECLARE(CTracerProjectile);

	void Draw();
	void Update();
	void Init(const float3& pos, CUnit *owner GML_PARG_H);
	CTracerProjectile();
	CTracerProjectile(const float3 pos,const float3 speed,const float range,CUnit* owner GML_PARG_H);
	virtual ~CTracerProjectile();
	float speedf;
	float length;
	float drawLength;
	float3 dir;

};

#endif /* TRACERPROJECTILE_H */
