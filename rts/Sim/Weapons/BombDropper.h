/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef BOMB_DROPPER_H
#define BOMB_DROPPER_H

#include "Weapon.h"

class CBombDropper : public CWeapon
{
	CR_DECLARE(CBombDropper);
public:
	CBombDropper(CUnit* owner, bool useTorps);
	~CBombDropper();

	bool TryTarget(const float3& pos, bool userTarget, CUnit* unit);
	void Init();
	void Update();
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
