#ifndef __FIRE_BALL_PROJECTILE_H__
#define __FIRE_BALL_PROJECTILE_H__

#include "archdef.h"

#include "WeaponProjectile.h"
#include <deque>

class CFireBallProjectile :
	public CWeaponProjectile
{
public:
	CFireBallProjectile(const float3& pos,const float3& speed, CUnit* owner, CUnit *target, const float3 &targetPos, WeaponDef *weaponDef);
	~CFireBallProjectile(void);

	void Draw();
	void Update();

	struct Spark {
		float3 pos;
		float3 speed;
		float size;
		int ttl;
	};

	void Collision();

private:
	std::deque<Spark> sparks;

	void EmitSpark();
};

#endif // __FIRE_BALL_PROJECTILE_H__
