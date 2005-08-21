#ifndef PLASMAREPULSER_H
#define PLASMAREPULSER_H

#include "Weapon.h"

class CProjectile;
class CRepulseGfx;

class CPlasmaRepulser :
	public CWeapon
{
public:
	CPlasmaRepulser(CUnit* owner);
	~CPlasmaRepulser(void);
	void Update(void);
	void NewPlasmaProjectile(CProjectile* p);

	std::list<CProjectile*> incoming;
	std::set<CProjectile*> hasGfx;
	void DependentDied(CObject* o);
	void SlowUpdate(void);
};

#endif
