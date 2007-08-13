// GameHelper.cpp: implementation of the CGameHelperHelper class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "GameHelper.h"
#include "Game.h"
#include "Map/Ground.h"
#include "Sim/Projectiles/TracerProjectile.h"
#include "Map/ReadMap.h"
#include "Sim/Units/Unit.h"
#include "LogOutput.h"
#include "Sim/Misc/QuadField.h"
#include "Sync/SyncTracer.h"
#include "Sim/Misc/LosHandler.h"
#include "Sound.h"
#include "Map/MapDamage.h"
#include "Camera.h"
#include "Game/UI/LuaUI.h"
#include "Lua/LuaCallInHandler.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Misc/DamageArray.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Misc/Feature.h"
#include "Sim/Misc/RadarHandler.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/UnitDef.h"
#include "Rendering/GroundDecalHandler.h"
#include "Sim/Misc/GeometricObjects.h"
#include "Rendering/Env/BaseWater.h"
#include "Game/GameSetup.h"
#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGameHelper* helper;


CGameHelper::CGameHelper(CGame* game)
{
	this->game=game;

	stdExplosionGenerator = SAFE_NEW CStdExplosionGenerator;
}

CGameHelper::~CGameHelper()
{
	delete stdExplosionGenerator;

	for(int a=0;a<128;++a){
		std::list<WaitingDamage*>* wd=&waitingDamages[a];
		while(!wd->empty()){
			delete wd->back();
			wd->pop_back();
		}
	}
}


void CGameHelper::Explosion(float3 pos, const DamageArray& damages,
                            float radius, float edgeEffectiveness,
                            float explosionSpeed, CUnit* owner,
                            bool damageGround, float gfxMod, bool ignoreOwner,
                            CExplosionGenerator* explosionGraphics, CUnit* hit,
                            const float3& impactDir, int weaponId)
{
	if (luaUI) {
		luaUI->ShockFront(damages.damages[0], pos, radius);
	}
	bool noGfx = luaCallIns.Explosion(weaponId, pos, owner);

#ifdef TRACE_SYNC
	tracefile << "Explosion: ";
	tracefile << pos.x << " " << damages[0] <<  " " << radius << "\n";
#endif
/*	if(pos.x<0 || pos.z<0 || pos.x>gs->mapx*SQUARE_SIZE || pos.z>gs->mapy*SQUARE_SIZE){
		logOutput.Print("Explosion outside map %.0f %.0f",pos.x,pos.z);
		return;
	}
*/
//	logOutput.Print("Explosion %i",damageGround);
	if (radius < 1.0f) {
		radius = 1.0f;
	}

	float h2 = ground->GetHeight2(pos.x, pos.z);
	if (pos.y < h2) {
		pos.y = h2;
	}

	float height = pos.y - h2;
	if (height < 0.0f) {
		height = 0.0f;
	}

	vector<CUnit*> units = qf->GetUnitsExact(pos, radius);
	//float gd=max(30.f,damages[0]/20);
	//float explosionSpeed=(8+gd*2.5f)/(9+sqrtf(gd)*0.7f)*0.5f;	//this is taken from the explosion graphics and could probably be simplified a lot

	// Damage Units
	for (vector<CUnit*>::iterator ui = units.begin(); ui != units.end(); ++ui) {
		CUnit* unit = *ui;
		if (ignoreOwner && (unit == owner)) {
			continue;
		}
		// dist = max(distance from center of unit to center of explosion, unit->radius+0.1)
		float3 dif = unit->midPos - pos;
		float dist = dif.Length();
		const float fudgeRad = unit->radius + 0.1f;
		if (dist < fudgeRad) {
			dist = fudgeRad;
		}
		// dist2 = distance from boundary of unit's hitsphere to center of explosion,
		// unless unit->isUnderwater and explosion is above water: then it's center to center distance
		float dist2 = dist - unit->radius;
		if (unit->isUnderWater && (pos.y > -1.0f)) {	//should make it harder to damage subs with above water weapons
			dist2 += unit->radius;
			if (dist2 > radius) {
				dist2 = radius;
			}
		}
		// Clamp dist to radius to prevent division by zero. (dist2 can never be > radius)
		// We still need the original dist later to normalize dif.
		float dist1 = dist;
		if (dist1 > radius) {
			dist1 = radius;
		}
		float mod =(radius-dist1)/(radius-dist1*edgeEffectiveness);
		float mod2=(radius-dist2)/(radius-dist2*edgeEffectiveness);
		dif /= dist; // dist > unit->radius+0.1f, see above
		dif.y += 0.12f;

		DamageArray damageDone = damages*mod2;
		float3 addedImpulse = dif * (damages.impulseFactor * mod * (damages[0] + damages.impulseBoost) * 3.2f);

		if (dist2 < (explosionSpeed * 4.0f)) { //damage directly
			unit->DoDamage(damageDone,owner,addedImpulse, weaponId);
		} else { //damage later
			WaitingDamage* wd = SAFE_NEW WaitingDamage(owner ? owner->id : -1, unit->id, damageDone, addedImpulse, weaponId);
			waitingDamages[(gs->frameNum+int(dist2/explosionSpeed)-3)&127].push_front(wd);
		}
	}

	vector<CFeature*> features=qf->GetFeaturesExact(pos,radius);
	vector<CFeature*>::iterator fi;
	for (fi = features.begin(); fi != features.end(); ++fi) {
		CFeature* feature = *fi;
		float3 dif= feature->midPos -pos;
		float dist = dif.Length();
		if (dist < 0.1f) {
			dist = 0.1f;
		}
		float mod = (radius - dist) / radius;
		// always do some damage with explosive stuff
		// (ddm wreckage etc is to big to normally be damaged otherwise, even by bb shells)
		if ((radius > 8.0f) && (dist < (feature->radius * 1.1f)) && (mod < 0.1f)) {
			mod = 0.1f;
		}
		if (mod > 0.0f) {
			feature->DoDamage(damages * mod, owner,
			                  dif * (damages.impulseFactor * mod / dist * (damages[0] + damages.impulseBoost)));
		}
		if (gs->randFloat() > 0.7f) {
			feature->StartFire();
		}
	}

	if (damageGround && !mapDamage->disabled &&
	    (radius > height) && (damages.craterMult > 0.0f)) {
		float damage = damages[0] * (1.0f - (height / radius));
		if (damage > (radius * 10.0f)) {
			damage = radius * 10.0f;  // limit the depth somewhat
		}
		mapDamage->Explosion(pos,(damage + damages.craterBoost)*damages.craterMult,radius-height);
	}

	// use CStdExplosionGenerator by default
	if (!noGfx) {
		if (!explosionGraphics) {
			explosionGraphics = stdExplosionGenerator;
		}
		explosionGraphics->Explosion(pos, damages[0], radius, owner, gfxMod, hit, impactDir);
	}
	groundDecals->AddExplosion(pos, damages[0], radius);
	// sound->PlaySample(explosionSounds[rand()*4/(RAND_MAX+1)],pos,damage*2);
	ENTER_UNSYNCED;
	water->AddExplosion(pos,damages[0],radius);
	ENTER_SYNCED;
}


float CGameHelper::TraceRay(const float3 &start, const float3 &dir, float length, float power, CUnit* owner, CUnit *&hit)
{
	float groundLength=ground->LineGroundCol(start,start+dir*length);

//	logOutput.Print("gl %f",groundLength);
	if(length>groundLength && groundLength>0)
		length=groundLength;


	int quads[1000];
	int* endQuad = quads;
	qf->GetQuadsOnRay(start,dir,length,endQuad);

	for(int* qi=quads;qi!=endQuad;++qi){
		CQuadField::Quad& quad = qf->baseQuads[*qi];
		for(list<CFeature*>::iterator ui=quad.features.begin();ui!=quad.features.end();++ui){
			if(!(*ui)->blocking)
				continue;
			float3 dif=(*ui)->midPos-start;
			float closeLength=dif.dot(dir);
			if(closeLength<0)
				continue;
			if(closeLength>length)
				continue;
			float3 closeVect=dif-dir*closeLength;
			if(closeVect.SqLength() < (*ui)->sqRadius){
				length=closeLength;
			}
		}
	}

//	float minLength=length;
	hit=0;

	for(int* qi=quads;qi!=endQuad;++qi){
		CQuadField::Quad& quad = qf->baseQuads[*qi];

		for(list<CUnit*>::iterator ui=quad.units.begin();ui!=quad.units.end();++ui){
			if((*ui)==owner)
				continue;
			float3 dif=(*ui)->midPos-start;
			float closeLength=dif.dot(dir);
			if(closeLength<0)
				continue;
			if(closeLength>length)
				closeLength=length;
			float3 closeVect=dif-dir*closeLength;

			/*float rad=(*ui)->radius;
			float tmp = rad * rad - closeVect.SqLength();*/

			/*if(tmp > 0 && length>closeLength+sqrt(tmp)){
				length=closeLength-sqrt(tmp)*0.5f;
				hit=*ui;
			}*/
			if(closeVect.SqLength() < (*ui)->sqRadius){
				length=closeLength;
				hit=*ui;
			}
		}
	}
	return length;
}

float CGameHelper::GuiTraceRay(const float3 &start, const float3 &dir, float length, CUnit *&hit,float sizeMod,bool useRadar,CUnit* exclude)
{
	const float groundLength = ground->LineGroundCol(start, start + dir * length);

//	logOutput.Print("gl %f",groundLength);
	if ((length > (groundLength + 200.0f)) && (groundLength > 0.0f)) {
		//need to add some cause we take the backside of the unit sphere;
		length = groundLength + 200.0f;
	}

//	float minLength = length;
	hit = 0;

	vector<int> quads = qf->GetQuadsOnRay(start, dir, length);
	vector<int>::iterator qi;

	for (qi = quads.begin(); qi != quads.end(); ++qi) {
		list<CUnit*>::iterator ui;
		for (ui = qf->baseQuads[*qi].units.begin(); ui != qf->baseQuads[*qi].units.end(); ++ui) {
			CUnit* unit = *ui;
			if (unit == exclude) {
				continue;
			}

			if((unit->allyteam == gu->myAllyTeam) ||
			   (unit->losStatus[gu->myAllyTeam] & (LOS_INLOS | LOS_CONTRADAR)) ||
			   (useRadar && radarhandler->InRadar(*ui, gu->myAllyTeam)) ||
				 gu->spectatingFullView){
				float3 pos;

				if (gu->spectatingFullView) {
					pos = unit->midPos;
				} else {
					pos = GetUnitErrorPos(*ui,gu->myAllyTeam);
				}

				if (unit->isIcon) {
					const float h = ground->GetHeight(pos.x, pos.z);
					if (pos.y < (h + unit->iconRadius)) {
						pos.y = h + unit->iconRadius;
					}
				}
				const float3 dif = pos - start;
				float closeLength = dif.dot(dir);
				if (closeLength < 0.0f) {
					continue;
				}
				if (closeLength > length) {
					closeLength = length;
				}
				float3 closeVect = dif - dir * closeLength;
				const float rad = (unit->isIcon) ? unit->iconRadius : unit->radius;

				// The argument to sqrt became negative (3.5f*10^-7) for some reason...
				// so tempstoring the value
				const float tmp = rad * rad - closeVect.SqLength();
				if ((tmp > 0.0f) && (length > (closeLength + sqrt(tmp)))){
					//note that we take the length to the backside of the units,
					// this is so you can select stuff inside factories
					length = closeLength + sqrt(tmp);
					hit = unit;
				}
			}
		}
	}
	if (!hit) {
		length -= 200.0f; //fix length from the previous fudge
	}
	return length;
}

float CGameHelper::TraceRayTeam(const float3& start,const float3& dir,float length, CUnit*& hit,bool useRadar,CUnit* exclude,int allyteam)
{
	float groundLength=ground->LineGroundCol(start,start+dir*length);

//	logOutput.Print("gl %f",groundLength);
	if(length>groundLength && groundLength>0)
		length=groundLength;

	vector<int> quads=qf->GetQuadsOnRay(start,dir,length);

//	float minLength=length;
	hit=0;

	vector<int>::iterator qi;
	for(qi=quads.begin();qi!=quads.end();++qi){
		list<CUnit*>::iterator ui;
		for(ui=qf->baseQuads[*qi].units.begin();ui!=qf->baseQuads[*qi].units.end();++ui){
			if((*ui)==exclude)
				continue;
			if(gs->Ally((*ui)->allyteam,allyteam) || ((*ui)->losStatus[allyteam] & LOS_INLOS)){
				float3 dif=(*ui)->midPos-start;
				float closeLength=dif.dot(dir);
				if(closeLength<0)
					continue;
				if(closeLength>length)
					continue;
				float3 closeVect=dif-dir*closeLength;
				if(closeVect.SqLength() < (*ui)->sqRadius){
					length=closeLength;
					hit=*ui;
				}
			} else if(useRadar && radarhandler->InRadar(*ui,allyteam)){
				float3 dif=(*ui)->midPos+(*ui)->posErrorVector*radarhandler->radarErrorSize[allyteam]-start;
				float closeLength=dif.dot(dir);
				if(closeLength<0)
					continue;
				if(closeLength>length)
					continue;
				float3 closeVect=dif-dir*closeLength;
				if(closeVect.SqLength() < (*ui)->sqRadius){
					length=closeLength;
					hit=*ui;
				}
			}
		}
	}
	return length;
}

void CGameHelper::GenerateTargets(CWeapon *weapon, CUnit* lastTarget,std::map<float,CUnit*> &targets)
{
	CUnit* attacker=weapon->owner;
	float radius=weapon->range;
	float3 pos=attacker->pos;
	float heightMod=weapon->heightMod;
	float aHeight=pos.y;
	float secDamage=weapon->damages[0]*weapon->salvoSize/weapon->reloadTime*30;			//how much damage the weapon deal over 1 seconds
	bool paralyzer=!!weapon->weaponDef->damages.paralyzeDamageTime;

	vector<int> quads=qf->GetQuads(pos,radius+(aHeight-max(0.f,readmap->minheight))*heightMod);

	int tempNum=gs->tempNum++;
	vector<int>::iterator qi;
	for(qi=quads.begin();qi!=quads.end();++qi){
		list<CUnit*>::iterator ui;
		for(int t=0;t<gs->activeAllyTeams;++t){
			if(gs->Ally(attacker->allyteam,t))
				continue;
			for(ui=qf->baseQuads[*qi].teamUnits[t].begin();ui!=qf->baseQuads[*qi].teamUnits[t].end();++ui){
				if((*ui)->tempNum!=tempNum && ((*ui)->category&weapon->onlyTargetCategory)){
					(*ui)->tempNum=tempNum;
					if((*ui)->isUnderWater && !weapon->weaponDef->waterweapon)
						continue;
					if((*ui)->isDead)
						continue;
					float3 targPos;
					float value=1;
					if(((*ui)->losStatus[attacker->allyteam] & LOS_INLOS)){
						targPos=(*ui)->midPos;
					} else if(((*ui)->losStatus[attacker->allyteam] & LOS_INRADAR)){
						targPos=(*ui)->midPos+(*ui)->posErrorVector*radarhandler->radarErrorSize[attacker->allyteam];
						value*=10;
					} else {
						continue;
					}
					float modRange=radius+(aHeight-targPos.y)*heightMod;
					if((pos-targPos).SqLength2D() <= modRange*modRange){
						float dist2d=(pos-targPos).Length2D();
						value*=(secDamage+(*ui)->health)*(dist2d+modRange*0.4f+100)*(0.01f+(*ui)->crashing)/(weapon->damages[(*ui)->armorType]*(*ui)->curArmorMultiple*(*ui)->power*(0.7f+gs->randFloat()*0.6f));
						if((*ui)==lastTarget)
							value*=0.4f;
						if((*ui)->category & weapon->badTargetCategory)
							value*=100;
						if(paralyzer && (*ui)->health-(*ui)->paralyzeDamage<(*ui)->maxHealth*0.09f)
							value*=4;
						if((*ui)->crashing)
							value*=10;
						targets.insert(pair<float,CUnit*>(value,*ui));
					}
				}
			}
		}
	}/*
#ifdef TRACE_SYNC
	tracefile << "TargetList: " << attacker->id << " " << radius << " ";
	std::map<float,CUnit*>::iterator ti;
	for(ti=targets.begin();ti!=targets.end();++ti)
		tracefile << (ti->first) <<  " " << (ti->second)->id <<  " ";
	tracefile << "\n";
#endif*/
}

CUnit* CGameHelper::GetClosestUnit(const float3 &pos, float radius)
{
	float closeDist = (radius * radius);
	CUnit* closeUnit = NULL;
	vector<int> quads = qf->GetQuads(pos, radius);

	int tempNum = gs->tempNum++;
	vector<int>::iterator qi;
	for (qi = quads.begin(); qi != quads.end(); ++qi) {
		list<CUnit*>& bqUnits = qf->baseQuads[*qi].units;
		list<CUnit*>::iterator ui;
		for(ui = bqUnits.begin(); ui != bqUnits.end(); ++ui) {
			CUnit* unit = *ui;
			if (unit->tempNum != tempNum) {
				unit->tempNum = tempNum;
				if ((unit->allyteam == gu->myAllyTeam) ||
						(unit->losStatus[gu->myAllyTeam] & (LOS_INLOS | LOS_INRADAR)) ||
						gu->spectatingFullView) {
					float3 unitPos;
					if (gu->spectatingFullView) {
						unitPos = unit->midPos;
					} else {
						unitPos = GetUnitErrorPos(*ui,gu->myAllyTeam);
					}
					float sqDist=(pos - unitPos).SqLength2D();
					if (sqDist <= closeDist) {
						closeDist = sqDist;
						closeUnit = unit;
					}
				}
			}
		}
	}
	return closeUnit;
}

CUnit* CGameHelper::GetClosestEnemyUnit(const float3 &pos, float radius,int searchAllyteam)
{
	float closeDist=radius*radius;
	CUnit* closeUnit=0;
	vector<int> quads=qf->GetQuads(pos,radius);

	int tempNum=gs->tempNum++;
	vector<int>::iterator qi;
	for(qi=quads.begin();qi!=quads.end();++qi){
		list<CUnit*>::iterator ui;
		for(ui=qf->baseQuads[*qi].units.begin();ui!=qf->baseQuads[*qi].units.end();++ui){
			if((*ui)->tempNum!=tempNum && !gs->Ally(searchAllyteam,(*ui)->allyteam) && (((*ui)->losStatus[searchAllyteam] & (LOS_INLOS | LOS_INRADAR)))){
				(*ui)->tempNum=tempNum;
				float sqDist=(pos-(*ui)->midPos).SqLength2D();
				if(sqDist <= closeDist){
					closeDist=sqDist;
					closeUnit=*ui;
				}
			}
		}
	}
	return closeUnit;
}

CUnit* CGameHelper::GetClosestEnemyUnitNoLosTest(const float3 &pos, float radius,int searchAllyteam)
{
	float closeDist=radius*radius;
	CUnit* closeUnit=0;
	vector<int> quads=qf->GetQuads(pos,radius);

	int tempNum=gs->tempNum++;
	vector<int>::iterator qi;
	for(qi=quads.begin();qi!=quads.end();++qi){
		list<CUnit*>::iterator ui;
		for(ui=qf->baseQuads[*qi].units.begin();ui!=qf->baseQuads[*qi].units.end();++ui){
			if((*ui)->tempNum!=tempNum && !gs->Ally(searchAllyteam,(*ui)->allyteam)){
				(*ui)->tempNum=tempNum;
				float sqDist=(pos-(*ui)->midPos).SqLength2D();
				if(sqDist <= closeDist){
					closeDist=sqDist;
					closeUnit=*ui;
				}
			}
		}
	}
	return closeUnit;
}

CUnit* CGameHelper::GetClosestFriendlyUnit(const float3 &pos, float radius,int searchAllyteam)
{
	float closeDist=radius*radius;
	CUnit* closeUnit=0;
	vector<int> quads=qf->GetQuads(pos,radius);

	int tempNum=gs->tempNum++;
	vector<int>::iterator qi;
	for(qi=quads.begin();qi!=quads.end();++qi){
		list<CUnit*>::iterator ui;
		for(ui=qf->baseQuads[*qi].units.begin();ui!=qf->baseQuads[*qi].units.end();++ui){
			if((*ui)->tempNum!=tempNum && gs->Ally(searchAllyteam,(*ui)->allyteam)){
				(*ui)->tempNum=tempNum;
				float sqDist=(pos-(*ui)->midPos).SqLength2D();
				if(sqDist <= closeDist){
					closeDist=sqDist;
					closeUnit=*ui;
				}
			}
		}
	}
	return closeUnit;
}

CUnit* CGameHelper::GetClosestEnemyAircraft(const float3 &pos, float radius,int searchAllyteam)
{
	float closeDist=radius*radius;
	CUnit* closeUnit=0;
	vector<int> quads=qf->GetQuads(pos,radius);

	int tempNum=gs->tempNum++;
	vector<int>::iterator qi;
	for(qi=quads.begin();qi!=quads.end();++qi){
		list<CUnit*>::iterator ui;
		for(ui=qf->baseQuads[*qi].units.begin();ui!=qf->baseQuads[*qi].units.end();++ui){
			if((*ui)->unitDef->canfly && (*ui)->tempNum!=tempNum && !gs->Ally(searchAllyteam,(*ui)->allyteam) && !(*ui)->crashing && (((*ui)->losStatus[searchAllyteam] & (LOS_INLOS | LOS_INRADAR)))){
				(*ui)->tempNum=tempNum;
				float sqDist=(pos-(*ui)->midPos).SqLength2D();
				if(sqDist <= closeDist){
					closeDist=sqDist;
					closeUnit=*ui;
				}
			}
		}
	}
	return closeUnit;
}

void CGameHelper::GetEnemyUnits(const float3 &pos, float radius, int searchAllyteam, vector<int> &found)
{
	float sqRadius=radius*radius;
	vector<int> quads=qf->GetQuads(pos,radius);

	int tempNum=gs->tempNum++;
	vector<int>::iterator qi;
	for(qi=quads.begin();qi!=quads.end();++qi){
		list<CUnit*>::iterator ui;
		for(ui=qf->baseQuads[*qi].units.begin();ui!=qf->baseQuads[*qi].units.end();++ui){
			if((*ui)->tempNum!=tempNum && !gs->Ally(searchAllyteam,(*ui)->allyteam) && (((*ui)->losStatus[searchAllyteam] & (LOS_INLOS | LOS_INRADAR)))){
				(*ui)->tempNum=tempNum;
				if((pos-(*ui)->midPos).SqLength2D() <= sqRadius){
					found.push_back((*ui)->id);
				}
			}
		}
	}
}

bool CGameHelper::TestCone(const float3 &from, const float3 &dir,float length, float spread, int allyteam,CUnit* owner)
{
	int quads[1000];
	int* endQuad = quads;
	qf->GetQuadsOnRay(from,dir,length,endQuad);

	for(int* qi=quads;qi!=endQuad;++qi){
		CQuadField::Quad& quad = qf->baseQuads[*qi];
		for(list<CUnit*>::iterator ui=quad.teamUnits[allyteam].begin();ui!=quad.teamUnits[allyteam].end();++ui){
			if((*ui)==owner)
				continue;
			CUnit* u=*ui;
			float3 dif=u->midPos-from;
			float closeLength=dif.dot(dir);
			if(closeLength<=0)
				continue;//closeLength=0;
			if(closeLength>length)
				closeLength=length;
			float3 closeVect=dif-dir*closeLength;
			float r=u->radius+spread*closeLength+1;
			if(closeVect.SqLength() < r*r){
				return true;
			}
		}
	}
	return false;
}

bool CGameHelper::LineFeatureCol(const float3& start, const float3& dir, float length)
{
	int quads[1000];
	int* endQuad = quads;
	qf->GetQuadsOnRay(start,dir,length,endQuad);

	for(int* qi=quads;qi!=endQuad;++qi){
		CQuadField::Quad& quad = qf->baseQuads[*qi];
		for(list<CFeature*>::iterator ui=quad.features.begin();ui!=quad.features.end();++ui){
			if(!(*ui)->blocking)
				continue;
			float3 dif=(*ui)->midPos-start;
			float closeLength=dif.dot(dir);
			if(closeLength<0)
				continue;
			if(closeLength>length)
				continue;
			float3 closeVect=dif-dir*closeLength;
			if(closeVect.SqLength() < (*ui)->sqRadius){
				return true;
			}
		}
	}
	return false;
}

float CGameHelper::GuiTraceRayFeature(const float3& start, const float3& dir, float length, CFeature*& feature)
{
	float nearHit=length;
	vector<int> quads=qf->GetQuadsOnRay(start,dir,length);

	vector<int>::iterator qi;
	for(qi=quads.begin();qi!=quads.end();++qi){
		list<CFeature*>::iterator ui;
		for(ui=qf->baseQuads[*qi].features.begin();ui!=qf->baseQuads[*qi].features.end();++ui){
			CFeature* f = *ui;
			if ((f->allyteam >= 0) && !gu->spectatingFullView &&
			    (f->allyteam != gu->myAllyTeam) &&
			    !loshandler->InLos(f->pos, gu->myAllyTeam)) {
				continue;
			}
			if (f->noSelect) {
				continue;
			}
			float3 dif = f->midPos-start;
			float closeLength=dif.dot(dir);
			if(closeLength<0)
				continue;
			if(closeLength>nearHit)
				continue;
			float3 closeVect=dif-dir*closeLength;
			if(closeVect.SqLength() < f->sqRadius){
				nearHit = closeLength;
				feature = f;
			}
		}
	}
	return nearHit;
}

float3 CGameHelper::GetUnitErrorPos(const CUnit* unit, int allyteam)
{
	float3 pos=unit->midPos;
	if(gs->Ally(allyteam,unit->allyteam) || (unit->losStatus[allyteam] & LOS_INLOS)){
		// ^ it's one of our own, or it's in LOS, so don't add an error ^
	} else if((!gameSetup || gameSetup->ghostedBuildings) && (unit->losStatus[allyteam] & LOS_PREVLOS) && (unit->losStatus[allyteam] & LOS_CONTRADAR) && !unit->mobility){
		// ^ this is a ghosted building, so don't add an error ^
	} else if((unit->losStatus[allyteam] & LOS_INRADAR)){
		pos+=unit->posErrorVector*radarhandler->radarErrorSize[allyteam];
	} else {
		pos+=unit->posErrorVector*radarhandler->baseRadarErrorSize*2;
	}
	return pos;
}

void CGameHelper::BuggerOff(float3 pos, float radius, CUnit* exclude)
{
	std::vector<CUnit*> units=qf->GetUnitsExact(pos,radius+8);
	for(std::vector<CUnit*>::iterator ui=units.begin();ui!=units.end();++ui){
		if((*ui)!=exclude)
			(*ui)->commandAI->BuggerOff(pos,radius+8);
	}
}

float3 CGameHelper::Pos2BuildPos(const float3& pos, UnitDef* ud)
{
	return Pos2BuildPos(BuildInfo(ud,pos,0));
}

float3 CGameHelper::Pos2BuildPos(const BuildInfo& buildInfo)
{
	float3 pos;
	if(buildInfo.GetXSize()&2)
		pos.x=floor((buildInfo.pos.x)/(SQUARE_SIZE*2))*SQUARE_SIZE*2+8;
	else
		pos.x=floor((buildInfo.pos.x+8)/(SQUARE_SIZE*2))*SQUARE_SIZE*2;

	if(buildInfo.GetYSize()&2)
		pos.z=floor((buildInfo.pos.z)/(SQUARE_SIZE*2))*SQUARE_SIZE*2+8;
	else
		pos.z=floor((buildInfo.pos.z+8)/(SQUARE_SIZE*2))*SQUARE_SIZE*2;

	pos.y=uh->GetBuildHeight(pos,buildInfo.def);
	if(buildInfo.def->floater && pos.y<0)
		pos.y = -buildInfo.def->waterline;

	return pos;
}

void CGameHelper::Update(void)
{
	std::list<WaitingDamage*>* wd=&waitingDamages[gs->frameNum&127];
	while(!wd->empty()){
		WaitingDamage* w=wd->back();
		wd->pop_back();
		if(uh->units[w->target])
			uh->units[w->target]->DoDamage(w->damage,w->attacker==-1?0:uh->units[w->attacker],w->impulse,w->weaponId);
		delete w;
	}
}

bool CGameHelper::TestTrajectoryCone(const float3 &from, const float3 &flatdir,float length, float linear, float quadratic, float spread, float baseSize, int allyteam,CUnit* owner)
{
	int quads[1000];
	int* endQuad = quads;
	qf->GetQuadsOnRay(from,flatdir,length,endQuad);

	for(int* qi=quads;qi!=endQuad;++qi){
		CQuadField::Quad& quad = qf->baseQuads[*qi];
		for(list<CUnit*>::iterator ui=quad.teamUnits[allyteam].begin();ui!=quad.teamUnits[allyteam].end();++ui){
			if((*ui)==owner)
				continue;
			CUnit* u=*ui;
			float3 dif=u->midPos-from;
			float3 flatdif(dif.x,0,dif.z);
			float closeFlatLength=flatdif.dot(flatdir);
			if(closeFlatLength<=0)
				continue;//closeLength=0;
			if(closeFlatLength>length)
				closeFlatLength=length;
/*
			float3 newfrom=from+flatdir*closeFlatLength;
			newfrom.y+=(linear+quadratic*closeFlatLength)*closeFlatLength;
			geometricObjects->AddLine(newfrom-UpVector*(spread*closeFlatLength+baseSize),newfrom+UpVector*(spread*closeFlatLength+baseSize),3,0,16);
/**/
			if(fabs(linear-quadratic*closeFlatLength)<0.15f){		//relativly flat region -> use approximation
				dif.y-=(linear+quadratic*closeFlatLength)*closeFlatLength;

				float3 closeVect=dif-flatdir*closeFlatLength;
				float r=u->radius+spread*closeFlatLength+baseSize;
				if(closeVect.SqLength() < r*r){
					return true;
				}
			} else {
				float3 newfrom=from+flatdir*closeFlatLength;
				newfrom.y+=(linear+quadratic*closeFlatLength)*closeFlatLength;
				float3 dir=flatdir;
				dir.y=linear+quadratic*closeFlatLength;
				dir.Normalize();

				dif=u->midPos-newfrom;
				float closeLength=dif.dot(dir);

				float3 closeVect=dif-dir*closeLength;
				float r=u->radius+spread*closeFlatLength+baseSize;
				if(closeVect.SqLength() < r*r){
					return true;
				}
			}
		}
	}
	return false;
}
