/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef INTERCEPTHANDLER_H
#define INTERCEPTHANDLER_H

#include "Object.h"

#include <list>
#include <boost/noncopyable.hpp>
#include "float3.h"

class CWeapon;
class CWeaponProjectile;
class CPlasmaRepulser;
class CProjectile;

class CInterceptHandler : public boost::noncopyable
{
	CR_DECLARE(CInterceptHandler)

public:
	CInterceptHandler(void);
	~CInterceptHandler(void);

	void AddInterceptorWeapon(CWeapon* weapon);
	void RemoveInterceptorWeapon(CWeapon* weapon);
	void AddInterceptTarget(CWeaponProjectile* target, float3 destination);

	void AddShieldInterceptableProjectile(CWeaponProjectile* p);
	float AddShieldInterceptableBeam(CWeapon* emitter, float3 start, float3 dir, float length, float3& newDir, CPlasmaRepulser*& repulsedBy);
	void AddPlasmaRepulser(CPlasmaRepulser* r);
	void RemovePlasmaRepulser(CPlasmaRepulser* r);

private:
	std::list<CWeapon*> interceptors;
	std::list<CPlasmaRepulser*> plasmaRepulsors;
};

extern CInterceptHandler interceptHandler;

#endif /* INTERCEPTHANDLER_H */
