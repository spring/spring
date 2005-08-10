#include "ModelProjectile.h"
#include "3DOParser.h"
#include "WeaponDefHandler.h"
#include "myGL.h"
#include "GlobalStuff.h"
#include "Matrix44f.h"
//#include "mmgr.h"

CModelProjectile::CModelProjectile(const float3& pos,const float3& speed, CUnit* owner, CUnit *target, const float3 &targetPos, WeaponDef *weaponDef)
	: CWeaponProjectile(pos,speed, owner, target, targetPos, weaponDef,0)
{
	//dir=speed;
	//dir.Normalize();

	model = unit3doparser->Load3DO(("objects3d/"+weaponDef->visuals.modelName+".3do").c_str(),1,0);
	isUnitPart = true;
}

CModelProjectile::~CModelProjectile(void)
{
}

void CModelProjectile::Update()
{
	pos+=speed;

	if(weaponDef->movement.gravityAffected)
		speed.y+=gs->gravity;

	//göra om till ttl sedan kanske
	if(weaponDef->movement.noExplode)
	{
        if(TraveledRange())
			CProjectile::Collision();
	}
}

void CModelProjectile::DrawUnitPart()
{
	float3 interPos=pos+speed*gu->timeOffset;

	glPushMatrix();


	float3 frontdir = speed;
	frontdir.Normalize();

	float3 updir=UpVector;
	float3 rightdir=frontdir.cross(updir);
	rightdir.Normalize();
	updir=rightdir.cross(frontdir);

	CMatrix44f transMatrix;

	transMatrix[0]=-rightdir.x;
	transMatrix[1]=-rightdir.y;
	transMatrix[2]=-rightdir.z;
	transMatrix[4]=updir.x;
	transMatrix[5]=updir.y;
	transMatrix[6]=updir.z;
	transMatrix[8]=frontdir.x;
	transMatrix[9]=frontdir.y;
	transMatrix[10]=frontdir.z;
	transMatrix[12]=interPos.x;
	transMatrix[13]=interPos.y;
	transMatrix[14]=interPos.z;
	glMultMatrixf(&transMatrix[0]);	

	model->rootobject->DrawStatic();

	glPopMatrix();
}

