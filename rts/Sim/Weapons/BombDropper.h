/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef BOMB_DROPPER_H
#define BOMB_DROPPER_H

#include "Weapon.h"

class CBombDropper : public CWeapon
{
	CR_DECLARE(CBombDropper);
public:
	void Update();
	CBombDropper(CUnit* owner, bool useTorps);
	virtual ~CBombDropper();
	bool TryTarget(const float3& pos, bool userTarget, CUnit* unit);
	void Init();
	bool AttackUnit(CUnit* unit, bool userTarget);
	bool AttackGround(float3 pos, bool userTarget);
	void SlowUpdate();

	/// if we should drop torpedoes
	bool dropTorpedoes;
	/// range of bombs (torpedoes) after they hit ground/water
	float bombMoveRange;
	float tracking;

private:
	virtual void FireImpl();
};

#endif /* BOMB_DROPPER_H */
