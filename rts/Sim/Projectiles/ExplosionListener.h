/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _EXPLOSION_LISTENER_H
#define _EXPLOSION_LISTENER_H

#include "System/float3.h"

#include <vector>

struct WeaponDef;


class CExplosionEvent {
public:
	CExplosionEvent(const float3& pos, float damage, float radius,
			const WeaponDef* weaponDef)
		: pos(pos)
		, damage(damage)
		, radius(radius)
		, weaponDef(weaponDef)
	{}

	const float3& GetPos() const { return pos; }
	float GetDamage() const { return damage; }
	float GetRadius() const { return radius; }
	const WeaponDef* GetWeaponDef() const { return weaponDef; }

private:
	float3 pos;
	float damage;
	float radius;
	const WeaponDef* weaponDef;
};


class IExplosionListener
{
public:
	/**
	 * Informs listeners about an explosion that has occured.
	 * @see EventClient#Explosion
	 */
	virtual void ExplosionOccurred(const CExplosionEvent& event) = 0;
protected:
	~IExplosionListener();
};


/**
 * Base
 */
class CExplosionCreator
{
public:
	static void AddExplosionListener(IExplosionListener* listener);
	static void RemoveExplosionListener(IExplosionListener* listener);

	/**
	 * Sends the event to all registered listeners.
	 */
	static void FireExplosionEvent(const CExplosionEvent& event);

private:
	static std::vector<IExplosionListener*> explosionListeners;
};

#endif /* _EXPLOSION_LISTENER_H */
