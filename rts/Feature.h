#ifndef __FEATURE_H__
#define __FEATURE_H__

#include "SolidObject.h"
#include "Matrix44f.h"
#include <vector>
#include <list>
#include <string>

struct FeatureDef;
class CUnit;
struct DamageArray;
class CFireProjectile;

class CFeature :
	public CSolidObject
{
public:
	CFeature(const float3& pos,FeatureDef* def,short int heading,int allyteam,std::string fromUnit);	//pos of quad must not change after this
	~CFeature(void);
	bool AddBuildPower(float amount, CUnit* builder);								//negative amount=reclaim,return=true->reclaimed
	void DoDamage(const DamageArray& damages, CUnit* attacker,const float3& impulse);
	void Kill(float3& impulse);
	virtual bool Update(void);
	void StartFire(void);
	void DrawS3O();

	std::string createdFromUnit;
	float resurrectProgress;

	float health;
	float reclaimLeft;
	int id;
	int allyteam;

	int tempNum;
	int lastReclaim;

	FeatureDef* def;

	CMatrix44f transMatrix;
//	float3 residualImpulse;	//impulse energy that havent been acted on

	bool inUpdateQue;
	int drawQuad;							//which drawquad we are part of

	float finalHeight;

	CFireProjectile* myFire;
	int fireTime;
	int emitSmokeTime;

//	float3 addPos;
//	float addRadius;
};

#endif // __FEATURE_H__
