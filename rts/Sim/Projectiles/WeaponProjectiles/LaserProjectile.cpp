/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "Game/Camera.h"
#include "LaserProjectile.h"
#include "Map/Ground.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Projectiles/Unsynced/SimpleParticleSystem.h"
#include "Sim/Weapons/WeaponDef.h"

#ifdef TRACE_SYNC
	#include "System/Sync/SyncTracer.h"
#endif

CR_BIND_DERIVED(CLaserProjectile, CWeaponProjectile, (ProjectileParams()));

CR_REG_METADATA(CLaserProjectile,(
	CR_SETFLAG(CF_Synced),
	CR_MEMBER(intensity),
	CR_MEMBER(color),
	CR_MEMBER(color2),
	CR_MEMBER(length),
	CR_MEMBER(curLength),
	CR_MEMBER(speedf),
	CR_MEMBER(stayTime),
	CR_MEMBER(intensityFalloff),
	CR_MEMBER(midtexx)
));

CLaserProjectile::CLaserProjectile(const ProjectileParams& params): CWeaponProjectile(params)
	, speedf(0.0f)

	, length(0.0f)
	, curLength(0.0f)
	, intensity(0.0f)
	, intensityFalloff(0.0f)
	, midtexx(0.0f)

	, stayTime(0)
{
	projectileType = WEAPON_LASER_PROJECTILE;

	speedf = speed.Length();
	dir = speed / speedf;

	if (weaponDef != NULL) {
		SetRadiusAndHeight(weaponDef->collisionSize, 0.0f);

		length = weaponDef->duration * (weaponDef->projectilespeed * GAME_SPEED);
		intensity = weaponDef->intensity;
		intensityFalloff = intensity * weaponDef->falloffRate;

		midtexx =
			(weaponDef->visuals.texture2->xstart +
			(weaponDef->visuals.texture2->xend - weaponDef->visuals.texture2->xstart) * 0.5f);

		color = weaponDef->visuals.color;
		color2 = weaponDef->visuals.color2;
	}

	drawRadius = length;

#ifdef TRACE_SYNC
	tracefile << "New laser: ";
	tracefile << pos.x << " " << pos.y << " " << pos.z << " " << speed.x << " " << speed.y << " " << speed.z << "\n";
#endif
}

void CLaserProjectile::Update()
{
	if (!luaMoveCtrl) {
		pos += speed;
	}

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

		if (curLength <= 0.0f) {
			deleteMe = true;
			curLength = 0.0f;
		}
	}


	if (--ttl > 0 && checkCol) {
		gCEG->Explosion(cegID, pos, ttl, intensity, NULL, 0.0f, NULL, speed);
	}

	if (weaponDef->laserHardStop) {
		if (ttl == 0 && checkCol) {
			checkCol = false;
			speed = ZeroVector;
			if (curLength < length) {
				// if the laser wasn't fully extended yet,
				// remember how long until it would have been
				// fully extended
				stayTime = 1 + (length - curLength) / speedf;
			}
		}
	} else {
		if (ttl < 5 && checkCol) {
			intensity -= (intensityFalloff * 0.2f);

			if (intensity <= 0.0f) {
				intensity = 0.0f;
				deleteMe = true;
			}
		}
	}

	if (luaMoveCtrl)
		return;

	float3 tempSpeed = speed;
	UpdateGroundBounce();
	UpdateInterception();

	if (tempSpeed != speed) {
		dir = speed;
		dir.Normalize();
	}
}



void CLaserProjectile::Collision(CUnit* unit)
{
	float3 oldPos = pos;
	CWeaponProjectile::Collision(unit);

	deleteMe = false; // we will fade out over some time
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
			stayTime = 1 + (length - curLength) / speedf;
		}
	}
}

void CLaserProjectile::Collision(CFeature* feature)
{
	float3 oldPos = pos;
	CWeaponProjectile::Collision(feature);

	// we will fade out over some time
	deleteMe = false;

	if (weaponDef->noExplode)
		return;

	checkCol = false;
	speed = ZeroVector;
	pos = oldPos;

	if (curLength < length) {
		// if the laser wasn't fully extended yet,
		// remember how long until it would have been
		// fully extended
		stayTime = 1 + (length - curLength) / speedf;
	}
}

void CLaserProjectile::Collision()
{
	float3 oldPos = pos;
	CWeaponProjectile::Collision();

	// we will fade out over some time
	deleteMe = false;

	if (weaponDef->noExplode)
		return;

	checkCol = false;
	speed = ZeroVector;
	pos = oldPos;

	if (curLength < length) {
		// if the laser wasn't fully extended yet,
		// remember how long until it would have been
		// fully extended
		stayTime = 1 + (length - curLength) / speedf;
	}
}



void CLaserProjectile::Draw()
{
	if (model) {
		// dont draw if a 3d model has been defined for us
		return;
	}

	inArray = true;
	float3 dif(pos - camera->pos);
	const float camDist = dif.Length();
	dif /= camDist;
	float3 dir1(dif.cross(dir));
	dir1.Normalize();
	float3 dir2(dif.cross(dir1));

	unsigned char col[4];
	col[0]  = (unsigned char) (color.x  * intensity * 255);
	col[1]  = (unsigned char) (color.y  * intensity * 255);
	col[2]  = (unsigned char) (color.z  * intensity * 255);
	col[3]  = 1; //intensity*255;
	unsigned char col2[4];
	col2[0] = (unsigned char) (color2.x * intensity * 255);
	col2[1] = (unsigned char) (color2.y * intensity * 255);
	col2[2] = (unsigned char) (color2.z * intensity * 255);
	col2[3] = 1; //intensity*255;

	const float size = weaponDef->visuals.thickness;
	const float coresize = size * weaponDef->visuals.corethickness;

	va->EnlargeArrays(32, 0, VA_SIZE_TC);
	if (camDist < weaponDef->visuals.lodDistance) {
		const float3 pos2 = drawPos - (dir * curLength);
		float texStartOffset;
		float texEndOffset;
		if (checkCol) { // expanding or contracting?
			texStartOffset = 0;
			texEndOffset   = (1.0f - (curLength / length)) * (weaponDef->visuals.texture1->xstart - weaponDef->visuals.texture1->xend);
		} else {
			texStartOffset = (-1.0f + (curLength / length) + ((float)stayTime * (speedf / length))) * (weaponDef->visuals.texture1->xstart - weaponDef->visuals.texture1->xend);
			texEndOffset   = ((float)stayTime * (speedf / length)) * (weaponDef->visuals.texture1->xstart - weaponDef->visuals.texture1->xend);
		}

		va->AddVertexQTC(drawPos - (dir1 * size),                       midtexx,                             weaponDef->visuals.texture2->ystart, col);
		va->AddVertexQTC(drawPos + (dir1 * size),                       midtexx,                             weaponDef->visuals.texture2->yend,   col);
		va->AddVertexQTC(drawPos + (dir1 * size) - (dir2 * size),       weaponDef->visuals.texture2->xstart, weaponDef->visuals.texture2->yend,   col);
		va->AddVertexQTC(drawPos - (dir1 * size) - (dir2 * size),       weaponDef->visuals.texture2->xstart, weaponDef->visuals.texture2->ystart, col);
		va->AddVertexQTC(drawPos - (dir1 * coresize),                   midtexx,                             weaponDef->visuals.texture2->ystart, col2);
		va->AddVertexQTC(drawPos + (dir1 * coresize),                   midtexx,                             weaponDef->visuals.texture2->yend,   col2);
		va->AddVertexQTC(drawPos + (dir1 * coresize)-(dir2 * coresize), weaponDef->visuals.texture2->xstart, weaponDef->visuals.texture2->yend,   col2);
		va->AddVertexQTC(drawPos - (dir1 * coresize)-(dir2 * coresize), weaponDef->visuals.texture2->xstart, weaponDef->visuals.texture2->ystart, col2);

		va->AddVertexQTC(drawPos - (dir1 * size),     weaponDef->visuals.texture1->xstart + texStartOffset, weaponDef->visuals.texture1->ystart, col);
		va->AddVertexQTC(drawPos + (dir1 * size),     weaponDef->visuals.texture1->xstart + texStartOffset, weaponDef->visuals.texture1->yend,   col);
		va->AddVertexQTC(pos2    + (dir1 * size),     weaponDef->visuals.texture1->xend   + texEndOffset,   weaponDef->visuals.texture1->yend,   col);
		va->AddVertexQTC(pos2    - (dir1 * size),     weaponDef->visuals.texture1->xend   + texEndOffset,   weaponDef->visuals.texture1->ystart, col);
		va->AddVertexQTC(drawPos - (dir1 * coresize), weaponDef->visuals.texture1->xstart + texStartOffset, weaponDef->visuals.texture1->ystart, col2);
		va->AddVertexQTC(drawPos + (dir1 * coresize), weaponDef->visuals.texture1->xstart + texStartOffset, weaponDef->visuals.texture1->yend,   col2);
		va->AddVertexQTC(pos2    + (dir1 * coresize), weaponDef->visuals.texture1->xend   + texEndOffset,   weaponDef->visuals.texture1->yend,   col2);
		va->AddVertexQTC(pos2    - (dir1 * coresize), weaponDef->visuals.texture1->xend   + texEndOffset,   weaponDef->visuals.texture1->ystart, col2);

		va->AddVertexQTC(pos2    - (dir1 * size),                         midtexx,                           weaponDef->visuals.texture2->ystart, col);
		va->AddVertexQTC(pos2    + (dir1 * size),                         midtexx,                           weaponDef->visuals.texture2->yend,   col);
		va->AddVertexQTC(pos2    + (dir1 * size) + (dir2 * size),         weaponDef->visuals.texture2->xend, weaponDef->visuals.texture2->yend,   col);
		va->AddVertexQTC(pos2    - (dir1 * size) + (dir2 * size),         weaponDef->visuals.texture2->xend, weaponDef->visuals.texture2->ystart, col);
		va->AddVertexQTC(pos2    - (dir1 * coresize),                     midtexx,                           weaponDef->visuals.texture2->ystart, col2);
		va->AddVertexQTC(pos2    + (dir1 * coresize),                     midtexx,                           weaponDef->visuals.texture2->yend,   col2);
		va->AddVertexQTC(pos2    + (dir1 * coresize) + (dir2 * coresize), weaponDef->visuals.texture2->xend, weaponDef->visuals.texture2->yend,   col2);
		va->AddVertexQTC(pos2    - (dir1 * coresize) + (dir2 * coresize), weaponDef->visuals.texture2->xend, weaponDef->visuals.texture2->ystart, col2);
	} else {
		const float3 pos1 = drawPos + (dir * (size * 0.5f));
		const float3 pos2 = pos1 - (dir * (curLength + size));
		float texStartOffset;
		float texEndOffset;
		if (checkCol) { // expanding or contracting?
			texStartOffset = 0;
			texEndOffset   = (1.0f - (curLength / length)) * (weaponDef->visuals.texture1->xstart - weaponDef->visuals.texture1->xend);
		} else {
			texStartOffset = (-1.0f + (curLength / length) + ((float)stayTime * (speedf / length))) * (weaponDef->visuals.texture1->xstart - weaponDef->visuals.texture1->xend);
			texEndOffset   = ((float)stayTime * (speedf / length)) * (weaponDef->visuals.texture1->xstart - weaponDef->visuals.texture1->xend);
		}

		va->AddVertexQTC(pos1 - (dir1 * size),     weaponDef->visuals.texture1->xstart + texStartOffset, weaponDef->visuals.texture1->ystart, col);
		va->AddVertexQTC(pos1 + (dir1 * size),     weaponDef->visuals.texture1->xstart + texStartOffset, weaponDef->visuals.texture1->yend,   col);
		va->AddVertexQTC(pos2 + (dir1 * size),     weaponDef->visuals.texture1->xend   + texEndOffset,   weaponDef->visuals.texture1->yend,   col);
		va->AddVertexQTC(pos2 - (dir1 * size),     weaponDef->visuals.texture1->xend   + texEndOffset,   weaponDef->visuals.texture1->ystart, col);
		va->AddVertexQTC(pos1 - (dir1 * coresize), weaponDef->visuals.texture1->xstart + texStartOffset, weaponDef->visuals.texture1->ystart, col2);
		va->AddVertexQTC(pos1 + (dir1 * coresize), weaponDef->visuals.texture1->xstart + texStartOffset, weaponDef->visuals.texture1->yend,   col2);
		va->AddVertexQTC(pos2 + (dir1 * coresize), weaponDef->visuals.texture1->xend   + texEndOffset,   weaponDef->visuals.texture1->yend,   col2);
		va->AddVertexQTC(pos2 - (dir1 * coresize), weaponDef->visuals.texture1->xend   + texEndOffset,   weaponDef->visuals.texture1->ystart, col2);
	}
}

int CLaserProjectile::ShieldRepulse(CPlasmaRepulser* shield, float3 shieldPos, float shieldForce, float shieldMaxSpeed)
{
	if (!luaMoveCtrl) {
		const float3 rdir = (pos - shieldPos).Normalize();

		if (rdir.dot(speed) < 0.0f) {
			speed -= (rdir * rdir.dot(speed) * 2.0f);
			dir = speed;
			dir.Normalize();
			return 1;
		}
	}

	return 0;
}
