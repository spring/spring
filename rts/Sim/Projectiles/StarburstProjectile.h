#ifndef STARBURSTPROJECTILE_H
#define STARBURSTPROJECTILE_H

#include "WeaponProjectile.h"
#include "Sim/Misc/DamageArray.h"
#include <vector>

class CSmokeTrailProjectile;

class CStarburstProjectile :
	public CWeaponProjectile
{
public:
	CStarburstProjectile(const float3& pos,const float3& speed,CUnit* owner,float3 targetPos,float areaOfEffect, float maxSpeed,float tracking, int uptime,CUnit* target, WeaponDef *weaponDef,CWeaponProjectile* interceptTarget);
	~CStarburstProjectile(void);
	void Collision(CUnit* unit);
	void Collision();
	void Update(void);
	void Draw(void);
	void DrawUnitPart(void);
	int ShieldRepulse(CPlasmaRepulser* shield,float3 shieldPos, float shieldForce, float shieldMaxSpeed);

	float tracking;
	float maxGoodDif;
	float3 dir;
	float maxSpeed;
	float curSpeed;
	int ttl;
	int uptime;
	float areaOfEffect;
	int age;
	float3 oldSmoke,oldSmokeDir;
	bool drawTrail;
	int numParts;
	bool doturn;
	CSmokeTrailProjectile* curCallback;
	void DrawCallback(void);
	int* numCallback;
	int missileAge;

	struct OldInfo{
		float3 pos;
		float3 dir;
		float speedf;
		std::vector<float> ageMods;
	};
	OldInfo* oldInfos[5];
};


#endif /* STARBURSTPROJECTILE_H */
