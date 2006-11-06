#ifndef EXPGENSPAWNER_H
#define EXPGENSPAWNER_H

#include "Projectile.h"

class CExplosionGenerator;

//class used to spawn an explosiongenerator after a delay
class CExpGenSpawner : public CProjectile
{
	CR_DECLARE(CExpGenSpawner);
public:
	int delay;
	float3 dir;
	float damage;
	CExplosionGenerator* explosionGenerator;

	CExpGenSpawner(void);
	~CExpGenSpawner(void);
	void Update();
	void Draw(){};
};

#endif