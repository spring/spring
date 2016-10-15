/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef INTERCEPT_HANDLER_H
#define INTERCEPT_HANDLER_H

#include <deque>
#include "System/Misc/NonCopyable.h"
#include "System/Object.h"

class CWeapon;
class CWeaponProjectile;
class CProjectile;
class float3;

class CInterceptHandler : public CObject, spring::noncopyable
{
	CR_DECLARE(CInterceptHandler)

public:
	void Update(bool forced);

	void AddInterceptorWeapon(CWeapon* weapon);
	void RemoveInterceptorWeapon(CWeapon* weapon);

	void AddInterceptTarget(CWeaponProjectile* target, const float3& destination);

	void DependentDied(CObject* o);

private:
	std::deque<CWeapon*> interceptors;
	std::deque<CWeaponProjectile*> interceptables;
};

extern CInterceptHandler interceptHandler;

#endif /* INTERCEPT_HANDLER_H */
