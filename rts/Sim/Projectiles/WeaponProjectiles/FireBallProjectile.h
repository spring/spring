#ifndef __FIRE_BALL_PROJECTILE_H__
#define __FIRE_BALL_PROJECTILE_H__

#include "WeaponProjectile.h"
#include <deque>

#if defined(USE_GML) && GML_ENABLE_SIM
#define SPARK_QUEUE gmlCircularQueue<CFireBallProjectile::Spark,32>
#else
#define SPARK_QUEUE std::deque<Spark>
#endif

class CFireBallProjectile :
	public CWeaponProjectile
{
	CR_DECLARE(CFireBallProjectile);
	CR_DECLARE_SUB(Spark);
public:
	CFireBallProjectile(const float3& pos,const float3& speed, CUnit* owner,
			CUnit *target, const float3 &targetPos, const WeaponDef* weaponDef GML_PARG_H);
	~CFireBallProjectile(void);

	void Draw();
	void Update();

	struct Spark {
		CR_DECLARE_STRUCT(Spark);
		float3 pos;
		float3 speed;
		float size;
		int ttl;
	};

	void Collision();

private:
	SPARK_QUEUE sparks;

	void EmitSpark();
};

#endif // __FIRE_BALL_PROJECTILE_H__
