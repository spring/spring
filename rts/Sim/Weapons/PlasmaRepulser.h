#ifndef PLASMAREPULSER_H
#define PLASMAREPULSER_H

#include "Weapon.h"

class CProjectile;
class CRepulseGfx;
class CShieldPartProjectile;

class CPlasmaRepulser :
	public CWeapon
{
	CR_DECLARE(CPlasmaRepulser);
public:
	CPlasmaRepulser(CUnit* owner);
	~CPlasmaRepulser(void);
	void Update(void);
	void NewProjectile(CWeaponProjectile* p);
	float NewBeam(CWeapon* emitter, float3 start, float3 dir, float length, float3& newDir);
	bool BeamIntercepted(CWeapon* emitter, float damageMultiplier = 1.0f);		//returns true if its a repulsing shield
	void Init(void);

	std::list<CWeaponProjectile*> incoming;
	std::list<CWeaponProjectile*> hasGfx;
	std::list<CShieldPartProjectile*> visibleShieldParts;

	void DependentDied(CObject* o);
	void SlowUpdate(void);

	float curPower;

	float radius;
	float sqRadius;

	int hitFrames;
	int rechargeDelay;

	bool isEnabled;
	bool wasDrawn;
	bool startShowingShield;

private:	
	virtual void FireImpl() {};
};

#endif
