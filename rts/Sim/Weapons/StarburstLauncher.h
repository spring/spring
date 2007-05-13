#ifndef STARBURSTLAUNCHER_H
#define STARBURSTLAUNCHER_H

#include "Weapon.h"

class CStarburstLauncher :
	public CWeapon
{
	CR_DECLARE(CStarburstLauncher);
public:
	CStarburstLauncher(CUnit* owner);
	~CStarburstLauncher(void);

	void Fire(void);
	void Update(void);
	bool TryTarget(const float3& pos,bool userTarget,CUnit* unit);
	float GetRange2D(float yDiff);

	float tracking;
	float uptime;
};


#endif /* STARBURSTLAUNCHER_H */
