#ifndef TORPEDOLAUNCHER_H
#define TORPEDOLAUNCHER_H
/*pragma once removed*/
#include "Weapon.h"

class CTorpedoLauncher :
	public CWeapon
{
public:
	CTorpedoLauncher(CUnit* owner);
	virtual ~CTorpedoLauncher(void);

	void Fire(void);
	void Update(void);
	bool TryTarget(const float3& pos,bool userTarget,CUnit* unit);

	float tracking;
};


#endif /* TORPEDOLAUNCHER_H */
