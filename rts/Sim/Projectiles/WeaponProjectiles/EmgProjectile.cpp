#include "StdAfx.h"
#include "mmgr.h"

#include "EmgProjectile.h"
#include "Game/Camera.h"
#include "Map/Ground.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sync/SyncTracer.h"
#include "GlobalUnsynced.h"

CR_BIND_DERIVED(CEmgProjectile, CWeaponProjectile, (float3(0,0,0),float3(0,0,0),NULL,float3(0,0,0),0,0,NULL));

CR_REG_METADATA(CEmgProjectile,(
    CR_MEMBER(intensity),
    CR_MEMBER(color),
    CR_RESERVED(8)
    ));

CEmgProjectile::CEmgProjectile(const float3& pos, const float3& speed,
		CUnit* owner, const float3& color, float intensity, int ttl,
		const WeaponDef* weaponDef GML_PARG_C)
:	CWeaponProjectile(pos, speed, owner, 0, ZeroVector, weaponDef, 0, true,  ttl GML_PARG_P),
	intensity(intensity),
	color(color)
{
	if (weaponDef) {
		SetRadius(weaponDef->collisionSize);
		drawRadius=weaponDef->size;
	}
#ifdef TRACE_SYNC
	tracefile << "New emg: ";
	tracefile << pos.x << " " << pos.y << " " << pos.z << " " << speed.x << " " << speed.y << " " << speed.z << "\n";
#endif


	if (cegTag.size() > 0) {
		ceg.Load(explGenHandler, cegTag);
	}
}

CEmgProjectile::~CEmgProjectile(void)
{
}

void CEmgProjectile::Update(void)
{
	pos += speed;
	ttl--;

	if (ttl < 0) {
		intensity -= 0.1f;
		if (intensity <= 0){
			deleteMe = true;
			intensity = 0;
		}
	}
	else {
		if (cegTag.size() > 0) {
			ceg.Explosion(pos, ttl, intensity, 0x0, 0.0f, 0x0, speed);
		}
	}
	UpdateGroundBounce();
}

void CEmgProjectile::Collision(CUnit* unit)
{
	CWeaponProjectile::Collision(unit);
}

void CEmgProjectile::Collision() {
	if (!(weaponDef->waterweapon && ground->GetHeight2(pos.x, pos.z) < pos.y))
		CWeaponProjectile::Collision();
}

void CEmgProjectile::Draw(void)
{
	inArray=true;
	unsigned char col[4];
	col[0]=(unsigned char) (color.x*intensity*255);
	col[1]=(unsigned char) (color.y*intensity*255);
	col[2]=(unsigned char) (color.z*intensity*255);
	col[3]=5;//intensity*255;
	va->AddVertexTC(drawPos-camera->right*drawRadius-camera->up*drawRadius,weaponDef->visuals.texture1->xstart,weaponDef->visuals.texture1->ystart,col);
	va->AddVertexTC(drawPos+camera->right*drawRadius-camera->up*drawRadius,weaponDef->visuals.texture1->xend,weaponDef->visuals.texture1->ystart,col);
	va->AddVertexTC(drawPos+camera->right*drawRadius+camera->up*drawRadius,weaponDef->visuals.texture1->xend,weaponDef->visuals.texture1->yend,col);
	va->AddVertexTC(drawPos-camera->right*drawRadius+camera->up*drawRadius,weaponDef->visuals.texture1->xstart,weaponDef->visuals.texture1->yend,col);
}

int CEmgProjectile::ShieldRepulse(CPlasmaRepulser* shield,float3 shieldPos, float shieldForce, float shieldMaxSpeed)
{
	float3 dir=pos-shieldPos;
	dir.Normalize();
	if(dir.dot(speed)<shieldMaxSpeed){
		speed+=dir*shieldForce;
		return 2;
	}
	return 0;
}
