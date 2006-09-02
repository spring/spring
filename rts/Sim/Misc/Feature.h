#ifndef __FEATURE_H__
#define __FEATURE_H__

#include "Sim/Objects/SolidObject.h"
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
	CR_DECLARE(CFeature);

	CFeature();	
	~CFeature();

	void Initialize(const float3& pos,FeatureDef* def,short int heading, int facing, int allyteam,std::string fromUnit); //pos of quad must not change after this
	bool AddBuildPower(float amount, CUnit* builder);								//negative amount=reclaim,return=true->reclaimed
	void DoDamage(const DamageArray& damages, CUnit* attacker,const float3& impulse);
	void Kill(float3& impulse);
	virtual bool Update(void);
	void StartFire(void);
	float RemainingResource(float res);
	float RemainingMetal(void);
	float RemainingEnergy(void);
	int ChunkNumber(float f);
	void DrawS3O();
	void CalculateTransform();
	void DependentDied(CObject *o);

	CUnit* lastBuilder;

	std::string createdFromUnit;
	// This flag is used to stop a potential exploit involving tripping a unit back and forth
	// across a chunk boundary to get unlimited resources. Basically, once a corspe has been a little bit
	// reclaimed, if they start rezzing then they cannot reclaim again until the corpse has been fully
	// 'repaired'.
	bool isRepairingBeforeResurrect;
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

	CSolidObject *solidOnTop; // the solid object that is on top of the geothermal

//	float3 addPos;
//	float addRadius;
};

#endif // __FEATURE_H__
