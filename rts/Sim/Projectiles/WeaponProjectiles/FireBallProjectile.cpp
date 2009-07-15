#include "StdAfx.h"
#include "mmgr.h"

#include "creg/STL_Deque.h"
#include "FireBallProjectile.h"
#include "Game/Camera.h"
#include "Map/Ground.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "GlobalUnsynced.h"

CR_BIND_DERIVED(CFireBallProjectile, CWeaponProjectile, (float3(0,0,0),float3(0,0,0),NULL,NULL,float3(0,0,0),NULL));
CR_BIND(CFireBallProjectile::Spark, );

CR_REG_METADATA(CFireBallProjectile,(
	CR_MEMBER(sparks),
	CR_RESERVED(8)
	));

CR_REG_METADATA_SUB(CFireBallProjectile,Spark,(
	CR_MEMBER(pos),
	CR_MEMBER(speed),
	CR_MEMBER(size),
	CR_MEMBER(ttl),
	CR_RESERVED(8)
	));

CFireBallProjectile::CFireBallProjectile(const float3& pos, const float3& speed,
		CUnit* owner, CUnit* target, const float3 &targetPos, const WeaponDef* weaponDef GML_PARG_C)
:	CWeaponProjectile(pos, speed, owner, target, targetPos, weaponDef, 0, true,  1 GML_PARG_P)
{
	if (weaponDef) {
		SetRadius(weaponDef->collisionSize);
		drawRadius=weaponDef->size;
	}

	if (cegTag.size() > 0) {
		ceg.Load(explGenHandler, cegTag);
	}
}

CFireBallProjectile::~CFireBallProjectile(void)
{
}

void CFireBallProjectile::Draw()
{
	inArray=true;
	unsigned char col[4] = {255,150, 100, 1};

	float3 interPos = checkCol ? drawPos : pos;
	float size = radius*1.3f;

	int numSparks=sparks.size();
	int numFire=std::min(10,numSparks);
	va->EnlargeArrays((numSparks+numFire)*4,0,VA_SIZE_TC);
	for(int i=0; i<numSparks; i++) //! CAUTION: loop count must match EnlargeArrays above
	{
		col[0]=(numSparks-i)*12;
		col[1]=(numSparks-i)*6;
		col[2]=(numSparks-i)*4;
		va->AddVertexQTC(sparks[i].pos-camera->right*sparks[i].size-camera->up*sparks[i].size,ph->explotex.xstart,ph->explotex.ystart,col);
		va->AddVertexQTC(sparks[i].pos+camera->right*sparks[i].size-camera->up*sparks[i].size,ph->explotex.xend ,ph->explotex.ystart,col);
		va->AddVertexQTC(sparks[i].pos+camera->right*sparks[i].size+camera->up*sparks[i].size,ph->explotex.xend ,ph->explotex.yend ,col);
		va->AddVertexQTC(sparks[i].pos-camera->right*sparks[i].size+camera->up*sparks[i].size,ph->explotex.xstart,ph->explotex.yend ,col);
	}

	int maxCol=numFire;
	if (checkCol)
		maxCol = 10;

	for (int i = 0; i < numFire; i++) //! CAUTION: loop count must match EnlargeArrays above
	{
		col[0] = (maxCol - i) * 25;
		col[1] = (maxCol - i) * 15;
		col[2] = (maxCol - i) * 10;
		va->AddVertexQTC(interPos - camera->right * size - camera->up * size, ph->dguntex.xstart, ph->dguntex.ystart, col);
		va->AddVertexQTC(interPos + camera->right * size - camera->up * size, ph->dguntex.xend ,  ph->dguntex.ystart, col);
		va->AddVertexQTC(interPos + camera->right * size + camera->up * size, ph->dguntex.xend ,  ph->dguntex.yend,   col);
		va->AddVertexQTC(interPos  -camera->right * size + camera->up * size, ph->dguntex.xstart, ph->dguntex.yend,   col);
		interPos = interPos - speed * 0.5f;
	}
}

void CFireBallProjectile::Update()
{
	if (checkCol) {
		pos += speed;

		if (weaponDef->gravityAffected)
			speed.y += gravity;

		if (weaponDef->noExplode) {
			if (TraveledRange())
				checkCol = false;
		}

		EmitSpark();
	}
	else {
		if (sparks.size() == 0)
			deleteMe = true;
	}

	for (unsigned int i = 0; i < sparks.size(); i++) {
		sparks[i].ttl--;
		if (sparks[i].ttl == 0) {
			sparks.pop_back();
			break;
		}
		if (checkCol)
			sparks[i].pos += sparks[i].speed;
		sparks[i].speed *= 0.95f;
	}

	if (cegTag.size() > 0) {
		ceg.Explosion(pos, ttl, (sparks.size() > 0)? sparks[0].size: 0.0f, 0x0, 0.0f, 0x0, speed);
	}

	UpdateGroundBounce();
}

void CFireBallProjectile::EmitSpark()
{
	Spark spark;
	const float x = (rand() / (float) RAND_MAX) - 0.5f;
	const float y = (rand() / (float) RAND_MAX) - 0.5f;
	const float z = (rand() / (float) RAND_MAX) - 0.5f;
	spark.speed = (speed * 0.95f) + float3(x, y, z);
	spark.pos = pos - speed * (rand() / (float) RAND_MAX + 3) + spark.speed * 3;
	spark.size = 5.0f;
	spark.ttl = 15;

	sparks.push_front(spark);
}

void CFireBallProjectile::Collision()
{
	if (weaponDef->waterweapon && ground->GetHeight2(pos.x, pos.z) < pos.y) {
		// make waterweapons not explode in water
		return;
	}

	CWeaponProjectile::Collision();
	deleteMe = false;
}



