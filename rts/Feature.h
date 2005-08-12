#ifndef __FEATURE_H__
#define __FEATURE_H__

#include "SolidObject.h"
#include "Matrix44f.h"
#include <vector>
#include <list>

struct FeatureDef;
class CUnit;
struct DamageArray;
class CFireProjectile;

class CFeature :
	public CSolidObject
{
public:
	CFeature(const float3& pos,FeatureDef* def,short int heading,int allyteam);	//pos of quad must not change after this
	~CFeature(void);
	bool AddBuildPower(float amount, CUnit* builder);								//negative amount=reclaim,return=true->reclaimed
	void DoDamage(const DamageArray& damages, CUnit* attacker,const float3& impulse);
	void Kill(float3& impulse);
	virtual bool Update(void);
	void StartFire(void);

	float health;
	float reclaimLeft;
	int id;
	int allyteam;

	int tempNum;

	FeatureDef* def;

	CMatrix44f transMatrix;
//	float3 residualImpulse;	//impulse energy that havent been acted on

	bool inUpdateQue;
	int drawQueType;					//0 none,1 static,2 non static
	int drawQuad;							//which drawquad we are part of

	float finalHeight;

	CFireProjectile* myFire;
	int fireTime;
	int emitSmokeTime;

//	float3 addPos;
//	float addRadius;
};

#endif // __FEATURE_H__
