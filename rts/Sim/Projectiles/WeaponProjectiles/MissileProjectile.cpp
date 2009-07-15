#include "StdAfx.h"
#include "mmgr.h"

#include "Game/Camera.h"
#include "Game/GameHelper.h"
#include "LogOutput.h"
#include "Map/Ground.h"
#include "Matrix44f.h"
#include "MissileProjectile.h"
#include "myMath.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Misc/GeometricObjects.h"
#include "Sim/Projectiles/Unsynced/SmokeTrailProjectile.h"
#include "Sim/Units/Unit.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sync/SyncTracer.h"
#include "Rendering/UnitModels/IModelParser.h"
#include "Rendering/UnitModels/UnitDrawer.h"
#include "Rendering/UnitModels/s3oParser.h"
#include "Rendering/UnitModels/3DOParser.h"
#include "GlobalUnsynced.h"

static const float Smoke_Time=60;

CR_BIND_DERIVED(CMissileProjectile, CWeaponProjectile, (float3(0,0,0),float3(0,0,0),NULL,0,0,0,NULL,NULL,float3(0,0,0)));

CR_REG_METADATA(CMissileProjectile,(
	CR_MEMBER(dir),
	CR_MEMBER(maxSpeed),
	CR_MEMBER(curSpeed),
//	CR_MEMBER(ttl),
	CR_MEMBER(areaOfEffect),
	CR_MEMBER(age),
	CR_MEMBER(oldSmoke),
	CR_MEMBER(oldDir),
	CR_MEMBER(target),
	CR_MEMBER(decoyTarget),
	CR_MEMBER(drawTrail),
	CR_MEMBER(numParts),
	CR_MEMBER(targPos),
	CR_MEMBER(isWobbling),
	CR_MEMBER(wobbleDir),
	CR_MEMBER(wobbleTime),
	CR_MEMBER(wobbleDif),
	CR_MEMBER(danceMove),
	CR_MEMBER(danceCenter),
	CR_MEMBER(danceTime),
	CR_MEMBER(isDancing),
	CR_MEMBER(extraHeight),
	CR_MEMBER(extraHeightDecay),
	CR_MEMBER(extraHeightTime),
	CR_RESERVED(16)
	));

CMissileProjectile::CMissileProjectile(const float3& pos, const float3& speed, CUnit* owner,
		float areaOfEffect, float maxSpeed, int ttl, CUnit* target, const WeaponDef *weaponDef,
		float3 targetPos GML_PARG_C):
	CWeaponProjectile(pos, speed, owner, target, targetPos, weaponDef, 0, true,  ttl GML_PARG_P),
	dir(speed),
	maxSpeed(maxSpeed),
	areaOfEffect(areaOfEffect),
	age(0),
	oldSmoke(pos),
	target(target),
	decoyTarget(0),
	drawTrail(true),
	numParts(0),
	targPos(targetPos),
	isWobbling(weaponDef? (weaponDef->wobble > 0): false),
	wobbleDir(0, 0, 0),
	wobbleTime(1),
	wobbleDif(0, 0, 0),
	isDancing(weaponDef? (weaponDef->dance > 0): false),
	danceTime(1),
	danceMove(0, 0, 0),
	danceCenter(0, 0, 0),
	extraHeightTime(0)
{
	curSpeed = speed.Length();
	dir.Normalize();
	oldDir = dir;

	if (target) {
		AddDeathDependence(target);
	}

	SetRadius(0.0f);

	if (weaponDef) {
		s3domodel = LoadModel(weaponDef);
		if (s3domodel) {
			SetRadius(s3domodel->radius);
		}
	}

	drawRadius = radius + maxSpeed * 8;

	float3 camDir = (pos - camera->pos).Normalize();
	if ((camera->pos.distance(pos) * 0.2f + (1 - fabs(camDir.dot(dir))) * 3000) < 200) {
		drawTrail = false;
	}

	castShadow = true;

#ifdef TRACE_SYNC
	tracefile << "New missile: ";
	tracefile << pos.x << " " << pos.y << " " << pos.z << " " << speed.x << " " << speed.y << " " << speed.z << "\n";
#endif

	if (target) {
		target->IncomingMissile(this);
	}

	if ((weaponDef) && (weaponDef->trajectoryHeight > 0)) {
		float dist = pos.distance(targPos);
		extraHeight = (dist * weaponDef->trajectoryHeight);

		if (dist < maxSpeed)
			dist = maxSpeed;

		extraHeightTime = int(dist / maxSpeed);
		extraHeightDecay = extraHeight / extraHeightTime;
	}


	if (cegTag.size() > 0) {
		ceg.Load(explGenHandler, cegTag);
	}
}

CMissileProjectile::~CMissileProjectile(void)
{
}

void CMissileProjectile::DependentDied(CObject* o)
{
	if (o == target)
		target = 0;
	if (o == decoyTarget)
		decoyTarget = 0;
	CWeaponProjectile::DependentDied(o);
}

void CMissileProjectile::Collision()
{
	float h = ground->GetHeight2(pos.x, pos.z);

	if (weaponDef->waterweapon && h < pos.y) {
		// let waterweapons travel in water
		return;
	}

	if (h > pos.y && fabs(speed.y) > 0.001f) {
		pos -= speed * std::min(1.0f,float((h - pos.y) / fabs(speed.y)));
	}

	if (weaponDef->visuals.smokeTrail) {
		new CSmokeTrailProjectile(pos, oldSmoke, dir, oldDir, owner(), false, true, 7, Smoke_Time, 0.6f, drawTrail, 0, weaponDef->visuals.texture2);
	}

	CWeaponProjectile::Collision();
	oldSmoke = pos;
}

void CMissileProjectile::Collision(CUnit *unit)
{
	if (weaponDef->visuals.smokeTrail) {
		new CSmokeTrailProjectile(pos, oldSmoke, dir, oldDir, owner(), false, true, 7, Smoke_Time, 0.6f, drawTrail, 0, weaponDef->visuals.texture2);
	}

	CWeaponProjectile::Collision(unit);
	oldSmoke = pos;
}


void CMissileProjectile::Update(void)
{
	ttl--;
	if (ttl > 0) {
		if (curSpeed < maxSpeed)
			curSpeed += weaponDef->weaponacceleration;

		float3 targSpeed(0, 0, 0);

		if (weaponDef->tracks && (decoyTarget || target)) {
			if (decoyTarget) {
				targPos = decoyTarget->pos;
				targSpeed = decoyTarget->speed;
			} else {
				targSpeed = target->speed;
				if ((target->physicalState == CSolidObject::Flying && (target->midPos-pos).SqLength() < 150 * 150) || !owner())
					targPos = target->midPos;
				else
					targPos = helper->GetUnitErrorPos(target, owner()->allyteam);
			}
		}


		if (isWobbling) {
			--wobbleTime;
			if (wobbleTime == 0) {
				float3 newWob = gs->randVector();
				wobbleDif = (newWob - wobbleDir) * (1.0f / 16);
				wobbleTime = 16;
			}
			wobbleDir += wobbleDif;
			dir += wobbleDir * weaponDef->wobble * (owner()? (1 - owner()->limExperience * 0.5f): 1);
			dir.Normalize();
		}

		if (isDancing) {
			--danceTime;
			if (danceTime <= 0) {
				danceMove = gs->randVector() * weaponDef->dance - danceCenter;
				danceCenter += danceMove;
				danceTime = 8;
			}
			pos += danceMove;
		}


		float3 orgTargPos(targPos);
		float3 targetDir = (targPos - pos).Normalize();
		float dist = targPos.distance(pos);

		if (dist == 0) {
			dist = 0.1f;
		}

		if (extraHeightTime > 0) {
			extraHeight -= extraHeightDecay;
			--extraHeightTime;

			targPos.y += extraHeight;

			if (dir.y <= 0.0f) {
				// missile has reached apex, smoothly transition
				// to targetDir (can still overshoot when target
				// is too close or height difference too large)
				const float horDiff = (targPos - pos).Length2D() + 0.01f;
				const float verDiff = (targPos.y - pos.y) + 0.01f;
				const float dirDiff = fabs(targetDir.y - dir.y);
				const float ratio = fabs(verDiff / horDiff);
				dir.y -= (dirDiff * ratio);
			} else {
				// missile is still ascending
				dir.y -= (extraHeightDecay / dist);
			}
		}


		float3 dif(targPos + targSpeed * (dist / maxSpeed) * 0.7f - pos);
		dif.Normalize();
		float3 dif2 = dif - dir;
		float tracking = weaponDef->turnrate;

		if (dif2.SqLength() < Square(tracking)) {
			dir = dif;
		} else {
			dif2 -= (dir * (dif2.dot(dir)));
			dif2.Normalize();
			dir += (dif2 * tracking);
			dir.Normalize();
		}

		speed = dir * curSpeed;
		targPos = orgTargPos;

		if (cegTag.size() > 0) {
			ceg.Explosion(pos, ttl, areaOfEffect, 0x0, 0.0f, 0x0, dir);
		}
	} else {
		if (weaponDef->selfExplode) {
			Collision();
		} else {
			// only when TTL <= 0 do we (missiles)
			// get influenced by gravity and drag
			speed *= 0.98f;
			speed.y += gravity;
			dir = speed;
			dir.Normalize();
		}
	}

	pos += speed;
	age++;
	numParts++;


	if (weaponDef->visuals.smokeTrail && !(age & 7)) {
		CSmokeTrailProjectile* tp = new CSmokeTrailProjectile(pos, oldSmoke,
			dir, oldDir, owner(), age == 8, false, 7, Smoke_Time, 0.6f, drawTrail, 0,
			weaponDef->visuals.texture2);
		oldSmoke = pos;
		oldDir = dir;
		numParts = 0;
		useAirLos = tp->useAirLos;

		if (!drawTrail) {
			float3 camDir = (pos - camera->pos).Normalize();
			if ((camera->pos.distance(pos) * 0.2f + (1 - fabs(camDir.dot(dir))) * 3000) > 300)
				drawTrail = true;
		}
	}

	UpdateGroundBounce();
}

void CMissileProjectile::UpdateGroundBounce() {
	float3 tempSpeed = speed;
	CWeaponProjectile::UpdateGroundBounce();

	if (tempSpeed != speed) {
		curSpeed = speed.Length();
		dir = speed;
		dir.Normalize();
	}
}

void CMissileProjectile::Draw(void)
{
	inArray = true;
	float age2 = (age & 7) + gu->timeOffset;

	float color = 0.6f;
	unsigned char col[4];

	va->EnlargeArrays(8+4*numParts,0,VA_SIZE_TC);
	if (weaponDef->visuals.smokeTrail) {
		if (drawTrail) {
			// draw the trail as a single quad
			float3 dif(drawPos - camera->pos);
			dif.ANormalize();
			float3 dir1(dif.cross(dir));
			dir1.ANormalize();
			float3 dif2(oldSmoke - camera->pos);
			dif2.ANormalize();
			float3 dir2(dif2.cross(oldDir));
			dir2.ANormalize();

			float a1 = (1.0f / (Smoke_Time)) * 255;
			a1 *= 0.7f + fabs(dif.dot(dir));
			float alpha = std::min(255.0f, std::max(0.0f, a1));
			col[0] = (unsigned char) (color * alpha);
			col[1] = (unsigned char) (color * alpha);
			col[2] = (unsigned char) (color * alpha);
			col[3] = (unsigned char) alpha;

			unsigned char col2[4];
			float a2 = (1 - float(age2) / (Smoke_Time)) * 255;

			if (age < 8)
				a2 = 0;

			a2 *= 0.7f + fabs(dif2.dot(oldDir));
			alpha = std::min(255.0f, std::max(0.0f, a2));
			col2[0] = (unsigned char) (color * alpha);
			col2[1] = (unsigned char) (color * alpha);
			col2[2] = (unsigned char) (color * alpha);
			col2[3] = (unsigned char) alpha;

			float size = 1.0f;
			float size2 = (1 + (age2 * (1 / Smoke_Time)) * 7);
			float txs = weaponDef->visuals.texture2->xend - (weaponDef->visuals.texture2->xend - weaponDef->visuals.texture2->xstart) * (age2 / 8.0f);

			va->AddVertexQTC(drawPos - dir1 * size,  txs,                               weaponDef->visuals.texture2->ystart, col);
			va->AddVertexQTC(drawPos + dir1 * size,  txs,                               weaponDef->visuals.texture2->yend,   col);
			va->AddVertexQTC(oldSmoke + dir2 * size2, weaponDef->visuals.texture2->xend, weaponDef->visuals.texture2->yend,   col2);
			va->AddVertexQTC(oldSmoke - dir2 * size2, weaponDef->visuals.texture2->xend, weaponDef->visuals.texture2->ystart, col2);
		} else {
			// draw the trail as particles
			float dist = pos.distance(oldSmoke);
			float3 dirpos1 = pos - dir * dist * 0.33f;
			float3 dirpos2 = oldSmoke + oldDir * dist * 0.33f;

			for (int a = 0; a < numParts; ++a) { //! CAUTION: loop count must match EnlargeArrays above
				float alpha = 255.0f;
				col[0] = (unsigned char) (color * alpha);
				col[1] = (unsigned char) (color * alpha);
				col[2] = (unsigned char) (color * alpha);
				col[3] = (unsigned char) alpha;

				float size = (1 + (a * (1 / Smoke_Time)) * 7);
				float3 pos1 = CalcBeizer(float(a) / (numParts), pos, dirpos1, dirpos2, oldSmoke);

				va->AddVertexQTC(pos1 + ( camera->up + camera->right) * size, ph->smoketex[0].xstart, ph->smoketex[0].ystart, col);
				va->AddVertexQTC(pos1 + ( camera->up - camera->right) * size, ph->smoketex[0].xend,   ph->smoketex[0].ystart, col);
				va->AddVertexQTC(pos1 + (-camera->up - camera->right) * size, ph->smoketex[0].xend,   ph->smoketex[0].ystart, col);
				va->AddVertexQTC(pos1 + (-camera->up + camera->right) * size, ph->smoketex[0].xstart, ph->smoketex[0].ystart, col);
			}
		}
	}

	// rocket flare
	col[0] = 255;
	col[1] = 210;
	col[2] = 180;
	col[3] = 1;
	float fsize = radius * 0.4f;
	va->AddVertexQTC(drawPos - camera->right * fsize-camera->up * fsize, weaponDef->visuals.texture1->xstart, weaponDef->visuals.texture1->ystart, col);
	va->AddVertexQTC(drawPos + camera->right * fsize-camera->up * fsize, weaponDef->visuals.texture1->xend,   weaponDef->visuals.texture1->ystart, col);
	va->AddVertexQTC(drawPos + camera->right * fsize+camera->up * fsize, weaponDef->visuals.texture1->xend,   weaponDef->visuals.texture1->yend,   col);
	va->AddVertexQTC(drawPos - camera->right * fsize+camera->up * fsize, weaponDef->visuals.texture1->xstart, weaponDef->visuals.texture1->yend,   col);

/*	col[0]=200;
	col[1]=200;
	col[2]=200;
	col[3]=255;
	float3 r=dir.cross(UpVector);
	r.Normalize();
	float3 u=dir.cross(r);
	va->AddVertexTC(interPos+r*1.0f,1.0f/16,1.0f/16,col);
	va->AddVertexTC(interPos-r*1.0f,1.0f/16,1.0f/16,col);
	va->AddVertexTC(interPos+dir*9,1.0f/16,1.0f/16,col);
	va->AddVertexTC(interPos+dir*9,1.0f/16,1.0f/16,col);

	va->AddVertexTC(interPos+u*1.0f,1.0f/16,1.0f/16,col);
	va->AddVertexTC(interPos-u*1.0f,1.0f/16,1.0f/16,col);
	va->AddVertexTC(interPos+dir*9,1.0f/16,1.0f/16,col);
	va->AddVertexTC(interPos+dir*9,1.0f/16,1.0f/16,col);*/
}

void CMissileProjectile::DrawUnitPart(void)
{
	glPushMatrix();
	float3 rightdir;

	if (dir.y != 1) {
		rightdir = dir.cross(UpVector);
	} else {
		rightdir = float3(1, 0, 0);
	}

	rightdir.Normalize();
	float3 updir = rightdir.cross(dir);

	CMatrix44f transMatrix(drawPos + dir * radius * 0.9f,-rightdir,updir,dir);

	glMultMatrixf(&transMatrix[0]);
	glCallList(s3domodel->rootobject->displist); // dont cache displists because of delayed loading (GML)

	glPopMatrix();
}

int CMissileProjectile::ShieldRepulse(CPlasmaRepulser* shield,float3 shieldPos, float shieldForce, float shieldMaxSpeed)
{
	float3 sdir = pos - shieldPos;
	sdir.Normalize();

	if (ttl > 0) {
		// steer away twice as fast as we can steer toward target
		float3 dif2 = sdir - dir;
		float tracking = std::max(shieldForce * 0.05f, weaponDef->turnrate * 2);

		if (dif2.SqLength() < Square(tracking)) {
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

void CMissileProjectile::DrawS3O(void)
{
	unitDrawer->SetTeamColour(colorTeam);
	DrawUnitPart();
}
