#include "StdAfx.h"
#include "BeamLaser.h"
#include "Sim/Units/Unit.h"
#include "Sound.h"
#include "Game/GameHelper.h"
#include "Sim/Projectiles/LaserProjectile.h"
#include "Map/Ground.h"
#include "WeaponDefHandler.h"
#include "Sim/MoveTypes/AirMoveType.h"
#include "LogOutput.h"
#include "Sim/Projectiles/BeamLaserProjectile.h"
#include "Sim/Weapons/PlasmaRepulser.h"
#include "Sim/Misc/InterceptHandler.h"
#include "mmgr.h"
#include "Sim/Projectiles/LargeBeamLaserProjectile.h"
#include "Sim/Units/COB/CobInstance.h"
#include "Sim/Units/COB/CobFile.h"
#include "Matrix44f.h"
#include "Rendering/UnitModels/3DOParser.h"
#include "Game/Team.h"

CR_BIND_DERIVED(CBeamLaser, CWeapon, (NULL));

CR_REG_METADATA(CBeamLaser,(
	CR_MEMBER(color),
	CR_MEMBER(oldDir),
	CR_MEMBER(damageMul)
	));

CBeamLaser::CBeamLaser(CUnit* owner)
: CWeapon(owner),
	oldDir(ZeroVector),
	lastFireFrame(0)
{
}

CBeamLaser::~CBeamLaser(void)
{
}

void CBeamLaser::Update(void)
{
	if(targetType!=Target_None){
		weaponPos=owner->pos+owner->frontdir*relWeaponPos.z+owner->updir*relWeaponPos.y+owner->rightdir*relWeaponPos.x;
		if(!onlyForward){
			wantedDir=targetPos-weaponPos;
			wantedDir.Normalize();
		}
		predict=salvoSize/2;
	}
	CWeapon::Update();

	if(lastFireFrame > gs->frameNum - 18 && lastFireFrame != gs->frameNum  && weaponDef->sweepFire)
	{
		if (gs->Team(owner->team)->metal>=metalFireCost && gs->Team(owner->team)->energy>=energyFireCost)
		{
			owner->UseEnergy(energyFireCost / salvoSize);
			owner->UseMetal(metalFireCost / salvoSize);

			std::vector<int> args;
			args.push_back(0);
			owner->cob->Call(COBFN_QueryPrimary+weaponNum,args);
			CMatrix44f weaponMat = owner->localmodel->GetPieceMatrix(args[0]);

			float3 relWeaponPos = weaponMat.GetPos();
			weaponPos=owner->pos+owner->frontdir*-relWeaponPos.z+owner->updir*relWeaponPos.y+owner->rightdir*-relWeaponPos.x;

			float3 dir = owner->frontdir * weaponMat[10] + owner->updir * weaponMat[6] + -owner->rightdir * weaponMat[2];
			FireInternal(dir, true);
		}
	}
}

bool CBeamLaser::TryTarget(const float3& pos,bool userTarget,CUnit* unit)
{
	if(!CWeapon::TryTarget(pos,userTarget,unit))
		return false;

	if(unit){
		if(unit->isUnderWater)
			return false;
	} else {
		if(pos.y<0)
			return false;
	}

	float3 dir=pos-weaponPos;
	float length=dir.Length();
	if(length==0)
		return true;

	dir/=length;

	if(!onlyForward){		//skip ground col testing for aircrafts
		float g=ground->LineGroundCol(weaponPos,pos);
		if(g>0 && g<length*0.9f)
			return false;
	}
	if(avoidFeature && helper->LineFeatureCol(weaponPos,dir,length))
		return false;

	if(avoidFriendly && helper->TestCone(weaponPos,dir,length,(accuracy+sprayangle)*(1-owner->limExperience*0.7f),owner->allyteam,owner))
		return false;
	return true;
}

void CBeamLaser::Init(void)
{
	salvoDelay=0;
	salvoSize=(int)(weaponDef->beamtime*30);
	if (salvoSize <= 0) salvoSize = 1;
	damageMul = 1.0f/(float)salvoSize;		//multiply damage with this on each shot so the total damage done is correct

	CWeapon::Init();

	muzzleFlareSize = 0;
}

void CBeamLaser::Fire(void)
{
	float3 dir;
	if(onlyForward && dynamic_cast<CAirMoveType*>(owner->moveType)){		//the taairmovetype can't align itself properly, change back when that is fixed
		dir=owner->frontdir;
	} else {
		if(salvoLeft==salvoSize-1){
			if(fireSoundId)
				sound->PlaySample(fireSoundId,owner,fireSoundVolume);
			dir=targetPos-weaponPos;
			dir.Normalize();
			oldDir=dir;
		} else {
			dir=oldDir;
		}
	}
	dir+=(salvoError)*(1-owner->limExperience*0.7f);
	dir.Normalize();

	FireInternal(dir, false);
}

void CBeamLaser::FireInternal(float3 dir, bool sweepFire)
{
	float rangeMod=1.3f;
#ifdef DIRECT_CONTROL_ALLOWED
	if(owner->directControl)
		rangeMod=0.95f;
#endif
	float maxLength=range*rangeMod;
	float curLength=0;
	float3 curPos=weaponPos;
	float3 hitPos;

	bool tryAgain=true;
	CUnit* hit;
	for(int tries=0;tries<5 && tryAgain;++tries){
		tryAgain=false;
		hit=0;
		float length=helper->TraceRay(curPos,dir,maxLength-curLength,damages[0],owner,hit);

		if(hit && hit->allyteam == owner->allyteam && sweepFire){	//never damage friendlies with sweepfire
			lastFireFrame = 0;
			return;
		}

		float3 newDir;
		CPlasmaRepulser* shieldHit;
		float shieldLength=interceptHandler.AddShieldInterceptableBeam(this,curPos,dir,length,newDir,shieldHit);
		if(shieldLength<length){
			length=shieldLength;
			bool repulsed=shieldHit->BeamIntercepted(this);
			if(repulsed){
				tryAgain=true;
			}
		}
		hitPos=curPos+dir*length;

		float baseAlpha=weaponDef->intensity*255;
		float startAlpha=(1-curLength/(range*1.3f))*baseAlpha;
		float endAlpha=(1-(curLength+length)/(range*1.3f))*baseAlpha;

		if(weaponDef->largeBeamLaser)
			SAFE_NEW CLargeBeamLaserProjectile(curPos, hitPos, color, weaponDef->visuals.color2, owner,weaponDef);
		else
			SAFE_NEW CBeamLaserProjectile(curPos,hitPos,startAlpha,endAlpha,color,weaponDef->visuals.color2, owner,weaponDef->thickness,weaponDef->corethickness, weaponDef->laserflaresize, weaponDef);

		curPos=hitPos;
		curLength+=length;
		dir=newDir;
	}
	float	intensity=1-(curLength)/(range*2);
	if(curLength<maxLength)
		helper->Explosion(hitPos, weaponDef->damages*(intensity*damageMul), areaOfEffect, weaponDef->edgeEffectiveness, weaponDef->explosionSpeed,owner, true, 1.0f, false, weaponDef->explosionGenerator, hit, dir, weaponDef->id);

	if(targetUnit)
		lastFireFrame = gs->frameNum;
}
