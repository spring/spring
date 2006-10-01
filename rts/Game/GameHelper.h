// GameHelper.h: interface for the CGameHelper class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __GAME_HELPER_H__
#define __GAME_HELPER_H__

#include <list>
#include <map>
#include "Object.h"
class CGame;
class CUnit;
class CWeapon;
class CSolidObject;
class CFeature;

struct UnitDef;
#include <vector>
#include "Sim/Misc/DamageArray.h"
#include <list>
#include "MemPool.h"
#include "Sim/Units/UnitDef.h"

class CExplosionGenerator;

class CGameHelper  
{
public:
	CGameHelper(CGame* game);
	virtual ~CGameHelper();
	bool TestCone(const float3& from,const float3& dir,float length,float spread,int allyteam,CUnit* owner);
	bool TestTrajectoryCone(const float3 &from, const float3 &flatdir,float length, float linear, float quadratic, float spread, float baseSize, int allyteam,CUnit* owner);
	void GetEnemyUnits(float3& pos,float radius,int searchAllyteam,std::vector<int>& found);
	CUnit* GetClosestUnit(const float3& pos,float radius);
	CUnit* GetClosestEnemyUnit(const float3& pos,float radius,int searchAllyteam);
	CUnit* GetClosestEnemyUnitNoLosTest(const float3& pos,float radius,int searchAllyteam);
	CUnit* GetClosestFriendlyUnit(const float3& pos,float radius,int searchAllyteam);
	CUnit* GetClosestEnemyAircraft(const float3& pos,float radius,int searchAllyteam);
	void GenerateTargets(CWeapon *attacker, CUnit* lastTarget,std::map<float,CUnit*> &targets);
	float TraceRay(const float3& start,const float3& dir,float length,float power,CUnit* owner, CUnit*& hit);
	float GuiTraceRay(const float3& start,const float3& dir,float length, CUnit*& hit,float sizeMod,bool useRadar,CUnit* exclude=0);
	float GuiTraceRayFeature(const float3& start, const float3& dir, float length,CFeature*& feature);
	void Explosion(float3 pos,const DamageArray& damages,float radius, float edgeEffectivness, float explosionSpeed, CUnit* owner,bool damageGround,float gfxMod,bool ignoreOwner, CExplosionGenerator *explosionGraphics,CUnit *hit, const float3 &impactDir);
	float TraceRayTeam(const float3& start,const float3& dir,float length, CUnit*& hit,bool useRadar,CUnit* exclude,int allyteam);
	void BuggerOff(float3 pos, float radius,CUnit* exclude=0);
	float3 Pos2BuildPos(const BuildInfo& buildInfo);
	float3 Pos2BuildPos(const float3& pos, UnitDef* ud);
	void Update(void);

	bool LineFeatureCol(const float3& start, const float3& dir,float length);

	//get the position of a unit + eventuall error due to lack of los
	float3 GetUnitErrorPos(const CUnit* unit, int allyteam);

	CGame *game;

protected:
	CExplosionGenerator *stdExplosionGenerator;

	struct WaitingDamage{
#ifndef SYNCIFY
		inline void* operator new(size_t size){return mempool.Alloc(size);};
		inline void operator delete(void* p,size_t size){mempool.Free(p,size);};
#endif
		WaitingDamage(int attacker,int target,const DamageArray& damage,const float3& impulse)
			:	attacker(attacker),
				target(target),
				damage(damage),
				impulse(impulse)
		{}

		int target;
		int attacker;
		DamageArray damage;
		float3 impulse;
	};

	std::list<WaitingDamage*> waitingDamages[128];		//probably a symptom of some other problems but im getting paranoid about putting whole classes into high trafic stl containers instead of pointers to them
};

extern CGameHelper* helper;

#endif // __GAME_HELPER_H__
