#ifndef INTERCEPTHANDLER_H
#define INTERCEPTHANDLER_H
/*pragma once removed*/
#include "Object.h"

#include <list>

class CWeapon;
class CWeaponProjectile;

class CInterceptHandler :
	public CObject
{
public:
	CInterceptHandler(void);
	~CInterceptHandler(void);
	void AddInterceptorWeapon(CWeapon* weapon);
	void RemoveInterceptorWeapon(CWeapon* weapon);
	void AddInterceptTarget(CWeaponProjectile* target,float3 destination);

	std::list<CWeapon*> interceptors;
};

extern CInterceptHandler interceptHandler;

#endif /* INTERCEPTHANDLER_H */
