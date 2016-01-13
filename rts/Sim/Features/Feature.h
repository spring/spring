/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _FEATURE_H
#define _FEATURE_H

#include <vector>
#include <list>
#include <string>
#include <boost/noncopyable.hpp>

#include "Sim/Objects/SolidObject.h"
#include "System/Matrix44f.h"
#include "Sim/Misc/Resource.h"

#define TREE_RADIUS 20

struct FeatureDef;
struct FeatureLoadParams;
class CUnit;
struct UnitDef;
struct DamageArray;
class CFireProjectile;



class CFeature: public CSolidObject, public boost::noncopyable
{
	CR_DECLARE(CFeature)

public:
	CFeature();
	~CFeature();

	/**
	 * Pos of quad must not change after this.
	 * This will add this to the FeatureHandler.
	 */
	void Initialize(const FeatureLoadParams& params);

	int GetBlockingMapID() const;

	/**
	 * Negative amount = reclaim
	 * @return true if reclaimed
	 */
	bool AddBuildPower(CUnit* builder, float amount);
	void DoDamage(const DamageArray& damages, const float3& impulse, CUnit* attacker, int weaponDefID, int projectileID);
	void SetVelocity(const float3& v);
	void ForcedMove(const float3& newPos);
	void ForcedSpin(const float3& newDir);

	bool Update();
	bool UpdatePosition();
	void UpdateTransform() { transMatrix = CMatrix44f(pos, -rightdir, updir, frontdir); }
	void UpdateTransformAndPhysState();
	void UpdateFinalHeight(bool useGroundHeight);

	void StartFire();
	void EmitGeoSmoke();

	void DependentDied(CObject *o);
	void ChangeTeam(int newTeam);

	bool IsInLosForAllyTeam(int argAllyTeam) const;

	// NOTE:
	//   unlike CUnit which recalculates the matrix on each call
	//   (and uses the synced and error args) CFeature caches it
	//   this matrix is identical in synced and unsynced context!
	CMatrix44f GetTransformMatrix(const bool synced = false, const bool error = false) const { return transMatrix; }
	const CMatrix44f& GetTransformMatrixRef() const { return transMatrix; }
	void SetTransform(const CMatrix44f& m) { transMatrix = m; }
private:
	static int ChunkNumber(float f);

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
	bool inUpdateQue;
	bool deleteMe;

	float finalHeight;

	float resurrectProgress;
	float reclaimLeft;
	int lastReclaim;
	SResourcePack resources;

	/// which drawQuad we are part of
	int drawQuad;
	int drawFlag;

	float drawAlpha;
	bool alphaFade;

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
