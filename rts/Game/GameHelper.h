/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GAME_HELPER_H
#define GAME_HELPER_H

#include "Sim/Misc/DamageArray.h"
#include "Sim/Projectiles/ExplosionListener.h"
#include "System/float3.h"
#include "System/MemPool.h"

#include <list>
#include <map>
#include <vector>

class CGame;
class CUnit;
class CWeapon;
class CSolidObject;
class CFeature;
class CMobileCAI;
struct UnitDef;
struct BuildInfo;
class IExplosionGenerator;
class CStdExplosionGenerator;

class CGameHelper : public CExplosionCreator
{
public:
	enum {
		TEST_ALLIED  = 1,
		TEST_NEUTRAL = 2,
	};

	struct ExplosionParams {
		const float3& pos;
		const float3& dir;
		const DamageArray& damages;
		const WeaponDef* weaponDef;

		CUnit* owner;
		CUnit* hitUnit;
		CFeature* hitFeature;

		float craterAreaOfEffect;
		float damageAreaOfEffect; // radius
		float edgeEffectiveness;
		float explosionSpeed;
		float gfxMod;
		bool impactOnly;
		bool ignoreOwner;
		bool damageGround;
	};

	CGameHelper();
	~CGameHelper();

	void GetEnemyUnits(const float3& pos, float searchRadius, int searchAllyteam, std::vector<int>& found);
	void GetEnemyUnitsNoLosTest(const float3& pos, float searchRadius, int searchAllyteam, std::vector<int>& found);
	CUnit* GetClosestUnit(const float3& pos, float searchRadius);
	CUnit* GetClosestEnemyUnit(const float3& pos, float searchRadius, int searchAllyteam);
	CUnit* GetClosestValidTarget(const float3& pos, float radius, int searchAllyteam, const CMobileCAI* cai);
	CUnit* GetClosestEnemyUnitNoLosTest(const float3& pos, float searchRadius, int searchAllyteam, bool sphere, bool canBeBlind);
	CUnit* GetClosestFriendlyUnit(const float3& pos, float searchRadius, int searchAllyteam);
	CUnit* GetClosestEnemyAircraft(const float3& pos, float searchRadius, int searchAllyteam);

	//get the position of a unit + eventuall error due to lack of los
	float3 GetUnitErrorPos(const CUnit* unit, int allyteam);

	void BuggerOff(float3 pos, float radius, bool spherical, bool forced, int teamId, CUnit* exclude);
	float3 Pos2BuildPos(const BuildInfo& buildInfo, bool synced);

	/**
	 * @param minDist measured in 1/(SQUARE_SIZE * 2) = 1/16 of full map resolution.
	 */
	float3 ClosestBuildSite(int team, const UnitDef* unitDef, float3 pos, float searchRadius, int minDist, int facing = 0);

	void Update();
	void GenerateWeaponTargets(const CWeapon* weapon, const CUnit* lastTargetUnit, std::multimap<float, CUnit*>& targets);

	void DoExplosionDamage(CUnit* unit, CUnit* owner, const float3& expPos, float expRad, float expSpeed, float edgeEffectiveness, bool ignoreOwner, const DamageArray& damages, const int weaponDefID);
	void DoExplosionDamage(CFeature* feature, const float3& expPos, float expRad, const DamageArray& damages, const int weaponDefID);

	void Explosion(const ExplosionParams& params);

private:
	CStdExplosionGenerator* stdExplosionGenerator;

	struct WaitingDamage{
#if !defined(SYNCIFY) && !defined(USE_MMGR)
		inline void* operator new(size_t size) {
			return mempool.Alloc(size);
		};
		inline void operator delete(void* p, size_t size) {
			mempool.Free(p, size);
		};
#endif
		WaitingDamage(int attacker, int target, const DamageArray& damage, const float3& impulse, const int weaponId)
			:	target(target),
				attacker(attacker),
				weaponId(weaponId),
				damage(damage),
				impulse(impulse)
		{}

		int target;
		int attacker;
		int weaponId;
		DamageArray damage;
		float3 impulse;
	};

	/**
	 * probably a symptom of some other problems,
	 * but im getting paranoid about putting whole classes
	 * into high trafic STL containers instead of pointers to them
	 */
	std::list<WaitingDamage*> waitingDamages[128];
};

extern CGameHelper* helper;

#endif // GAME_HELPER_H
