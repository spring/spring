#ifndef STARBURSTPROJECTILE_H
#define STARBURSTPROJECTILE_H

#include "WeaponProjectile.h"
#include "Sim/Misc/DamageArray.h"
#include <vector>

#if defined(USE_GML) && GML_ENABLE_SIM
#define AGEMOD_VECTOR gmlCircularQueue<float,64>
#else
#define AGEMOD_VECTOR std::vector<float>
#endif

class CSmokeTrailProjectile;

class CStarburstProjectile :
	public CWeaponProjectile
{
	CR_DECLARE(CStarburstProjectile);
	void creg_Serialize(creg::ISerializer& s);
public:
	CStarburstProjectile(const float3& pos, const float3& speed, CUnit* owner,
			float3 targetPos,float areaOfEffect, float maxSpeed,float tracking,
			int uptime, CUnit* target, const WeaponDef* weaponDef,
			CWeaponProjectile* interceptTarget, float maxdistance, float3 aimError GML_PARG_H);
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
	float acceleration;
	int uptime;
	float areaOfEffect;
	int age;
	float3 oldSmoke,oldSmokeDir;
	float3 aimError;
	bool drawTrail;
	int numParts;
	bool doturn;
	CSmokeTrailProjectile* curCallback;
	void DrawCallback(void);
	int* numCallback;
	int missileAge;
	float distanceToTravel;

	struct OldInfo{
		float3 pos;
		float3 dir;
		float speedf;
		AGEMOD_VECTOR ageMods;
	};
	OldInfo* oldInfos[5];
};


#endif /* STARBURSTPROJECTILE_H */
