#ifndef MUZZLEFLAME_H
#define MUZZLEFLAME_H

#include "Sim/Projectiles/Projectile.h"

class CMuzzleFlame :
	public CProjectile
{
	CR_DECLARE(CMuzzleFlame);

	void creg_Serialize(creg::ISerializer& s);
public:
	CMuzzleFlame(const float3& pos,const float3& speed,const float3& dir,float size GML_PARG_H);
	~CMuzzleFlame(void);

	void Draw(void);
	void Update(void);

	float3 dir;
	float size;
	int age;
	int numFlame;
	int numSmoke;

//	float3* randSmokeDir;
	std::vector<float3> randSmokeDir;
};


#endif /* MUZZLEFLAME_H */
