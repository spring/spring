#ifndef STARBURSTLAUNCHER_H
#define STARBURSTLAUNCHER_H

#include "Weapon.h"

class CStarburstLauncher :
	public CWeapon
{
public:
	CStarburstLauncher(CUnit* owner);
	~CStarburstLauncher(void);

	void Fire(void);
	void Update(void);
	bool TryTarget(const float3& pos,bool userTarget,CUnit* unit);

	float tracking;
	float uptime;
};


#endif /* STARBURSTLAUNCHER_H */
