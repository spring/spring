#ifndef BOMBDROPPER_H
#define BOMBDROPPER_H
// Cannon.h: interface for the CCannon class.
//
//////////////////////////////////////////////////////////////////////

#include "Weapon.h"

class CBombDropper : public CWeapon
{
	CR_DECLARE(CBombDropper);
public:
	void Update();
	CBombDropper(CUnit* owner, bool useTorps);
	virtual ~CBombDropper();
	bool TryTarget(const float3& pos,bool userTarget,CUnit* unit);
	void Init(void);
	bool AttackUnit(CUnit* unit, bool userTarget);
	bool AttackGround(float3 pos, bool userTarget);
	void SlowUpdate(void);

	bool dropTorpedoes;			//if we should drop torpedoes
	float bombMoveRange;		//range of bombs (torpedoes) after they hit ground/water
	float tracking;

private:
	virtual void FireImpl(void);
};

#endif /* BOMBDROPPER_H */
