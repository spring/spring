/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _FEATURE_H
#define _FEATURE_H

#include <vector>
#include <list>
#include <string>
#include <boost/noncopyable.hpp>

#include "Sim/Objects/SolidObject.h"
#include "Sim/Units/UnitHandler.h"
#include "System/Matrix44f.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/ModInfo.h"

#define TREE_RADIUS 20

struct FeatureDef;
struct FeatureLoadParams;
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
	void Initialize(const FeatureLoadParams& params);
	int GetBlockingMapID() const { return id + (10 * uh->MaxUnits()); }

	/**
	 * Negative amount = reclaim
	 * @return true if reclaimed
	 */
	bool AddBuildPower(float amount, CUnit* builder);
	void DoDamage(const DamageArray& damages, const float3& impulse, CUnit* attacker, int weaponDefID);
	void ForcedMove(const float3& newPos);
	void ForcedSpin(const float3& newDir);
	bool Update();
	bool UpdatePosition();
	void UpdateFinalHeight(bool useGroundHeight);
	void StartFire();
	void EmitGeoSmoke();
	float RemainingResource(float res) const;
	float RemainingMetal() const;
	float RemainingEnergy() const;
	int ChunkNumber(float f) const;
	void CalculateTransform();
	void DependentDied(CObject *o);
	void ChangeTeam(int newTeam);

	bool IsInLosForAllyTeam(int argAllyTeam) const
	{
		if (alwaysVisible)
			return true;

		const bool inLOS = (argAllyTeam == -1 || loshandler->InLos(this->pos, argAllyTeam));

		switch (modInfo.featureVisibility) {
			case CModInfo::FEATURELOS_NONE:
			default:
				return inLOS;
			case CModInfo::FEATURELOS_GAIAONLY:
				return (this->allyteam == -1 || inLOS);
			case CModInfo::FEATURELOS_GAIAALLIED:
				return (this->allyteam == -1 || this->allyteam == argAllyTeam || inLOS);
			case CModInfo::FEATURELOS_ALL:
				return true;
		}
	}

	// NOTE:
	//   unlike CUnit which recalculates the matrix on each call
	//   (and uses the synced and error args) CFeature caches it
	//   this matrix is identical in synced and unsynced context!
	CMatrix44f GetTransformMatrix(const bool synced = false, const bool error = false) const { return transMatrix; }
	const CMatrix44f& GetTransformMatrixRef() const { return transMatrix; }

public:
	int defID;

	/**
	 * This flag is used to stop a potential exploit involving tripping
	 * a unit back and forth across a chunk boundary to get unlimited resources.
	 * Basically, once a corspe has been a little bit reclaimed,
	 * if they start rezzing, then they cannot reclaim again
	 * until the corpse has been fully 'repaired'.
	 */
	bool isRepairingBeforeResurrect;
	bool isAtFinalHeight;
	bool isMoving;
	bool inUpdateQue;

	float resurrectProgress;
	float reclaimLeft;
	float finalHeight;

	int tempNum;
	int lastReclaim;

	/// which drawQuad we are part of
	int drawQuad;
	int fireTime;
	int smokeTime;

	const FeatureDef* def;
	const UnitDef* udef; /// type of unit this feature should be resurrected to

	CFireProjectile* myFire;

	/// the solid object that is on top of the geothermal
	CSolidObject* solidOnTop;

private:
	void PostLoad();

	CMatrix44f transMatrix;
};

#endif // _FEATURE_H
