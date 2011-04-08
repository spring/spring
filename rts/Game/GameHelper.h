/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __GAME_HELPER_H__
#define __GAME_HELPER_H__

#include <list>
#include <map>
#include <vector>

#include "Sim/Misc/DamageArray.h"
#include "System/float3.h"
#include "System/MemPool.h"

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

class CGameHelper
{
public:
	CGameHelper();
	~CGameHelper();

public:
	void GetEnemyUnits(const float3& pos, float searchRadius, int searchAllyteam, std::vector<int>& found);
	void GetEnemyUnitsNoLosTest(const float3& pos, float searchRadius, int searchAllyteam, std::vector<int>& found);
	CUnit* GetClosestUnit(const float3& pos, float searchRadius);
	CUnit* GetClosestEnemyUnit(const float3& pos, float searchRadius, int searchAllyteam);
	CUnit* GetClosestValidTarget(const float3& pos, float radius, int searchAllyteam, const CMobileCAI* cai);
	CUnit* GetClosestEnemyUnitNoLosTest(const float3& pos, float searchRadius, int searchAllyteam, bool sphere, bool canBeBlind);
	CUnit* GetClosestFriendlyUnit(const float3& pos, float searchRadius, int searchAllyteam);
	CUnit* GetClosestEnemyAircraft(const float3& pos, float searchRadius, int searchAllyteam);

public:
	//get the position of a unit + eventuall error due to lack of los
	float3 GetUnitErrorPos(const CUnit* unit, int allyteam);

	void BuggerOff(float3 pos, float radius, bool spherical, bool forced, int teamId, CUnit* exclude);
	float3 Pos2BuildPos(const BuildInfo& buildInfo);
	float3 Pos2BuildPos(const float3& pos, const UnitDef* ud);

	/**
	 * @param minDist measured in 1/(SQUARE_SIZE * 2) = 1/16 of full map resolution.
	 */
	float3 ClosestBuildSite(int team, const UnitDef* unitDef, float3 pos, float searchRadius, int minDist, int facing = 0);

public:
	void Update();
	void GenerateWeaponTargets(const CWeapon* weapon, const CUnit* lastTargetUnit, std::multimap<float, CUnit*>& targets);
	void DoExplosionDamage(CUnit* unit, const float3& expPos, float expRad, float expSpeed, bool ignoreOwner, CUnit* owner, float edgeEffectiveness, const DamageArray& damages, int weaponId);
	void DoExplosionDamage(CFeature* feature, const float3& expPos, float expRad, const DamageArray& damages);
	void Explosion(float3 pos, const DamageArray& damages, float radius, float edgeEffectiveness, float explosionSpeed, CUnit* owner, bool damageGround, float gfxMod, bool ignoreOwner, bool impactOnly, IExplosionGenerator* explosionGraphics, CUnit* hit, const float3& impactDir, int weaponId, CFeature* hitfeature = NULL);

	enum {
		TEST_ALLIED  = 1,
		TEST_NEUTRAL = 2,
	};

protected:
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

#endif // __GAME_HELPER_H__
