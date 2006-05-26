#ifndef INTERCEPTHANDLER_H
#define INTERCEPTHANDLER_H

#include "Object.h"

#include <list>

class CWeapon;
class CWeaponProjectile;
class CPlasmaRepulser;
class CProjectile;

class CInterceptHandler :
	public CObject
{
public:
	CInterceptHandler(void);
	~CInterceptHandler(void);
	void AddInterceptorWeapon(CWeapon* weapon);
	void RemoveInterceptorWeapon(CWeapon* weapon);
	void AddInterceptTarget(CWeaponProjectile* target,float3 destination);

	void AddShieldInterceptableProjectile(CWeaponProjectile* p);
	float AddShieldInterceptableBeam(CWeapon* emitter, float3 start, float3 dir, float length, float3& newDir,CPlasmaRepulser*& repulsedBy);
	void AddPlasmaRepulser(CPlasmaRepulser* r);
	void RemovePlasmaRepulser(CPlasmaRepulser* r);

	std::list<CWeapon*> interceptors;
	std::list<CPlasmaRepulser*> plasmaRepulsors;
};

extern CInterceptHandler interceptHandler;

#endif /* INTERCEPTHANDLER_H */
