#ifndef WRECKPROJECTILE_H
#define WRECKPROJECTILE_H
// WreckProjectile.h: interface for the CWreckProjectile class.
//
//////////////////////////////////////////////////////////////////////

#include "Projectile.h"

class CWreckProjectile : public CProjectile  
{
public:
	CR_DECLARE(CWreckProjectile);
	void Update();
	CWreckProjectile(float3 pos,float3 speed,float temperature,CUnit* owner);
	virtual ~CWreckProjectile();

	void Draw(void);
};

#endif /* WRECKPROJECTILE_H */
