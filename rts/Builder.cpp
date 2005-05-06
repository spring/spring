#include "StdAfx.h"
// Builder.cpp: implementation of the CBuilder class.
//
//////////////////////////////////////////////////////////////////////

#include "Builder.h"
#include "Building.h"
#include "UnitLoader.h"
#include "GfxProjectile.h"
#include "GameHelper.h"
#include "UnitHandler.h"
#include "3DOParser.h"
#include "CobInstance.h"
#include "Builder.h"
#include "myMath.h"
#include "InfoConsole.h"
#include "Ground.h"
#include "Sound.h"
#include "MapDamage.h"
#include "UnitDefHandler.h"
#include "Feature.h"
#include "FeatureHandler.h"
//#include "mmgr.h"
#include <algorithm>
using namespace std;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBuilder::CBuilder(const float3 &pos,int team,UnitDef* unitDef)
: CUnit(pos,team,unitDef),
	buildSpeed(100),
	buildDistance(16),
	curBuild(0),
	curReclaim(0),
	terraforming(false),
	terraformLeft(0),
	terraformType(Terraform_Building),
	tx1(0),
	tx2(0),
	tz1(0),
	tz2(0),
	terraformCenter(0,0,0),
	terraformRadius(0),
	nextBuildPos(0,0,0),
	nextBuildType(""),
	terraformHelp(0),
	helpTerraform(0)
{
	buildSpeed=unitDef->buildSpeed/32.0f;
	buildDistance=unitDef->buildDistance;
}

CBuilder::~CBuilder()
{
}

void CBuilder::Update()
{
	if(beingBuilt)
		return;

	if(terraforming && inBuildStance){
		if(terraformLeft>0){
			float terraformSpeed=(buildSpeed+terraformHelp)/terraformLeft;
			terraformLeft-=(buildSpeed+terraformHelp);
			terraformHelp=0;
			if(terraformSpeed>1)
				terraformSpeed=1;
			switch(terraformType){
			case Terraform_Building:
				if(curBuild)
					curBuild->AddBuildPower(0.00000001,this);			//prevent building from timing out while terraforming for it
				for(int z=tz1; z<=tz2; z++){
					for(int x=tx1; x<=tx2; x++){
						float ch=readmap->heightmap[z*(gs->mapx+1)+x];
						readmap->heightmap[z*(gs->mapx+1)+x]+=(nextBuildPos.y-ch)*terraformSpeed;
					}
				}
				break;
			case Terraform_Restore:
				for(int z=tz1; z<=tz2; z++){
					for(int x=tx1; x<=tx2; x++){
						float ch=readmap->heightmap[z*(gs->mapx+1)+x];
						float oh=readmap->orgheightmap[z*(gs->mapx+1)+x];
						readmap->heightmap[z*(gs->mapx+1)+x]+=(oh-ch)*terraformSpeed;
					}
				}
				break;
			}
			for(int z=tz1; z<=tz2; z++){		//smooth the borders x
				for(int x=1; x<=3; x++){
					if(tx1-3>=0){
						float ch3=readmap->heightmap[z*(gs->mapx+1)+tx1];
						float ch=readmap->heightmap[z*(gs->mapx+1)+tx1-x];
						float ch2=readmap->heightmap[z*(gs->mapx+1)+tx1-3];
						readmap->heightmap[z*(gs->mapx+1)+tx1-x]+=((ch3*(3-x)+ch2*x)/3-ch)*terraformSpeed;
					}
					if(tx2+3<gs->mapx){
						float ch3=readmap->heightmap[z*(gs->mapx+1)+tx2];
						float ch=readmap->heightmap[z*(gs->mapx+1)+tx2+x];
						float ch2=readmap->heightmap[z*(gs->mapx+1)+tx2+3];
						readmap->heightmap[z*(gs->mapx+1)+tx2+x]+=((ch3*(3-x)+ch2*x)/3-ch)*terraformSpeed;
					}
				}
			}
			for(int z=1; z<=3; z++){		//smooth the borders z
				for(int x=tx1; x<=tx2; x++){
					if(tz1-3>=0){
						float ch3=readmap->heightmap[(tz1)*(gs->mapx+1)+x];
						float ch=readmap->heightmap[(tz1-z)*(gs->mapx+1)+x];
						float ch2=readmap->heightmap[(tz1-3)*(gs->mapx+1)+x];
						readmap->heightmap[(tz1-z)*(gs->mapx+1)+x]+=((ch3*(3-z)+ch2*z)/3-ch)*terraformSpeed;
					}
					if(tz2+3<gs->mapy){
						float ch3=readmap->heightmap[(tz2)*(gs->mapx+1)+x];
						float ch=readmap->heightmap[(tz2+z)*(gs->mapx+1)+x];
						float ch2=readmap->heightmap[(tz2+3)*(gs->mapx+1)+x];
						readmap->heightmap[(tz2+z)*(gs->mapx+1)+x]+=((ch3*(3-z)+ch2*z)/3-ch)*terraformSpeed;
					}
				}
			}
			CreateNanoParticle(terraformCenter,terraformRadius*0.5,false);
		}
		if(terraformLeft<=0){
			terraforming=false;
			mapDamage->RecalcArea(tx1,tx2,tz1,tz2);
			if(terraformType==Terraform_Building){
//				if(curBuild)
//					SetBuildStanceToward(curBuild->midPos);
			} else {
				StopBuild();
			}
		}
	} else if(helpTerraform && inBuildStance){
		if(helpTerraform->terraforming){
			helpTerraform->terraformHelp+=buildSpeed;
			CreateNanoParticle(helpTerraform->terraformCenter,helpTerraform->terraformRadius*0.5,false);
		} else {
			DeleteDeathDependence(helpTerraform);
			helpTerraform=0;
			StopBuild(true);
		}
	} else if(curBuild && curBuild->pos.distance2D(pos)<buildDistance+curBuild->radius){
		if(inBuildStance){
			if(curBuild->AddBuildPower(buildSpeed,this)){
				CreateNanoParticle(curBuild->midPos,curBuild->radius*0.5,false);
			} else {
				if(!curBuild->beingBuilt && curBuild->health>=curBuild->maxHealth){
					StopBuild();
				}
			}
		} else {
			curBuild->AddBuildPower(0.00001,this);	//prevent building timing out
		}
	} else if(curReclaim && curReclaim->pos.distance2D(pos)<buildDistance+curReclaim->radius && inBuildStance){
		if(curReclaim->AddBuildPower(-buildSpeed,this)){
			CreateNanoParticle(curReclaim->midPos,curReclaim->radius*0.7,true);
		}
	}
	CUnit::Update();
}

void CBuilder::SlowUpdate(void)
{
	if(terraforming){
		mapDamage->RecalcArea(tx1,tx2,tz1,tz2);		
	}
	CUnit::SlowUpdate();
}

void CBuilder::SetRepairTarget(CUnit* target)
{
	if(target==curBuild)
		return;

	StopBuild(false);

	curBuild=target;
	AddDeathDependence(curBuild);

	SetBuildStanceToward(target->pos);
}


void CBuilder::SetReclaimTarget(CSolidObject* target)
{
	if(dynamic_cast<CFeature*>(target) && !((CFeature*)target)->def->destructable)
		return;

	if(curReclaim==target)
		return;

	StopBuild(false);

	curReclaim=target;
	AddDeathDependence(target);

	SetBuildStanceToward(target->pos);
}


void CBuilder::StartRestore(float3 centerPos, float radius)
{
	StopBuild(false);

	terraforming=true;
	terraformType=Terraform_Restore;
	terraformCenter=centerPos;
	terraformRadius=radius;

	tx1 = max((float)0,(centerPos.x-radius)/SQUARE_SIZE);
	tx2 = min((float)gs->mapx,(centerPos.x+radius)/SQUARE_SIZE);
	tz1 = max((float)0,(centerPos.z-radius)/SQUARE_SIZE);
	tz2 = min((float)gs->mapy,(centerPos.z+radius)/SQUARE_SIZE);

	float tcost=0;
	for(int z=tz1; z<=tz2; z++){
		for(int x=tx1; x<=tx2; x++){
			float delta=readmap->orgheightmap[z*(gs->mapx+1)+x]-readmap->heightmap[z*(gs->mapx+1)+x];
			tcost+=fabs(delta);			
		}
	}
	terraformLeft=tcost;

	SetBuildStanceToward(centerPos);
}

void CBuilder::StopBuild(bool callScript)
{
	if(curBuild)
		DeleteDeathDependence(curBuild);
	if(curReclaim)
		DeleteDeathDependence(curReclaim);
	if(helpTerraform)
		DeleteDeathDependence(helpTerraform);
	curBuild=0;
	curReclaim=0;
	helpTerraform=0;
	terraforming=false;
	if(callScript)
		cob->Call("StopBuilding");
//	info->AddLine("stop build");
}

bool CBuilder::StartBuild(const string &type, float3 &buildPos)
{
	StopBuild(false);
//	info->AddLine("start build");

	UnitDef *unitdef = unitDefHandler->GetUnitByName(type);
	buildPos.x=floor(buildPos.x/SQUARE_SIZE+0.5)*SQUARE_SIZE;
	buildPos.z=floor(buildPos.z/SQUARE_SIZE+0.5)*SQUARE_SIZE;
	buildPos.y=uh->GetBuildHeight(buildPos,unitdef);

	if(unitdef->floater && buildPos.y<0)
		buildPos.y = -unitdef->waterline;

	CFeature* feature;
	int canBuild=uh->TestUnitBuildSquare(buildPos,unitdef,feature);
	if(canBuild<2){
		CUnit* u=helper->GetClosestFriendlyUnit(buildPos,5,allyteam);
		if(u && u->unitDef==unitdef){
			curBuild=u;
			AddDeathDependence(u);
			SetBuildStanceToward(buildPos);
			return true;
		}
		return false;
	}
	if(feature)
		return false;

	tx1 = max((float)0,(buildPos.x-(unitdef->xsize*0.5f*SQUARE_SIZE))/SQUARE_SIZE);
	tx2 = min(gs->mapx,tx1+unitdef->xsize);
	tz1 = max((float)0,(buildPos.z-(unitdef->ysize*0.5f*SQUARE_SIZE))/SQUARE_SIZE);
	tz2 = min(gs->mapy,tz1+unitdef->ysize);

	float tcost=0;
	for(int z=tz1; z<=tz2; z++){
		for(int x=tx1; x<=tx2; x++){
			float delta=buildPos.y-readmap->heightmap[z*(gs->mapx+1)+x];
			float cost;
			if(delta>0){
				cost=max(3.,readmap->heightmap[z*(gs->mapx+1)+x]-readmap->orgheightmap[z*(gs->mapx+1)+x]+delta*0.5);
			} else {
				cost=max(3.,readmap->orgheightmap[z*(gs->mapx+1)+x]-readmap->heightmap[z*(gs->mapx+1)+x]-delta*0.5);
			}
			tcost+=fabs(delta)*cost;			
		}
	}
	terraformLeft=tcost;

	if(unitdef->floater)
		terraformLeft=0;

	terraforming=true;
	terraformType=Terraform_Building;
	terraformCenter=buildPos;
	terraformRadius=(tx2-tx1)*SQUARE_SIZE;

	SetBuildStanceToward(buildPos);

	nextBuildType=type;
	nextBuildPos=buildPos;

	CUnit* b=unitLoader.LoadUnit(nextBuildType,nextBuildPos,team,true);
	AddDeathDependence(b);
	curBuild=b;
	float d=nextBuildPos.y-curBuild->pos.y;
	curBuild->pos.y+=d;
	curBuild->midPos.y+=d;

	return true;
}

void CBuilder::DependentDied(CObject *o)
{
	if(o==curBuild){
		curBuild=0;
		StopBuild();
	}
	if(o==curReclaim){
		curReclaim=0;
		StopBuild();
	}
	if(o==helpTerraform){
		helpTerraform=0;
		StopBuild();
	}
	CUnit::DependentDied(o);
}


void CBuilder::SetBuildStanceToward(float3 pos)
{
	float3 wantedDir=(pos-this->pos).Normalize();
	short int h=GetHeadingFromVector(wantedDir.x,wantedDir.z);

	std::vector<long> args;
	args.push_back(short(h-heading));
	cob->Call("StartBuilding",args);

#ifndef NO_SOUND
	if(unitDef->sounds.build.id)
		sound->PlaySound(unitDef->sounds.build.id, pos, unitDef->sounds.build.volume);
#endif	
}

void CBuilder::HelpTerraform(CBuilder* unit)
{
	if(helpTerraform==unit)
		return;

	StopBuild(false);

	helpTerraform=unit;
	SetBuildStanceToward(unit->terraformCenter);
}

void CBuilder::CreateNanoParticle(float3 goal, float radius, bool inverse)
{
	std::vector<long> args;
	args.push_back(0);
	cob->Call("QueryNanoPiece",args);
	float3 relWeaponFirePos=localmodel->GetPiecePos(args[0]);
	float3 weaponPos=pos + frontdir*relWeaponFirePos.z + updir*relWeaponFirePos.y + rightdir*relWeaponFirePos.x;

	float3 dif=goal-weaponPos;
	float l=dif.Length();
	dif/=l;
	float3 error=gs->randVector()*(radius/l);

	if(inverse)
		new CGfxProjectile(weaponPos+(dif+error)*l,-(dif+error)*3,l/3,float3(0.2f,0.7f,0.2f));
	else
		new CGfxProjectile(weaponPos,(dif+error)*3,l/3,float3(0.2f,0.7f,0.2f));
}
