// GameHelper.h: interface for the CGameHelper class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GAMEHELPER_H__E2547961_C62A_11D4_AD55_0080ADA84DE3__INCLUDED_)
#define AFX_GAMEHELPER_H__E2547961_C62A_11D4_AD55_0080ADA84DE3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include <list>
#include <map>
	// Added by ClassView
#include "object.h"
class CGame;
class CUnit;
class CWeapon;
class CSolidObject;
class CFeature;
#include <vector>
#include "damagearray.h"

using namespace std;

class CExplosionGraphics;

class CGameHelper  
{
public:
	CGameHelper(CGame* game);
	virtual ~CGameHelper();
	bool TestCone(const float3& from,const float3& dir,float length,float spread,int allyteam,CUnit* owner);
	void GetEnemyUnits(float3& pos,float radius,int searchAllyteam,vector<int>& found);
	CUnit* GetClosestUnit(const float3& pos,float radius);
	CUnit* GetClosestEnemyUnit(const float3& pos,float radius,int searchAllyteam);
	CUnit* GetClosestFriendlyUnit(const float3& pos,float radius,int searchAllyteam);
	CUnit* GetClosestEnemyAircraft(const float3& pos,float radius,int searchAllyteam);
	void GenerateTargets(CWeapon *attacker, CUnit* lastTarget,std::map<float,CUnit*> &targets);
	float TraceRay(const float3& start,const float3& dir,float length,float power,CUnit* owner, CUnit*& hit);
	float GuiTraceRay(const float3& start,const float3& dir,float length, CUnit*& hit,float sizeMod,bool useRadar,CUnit* exclude=0);
	float GuiTraceRayFeature(const float3& start, const float3& dir, float length,CFeature*& feature);
	void Explosion(const float3& pos,const DamageArray& damages,float radius,CUnit* owner,bool damageGround=true,float gfxMod=1,bool ignoreOwner=false,int graphicType=0);
	float TraceRayTeam(const float3& start,const float3& dir,float length, CUnit*& hit,bool useRadar,CUnit* exclude,int allyteam);

	CGame* game;
	bool LineFeatureCol(const float3& start, const float3& dir,float length);
	float3 GetUnitErrorPos(CUnit* unit, int allyteam);	//get the position of a unit + eventuall error due to lack of los

	std::vector<CExplosionGraphics*> explosionGraphics;
	void BuggerOff(float3 pos, float radius,CUnit* exclude=0);
};

extern CGameHelper* helper;

#endif // !defined(AFX_GAMEHELPER_H__E2547961_C62A_11D4_AD55_0080ADA84DE3__INCLUDED_)
