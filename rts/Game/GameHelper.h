// GameHelper.h: interface for the CGameHelper class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __GAME_HELPER_H__
#define __GAME_HELPER_H__

#include <list>
#include <map>
#include <vector>
#include "Object.h"
class CGame;
class CUnit;
class CWeapon;
class CSolidObject;
class CFeature;
class CMobileCAI;

struct UnitDef;
#include "Sim/Misc/DamageArray.h"
#include "MemPool.h"
#include "Sim/Units/UnitDef.h"

class CExplosionGenerator;

class CGameHelper
{
public:
	CGameHelper();
	~CGameHelper();
	bool TestAllyCone(const float3& from, const float3& dir, float length, float spread, int allyteam, CUnit* owner);
	bool TestNeutralCone(const float3& from, const float3& dir, float length, float spread, CUnit* owner);
	bool TestTrajectoryAllyCone(const float3 &from, const float3& flatdir, float length, float linear, float quadratic, float spread, float baseSize, int allyteam, CUnit* owner);
	bool TestTrajectoryNeutralCone(const float3 &from, const float3& flatdir, float length, float linear, float quadratic, float spread, float baseSize, CUnit* owner);

	void GetEnemyUnits(const float3& pos, float searchRadius, int searchAllyteam, std::vector<int>& found);
	CUnit* GetClosestUnit(const float3& pos, float searchRadius);
	CUnit* GetClosestEnemyUnit(const float3& pos, float searchRadius, int searchAllyteam);
	CUnit* GetClosestValidTarget(const float3& pos, float radius, int searchAllyteam, const CMobileCAI* cai);
	CUnit* GetClosestEnemyUnitNoLosTest(const float3& pos, float searchRadius, int searchAllyteam, bool sphere, bool canBeBlind);
	CUnit* GetClosestFriendlyUnit(const float3& pos, float searchRadius, int searchAllyteam);
	CUnit* GetClosestEnemyAircraft(const float3& pos, float searchRadius, int searchAllyteam);

	void GenerateTargets(const CWeapon *attacker, CUnit* lastTarget,std::map<float,CUnit*> &targets);
	float TraceRay(const float3& start,const float3& dir,float length,float power,const CUnit* owner,const CUnit*& hit,int collisionFlags=0);
	float GuiTraceRay(const float3& start,const float3& dir,float length,const CUnit*& hit,bool useRadar,const CUnit* exclude=NULL);
	float GuiTraceRayFeature(const float3& start, const float3& dir, float length,const CFeature*& feature);

	void DoExplosionDamage(CUnit*, const float3&, float, float, bool, CUnit*, float, const DamageArray&, int);
	void DoExplosionDamage(CFeature*, const float3&, float, CUnit*, const DamageArray&);
	void Explosion(float3 pos, const DamageArray& damages,float radius, float edgeEffectiveness, float explosionSpeed, CUnit* owner,bool damageGround,float gfxMod,bool ignoreOwner,bool impactOnly, CExplosionGenerator *explosionGraphics,CUnit *hit, const float3 &impactDir, int weaponId);

	float TraceRayTeam(const float3& start,const float3& dir,float length, CUnit*& hit,bool useRadar,CUnit* exclude,int allyteam);
	void BuggerOff(float3 pos, float radius, bool spherical = true, CUnit* exclude = 0);
	float3 Pos2BuildPos(const BuildInfo& buildInfo);
	float3 Pos2BuildPos(const float3& pos, const UnitDef* ud);
	/**
	 * @param minDist messured in 1/(SQUARE_SIZE * 2) = 1/16 of full map resolution.
	 */
	float3 ClosestBuildSite(int team, const UnitDef* unitDef, float3 pos, float searchRadius, int minDist, int facing = 0);
	void Update(void);

	bool LineFeatureCol(const float3& start, const float3& dir,float length);

	//get the position of a unit + eventuall error due to lack of los
	float3 GetUnitErrorPos(const CUnit* unit, int allyteam);

protected:
	CExplosionGenerator *stdExplosionGenerator;

	struct WaitingDamage{
#if !defined(SYNCIFY) && !defined(USE_MMGR)
		inline void* operator new(size_t size){return mempool.Alloc(size);};
		inline void operator delete(void* p,size_t size){mempool.Free(p,size);};
#endif
		WaitingDamage(int attacker,int target,const DamageArray& damage,const float3& impulse, const int weaponId)
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

	std::list<WaitingDamage*> waitingDamages[128];		//probably a symptom of some other problems but im getting paranoid about putting whole classes into high trafic stl containers instead of pointers to them

private:
	bool TestConeHelper(const float3& from, const float3& dir, float length, float spread, const CUnit* u);
	bool TestTrajectoryConeHelper(const float3& from, const float3& flatdir, float length, float linear, float quadratic, float spread, float baseSize, const CUnit* u);
};

extern CGameHelper* helper;

#endif // __GAME_HELPER_H__
