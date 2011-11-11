/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef INTERCEPT_HANDLER_H
#define INTERCEPT_HANDLER_H

#include "System/Object.h"

#include <list>
#include <map>
#include <boost/noncopyable.hpp>
#include "System/Object.h"

class CWeapon;
class CWeaponProjectile;
class CPlasmaRepulser;
class CProjectile;
class float3;

class CInterceptHandler : public CObject, boost::noncopyable
{
	CR_DECLARE(CInterceptHandler)

public:
	void Update(bool forced);

	void AddInterceptorWeapon(CWeapon* weapon) { interceptors.push_back(weapon); }
	void RemoveInterceptorWeapon(CWeapon* weapon) { interceptors.remove(weapon); }

	void AddInterceptTarget(CWeaponProjectile* target, const float3& destination);
	void AddShieldInterceptableProjectile(CWeaponProjectile* p);

	float AddShieldInterceptableBeam(CWeapon* emitter, const float3& start, const float3& dir, float length, float3& newDir, CPlasmaRepulser*& repulsedBy);

	void AddPlasmaRepulser(CPlasmaRepulser* r) { repulsors.push_back(r); }
	void RemovePlasmaRepulser(CPlasmaRepulser* r) { repulsors.remove(r); }

	void DependentDied(CObject* o);

private:
	std::list<CWeapon*> interceptors;
	std::list<CPlasmaRepulser*> repulsors;
	std::map<int, CWeaponProjectile*> interceptables;
};

extern CInterceptHandler interceptHandler;

#endif /* INTERCEPT_HANDLER_H */
