#include "StdAfx.h"
#include "mmgr.h"

#include "Game/Camera.h"
#include "Game/GameHelper.h"
#include "LogOutput.h"
#include "Map/Ground.h"
#include "Matrix44f.h"
#include "myMath.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/UnitModels/3DOParser.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Projectiles/Unsynced/SmokeTrailProjectile.h"
#include "Sim/Units/Unit.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "StarburstProjectile.h"
#include "Sync/SyncTracer.h"
#include "Rendering/UnitModels/s3oParser.h"
#include "GlobalUnsynced.h"

static const float Smoke_Time=70;

CR_BIND_DERIVED(CStarburstProjectile, CWeaponProjectile, (float3(0,0,0),float3(0,0,0),NULL,float3(0,0,0),0,0,0,0,NULL,NULL,NULL,0,float3(0,0,0)));

CR_REG_METADATA(CStarburstProjectile,(
	CR_MEMBER(tracking),
	CR_MEMBER(maxGoodDif),
	CR_MEMBER(dir),
	CR_MEMBER(maxSpeed),
	CR_MEMBER(curSpeed),
	CR_MEMBER(uptime),
	CR_MEMBER(areaOfEffect),
	CR_MEMBER(age),
	CR_MEMBER(oldSmoke),
	CR_MEMBER(oldSmokeDir),
	CR_MEMBER(drawTrail),
	CR_MEMBER(numParts),
	CR_MEMBER(doturn),
	CR_MEMBER(curCallback),
	CR_MEMBER(missileAge),
	CR_MEMBER(distanceToTravel),
	CR_MEMBER(aimError),
	CR_RESERVED(16)
	));

void CStarburstProjectile::creg_Serialize(creg::ISerializer& s)
{
	s.Serialize(numCallback, sizeof(int));
	// NOTE This could be tricky if gs is serialized after losHandler.
	for(int a=0;a<5;++a){
		s.Serialize(oldInfos[a],sizeof(struct CStarburstProjectile::OldInfo));
	}
}

CStarburstProjectile::CStarburstProjectile(const float3& pos, const float3& speed, CUnit* owner,
		float3 targetPos, float areaOfEffect, float maxSpeed, float tracking, int uptime, CUnit* target,
		const WeaponDef* weaponDef, CWeaponProjectile* interceptTarget, float maxdistance, float3 aimError GML_PARG_C):
	CWeaponProjectile(pos, speed, owner, target, targetPos, weaponDef, interceptTarget, true,  200 GML_PARG_P),
	tracking(tracking),
	dir(speed),
	maxSpeed(maxSpeed),
	areaOfEffect(areaOfEffect),
	age(0),
	oldSmoke(pos),
	aimError(aimError),
	drawTrail(true),
	numParts(0),
	doturn(true),
	curCallback(0),
	numCallback(0),
	missileAge(0),
	distanceToTravel(maxdistance)
{
	this->uptime=uptime;
	if (weaponDef) {
		if (weaponDef->flighttime == 0) {
			ttl = (int) std::min(3000.f,uptime+weaponDef->range/maxSpeed+100);
		}
		else {
			ttl=weaponDef->flighttime;
		}
	}
	maxGoodDif=cos(tracking*0.6f);
	curSpeed=speed.Length();
	dir.Normalize();
	oldSmokeDir=dir;

	drawRadius=maxSpeed*8;
	numCallback=new int;
	*numCallback=0;

	float3 camDir=(pos-camera->pos).Normalize();
	if(camera->pos.distance(pos)*0.2f+(1-fabs(camDir.dot(dir)))*3000 < 200)
		drawTrail=false;

	for(int a = 0; a < 5; ++a) {
		oldInfos[a]=new OldInfo;
		oldInfos[a]->dir=dir;
		oldInfos[a]->pos=pos;
		oldInfos[a]->speedf=curSpeed;
		for(float aa = 0; aa < curSpeed + 0.6f; aa += 0.15f) {
			float ageMod = 1.0f;
			oldInfos[a]->ageMods.push_back(ageMod);
		}
	}
	castShadow=true;

#ifdef TRACE_SYNC
	tracefile << "New starburst rocket: ";
	tracefile << pos.x << " " << pos.y << " " << pos.z << " " << speed.x << " " << speed.y << " " << speed.z << "\n";
#endif

	if (cegTag.size() > 0) {
		ceg.Load(explGenHandler, cegTag);
	}
}

CStarburstProjectile::~CStarburstProjectile(void)
{
	delete numCallback;
	if(curCallback)
		curCallback->drawCallbacker=0;
	for(int a=0;a<5;++a){
		delete oldInfos[a];
	}
}

void CStarburstProjectile::Collision()
{
	float h=ground->GetHeight2(pos.x,pos.z);
	if(weaponDef->waterweapon && h < pos.y) return; //prevent impact on water if waterweapon is set
	if(h>pos.y)
		pos+=speed*(h-pos.y)/speed.y;
	if (weaponDef->visuals.smokeTrail)
		new CSmokeTrailProjectile(pos,oldSmoke,dir,oldSmokeDir,owner(),false,true,7,Smoke_Time,0.7f,drawTrail,0,weaponDef->visuals.texture2);
	oldSmokeDir=dir;
	CWeaponProjectile::Collision();
	oldSmoke=pos;
}

void CStarburstProjectile::Collision(CUnit *unit)
{
	if (weaponDef->visuals.smokeTrail)
		new CSmokeTrailProjectile(pos,oldSmoke,dir,oldSmokeDir,owner(),false,true,7,Smoke_Time,0.7f,drawTrail,0,weaponDef->visuals.texture2);
	oldSmokeDir=dir;

	CWeaponProjectile::Collision(unit);
	oldSmoke=pos;
}

void CStarburstProjectile::Update(void)
{
	ttl--;
	uptime--;
	missileAge++;
	if (target && weaponDef->tracks && owner()) {
		targetPos = helper->GetUnitErrorPos(target, owner()->allyteam);
	}
	if (interceptTarget) {
		targetPos = interceptTarget->pos;
		if (targetPos.SqDistance(pos) < Square(areaOfEffect * 2)) {
			interceptTarget->Collision();
			Collision();
		}
	}
	if (uptime > 0) {
		if (curSpeed < maxSpeed)
			curSpeed += weaponDef->weaponacceleration;
		speed = dir * curSpeed;
	} else if (doturn && ttl > 0 && distanceToTravel > 0) {
		float3 dif(targetPos-pos);
		dif.Normalize();
		dif += aimError;
		dif.Normalize();
		if (dif.dot(dir) > 0.99f) {
			dir = dif;
			doturn = false;
		} else {
			dif = dif - dir;
			dif -= dir * (dif.dot(dir));
			dif.Normalize();
			if (weaponDef->turnrate != 0) {
				dir += dif * weaponDef->turnrate;
			}
			else {
				dir += dif * 0.06;
			}
			dir.Normalize();
		}
		speed = dir * curSpeed;
		if (distanceToTravel != MAX_WORLD_SIZE)
			distanceToTravel -= speed.Length2D();
	} else if (ttl > 0 && distanceToTravel > 0) {
		if (curSpeed < maxSpeed)
			curSpeed += weaponDef->weaponacceleration;
		float3 dif(targetPos - pos);
		dif.Normalize();
		if (dif.dot(dir) > maxGoodDif) {
			dir = dif;
		} else {
			dif = dif - dir;
			dif -= dir * (dif.dot(dir));
			dif.Normalize();
			dir += dif * tracking;
			dir.Normalize();
		}
		speed = dir * curSpeed;
		if (distanceToTravel != MAX_WORLD_SIZE)
			distanceToTravel -= speed.Length2D();
	} else {
		dir.y += gravity;
		dir.Normalize();
		curSpeed -= gravity;
		speed = dir * curSpeed;
	}

	pos += speed;

	if (ttl > 0) {
		if (cegTag.size() > 0) {
			ceg.Explosion(pos, ttl, areaOfEffect, 0x0, 0.0f, 0x0, dir);
		}
	}

	OldInfo* tempOldInfo = oldInfos[4];
	for (int a = 3; a >= 0; --a) {
		oldInfos[a + 1] = oldInfos[a];
	}
	oldInfos[0] = tempOldInfo;
	oldInfos[0]->pos = pos;
	oldInfos[0]->dir = dir;
	oldInfos[0]->speedf = curSpeed;
	int newsize = 0;
	for(float aa = 0; aa < curSpeed + 0.6f; aa += 0.15f, ++newsize) {
		float ageMod = (missileAge < 20) ? 1 : 0.6f + rand() * 0.8f / RAND_MAX;
		if(oldInfos[0]->ageMods.size()<=newsize)
			oldInfos[0]->ageMods.push_back(ageMod);
		else
			oldInfos[0]->ageMods[newsize] = ageMod;
	}
	if(oldInfos[0]->ageMods.size() != newsize)
		oldInfos[0]->ageMods.resize(newsize);

	age++;
	numParts++;

	if (weaponDef->visuals.smokeTrail && !(age & 7)) {
		if (curCallback)
			curCallback->drawCallbacker = 0;
		curCallback = new CSmokeTrailProjectile(pos, oldSmoke, dir, oldSmokeDir, owner(), age == 8,
			false, 7, Smoke_Time, 0.7f, drawTrail, this, weaponDef->visuals.texture2);
		oldSmoke = pos;
		oldSmokeDir = dir;
		numParts = 0;
		useAirLos = curCallback->useAirLos;
		if (!drawTrail) {
			float3 camDir = (pos - camera->pos).Normalize();
			if (camera->pos.distance(pos) * 0.2f + (1 - fabs(camDir.dot(dir))) * 3000 > 300)
				drawTrail = true;
		}
	}
}

void CStarburstProjectile::Draw(void)
{
	inArray=true;
	float age2=(age&7)+gu->timeOffset;

	float color=0.7f;
	unsigned char col[4];
	unsigned char col2[4];

	if (weaponDef->visuals.smokeTrail) {
#if defined(USE_GML) && GML_ENABLE_SIM
		int curNumParts = *(volatile int *)&numParts;
#else
		int curNumParts = numParts;
#endif
		va->EnlargeArrays(4+4*curNumParts,0,VA_SIZE_TC);
		if(drawTrail){		//draw the trail as a single quad

			float3 dif(drawPos-camera->pos);
			dif.Normalize();
			float3 dir1(dif.cross(dir));
			dir1.Normalize();
			float3 dif2(oldSmoke-camera->pos);
			dif2.Normalize();
			float3 dir2(dif2.cross(oldSmokeDir));
			dir2.Normalize();


			float a1=(1-float(0)/(Smoke_Time))*255;
			a1*=0.7f+fabs(dif.dot(dir));
			int alpha = std::min(255, (int) std::max(0.f,a1));
			col[0]=(unsigned char) (color*alpha);
			col[1]=(unsigned char) (color*alpha);
			col[2]=(unsigned char) (color*alpha);
			col[3]=(unsigned char)alpha;

			float a2=(1-float(age2)/(Smoke_Time))*255;
			a2*=0.7f+fabs(dif2.dot(oldSmokeDir));
			if(age<8)
				a2=0;
			alpha = std::min(255, (int) std::max(0.f,a2));
			col2[0]=(unsigned char) (color*alpha);
			col2[1]=(unsigned char) (color*alpha);
			col2[2]=(unsigned char) (color*alpha);
			col2[3]=(unsigned char)alpha;

			float size=1;
			float size2=(1+age2*(1/Smoke_Time)*7);

			float txs=weaponDef->visuals.texture2->xend - (weaponDef->visuals.texture2->xend-weaponDef->visuals.texture2->xstart)*(age2/8.0f);//(1-age2/8.0f);
			va->AddVertexQTC(drawPos-dir1*size, txs, weaponDef->visuals.texture2->ystart, col);
			va->AddVertexQTC(drawPos+dir1*size, txs, weaponDef->visuals.texture2->yend, col);
			va->AddVertexQTC(oldSmoke+dir2*size2, weaponDef->visuals.texture2->xend, weaponDef->visuals.texture2->yend, col2);
			va->AddVertexQTC(oldSmoke-dir2*size2, weaponDef->visuals.texture2->xend, weaponDef->visuals.texture2->ystart, col2);
		} else {	//draw the trail as particles
			float dist=pos.distance(oldSmoke);
			float3 dirpos1=pos-dir*dist*0.33f;
			float3 dirpos2=oldSmoke+oldSmokeDir*dist*0.33f;

			for(int a=0;a<curNumParts;++a){ //! CAUTION: loop count must match EnlargeArrays above
				//float a1=1-float(a)/Smoke_Time;
				col[0]=(unsigned char) (color*255);
				col[1]=(unsigned char) (color*255);
				col[2]=(unsigned char) (color*255);
				col[3]=255;//min(255,max(0,a1*255));
				float size=(1+(a)*(1/Smoke_Time)*7);

				float3 pos1=CalcBeizer(float(a)/(curNumParts),pos,dirpos1,dirpos2,oldSmoke);
				va->AddVertexQTC(pos1+( camera->up+camera->right)*size, ph->smoketex[0].xstart, ph->smoketex[0].ystart, col);
				va->AddVertexQTC(pos1+( camera->up-camera->right)*size, ph->smoketex[0].xend, ph->smoketex[0].ystart, col);
				va->AddVertexQTC(pos1+(-camera->up-camera->right)*size, ph->smoketex[0].xend, ph->smoketex[0].ystart, col);
				va->AddVertexQTC(pos1+(-camera->up+camera->right)*size, ph->smoketex[0].xstart, ph->smoketex[0].ystart, col);
			}
		}
	}
	DrawCallback();
	if(curCallback==0)
		DrawCallback();
}

void CStarburstProjectile::DrawCallback(void)
{
	if(*numCallback != gu->drawFrame) {
		*numCallback = gu->drawFrame;
		return;
	}
	*numCallback = 0;

	inArray=true;
	unsigned char col[4];

	for(int a = 0; a < 5; ++a) {
#if defined(USE_GML) && GML_ENABLE_SIM
		OldInfo *oldinfo = *(OldInfo * volatile *)&oldInfos[a];
#else
		OldInfo *oldinfo = oldInfos[a];
#endif
		float3 opos=oldinfo->pos;
		float3 odir=oldinfo->dir;
		float ospeed=oldinfo->speedf;
		float aa = 0;
		for(AGEMOD_VECTOR::iterator ai = oldinfo->ageMods.begin(); ai != oldinfo->ageMods.end(); ++ai, aa += 0.15f) {
			float ageMod = *ai;
			float age2=((a+aa/(ospeed+0.01f)))*0.2f;
			float3 interPos=opos-odir*(a*0.5f+aa);
			float drawsize;
			col[3]=1;
			if (missileAge < 20) {
				float alpha = std::max(0.f,((1-age2)*(1-age2)));
				col[0]=(unsigned char) (255*alpha);
				col[1]=(unsigned char) (200*alpha);
				col[2]=(unsigned char) (150*alpha);
			} else {
				float alpha = std::max(0.f,((1-age2) * std::max(0.f,(1-age2))));
				col[0]=(unsigned char) (255*alpha);
				col[1]=(unsigned char) (200*alpha);
				col[2]=(unsigned char) (150*alpha);
			}
			drawsize=1+age2*0.8f*ageMod*7;
			va->AddVertexTC(interPos-camera->right*drawsize-camera->up*drawsize,weaponDef->visuals.texture3->xstart,weaponDef->visuals.texture3->ystart,col);
			va->AddVertexTC(interPos+camera->right*drawsize-camera->up*drawsize,weaponDef->visuals.texture3->xend,weaponDef->visuals.texture3->ystart,col);
			va->AddVertexTC(interPos+camera->right*drawsize+camera->up*drawsize,weaponDef->visuals.texture3->xend,weaponDef->visuals.texture3->yend,col);
			va->AddVertexTC(interPos-camera->right*drawsize+camera->up*drawsize,weaponDef->visuals.texture3->xstart,weaponDef->visuals.texture3->yend,col);
		}
	}

	//rita flaren
	col[0]=255;
	col[1]=180;
	col[2]=180;
	col[3]=1;
	float fsize = 25.0f;
	va->AddVertexTC(drawPos-camera->right*fsize-camera->up*fsize,weaponDef->visuals.texture1->xstart,weaponDef->visuals.texture1->ystart,col);
	va->AddVertexTC(drawPos+camera->right*fsize-camera->up*fsize,weaponDef->visuals.texture1->xend,weaponDef->visuals.texture1->ystart,col);
	va->AddVertexTC(drawPos+camera->right*fsize+camera->up*fsize,weaponDef->visuals.texture1->xend,weaponDef->visuals.texture1->yend,col);
	va->AddVertexTC(drawPos-camera->right*fsize+camera->up*fsize,weaponDef->visuals.texture1->xstart,weaponDef->visuals.texture1->yend,col);
}

void CStarburstProjectile::DrawUnitPart(void)
{
	glPushMatrix();
	float3 rightdir;
	if(dir.y!=1)
		rightdir=dir.cross(UpVector);
	else
		rightdir=float3(1,0,0);
	rightdir.Normalize();
	float3 updir=rightdir.cross(dir);

	CMatrix44f transMatrix(drawPos,-rightdir,updir,dir);
	glMultMatrixf(&transMatrix[0]);

	glCallList(s3domodel->rootobject->displist); // dont cache displists because of delayed loading
	glPopMatrix();
}

int CStarburstProjectile::ShieldRepulse(CPlasmaRepulser* shield,float3 shieldPos, float shieldForce, float shieldMaxSpeed)
{
	float3 sdir=pos-shieldPos;
	sdir.Normalize();
	if (ttl > 0) {
		float3 dif2 = sdir - dir;
		// steer away twice as fast as we can steer toward target
		float tracking = std::max(shieldForce * 0.05f, weaponDef->turnrate * 2);
		if (dif2.Length() < tracking) {
			dir = sdir;
		} else {
			dif2 -= dir * (dif2.dot(dir));
			dif2.Normalize();
			dir += dif2 * tracking;
			dir.Normalize();
		}
		return 2;
	}
	return 0;
}
