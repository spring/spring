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
	CStarburstProjectile(const float3& pos,const float3& speed,CUnit* owner,float3 targetPos,const DamageArray& damages,float areaOfEffect, float maxSpeed,float tracking, int uptime,CUnit* target, WeaponDef *weaponDef,CWeaponProjectile* interceptTarget);
	~CStarburstProjectile(void);
	void DependentDied(CObject* o);
	void Collision(CUnit* unit);
	void Collision();
	void Update(void);
	void Draw(void);
	void DrawUnitPart(void);

	float tracking;
	float maxGoodDif;
	float3 dir;
	float maxSpeed;
	float curSpeed;
	int ttl;
	int uptime;
	DamageArray damages;
	float areaOfEffect;
	int age;
	float3 oldSmoke,oldSmokeDir;
	CUnit* target;
	bool drawTrail;
	int numParts;
	float3 targetPos;
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
