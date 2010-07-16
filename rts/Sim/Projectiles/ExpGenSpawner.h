/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef EXPGENSPAWNER_H
#define EXPGENSPAWNER_H

#include "Projectile.h"

class CExplosionGenerator;

/// Spawn an explosiongenerator after a delay
class CExpGenSpawner : public CProjectile
{
	CR_DECLARE(CExpGenSpawner);
public:
	int delay;
	float damage;
	CExplosionGenerator* explosionGenerator;

	CExpGenSpawner(void);
	~CExpGenSpawner(void);
	void Update();
	void Draw(){};
};

#endif
