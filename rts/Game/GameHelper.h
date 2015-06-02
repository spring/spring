/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GAME_HELPER_H
#define GAME_HELPER_H

#include "Sim/Misc/DamageArray.h"
#include "Sim/Projectiles/ExplosionListener.h"
#include "Sim/Units/CommandAI/Command.h"
#include "System/float3.h"
#include "System/type2.h"
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
struct MoveDef;
struct BuildInfo;
class IExplosionGenerator;
class CStdExplosionGenerator;

class CGameHelper
{
public:
	enum {
		TEST_ALLIED  = 1,
		TEST_NEUTRAL = 2,
	};
	enum BuildSquareStatus {
		BUILDSQUARE_BLOCKED     = 0,
		BUILDSQUARE_OCCUPIED    = 1,
		BUILDSQUARE_RECLAIMABLE = 2,
		BUILDSQUARE_OPEN        = 3
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

		unsigned int projectileID;
	};

	CGameHelper();
	~CGameHelper();

	CGameHelper(const CGameHelper&) = delete; // no-copy

	static void GetEnemyUnits(const float3& pos, float searchRadius, int searchAllyteam, std::vector<int>& found);
	static void GetEnemyUnitsNoLosTest(const float3& pos, float searchRadius, int searchAllyteam, std::vector<int>& found);
	static CUnit* GetClosestUnit(const float3& pos, float searchRadius);
	static CUnit* GetClosestEnemyUnit(const CUnit* excludeUnit, const float3& pos, float searchRadius, int searchAllyteam);
	static CUnit* GetClosestValidTarget(const float3& pos, float radius, int searchAllyteam, const CMobileCAI* cai);
	static CUnit* GetClosestEnemyUnitNoLosTest(
		const CUnit* excludeUnit,
		const float3& pos,
		float searchRadius,
		int searchAllyteam,
		bool sphere,
		bool canBeBlind
	);
	static CUnit* GetClosestFriendlyUnit(const CUnit* excludeUnit, const float3& pos, float searchRadius, int searchAllyteam);
	static CUnit* GetClosestEnemyAircraft(const CUnit* excludeUnit, const float3& pos, float searchRadius, int searchAllyteam);

	static void BuggerOff(float3 pos, float radius, bool spherical, bool forced, int teamId, CUnit* exclude);
	static float3 Pos2BuildPos(const BuildInfo& buildInfo, bool synced);

	///< test a single mapsquare for build possibility
	static BuildSquareStatus TestBuildSquare(
		const float3& pos,
		const int2& xrange,
		const int2& zrange,
		const UnitDef* unitDef,
		const MoveDef* moveDef,
		CFeature *&feature,
		int allyteam,
		bool synced
	);

	///< test if a unit can be built at specified position
	static BuildSquareStatus TestUnitBuildSquare(
		const BuildInfo&,
		CFeature*&,
		int allyteam,
		bool synced,
		std::vector<float3>* canbuildpos = NULL,
		std::vector<float3>* featurepos = NULL,
		std::vector<float3>* nobuildpos = NULL,
		const std::vector<Command>* commands = NULL
	);
	static float GetBuildHeight(const float3& pos, const UnitDef* unitdef, bool synced = true);
	static Command GetBuildCommand(const float3& pos, const float3& dir);

	/**
	 * @param minDist measured in 1/(SQUARE_SIZE * 2) = 1/16 of full map resolution.
	 */
	static float3 ClosestBuildSite(int team, const UnitDef* unitDef, float3 pos, float searchRadius, int minDist, int facing = 0);

	static void GenerateWeaponTargets(const CWeapon* weapon, const CUnit* avoidUnit, std::multimap<float, CUnit*>& targets);

	void Update();

	static float CalcImpulseScale(const DamageArray& damages, const float expDistanceMod);

	void DoExplosionDamage(
		CUnit* unit,
		CUnit* owner,
		const float3& expPos,
		const float expRadius,
		const float expSpeed,
		const float expEdgeEffect,
		const bool ignoreOwner,
		const DamageArray& damages,
		const int weaponDefID,
		const int projectileID
	);
	void DoExplosionDamage(
		CFeature* feature,
		CUnit* owner,
		const float3& expPos,
		const float expRadius,
		const float expEdgeEffect,
		const DamageArray& damages,
		const int weaponDefID,
		const int projectileID
	);

	void DamageObjectsInExplosionRadius(const ExplosionParams& params, const float expRad, const int weaponDefID);
	void Explosion(const ExplosionParams& params);

private:
	CStdExplosionGenerator* stdExplosionGenerator;

	struct WaitingDamage {
#if !defined(SYNCIFY)
		inline void* operator new(size_t size) {
			return mempool.Alloc(size);
		};
		inline void operator delete(void* p, size_t size) {
			mempool.Free(p, size);
		};
#endif
		WaitingDamage(int attacker, int target, const DamageArray& damage, const float3& impulse, const int _weaponID, const int _projectileID)
		: target(target)
		, attacker(attacker)
		, weaponID(_weaponID)
		, projectileID(_projectileID)
		, damage(damage)
		, impulse(impulse)
		{}

		int target;
		int attacker;
		int weaponID;
		int projectileID;

		DamageArray damage;
		float3 impulse;
	};

	struct ObjectCache {
	public:
		ObjectCache() : numUnits(0), numFeatures(0) {}
		bool Empty() const { return (units.empty() || features.empty()); }
		void Init(unsigned int maxUnits, unsigned int maxFeatures) {
			units.resize(maxUnits, NULL);
			features.resize(maxFeatures, NULL);

			numUnits = 0;
			numFeatures = 0;
		}
		void Kill() {
			units.clear();
			features.clear();
		}
		void Reset(unsigned int _numUnits, unsigned int _numFeatures) {
			numUnits = _numUnits;
			numFeatures = _numFeatures;

			units[numUnits] = NULL;
			features[numFeatures] = NULL;
		}

		std::vector<CUnit*>& GetUnits() { return units; }
		std::vector<CFeature*>& GetFeatures() { return features; }

		unsigned int* GetNumUnitsPtr() { return &numUnits; }
		unsigned int* GetNumFeaturesPtr() { return &numFeatures; }

	private:
		std::vector<CUnit*> units;
		std::vector<CFeature*> features;

		unsigned int numUnits;
		unsigned int numFeatures;
	};

	std::vector< std::list<WaitingDamage*> > waitingDamageLists;
};

extern CGameHelper* helper;

#endif // GAME_HELPER_H
