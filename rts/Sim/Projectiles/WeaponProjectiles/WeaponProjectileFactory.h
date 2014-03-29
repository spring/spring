/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef WEAPONPROJECTILE_FACTORY_H
#define WEAPONPROJECTILE_FACTORY_H

struct ProjectileParams;

class WeaponProjectileFactory {
public:
	static unsigned int LoadProjectile(const ProjectileParams& params);
};

#endif

