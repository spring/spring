/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _FEATURE_H
#define _FEATURE_H

#include <vector>
#include <list>
#include <string>
#include "System/Misc/NonCopyable.h"

#include "Sim/Objects/SolidObject.h"
#include "System/Matrix44f.h"
#include "Sim/Misc/Resource.h"

#define TREE_RADIUS 20

struct SolidObjectDef;
struct FeatureDef;
struct FeatureLoadParams;
class CUnit;
struct UnitDef;
class DamageArray;
class CFireProjectile;



class CFeature: public CSolidObject, public spring::noncopyable
{
	CR_DECLARE(CFeature)

public:
	CFeature();
	~CFeature();

	CR_DECLARE_SUB(MoveCtrl)
	struct MoveCtrl {
		CR_DECLARE_STRUCT(MoveCtrl)
	public:
		MoveCtrl(): enabled(false) {
			movementMask = OnesVector;
			velocityMask = OnesVector;
			 impulseMask = OnesVector;
		}

		void SetMovementMask(const float3& movMask) { movementMask = movMask; }
		void SetVelocityMask(const float3& velMask) { velocityMask = velMask; }

	public:
		// if true, feature will not apply any unwanted position
		// updates (but is still considered moving so long as its
		// velocity is non-zero, so it stays in the UQ)
		bool enabled;

		// dimensions in which feature can move or receive impulse
		// note: these should always be binary vectors (.xyz={0,1})
		float3 movementMask;
		float3 velocityMask;
		float3 impulseMask;

		float3 velVector;
		float3 accVector;
	};

	enum {
		FD_NODRAW_FLAG = 0, // must be 0
		FD_OPAQUE_FLAG = 1,
		FD_ALPHAF_FLAG = 2,
		FD_SHADOW_FLAG = 3,
		FD_FARTEX_FLAG = 4,
	};


	/**
	 * Pos of quad must not change after this.
	 * This will add this to the FeatureHandler.
	 */
	void Initialize(const FeatureLoadParams& params);

	const SolidObjectDef* GetDef() const { return ((const SolidObjectDef*) def); }

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
	bool UpdateVelocity(const float3& dragAccel, const float3& gravAccel, const float3& movMask, const float3& velMask);

	void SetTransform(const CMatrix44f& m, bool synced) { transMatrix[synced] = m; }
	void UpdateTransform(const float3& p, bool synced) { transMatrix[synced] = std::move(ComposeMatrix(p)); }
	void UpdateTransformAndPhysState();
	void UpdateQuadFieldPosition(const float3& moveVec);

	void StartFire();
	void EmitGeoSmoke();

	void DependentDied(CObject *o);
	void ChangeTeam(int newTeam);

	bool IsInLosForAllyTeam(int argAllyTeam) const;

	// NOTE:
	//   unlike CUnit which recalculates the matrix on each call
	//   (and uses the synced and error args) CFeature caches it
	CMatrix44f GetTransformMatrix(bool synced = false, bool fullread = false) const final { return transMatrix[synced]; }
	const CMatrix44f& GetTransformMatrixRef(bool synced = false) const { return transMatrix[synced]; }

private:
	static int ChunkNumber(float f);

public:
	/**
	 * This flag is used to stop a potential exploit involving tripping
	 * a unit back and forth across a chunk boundary to get unlimited resources.
	 * Basically, once a corspe has been a little bit reclaimed,
	 * if they start rezzing, then they cannot reclaim again
	 * until the corpse has been fully 'repaired'.
	 */
	bool isRepairingBeforeResurrect = false;
	bool inUpdateQue = false;
	bool deleteMe = false;
	bool alphaFade = true; // unsynced

	float drawAlpha = 1.0f; // unsynced
	float resurrectProgress = 0.0f;
	float reclaimTime = 0.0f;
	float reclaimLeft = 1.0f;

	SResourcePack defResources = {0.0f, 1.0f};
	SResourcePack resources = {0.0f, 1.0f};

	int lastReclaimFrame = 0;
	int fireTime = 0;
	int smokeTime = 0;

	int drawQuad = -1; /// which drawQuad we are part of (unsynced)
	int drawFlag = -2; /// one of FD_*_FLAG (unsynced)

	const FeatureDef* def = nullptr;
	const UnitDef* udef = nullptr; /// type of unit this feature should be resurrected to

	MoveCtrl moveCtrl;

	CFireProjectile* myFire = nullptr;

	/// object on top of us if we are a geothermal vent
	CSolidObject* solidOnTop = nullptr;


private:
	void PostLoad();

	// [0] := unsynced, [1] := synced
	CMatrix44f transMatrix[2];
};

#endif // _FEATURE_H
