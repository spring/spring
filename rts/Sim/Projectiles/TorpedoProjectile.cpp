#include "StdAfx.h"
#include "TorpedoProjectile.h"
#include "Rendering/GL/VertexArray.h"
#include "Game/Camera.h"
#include "Sim/Units/Unit.h"
#include "SmokeTrailProjectile.h"
#include "Map/Ground.h"
#include "Game/GameHelper.h"
#include "myMath.h"
#include "BubbleProjectile.h"
#include "mmgr.h"
#include "ProjectileHandler.h"

CTorpedoProjectile::CTorpedoProjectile(const float3& pos,const float3& speed,CUnit* owner,float areaOfEffect,float maxSpeed,float tracking, int ttl,CUnit* target, WeaponDef *weaponDef)
: CWeaponProjectile(pos,speed,owner,target,ZeroVector,weaponDef,0),
	ttl(ttl),
	maxSpeed(maxSpeed),
	tracking(tracking),
	target(target),
	dir(speed),
	areaOfEffect(areaOfEffect),
	nextBubble(4)
{
	curSpeed=speed.Length();
	dir.Normalize();
	if(target)
		AddDeathDependence(target);

	SetRadius(0.0f);
	drawRadius=maxSpeed*8;
	float3 camDir=(pos-camera->pos).Normalize();
	texx = ph->circularthingytex.xstart - (ph->circularthingytex.xend-ph->circularthingytex.xstart)*0.5f;
	texy = ph->circularthingytex.ystart - (ph->circularthingytex.yend-ph->circularthingytex.ystart)*0.5f;
#ifdef TRACE_SYNC
	tracefile << "New projectile: ";
	tracefile << pos.x << " " << pos.y << " " << pos.z << " " << speed.x << " " << speed.y << " " << speed.z << "\n";
#endif
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
//	helper->Explosion(pos,damages,areaOfEffect,owner);
	CWeaponProjectile::Collision();
}

void CTorpedoProjectile::Collision(CUnit *unit)
{
//	unit->DoDamage(damages,owner);
//	helper->Explosion(pos,damages,areaOfEffect,owner);

	CWeaponProjectile::Collision(unit);
}

void CTorpedoProjectile::Update(void)
{
	if(pos.y>-3){		//tracking etc only work when we have gotten underwater
		speed.y+=gs->gravity;
		if(dir.y>0)
			dir.y=0;
		dir=speed;
		dir.Normalize();
	} else {
		if(pos.y-speed.y>-3){		//level out torpedo a bit when hitting water
			dir.y*=0.5f;
			dir.Normalize();
		}
		ttl--;
		if(ttl>0){
			if(curSpeed<maxSpeed)
				curSpeed+=max((float)0.2f,tracking);
			if(target){
				float3 targPos;
				if((target->midPos-pos).SqLength()<150*150 || !owner)
					targPos=target->midPos;
				else
					targPos=helper->GetUnitErrorPos(target,owner->allyteam);
				if(targPos.y>0)
					targPos.y=0;

				float dist=targPos.distance(pos);
				float3 dif(targPos + target->speed*(dist/maxSpeed)*0.7f - pos);
				dif.Normalize();
				float3 dif2=dif-dir;
				if(dif2.Length()<tracking){
					dir=dif;
				} else {
					dif2-=dir*(dif2.dot(dir));
					dif2.Normalize();
					dir+=dif2*tracking;
					dir.Normalize();
				}
			}
			speed=dir*curSpeed;
		} else {
			speed*=0.98f;
			speed.y+=gs->gravity;
			dir=speed;
			dir.Normalize();
		}
	}
	pos+=speed;

	if(pos.y<-2){
		--nextBubble;
		if(nextBubble==0){
			nextBubble=1+(int)(gs->randFloat()*1.5f);

			float3 pspeed=gs->randVector()*0.1f;
			pspeed.y+=0.2f;
			SAFE_NEW CBubbleProjectile(pos+gs->randVector(),pspeed,40+gs->randFloat()*30,1+gs->randFloat()*2,0.01f,owner,0.3f+gs->randFloat()*0.3f);
		}
	}
}

void CTorpedoProjectile::Draw(void)
{
	if(s3domodel)	//dont draw if a 3d model has been defined for us
		return;

	float3 interPos=pos+speed*gu->timeOffset;
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
	va->AddVertexTC(interPos+r*w, texx,texy,col);
	va->AddVertexTC(interPos+u*w, texx,texy,col);
	va->AddVertexTC(interPos+u*w+dir*h, texx,texy,col);
	va->AddVertexTC(interPos+r*w+dir*h, texx,texy,col);

	va->AddVertexTC(interPos+u*w, texx,texy,col);
	va->AddVertexTC(interPos-r*w, texx,texy,col);
	va->AddVertexTC(interPos-r*w+dir*h, texx,texy,col);
	va->AddVertexTC(interPos+u*w+dir*h, texx,texy,col);

	va->AddVertexTC(interPos-r*w, texx,texy,col);
	va->AddVertexTC(interPos-u*w, texx,texy,col);
	va->AddVertexTC(interPos-u*w+dir*h, texx,texy,col);
	va->AddVertexTC(interPos-r*w+dir*h, texx,texy,col);

	va->AddVertexTC(interPos-u*w, texx,texy,col);
	va->AddVertexTC(interPos+r*w, texx,texy,col);
	va->AddVertexTC(interPos+r*w+dir*h, texx,texy,col);
	va->AddVertexTC(interPos-u*w+dir*h, texx,texy,col);


	va->AddVertexTC(interPos+r*w+dir*h, texx,texy,col);
	va->AddVertexTC(interPos+u*w+dir*h, texx,texy,col);
	va->AddVertexTC(interPos+dir*h*1.2f, texx,texy,col);
	va->AddVertexTC(interPos+dir*h*1.2f, texx,texy,col);

	va->AddVertexTC(interPos+u*w+dir*h, texx,texy,col);
	va->AddVertexTC(interPos-r*w+dir*h, texx,texy,col);
	va->AddVertexTC(interPos+dir*h*1.2f, texx,texy,col);
	va->AddVertexTC(interPos+dir*h*1.2f, texx,texy,col);

	va->AddVertexTC(interPos-r*w+dir*h, texx,texy,col);
	va->AddVertexTC(interPos-u*w+dir*h, texx,texy,col);
	va->AddVertexTC(interPos+dir*h*1.2f, texx,texy,col);
	va->AddVertexTC(interPos+dir*h*1.2f, texx,texy,col);

	va->AddVertexTC(interPos-u*w+dir*h, texx,texy,col);
	va->AddVertexTC(interPos+r*w+dir*h, texx,texy,col);
	va->AddVertexTC(interPos+dir*h*1.2f, texx,texy,col);
	va->AddVertexTC(interPos+dir*h*1.2f, texx,texy,col);
}
