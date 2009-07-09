#include "StdAfx.h"
#include "mmgr.h"

#include "Game/Camera.h"
#include "Game/GameHelper.h"
#include "Map/Ground.h"
#include "myMath.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Projectiles/Unsynced/BubbleProjectile.h"
#include "Sim/Projectiles/Unsynced/SmokeTrailProjectile.h"
#include "Sim/Units/Unit.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "TorpedoProjectile.h"
#include "GlobalUnsynced.h"

#ifdef TRACE_SYNC
	#include "Sync/SyncTracer.h"
#endif

CR_BIND_DERIVED(CTorpedoProjectile, CTorpedoProjectile, (float3(0,0,0),float3(0,0,0),NULL,0,0,0,0,NULL,NULL));

CR_REG_METADATA(CTorpedoProjectile,(
	CR_MEMBER(tracking),
	CR_MEMBER(dir),
	CR_MEMBER(maxSpeed),
	CR_MEMBER(curSpeed),
	CR_MEMBER(areaOfEffect),
	CR_MEMBER(target),
	CR_MEMBER(nextBubble),
	CR_MEMBER(texx),
	CR_MEMBER(texy),
	CR_RESERVED(16)
	));

CTorpedoProjectile::CTorpedoProjectile(const float3& pos, const float3& speed, CUnit* owner,
		float areaOfEffect, float maxSpeed, float tracking, int ttl, CUnit* target,
		const WeaponDef *weaponDef GML_PARG_C):
	CWeaponProjectile(pos, speed, owner, target, ZeroVector, weaponDef, 0, true,  ttl GML_PARG_P),
	tracking(tracking),
	dir(speed),
	maxSpeed(maxSpeed),
	areaOfEffect(areaOfEffect),
	target(target),
	nextBubble(4)
{
	curSpeed=speed.Length();
	dir.Normalize();
	if (target) {
		AddDeathDependence(target);
	}

	SetRadius(0.0f);
	drawRadius=maxSpeed*8;
	float3 camDir=(pos-camera->pos).Normalize();
	texx = ph->torpedotex.xstart - (ph->torpedotex.xend-ph->torpedotex.xstart)*0.5f;
	texy = ph->torpedotex.ystart - (ph->torpedotex.yend-ph->torpedotex.ystart)*0.5f;
#ifdef TRACE_SYNC
	tracefile << "New projectile: ";
	tracefile << pos.x << " " << pos.y << " " << pos.z << " " << speed.x << " " << speed.y << " " << speed.z << "\n";
#endif

	if (cegTag.size() > 0) {
		ceg.Load(explGenHandler, cegTag);
	}
}

CTorpedoProjectile::~CTorpedoProjectile(void)
{
}

void CTorpedoProjectile::DependentDied(CObject* o)
{
	if(o==target)
		target=0;
	CWeaponProjectile::DependentDied(o);
}

void CTorpedoProjectile::Collision()
{
	if(ground->GetHeight2(pos.x,pos.z)<pos.y+4)
		return;
	CWeaponProjectile::Collision();
}

void CTorpedoProjectile::Collision(CUnit *unit)
{
	CWeaponProjectile::Collision(unit);
}

void CTorpedoProjectile::Update(void)
{
	if (!(weaponDef->submissile) && pos.y > -3) {
		// tracking etc only works when we are underwater
		speed.y += gravity;
		if (dir.y > 0)
			dir.y = 0;
		dir = speed;
		dir.Normalize();
	} else {
		if (!(weaponDef->submissile) && pos.y-speed.y > -3) {
			// level out torpedo a bit when hitting water
			dir.y *= 0.5f;
			dir.Normalize();
		}

		ttl--;

		if (ttl > 0) {
			if (curSpeed < maxSpeed)
				curSpeed += std::max(0.2f, tracking);
			if (target) {
				float3 targPos;
				if ((target->midPos - pos).SqLength() < 150 * 150 || !owner())
					targPos = target->midPos;
				else
					targPos = helper->GetUnitErrorPos(target, owner()->allyteam);
				if (!(weaponDef->submissile) && targPos.y > 0)
					targPos.y = 0;

				float dist = targPos.distance(pos);
				float3 dif(targPos + target->speed * (dist / maxSpeed) * 0.7f - pos);
				dif.Normalize();
				float3 dif2 = dif - dir;

				if (dif2.Length() < tracking) {
					dir = dif;
				} else {
					dif2 -= dir * (dif2.dot(dir));
					dif2.Normalize();
					dir += dif2 * tracking;
					dir.Normalize();
				}
			}

			speed = dir * curSpeed;

			if (cegTag.size() > 0) {
				ceg.Explosion(pos, ttl, areaOfEffect, 0x0, 0.0f, 0x0, speed);
			}
		} else {
			speed *= 0.98f;
			speed.y += gravity;
			dir = speed;
			dir.Normalize();
		}
	}

	pos += speed;

	if (pos.y < -2) {
		--nextBubble;

		if (nextBubble == 0) {
			nextBubble = 1 + (int) (gs->randFloat() * 1.5f);

			float3 pspeed = gs->randVector() * 0.1f;
			pspeed.y += 0.2f;
			new CBubbleProjectile(pos + gs->randVector(), pspeed, 40 + gs->randFloat() * 30,
				1 + gs->randFloat() * 2, 0.01f, owner(), 0.3f + gs->randFloat() * 0.3f);
		}
	}

	UpdateGroundBounce();
}

void CTorpedoProjectile::Draw(void)
{
	if(s3domodel)	//dont draw if a 3d model has been defined for us
		return;

	inArray=true;

	unsigned char col[4];
	col[0]=60;
	col[1]=60;
	col[2]=100;
	col[3]=255;

	float3 r=dir.cross(UpVector);
	if(r.Length()<0.001f)
		r=float3(1,0,0);
	r.Normalize();
	float3 u=dir.cross(r);
	const float h=12;
	const float w=2;

	va->EnlargeArrays(32,0,VA_SIZE_TC);

	va->AddVertexQTC(drawPos+r*w, texx,texy,col);
	va->AddVertexQTC(drawPos+u*w, texx,texy,col);
	va->AddVertexQTC(drawPos+u*w+dir*h, texx,texy,col);
	va->AddVertexQTC(drawPos+r*w+dir*h, texx,texy,col);

	va->AddVertexQTC(drawPos+u*w, texx,texy,col);
	va->AddVertexQTC(drawPos-r*w, texx,texy,col);
	va->AddVertexQTC(drawPos-r*w+dir*h, texx,texy,col);
	va->AddVertexQTC(drawPos+u*w+dir*h, texx,texy,col);

	va->AddVertexQTC(drawPos-r*w, texx,texy,col);
	va->AddVertexQTC(drawPos-u*w, texx,texy,col);
	va->AddVertexQTC(drawPos-u*w+dir*h, texx,texy,col);
	va->AddVertexQTC(drawPos-r*w+dir*h, texx,texy,col);

	va->AddVertexQTC(drawPos-u*w, texx,texy,col);
	va->AddVertexQTC(drawPos+r*w, texx,texy,col);
	va->AddVertexQTC(drawPos+r*w+dir*h, texx,texy,col);
	va->AddVertexQTC(drawPos-u*w+dir*h, texx,texy,col);


	va->AddVertexQTC(drawPos+r*w+dir*h, texx,texy,col);
	va->AddVertexQTC(drawPos+u*w+dir*h, texx,texy,col);
	va->AddVertexQTC(drawPos+dir*h*1.2f, texx,texy,col);
	va->AddVertexQTC(drawPos+dir*h*1.2f, texx,texy,col);

	va->AddVertexQTC(drawPos+u*w+dir*h, texx,texy,col);
	va->AddVertexQTC(drawPos-r*w+dir*h, texx,texy,col);
	va->AddVertexQTC(drawPos+dir*h*1.2f, texx,texy,col);
	va->AddVertexQTC(drawPos+dir*h*1.2f, texx,texy,col);

	va->AddVertexQTC(drawPos-r*w+dir*h, texx,texy,col);
	va->AddVertexQTC(drawPos-u*w+dir*h, texx,texy,col);
	va->AddVertexQTC(drawPos+dir*h*1.2f, texx,texy,col);
	va->AddVertexQTC(drawPos+dir*h*1.2f, texx,texy,col);

	va->AddVertexQTC(drawPos-u*w+dir*h, texx,texy,col);
	va->AddVertexQTC(drawPos+r*w+dir*h, texx,texy,col);
	va->AddVertexQTC(drawPos+dir*h*1.2f, texx,texy,col);
	va->AddVertexQTC(drawPos+dir*h*1.2f, texx,texy,col);
}
