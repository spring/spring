#ifndef BOMBDROPPER_H
#define BOMBDROPPER_H
// Cannon.h: interface for the CCannon class.
//
//////////////////////////////////////////////////////////////////////

#include "Weapon.h"

class CBombDropper : public CWeapon  
{
public:
	void Update();
	CBombDropper(CUnit* owner,bool useTorps);
	virtual ~CBombDropper();

	bool TryTarget(const float3& pos,bool userTarget,CUnit* unit);
	void Fire(void);
	void Init(void);
	void SlowUpdate(void);

	bool dropTorpedoes;			//if we should drop torpedoes
	float bombMoveRange;		//range of bombs (torpedoes) after they hit ground/water
	float tracking;
};

#endif /* BOMBDROPPER_H */
