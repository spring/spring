#include "StdAfx.h"
#include "LaserProjectile.h"
#include "Game/Camera.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "LogOutput.h"
#include "mmgr.h"
#include "ProjectileHandler.h"
#include "SimpleParticleSystem.h"

CLaserProjectile::CLaserProjectile(const float3& pos,const float3& speed,CUnit* owner,float length,const float3& color, const float3& color2, float intensity, WeaponDef *weaponDef, int ttl)
: CWeaponProjectile(pos,speed,owner,0,ZeroVector,weaponDef,0),
	ttl(ttl),
	color(color),
	color2(color2),
	length(length),
	curLength(0),
	intensity(intensity),
	intensityFalloff(intensity*0.1f)
{
	dir=speed;
	dir.Normalize();
	speedf=speed.Length();

	SetRadius(weaponDef->collisionSize);
	drawRadius=length;
	midtexx = weaponDef->visuals.texture2->xstart + (weaponDef->visuals.texture2->xend-weaponDef->visuals.texture2->xstart)*0.5f;
#ifdef TRACE_SYNC
	tracefile << "New laser: ";
	tracefile << pos.x << " " << pos.y << " " << pos.z << " " << speed.x << " " << speed.y << " " << speed.z << "\n";
#endif
}

CLaserProjectile::~CLaserProjectile(void)
{
}

void CLaserProjectile::Update(void)
{
	pos+=speed;
	if(checkCol){	//normal;
		curLength+=speedf;
		if(curLength>length)
			curLength=length;
	} else {	//fading out after hit
		curLength-=speedf;
		if(curLength<=0){
			deleteMe=true;
			curLength=0;
		}
	}
	ttl--;

	if(ttl<5){
		intensity-=intensityFalloff;
		if(intensity<=0){
			deleteMe=true;
			intensity=0;
		}
	}
}

void CLaserProjectile::Collision(CUnit* unit)
{
	float3 oldPos=pos;
	CWeaponProjectile::Collision(unit);

	deleteMe=false;	//we will fade out over some time
	if (!weaponDef->noExplode) {
		checkCol=false;
		speed=ZeroVector;
		pos=oldPos;
	}
}

void CLaserProjectile::Collision(CFeature* feature)
{
	float3 oldPos=pos;
	CWeaponProjectile::Collision(feature);

	deleteMe=false;	//we will fade out over some time
	if (!weaponDef->noExplode) {
		checkCol=false;
		speed=ZeroVector;
		pos=oldPos;
	}
}

void CLaserProjectile::Collision()
{
	float3 oldPos=pos;
	CWeaponProjectile::Collision();

	deleteMe=false;	//we will fade out over some time
	if (!weaponDef->noExplode) {
		checkCol=false;
		speed=ZeroVector;
		pos=oldPos;
	}

	//CSimpleParticleSystem *ps = new CSimpleParticleSystem();
	//ps->particleLife = 30*3;
	//ps->particleLifeSpread = 30*3;
	//ps->particleSize = 6;
	//ps->particleSizeSpread = 2;
	//ps->emitVector = float3(0,0,0);
	//ps->emitSpread = float3(4,1,4).Normalize();
	//ps->pos = pos;
	//ps->numParticles = 2000;
	//ps->airdrag = 0.85f;
	//ps->speed = 30.0f;
	//ps->speedSpread = 8.0f;
	//ps->Init();
}


void CLaserProjectile::Draw(void)
{
	if(s3domodel)	//dont draw if a 3d model has been defined for us
		return;

	inArray=true;
	float3 dif(pos-camera->pos);
	float camDist=dif.Length();
	dif/=camDist;
	float3 dir1(dif.cross(dir));
	dir1.Normalize();
	float3 dir2(dif.cross(dir1));

	unsigned char col[4];
	col[0]=(unsigned char) (color.x*intensity*255);
	col[1]=(unsigned char) (color.y*intensity*255);
	col[2]=(unsigned char) (color.z*intensity*255);
	col[3]=1;//intensity*255;
	unsigned char col2[4];
	col2[0]=(unsigned char) (color2.x*intensity*255);
	col2[1]=(unsigned char) (color2.y*intensity*255);
	col2[2]=(unsigned char) (color2.z*intensity*255);
	col2[3]=1;//intensity*255;

	float size=weaponDef->thickness;
	float coresize=size * weaponDef->corethickness;

	if(camDist<1000){
		float3 pos1=pos+speed*gu->timeOffset;
		float3 pos2=pos1-dir*curLength;

		va->AddVertexTC(pos1-dir1*size,	midtexx,weaponDef->visuals.texture2->ystart,    col);
		va->AddVertexTC(pos1+dir1*size,	midtexx,weaponDef->visuals.texture2->yend,col);
		va->AddVertexTC(pos1+dir1*size-dir2*size, weaponDef->visuals.texture1->xstart,weaponDef->visuals.texture2->yend,col);
		va->AddVertexTC(pos1-dir1*size-dir2*size, weaponDef->visuals.texture1->xstart,weaponDef->visuals.texture2->ystart,col);
		va->AddVertexTC(pos1-dir1*coresize,midtexx,weaponDef->visuals.texture2->ystart,    col2);
		va->AddVertexTC(pos1+dir1*coresize,midtexx,weaponDef->visuals.texture2->yend,col2);
		va->AddVertexTC(pos1+dir1*coresize-dir2*coresize,weaponDef->visuals.texture1->xstart,weaponDef->visuals.texture2->yend,col2);
		va->AddVertexTC(pos1-dir1*coresize-dir2*coresize,weaponDef->visuals.texture1->xstart,weaponDef->visuals.texture2->ystart,col2);

		va->AddVertexTC(pos1-dir1*size,weaponDef->visuals.texture1->xstart,weaponDef->visuals.texture1->ystart,		col);
		va->AddVertexTC(pos1+dir1*size,weaponDef->visuals.texture1->xstart,weaponDef->visuals.texture1->yend,			col);
		va->AddVertexTC(pos2+dir1*size,weaponDef->visuals.texture1->xend,weaponDef->visuals.texture1->yend,			col);
		va->AddVertexTC(pos2-dir1*size,weaponDef->visuals.texture1->xend,weaponDef->visuals.texture1->ystart,			col);
		va->AddVertexTC(pos1-dir1*coresize,weaponDef->visuals.texture1->xstart,weaponDef->visuals.texture1->ystart,	col2);
		va->AddVertexTC(pos1+dir1*coresize,weaponDef->visuals.texture1->xstart,weaponDef->visuals.texture1->yend,	col2);
		va->AddVertexTC(pos2+dir1*coresize,weaponDef->visuals.texture1->xend,weaponDef->visuals.texture1->yend,		col2);
		va->AddVertexTC(pos2-dir1*coresize,weaponDef->visuals.texture1->xend,weaponDef->visuals.texture1->ystart,		col2);

		va->AddVertexTC(pos2-dir1*size,	midtexx,weaponDef->visuals.texture2->ystart,    col);
		va->AddVertexTC(pos2+dir1*size,	midtexx,weaponDef->visuals.texture2->yend,col);
		va->AddVertexTC(pos2+dir1*size+dir2*size, weaponDef->visuals.texture2->xend,weaponDef->visuals.texture2->yend,col);
		va->AddVertexTC(pos2-dir1*size+dir2*size, weaponDef->visuals.texture2->xend,weaponDef->visuals.texture2->ystart,col);
		va->AddVertexTC(pos2-dir1*coresize,midtexx,weaponDef->visuals.texture2->ystart,    col2);
		va->AddVertexTC(pos2+dir1*coresize,midtexx,weaponDef->visuals.texture2->yend,col2);
		va->AddVertexTC(pos2+dir1*coresize+dir2*coresize,weaponDef->visuals.texture2->xend,weaponDef->visuals.texture2->yend,col2);
		va->AddVertexTC(pos2-dir1*coresize+dir2*coresize,weaponDef->visuals.texture2->xend,weaponDef->visuals.texture2->ystart,col2);
	} else {
		float3 pos1=pos+speed*gu->timeOffset+dir*(size*0.5f);
		float3 pos2=pos1-dir*(curLength+size);

		va->AddVertexTC(pos1-dir1*size,weaponDef->visuals.texture1->xstart,weaponDef->visuals.texture1->ystart,		col);
		va->AddVertexTC(pos1+dir1*size,weaponDef->visuals.texture1->xstart,weaponDef->visuals.texture1->yend,			col);
		va->AddVertexTC(pos2+dir1*size,weaponDef->visuals.texture1->xend,weaponDef->visuals.texture1->yend,			col);
		va->AddVertexTC(pos2-dir1*size,weaponDef->visuals.texture1->xend,weaponDef->visuals.texture1->ystart,			col);
		va->AddVertexTC(pos1-dir1*coresize,weaponDef->visuals.texture1->xstart,weaponDef->visuals.texture1->ystart,	col2);
		va->AddVertexTC(pos1+dir1*coresize,weaponDef->visuals.texture1->xstart,weaponDef->visuals.texture1->yend,	col2);
		va->AddVertexTC(pos2+dir1*coresize,weaponDef->visuals.texture1->xend,weaponDef->visuals.texture1->yend,		col2);
		va->AddVertexTC(pos2-dir1*coresize,weaponDef->visuals.texture1->xend,weaponDef->visuals.texture1->ystart,		col2);
	}
}

int CLaserProjectile::ShieldRepulse(CPlasmaRepulser* shield,float3 shieldPos, float shieldForce, float shieldMaxSpeed)
{
	float3 sdir=pos-shieldPos;
	sdir.Normalize();
	if(sdir.dot(speed)<0){
		speed-=sdir*sdir.dot(speed)*2;
		dir=speed;
		dir.Normalize();
		return 1;
	}
	return 0;
}
