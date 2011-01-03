/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __FEATURE_H__
#define __FEATURE_H__

#include <vector>
#include <list>
#include <string>
#include <boost/noncopyable.hpp>

#include "Sim/Objects/SolidObject.h"
#include "Sim/Units/UnitHandler.h"
#include "Matrix44f.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/ModInfo.h"

#define TREE_RADIUS 20

struct FeatureDef;
class CUnit;
struct DamageArray;
class CFireProjectile;



class CFeature: public CSolidObject, public boost::noncopyable
{
	CR_DECLARE(CFeature);

public:
	CFeature();
	~CFeature();

	/**
	 * Pos of quad must not change after this.
	 * This will add this to the FeatureHandler.
	 */
	void Initialize(const float3& pos, const FeatureDef* def, short int heading, int facing,
		int team, int allyteam, std::string fromUnit, const float3& speed = ZeroVector, int smokeTime = 0);
	int GetBlockingMapID() const { return id + (10 * uh->MaxUnits()); }

	/**
	 * Negative amount = reclaim
	 * @return true if reclaimed
	 */
	bool AddBuildPower(float amount, CUnit* builder);
	void DoDamage(const DamageArray& damages, const float3& impulse);
	void Kill(const float3& impulse);
	void ForcedMove(const float3& newPos, bool snapToGround = true);
	void ForcedSpin(const float3& newDir);
	virtual bool Update(void);
	bool UpdatePosition(void);
	void StartFire(void);
	float RemainingResource(float res) const;
	float RemainingMetal(void) const;
	float RemainingEnergy(void) const;
	int ChunkNumber(float f);
	void CalculateTransform();
	void DependentDied(CObject *o);
	void ChangeTeam(int newTeam);

	bool IsInLosForAllyTeam(int allyteam) const
	{
		if (alwaysVisible)
			return true;
		switch (modInfo.featureVisibility) {
			case CModInfo::FEATURELOS_NONE:
			default:
				return loshandler->InLos(this->pos, allyteam);
			case CModInfo::FEATURELOS_GAIAONLY:
				return (this->allyteam == -1 || loshandler->InLos(this->pos, allyteam));
			case CModInfo::FEATURELOS_GAIAALLIED:
				return (this->allyteam == -1 || this->allyteam == allyteam
					|| loshandler->InLos(this->pos, allyteam));
			case CModInfo::FEATURELOS_ALL:
				return true;
		}
	}

public:
	std::string createdFromUnit;
	/**
	 * This flag is used to stop a potential exploit involving tripping
	 * a unit back and forth across a chunk boundary to get unlimited resources.
	 * Basically, once a corspe has been a little bit reclaimed,
	 * if they start rezzing, then they cannot reclaim again
	 * until the corpse has been fully 'repaired'.
	 */
	bool isRepairingBeforeResurrect;
	float resurrectProgress;

	float health;
	float reclaimLeft;

	bool luaDraw;
	bool noSelect;

	int tempNum;
	int lastReclaim;

	const FeatureDef* def;
	std::string defName;

	CMatrix44f transMatrix;

	bool inUpdateQue;
	/// which drawQuad we are part of
	int drawQuad;

	float finalHeight;
	bool reachedFinalPos;

	CFireProjectile* myFire;
	int fireTime;
	int emitSmokeTime;

	/// the solid object that is on top of the geothermal
	CSolidObject* solidOnTop;

	/// initially a copy of CUnit::speed
	float3 deathSpeed;
	float tempalpha;

private:
	void PostLoad();
};

#endif // __FEATURE_H__
