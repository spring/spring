// GameHelper.cpp: implementation of the CGameHelperHelper class.
//
//////////////////////////////////////////////////////////////////////
#pragma warning(disable:4786)

#include "StdAfx.h"
#include "GameHelper.h"
#include "Game.h"
#include "Ground.h"
#include "TracerProjectile.h"
#include "ReadMap.h"
#include "Unit.h"
#include "InfoConsole.h"
#include "QuadField.h"
#include "SyncTracer.h"
#include "LosHandler.h"
#include "Sound.h"
#include "MapDamage.h"
#include "Camera.h"
#include "UnitHandler.h"
#include "DamageArray.h"
#include "Weapon.h"
#include "Feature.h"
#include "RadarHandler.h"
#include "WeaponDefHandler.h"
#include "ExplosionGraphics.h"
#include "CommandAI.h"
#include "UnitDef.h"
#include "GroundDecalHandler.h"
//#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGameHelper* helper;


CGameHelper::CGameHelper(CGame* game)
{
	this->game=game;
/*	explosionSounds[0]=sound->GetWaveId("XPLOMED1.WAV");
	explosionSounds[1]=sound->GetWaveId("XPLOMED2.WAV");
	explosionSounds[2]=sound->GetWaveId("XPLOMED3.WAV");
	explosionSounds[3]=sound->GetWaveId("XPLOMED4.WAV");
*/
	explosionGraphics.push_back(new CStdExplosionGraphics());
}

CGameHelper::~CGameHelper()
{
	while(!explosionGraphics.empty()){
		delete explosionGraphics.back();
		explosionGraphics.pop_back();
	}
}

void CGameHelper::Explosion(float3 pos, const DamageArray& damages, float radius, CUnit *owner,bool damageGround,float gfxMod,bool ignoreOwner,int graphicType)
{
#ifdef TRACE_SYNC
	tracefile << "Explosion: ";
	tracefile << pos.x << " " << damages[0] <<  " " << radius << "\n";
#endif
/*	if(pos.x<0 || pos.z<0 || pos.x>gs->mapx*SQUARE_SIZE || pos.z>gs->mapy*SQUARE_SIZE){
		info->AddLine("Explosion outside map %.0f %.0f",pos.x,pos.z);
		return;
	}
*/
//	info->AddLine("Explosion %i",damageGround);
	if(radius<1)
		radius=1;

	float h2=ground->GetHeight2(pos.x,pos.z);
	if(pos.y<h2)
		pos.y=h2;

	float height=pos.y-h2;
	if(height<0)
		height=0;

	vector<CUnit*> units=qf->GetUnitsExact(pos,radius);
	float gd=max(20.f,damages[0]/20);
	float explosionSpeed=(8+gd*2.5)/(9+sqrt(gd)*0.7)*0.5;	//this is taken from the explosion graphics and could probably be simplified a lot

	for(vector<CUnit*>::iterator ui=units.begin();ui!=units.end();++ui){
		if(ignoreOwner && (*ui)==owner)
				continue;
			float3 dif=(*ui)->midPos-pos;
			float dist=dif.Length();
			if(dist<(*ui)->radius+0.1)
				dist=(*ui)->radius+0.1;
			float dist2=dist - (*ui)->radius;
			float mod=(radius-dist)/radius;
			float mod2=(radius-dist2)/radius;
			if(mod<0)
				mod=0;
			dif/=dist+0.0001;
			dif.y+=0.1;
		if(dist2<explosionSpeed*2){	//damage directly
			(*ui)->DoDamage(damages*mod2,owner,dif*(mod*damages[0]*3));
		}else {	//damage later
			WaitingDamage wd;
			wd.attacker=owner?owner->id:-1;
			wd.target=(*ui)->id;
			wd.impulse=dif*(mod*damages[0]*3);
			wd.damage=damages*mod2;
			waitingDamages[(gs->frameNum+int(dist2/explosionSpeed))&63].push_back(wd);
		}
	}

	vector<CFeature*> features=qf->GetFeaturesExact(pos,radius);
	vector<CFeature*>::iterator fi;
	for(fi=features.begin();fi!=features.end();++fi){
		float3 dif=(*fi)->midPos-pos;
		float dist=dif.Length();
		if(dist<0.1)
			dist=0.1;
		float mod=(radius-dist)/radius;
		if(radius>8 && dist<(*fi)->radius*1.1 && mod<0.1)		//always do some damage with explosive stuff (ddm wreckage etc is to big to normally be damaged otherwise, even by bb shells)
			mod=0.1;
		if(mod>0)
			(*fi)->DoDamage(damages*mod,owner,dif*(mod/dist*damages[0]));
		if(gs->randFloat()>0.7)
			(*fi)->StartFire();
	}

	if(damageGround && radius>height)
	{
		float damage = damages[0]*(1-height/radius);
		if(damage>radius*10)
			damage=radius*10;  //limit the depth somewhat
		mapDamage->Explosion(pos,damage,radius-height);
	}

	explosionGraphics[graphicType]->Explosion(pos,damages,radius,owner,gfxMod);
	groundDecals->AddExplosion(pos,damages[0],radius);
	//sound->PlaySound(explosionSounds[rand()*4/(RAND_MAX+1)],pos,damage*2);
}

float CGameHelper::TraceRay(const float3 &start, const float3 &dir, float length, float power, CUnit* owner, CUnit *&hit)
{
	float groundLength=ground->LineGroundCol(start,start+dir*length);
	
//	info->AddLine("gl %f",groundLength);
	if(length>groundLength && groundLength>0)
		length=groundLength;
	
	vector<int> quads=qf->GetQuadsOnRay(start,dir,length);

	vector<int>::iterator qi;
	for(qi=quads.begin();qi!=quads.end();++qi){
		list<CFeature*>::iterator ui;
		for(ui=qf->baseQuads[*qi].features.begin();ui!=qf->baseQuads[*qi].features.end();++ui){
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

	for(qi=quads.begin();qi!=quads.end();++qi){
		list<CUnit*>::iterator ui;
		for(ui=qf->baseQuads[*qi].units.begin();ui!=qf->baseQuads[*qi].units.end();++ui){
			if((*ui)==owner)
				continue;
			float3 dif=(*ui)->midPos-start;
			float closeLength=dif.dot(dir);
			if(closeLength<0)
				continue;
			if(closeLength>length)
				closeLength=length;
			float3 closeVect=dif-dir*closeLength;

			float rad=(*ui)->radius;
			float tmp = rad * rad - closeVect.SqLength();
			if(tmp > 0 && length>closeLength+sqrt(tmp)){
				length=closeLength-sqrt(tmp)*0.5;
				hit=*ui;
			}
/*			if(closeVect.SqLength() < (*ui)->sqRadius){
				length=closeLength;
				hit=*ui;
			}*/
		}
	}
	return length;
}

float CGameHelper::GuiTraceRay(const float3 &start, const float3 &dir, float length, CUnit *&hit,float sizeMod,bool useRadar,CUnit* exclude)
{
	float groundLength=ground->LineGroundCol(start,start+dir*length);
	
//	info->AddLine("gl %f",groundLength);
	if(length>groundLength+200 && groundLength>0)
		length=groundLength+200;	//need to add some cause we take the backside of the unit sphere;
	
	vector<int> quads=qf->GetQuadsOnRay(start,dir,length);

//	float minLength=length;
	hit=0;

	vector<int>::iterator qi;
	for(qi=quads.begin();qi!=quads.end();++qi){
		list<CUnit*>::iterator ui;
		for(ui=qf->baseQuads[*qi].units.begin();ui!=qf->baseQuads[*qi].units.end();++ui){
			if((*ui)==exclude)
				continue;
			if((*ui)->allyteam==gu->myAllyTeam || ((*ui)->losStatus[gu->myAllyTeam] & (LOS_INLOS | LOS_CONTRADAR)) || gu->spectating){
				float3 dif=(*ui)->midPos-start;
				float closeLength=dif.dot(dir);
				if(closeLength<0)
					continue;
				if(closeLength>length)
					closeLength=length;
				float3 closeVect=dif-dir*closeLength;
				float rad=(*ui)->radius;
				
				//The argument to sqrt became negative (3.5*10^-7) for some reason... so tempstoring the value
				float tmp = rad * rad - closeVect.SqLength();
				if(tmp > 0 && length>closeLength+sqrt(tmp)){
					length=closeLength+sqrt(tmp);		//note that we take the length to the backside of the units, this is so you can select stuff inside factories
					hit=*ui;
				}
			} else if(useRadar && radarhandler->InRadar(*ui,gu->myAllyTeam)){
				float3 dif=(*ui)->midPos+(*ui)->posErrorVector*radarhandler->radarErrorSize[gu->myAllyTeam]-start;
				float closeLength=dif.dot(dir);
				if(closeLength<0)
					continue;
				if(closeLength>length)
					closeLength=length;
				float3 closeVect=dif-dir*closeLength;
				float rad=20;
				
				//The argument to sqrt became negative (3.5*10^-7) for some reason... so tempstoring the value
				float tmp = rad * rad - closeVect.SqLength();
				if(tmp > 0 && length>closeLength+sqrt(tmp)){
					length=closeLength+sqrt(tmp);
					hit=*ui;
				}
			}
		}
	}
	if(!hit)
		length-=200;		//fix length from the previous fudge
	return length;
}

float CGameHelper::TraceRayTeam(const float3& start,const float3& dir,float length, CUnit*& hit,bool useRadar,CUnit* exclude,int allyteam)
{
	float groundLength=ground->LineGroundCol(start,start+dir*length);
	
//	info->AddLine("gl %f",groundLength);
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
			if(gs->allies[(*ui)->allyteam][allyteam] || ((*ui)->losStatus[allyteam] & LOS_INLOS)){
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
	bool paralyzer=weapon->weaponDef->damages.paralyzeDamage;

	vector<int> quads=qf->GetQuads(pos,radius+aHeight*heightMod);

	int tempNum=gs->tempNum++;
	vector<int>::iterator qi;
	for(qi=quads.begin();qi!=quads.end();++qi){
		list<CUnit*>::iterator ui;
		for(int t=0;t<gs->activeAllyTeams;++t){
			if(gs->allies[attacker->allyteam][t])
				continue;
			for(ui=qf->baseQuads[*qi].teamUnits[t].begin();ui!=qf->baseQuads[*qi].teamUnits[t].end();++ui){
				if((*ui)->tempNum!=tempNum && ((*ui)->category&weapon->onlyTargetCategory)){
					(*ui)->tempNum=tempNum;
					if((*ui)->isUnderWater && !weapon->weaponDef->waterweapon)
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
						value*=(secDamage+(*ui)->health)*(dist2d+modRange*0.4+100)*(0.01+(*ui)->crashing)/(weapon->damages[(*ui)->armorType]*(*ui)->curArmorMultiple*(*ui)->power*(0.7+gs->randFloat()*0.6));
						if((*ui)==lastTarget)
							value*=0.4;
						if((*ui)->category & weapon->badTargetCategory)
							value*=100;
						if(paralyzer && (*ui)->health-(*ui)->paralyzeDamage<(*ui)->maxHealth*0.09)
							value*=4;
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
	float closeDist=radius;
	CUnit* closeUnit=0;
	vector<CUnit*> units=qf->GetUnits(pos,radius);
	vector<CUnit*>::iterator ui;
	for(ui=units.begin();ui!=units.end();++ui){
		float dist=(*ui)->midPos.distance(pos);
		if(dist<closeDist){
			closeDist=dist;
			closeUnit=*ui;
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
			if((*ui)->tempNum!=tempNum && !gs->allies[searchAllyteam][(*ui)->allyteam] && (((*ui)->losStatus[searchAllyteam] & (LOS_INLOS | LOS_INRADAR)))){
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
			if((*ui)->tempNum!=tempNum && !gs->allies[searchAllyteam][(*ui)->allyteam]){
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
			if((*ui)->tempNum!=tempNum && gs->allies[searchAllyteam][(*ui)->allyteam]){
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
			if((*ui)->unitDef->canfly && (*ui)->tempNum!=tempNum && !gs->allies[searchAllyteam][(*ui)->allyteam] && !(*ui)->crashing && (((*ui)->losStatus[searchAllyteam] & (LOS_INLOS | LOS_INRADAR)))){
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

void CGameHelper::GetEnemyUnits(float3 &pos, float radius, int searchAllyteam, vector<int> &found)
{
	float sqRadius=radius*radius;
	vector<int> quads=qf->GetQuads(pos,radius);

	int tempNum=gs->tempNum++;
	vector<int>::iterator qi;
	for(qi=quads.begin();qi!=quads.end();++qi){
		list<CUnit*>::iterator ui;
		for(ui=qf->baseQuads[*qi].units.begin();ui!=qf->baseQuads[*qi].units.end();++ui){
			if((*ui)->tempNum!=tempNum && !gs->allies[searchAllyteam][(*ui)->allyteam] && (((*ui)->losStatus[searchAllyteam] & (LOS_INLOS | LOS_INRADAR)))){
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
	vector<int> quads=qf->GetQuadsOnRay(from,dir,length);

	vector<int>::iterator qi;
	for(qi=quads.begin();qi!=quads.end();++qi){
		list<CUnit*>::iterator ui;
		for(ui=qf->baseQuads[*qi].teamUnits[allyteam].begin();ui!=qf->baseQuads[*qi].teamUnits[allyteam].end();++ui){
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
	vector<int> quads=qf->GetQuadsOnRay(start,dir,length);

	vector<int>::iterator qi;
	for(qi=quads.begin();qi!=quads.end();++qi){
		list<CFeature*>::iterator ui;
		for(ui=qf->baseQuads[*qi].features.begin();ui!=qf->baseQuads[*qi].features.end();++ui){
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

float CGameHelper::GuiTraceRayFeature(const float3& start, const float3& dir, float length,CFeature*& feature)
{
	float nearHit=length;
	vector<int> quads=qf->GetQuadsOnRay(start,dir,length);

	vector<int>::iterator qi;
	for(qi=quads.begin();qi!=quads.end();++qi){
		list<CFeature*>::iterator ui;
		for(ui=qf->baseQuads[*qi].features.begin();ui!=qf->baseQuads[*qi].features.end();++ui){
			float3 dif=(*ui)->midPos-start;
			float closeLength=dif.dot(dir);
			if(closeLength<0)
				continue;
			if(closeLength>nearHit)
				continue;
			float3 closeVect=dif-dir*closeLength;
			if(closeVect.SqLength() < (*ui)->sqRadius){
				nearHit=closeLength;
				feature=*ui;
			}
		}
	}
	return nearHit;
}

float3 CGameHelper::GetUnitErrorPos(CUnit* unit, int allyteam)
{
	float3 pos=unit->midPos;
	if(gs->allies[allyteam][unit->allyteam] || (unit->losStatus[allyteam] & LOS_INLOS)){
	} else if((unit->losStatus[allyteam] & LOS_INRADAR)){
		pos+=unit->posErrorVector*radarhandler->radarErrorSize[allyteam];
	} else {
		pos+=unit->posErrorVector*radarhandler->baseRadarErrorSize*2;
	}	
	return pos;
}

void CGameHelper::BuggerOff(float3 pos, float radius,CUnit* exclude)
{
	std::vector<CUnit*> units=qf->GetUnitsExact(pos,radius+8);
	for(std::vector<CUnit*>::iterator ui=units.begin();ui!=units.end();++ui){
		if((*ui)!=exclude)
			(*ui)->commandAI->BuggerOff(pos,radius+8);
	}
}

float3 CGameHelper::Pos2BuildPos(float3 pos, const UnitDef* ud)
{
	if(ud->xsize&2)
		pos.x=floor((pos.x)/(SQUARE_SIZE*2))*SQUARE_SIZE*2+8;
	else
		pos.x=floor((pos.x+8)/(SQUARE_SIZE*2))*SQUARE_SIZE*2;

	if(ud->ysize&2)
		pos.z=floor((pos.z)/(SQUARE_SIZE*2))*SQUARE_SIZE*2+8;
	else
		pos.z=floor((pos.z+8)/(SQUARE_SIZE*2))*SQUARE_SIZE*2;

	pos.y=uh->GetBuildHeight(pos,ud);
	if(ud->floater && pos.y<0)
		pos.y = -ud->waterline;

	return pos;
}

void CGameHelper::Update(void)
{
	std::list<WaitingDamage>* wd=&waitingDamages[gs->frameNum&63];
	while(!wd->empty()){
		WaitingDamage* w=&wd->back();
		if(uh->units[w->target])
			uh->units[w->target]->DoDamage(w->damage,w->attacker==-1?0:uh->units[w->attacker],w->impulse);
		wd->pop_back();
	}
}
