#include "StdAfx.h"
#include "mmgr.h"

#include "Game/Camera.h"
#include "LaserProjectile.h"
#include "LogOutput.h"
#include "Map/Ground.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Projectiles/Unsynced/SimpleParticleSystem.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "GlobalUnsynced.h"

#ifdef TRACE_SYNC
	#include "Sync/SyncTracer.h"
#endif

CR_BIND_DERIVED(CLaserProjectile, CWeaponProjectile, (float3(0,0,0),float3(0,0,0),NULL,0,float3(0,0,0),float3(0,0,0),0,NULL,0));

CR_REG_METADATA(CLaserProjectile,(
	CR_MEMBER(dir),
	CR_MEMBER(intensity),
	CR_MEMBER(color),
	CR_MEMBER(color2),
	CR_MEMBER(length),
	CR_MEMBER(curLength),
	CR_MEMBER(speedf),
	CR_MEMBER(stayTime),
	CR_MEMBER(intensityFalloff),
	CR_MEMBER(midtexx),
	CR_RESERVED(16)
	));

CLaserProjectile::CLaserProjectile(const float3& pos, const float3& speed,
		CUnit* owner, float length, const float3& color, const float3& color2,
		float intensity, const WeaponDef *weaponDef, int ttl GML_PARG_C)
:	CWeaponProjectile(pos,speed,owner,0,ZeroVector,weaponDef,0, true,  ttl GML_PARG_P),
	intensity(intensity),
	color(color),
	color2(color2),
	length(length),
	curLength(0),
	intensityFalloff(weaponDef?intensity*weaponDef->falloffRate:0),
	stayTime(0)
{
	dir=speed;
	dir.Normalize();
	speedf=speed.Length();

	if (weaponDef) SetRadius(weaponDef->collisionSize);
	drawRadius=length;
	if (weaponDef)midtexx = weaponDef->visuals.texture2->xstart + (weaponDef->visuals.texture2->xend-weaponDef->visuals.texture2->xstart)*0.5f;
#ifdef TRACE_SYNC
	tracefile << "New laser: ";
	tracefile << pos.x << " " << pos.y << " " << pos.z << " " << speed.x << " " << speed.y << " " << speed.z << "\n";
#endif

	if (cegTag.size() > 0) {
		ceg.Load(explGenHandler, cegTag);
	}
}

CLaserProjectile::~CLaserProjectile(void)
{
}

void CLaserProjectile::Update(void)
{
	pos += speed;
	if (checkCol) {
		// normal
		curLength += speedf;
		if (curLength > length)
			curLength = length;
	} else {
		// fading out after hit
		if (stayTime <= 0)
			curLength -= speedf;
		else
			stayTime--;
		if (curLength <= 0) {
			deleteMe = true;
			curLength = 0;
		}
	}

	ttl--;

	if (ttl > 0 && checkCol) {
		if (cegTag.size() > 0) {
			ceg.Explosion(pos, ttl, intensity, 0x0, 0.0f, 0x0, speed);
		}
	}
	
	if (weaponDef->visuals.hardStop) {
		if (ttl == 0 && checkCol) {
			checkCol = false;
			speed = ZeroVector;
			if (curLength < length) {
				// if the laser wasn't fully extended yet,
	 			// remember how long until it would have been
				// fully extended
				stayTime = int(1 + (length - curLength) / speedf);
			}
		}
	} else {
		if (ttl < 5 && checkCol) {
			intensity -= intensityFalloff * 0.2f;
			if (intensity <= 0) {
				deleteMe = true;
				intensity = 0;
			}
		}
	}

	float3 tempSpeed = speed;
	UpdateGroundBounce();

	if (tempSpeed != speed) {
		dir = speed;
		dir.Normalize();
	}
}

void CLaserProjectile::Collision(CUnit* unit)
{
	float3 oldPos=pos;
	CWeaponProjectile::Collision(unit);

	deleteMe=false;	//we will fade out over some time
	if (!weaponDef->noExplode) {
		checkCol = false;
		speed = ZeroVector;
		pos = oldPos;
		if (curLength < length) {
			// if the laser wasn't fully extended yet
			// and was too short for some reason,
			// remember how long until it would have
			// been fully extended
			curLength += speedf;
			stayTime = int(1 + (length - curLength) / speedf);
		}
	}
}

void CLaserProjectile::Collision(CFeature* feature)
{
	float3 oldPos=pos;
	CWeaponProjectile::Collision(feature);

	// we will fade out over some time
	deleteMe = false;
	if (!weaponDef->noExplode) {
		checkCol = false;
		speed = ZeroVector;
		pos = oldPos;
		if (curLength < length) {
			// if the laser wasn't fully extended yet,
			// remember how long until it would have been
			// fully extended
			stayTime = int(1 + (length - curLength) / speedf);
		}
	}
}

void CLaserProjectile::Collision()
{
	if (weaponDef->waterweapon && ground->GetHeight2(pos.x, pos.z) < pos.y) {
		// prevent impact on water if waterweapon is set
		return;
	}
	float3 oldPos = pos;
	CWeaponProjectile::Collision();

	// we will fade out over some time
	deleteMe = false;
	if (!weaponDef->noExplode) {
		checkCol = false;
		speed = ZeroVector;
		pos = oldPos;
		if (curLength < length) {
			// if the laser wasn't fully extended yet,
			// remember how long until it would have been
			// fully extended
			stayTime = int(1 + (length - curLength) / speedf);
		}
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

	va->EnlargeArrays(32,0,VA_SIZE_TC);
	if(camDist<weaponDef->lodDistance){
		float3 pos2=drawPos-dir*curLength;
		float texStartOffset;
		float texEndOffset;
		if (checkCol) { //expanding or contracting?
			texStartOffset=0;
			texEndOffset=(1.0f - curLength/length)*(weaponDef->visuals.texture1->xstart - weaponDef->visuals.texture1->xend);
		}
		else {
			texStartOffset=(-1.0f + curLength/length + ((float)stayTime * speedf/length))*(weaponDef->visuals.texture1->xstart - weaponDef->visuals.texture1->xend);
			texEndOffset= ((float)stayTime * speedf/length)*(weaponDef->visuals.texture1->xstart - weaponDef->visuals.texture1->xend);
		}

		va->AddVertexQTC(drawPos-dir1*size,	midtexx,weaponDef->visuals.texture2->ystart,    col);
		va->AddVertexQTC(drawPos+dir1*size,	midtexx,weaponDef->visuals.texture2->yend,col);
		va->AddVertexQTC(drawPos+dir1*size-dir2*size, weaponDef->visuals.texture2->xstart,weaponDef->visuals.texture2->yend,col);
		va->AddVertexQTC(drawPos-dir1*size-dir2*size, weaponDef->visuals.texture2->xstart,weaponDef->visuals.texture2->ystart,col);
		va->AddVertexQTC(drawPos-dir1*coresize,midtexx,weaponDef->visuals.texture2->ystart,    col2);
		va->AddVertexQTC(drawPos+dir1*coresize,midtexx,weaponDef->visuals.texture2->yend,col2);
		va->AddVertexQTC(drawPos+dir1*coresize-dir2*coresize,weaponDef->visuals.texture2->xstart,weaponDef->visuals.texture2->yend,col2);
		va->AddVertexQTC(drawPos-dir1*coresize-dir2*coresize,weaponDef->visuals.texture2->xstart,weaponDef->visuals.texture2->ystart,col2);

		va->AddVertexQTC(drawPos-dir1*size,weaponDef->visuals.texture1->xstart + texStartOffset,weaponDef->visuals.texture1->ystart,		col);
		va->AddVertexQTC(drawPos+dir1*size,weaponDef->visuals.texture1->xstart + texStartOffset,weaponDef->visuals.texture1->yend,			col);
		va->AddVertexQTC(pos2+dir1*size,weaponDef->visuals.texture1->xend + texEndOffset,weaponDef->visuals.texture1->yend,			col);
		va->AddVertexQTC(pos2-dir1*size,weaponDef->visuals.texture1->xend + texEndOffset,weaponDef->visuals.texture1->ystart,			col);
		va->AddVertexQTC(drawPos-dir1*coresize,weaponDef->visuals.texture1->xstart + texStartOffset,weaponDef->visuals.texture1->ystart,	col2);
		va->AddVertexQTC(drawPos+dir1*coresize,weaponDef->visuals.texture1->xstart + texStartOffset,weaponDef->visuals.texture1->yend,	col2);
		va->AddVertexQTC(pos2+dir1*coresize,weaponDef->visuals.texture1->xend + texEndOffset,weaponDef->visuals.texture1->yend,		col2);
		va->AddVertexQTC(pos2-dir1*coresize,weaponDef->visuals.texture1->xend + texEndOffset,weaponDef->visuals.texture1->ystart,		col2);

		va->AddVertexQTC(pos2-dir1*size,	midtexx,weaponDef->visuals.texture2->ystart,    col);
		va->AddVertexQTC(pos2+dir1*size,	midtexx,weaponDef->visuals.texture2->yend,col);
		va->AddVertexQTC(pos2+dir1*size+dir2*size, weaponDef->visuals.texture2->xend,weaponDef->visuals.texture2->yend,col);
		va->AddVertexQTC(pos2-dir1*size+dir2*size, weaponDef->visuals.texture2->xend,weaponDef->visuals.texture2->ystart,col);
		va->AddVertexQTC(pos2-dir1*coresize,midtexx,weaponDef->visuals.texture2->ystart,    col2);
		va->AddVertexQTC(pos2+dir1*coresize,midtexx,weaponDef->visuals.texture2->yend,col2);
		va->AddVertexQTC(pos2+dir1*coresize+dir2*coresize,weaponDef->visuals.texture2->xend,weaponDef->visuals.texture2->yend,col2);
		va->AddVertexQTC(pos2-dir1*coresize+dir2*coresize,weaponDef->visuals.texture2->xend,weaponDef->visuals.texture2->ystart,col2);
	} else {
		float3 pos1=drawPos+dir*(size*0.5f);
		float3 pos2=pos1-dir*(curLength+size);
		float texStartOffset;
		float texEndOffset;
		if (checkCol) { //expanding or contracting?
			texStartOffset=0;
			texEndOffset=(1.0f - curLength/length)*(weaponDef->visuals.texture1->xstart - weaponDef->visuals.texture1->xend);
		}
		else {
			texStartOffset=(-1.0f + curLength/length + ((float)stayTime * speedf/length))*(weaponDef->visuals.texture1->xstart - weaponDef->visuals.texture1->xend);
			texEndOffset= ((float)stayTime * speedf/length)*(weaponDef->visuals.texture1->xstart - weaponDef->visuals.texture1->xend);
		}

		va->AddVertexQTC(pos1-dir1*size,weaponDef->visuals.texture1->xstart + texStartOffset,weaponDef->visuals.texture1->ystart,		col);
		va->AddVertexQTC(pos1+dir1*size,weaponDef->visuals.texture1->xstart + texStartOffset,weaponDef->visuals.texture1->yend,			col);
		va->AddVertexQTC(pos2+dir1*size,weaponDef->visuals.texture1->xend + texEndOffset,weaponDef->visuals.texture1->yend,			col);
		va->AddVertexQTC(pos2-dir1*size,weaponDef->visuals.texture1->xend + texEndOffset,weaponDef->visuals.texture1->ystart,			col);
		va->AddVertexQTC(pos1-dir1*coresize,weaponDef->visuals.texture1->xstart + texStartOffset,weaponDef->visuals.texture1->ystart,	col2);
		va->AddVertexQTC(pos1+dir1*coresize,weaponDef->visuals.texture1->xstart + texStartOffset,weaponDef->visuals.texture1->yend,	col2);
		va->AddVertexQTC(pos2+dir1*coresize,weaponDef->visuals.texture1->xend + texEndOffset,weaponDef->visuals.texture1->yend,		col2);
		va->AddVertexQTC(pos2-dir1*coresize,weaponDef->visuals.texture1->xend + texEndOffset,weaponDef->visuals.texture1->ystart,		col2);
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
