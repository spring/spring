/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "Game/Camera.h"
#include "LaserProjectile.h"
#include "Map/Ground.h"
#include "Rendering/Env/Particles/Classes/SimpleParticleSystem.h"
#include "Rendering/GL/RenderBuffers.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Weapons/WeaponDef.h"

CR_BIND_DERIVED(CLaserProjectile, CWeaponProjectile, )

CR_REG_METADATA(CLaserProjectile,(
	CR_SETFLAG(CF_Synced),
	CR_MEMBER(intensity),
	CR_MEMBER(color),
	CR_MEMBER(color2),
	CR_MEMBER(speedf),
	CR_MEMBER(maxLength),
	CR_MEMBER(curLength),
	CR_MEMBER(stayTime),
	CR_MEMBER(intensityFalloff),
	CR_MEMBER(midtexx)
))


CLaserProjectile::CLaserProjectile(const ProjectileParams& params): CWeaponProjectile(params)
	// NB: constant, assumes |speed=dir*projectileSpeed| never changes after creation
	, speedf(speed.w)
	, maxLength(0.0f)
	, curLength(0.0f)
	, intensity(0.0f)
	, intensityFalloff(0.0f)
	, midtexx(0.0f)

	, stayTime(0)
{
	projectileType = WEAPON_LASER_PROJECTILE;

	if (weaponDef != nullptr) {
		SetRadiusAndHeight(weaponDef->collisionSize, 0.0f);

		maxLength = weaponDef->duration * (speedf * GAME_SPEED);
		intensity = weaponDef->intensity;
		intensityFalloff = intensity * weaponDef->falloffRate;

		midtexx =
			(weaponDef->visuals.texture2->xstart +
			(weaponDef->visuals.texture2->xend - weaponDef->visuals.texture2->xstart) * 0.5f);

		color = weaponDef->visuals.color;
		color2 = weaponDef->visuals.color2;
	}

	drawRadius = maxLength;
}

void CLaserProjectile::Update()
{
	const float4 oldSpeed = speed;

	UpdateIntensity();
	UpdateLength();
	UpdateInterception();
	UpdatePos(oldSpeed);

	// pre-decrement ttl: if projectile has to live for N frames
	// we want to check for collisions only N (not N + 1) times!
	checkCol &= ((ttl -= 1) >= 0);
	deleteMe |= ((curLength <= 0.01f) && ( weaponDef->laserHardStop));
	deleteMe |= ((intensity <= 0.01f) && (!weaponDef->laserHardStop));
}

void CLaserProjectile::UpdateIntensity() {
	if (ttl > 0) {
		explGenHandler.GenExplosion(cegID, pos, speed, ttl, intensity, 0.0f, owner(), nullptr);
		return;
	}

	if (weaponDef->laserHardStop) {
		// bolt reached its max-range but wasn't fully extended yet
		if (curLength < maxLength && speed != ZeroVector)
			stayTime = 1 + int((maxLength - curLength) / speedf);

		SetVelocityAndSpeed(ZeroVector);
	} else {
		// fade out over the next 5 frames at most
		intensity -= std::max(intensityFalloff * 0.2f, 0.2f);
		intensity = std::max(intensity, 0.0f);
	}
}

void CLaserProjectile::UpdateLength() {
	if (speed != ZeroVector) {
		// expand bolt to maximum length if not
		// stopped / collided OR after hardstop
		curLength = std::min(curLength + speedf, maxLength);
	} else {
		// contract bolt to zero length after stayTime
		// expires (can be immediately if not hardstop)
		curLength = std::max(curLength - speedf * (stayTime == 0), 0.0f);
	}

	stayTime = std::max(stayTime - 1, 0);
}

void CLaserProjectile::UpdatePos(const float4& oldSpeed) {
	if (luaMoveCtrl)
		return;

	SetPosition(pos + speed);
	// note: this can change pos *and* speed
	UpdateGroundBounce();

	if (oldSpeed != speed) {
		SetVelocityAndSpeed(speed);
	}
}



void CLaserProjectile::CollisionCommon(const float3& oldPos) {
	// we will fade out over some time
	deleteMe = false;

	if (weaponDef->noExplode)
		return;

	checkCol = false;

	SetPosition(oldPos);
	SetVelocityAndSpeed(ZeroVector);

	if (curLength < maxLength) {
		stayTime = 1 + int((maxLength - curLength) / speedf);
	}
}

void CLaserProjectile::Collision(CUnit* unit)
{
	const float3 oldPos = pos;

	CWeaponProjectile::Collision(unit);
	CollisionCommon(oldPos);
}

void CLaserProjectile::Collision(CFeature* feature)
{
	const float3 oldPos = pos;

	CWeaponProjectile::Collision(feature);
	CollisionCommon(oldPos);
}

void CLaserProjectile::Collision()
{
	const float3 oldPos = pos;

	CWeaponProjectile::Collision();
	CollisionCommon(oldPos);
}



void CLaserProjectile::Draw()
{
	// dont draw if a 3d model has been defined for us
	if (model != nullptr)
		return;

	if (!validTextures[0])
		return;

	auto& rb = GetPrimaryRenderBuffer();

	float3 dif(pos - camera->GetPos());
	const float camDist = dif.LengthNormalize();

	float3 dir1(dif.cross(dir));
	dir1.Normalize();
	float3 dir2(dif.cross(dir1));

	const uint8_t col[4] = {
		(uint8_t)(color.x * intensity * 255),
		(uint8_t)(color.y * intensity * 255),
		(uint8_t)(color.z * intensity * 255),
		1 //intensity*255;
	};

	const uint8_t col2[4] = {
		(uint8_t)(color2.x * intensity * 255),
		(uint8_t)(color2.y * intensity * 255),
		(uint8_t)(color2.z * intensity * 255),
		1 //intensity*255;
	};

	const float size = weaponDef->visuals.thickness;
	const float coresize = size * weaponDef->visuals.corethickness;

	if (camDist < weaponDef->visuals.lodDistance) {
		const float3 pos2 = drawPos - (dir * curLength);
		float texStartOffset;
		float texEndOffset;
		if (checkCol) { // expanding or contracting?
			texStartOffset = 0.0f;
			texEndOffset   = (1.0f - (curLength / maxLength)) * (weaponDef->visuals.texture1->xstart - weaponDef->visuals.texture1->xend);
		} else {
			texStartOffset = (-1.0f + (curLength / maxLength) + ((float)stayTime * (speedf / maxLength))) * (weaponDef->visuals.texture1->xstart - weaponDef->visuals.texture1->xend);
			texEndOffset   = ((float)stayTime * (speedf / maxLength)) * (weaponDef->visuals.texture1->xstart - weaponDef->visuals.texture1->xend);
		}

		if (validTextures[2]) {
			rb.AddQuadTriangles(
				{ drawPos - (dir1 * size),                        midtexx,                             weaponDef->visuals.texture2->ystart, col },
				{ drawPos + (dir1 * size),                        midtexx,                             weaponDef->visuals.texture2->yend  , col },
				{ drawPos + (dir1 * size) - (dir2 * size),        weaponDef->visuals.texture2->xstart, weaponDef->visuals.texture2->yend  , col },
				{ drawPos - (dir1 * size) - (dir2 * size),        weaponDef->visuals.texture2->xstart, weaponDef->visuals.texture2->ystart, col }
			);

			rb.AddQuadTriangles(
				{ drawPos - (dir1 * coresize),                     midtexx,                             weaponDef->visuals.texture2->ystart, col2 },
				{ drawPos + (dir1 * coresize),                     midtexx,                             weaponDef->visuals.texture2->yend  , col2 },
				{ drawPos + (dir1 * coresize) - (dir2 * coresize), weaponDef->visuals.texture2->xstart, weaponDef->visuals.texture2->yend  , col2 },
				{ drawPos - (dir1 * coresize) - (dir2 * coresize), weaponDef->visuals.texture2->xstart, weaponDef->visuals.texture2->ystart, col2 }
			);
		}
		if (validTextures[1]) {
			rb.AddQuadTriangles(
				{ drawPos - (dir1 * size),     weaponDef->visuals.texture1->xstart + texStartOffset, weaponDef->visuals.texture1->ystart, col },
				{ drawPos + (dir1 * size),     weaponDef->visuals.texture1->xstart + texStartOffset, weaponDef->visuals.texture1->yend  , col },
				{ pos2    + (dir1 * size),     weaponDef->visuals.texture1->xend   + texEndOffset  , weaponDef->visuals.texture1->yend  , col },
				{ pos2    - (dir1 * size),     weaponDef->visuals.texture1->xend   + texEndOffset  , weaponDef->visuals.texture1->ystart, col }
			);

			rb.AddQuadTriangles(
				{ drawPos - (dir1 * coresize), weaponDef->visuals.texture1->xstart + texStartOffset, weaponDef->visuals.texture1->ystart, col2 },
				{ drawPos + (dir1 * coresize), weaponDef->visuals.texture1->xstart + texStartOffset, weaponDef->visuals.texture1->yend  , col2 },
				{ pos2    + (dir1 * coresize), weaponDef->visuals.texture1->xend   + texEndOffset  , weaponDef->visuals.texture1->yend  , col2 },
				{ pos2    - (dir1 * coresize), weaponDef->visuals.texture1->xend   + texEndOffset  , weaponDef->visuals.texture1->ystart, col2 }
			);
		}
		if (validTextures[2]) {
			rb.AddQuadTriangles(
				{ pos2 - (dir1 * size),                         midtexx,                           weaponDef->visuals.texture2->ystart, col },
				{ pos2 + (dir1 * size),                         midtexx,                           weaponDef->visuals.texture2->yend  , col },
				{ pos2 + (dir1 * size) + (dir2 * size),         weaponDef->visuals.texture2->xend, weaponDef->visuals.texture2->yend  , col },
				{ pos2 - (dir1 * size) + (dir2 * size),         weaponDef->visuals.texture2->xend, weaponDef->visuals.texture2->ystart, col }
			);

			rb.AddQuadTriangles(
				{ pos2 - (dir1 * coresize),                     midtexx,                           weaponDef->visuals.texture2->ystart, col2 },
				{ pos2 + (dir1 * coresize),                     midtexx,                           weaponDef->visuals.texture2->yend  , col2 },
				{ pos2 + (dir1 * coresize) + (dir2 * coresize), weaponDef->visuals.texture2->xend, weaponDef->visuals.texture2->yend  , col2 },
				{ pos2 - (dir1 * coresize) + (dir2 * coresize), weaponDef->visuals.texture2->xend, weaponDef->visuals.texture2->ystart, col2 }
			);
		}
	} else {
		const float3 pos1 = drawPos + (dir * (size * 0.5f));
		const float3 pos2 = pos1 - (dir * (curLength + size));
		float texStartOffset;
		float texEndOffset;

		if (checkCol) { // expanding or contracting?
			texStartOffset = 0;
			texEndOffset   = (1.0f - (curLength / maxLength)) * (weaponDef->visuals.texture1->xstart - weaponDef->visuals.texture1->xend);
		} else {
			texStartOffset = (-1.0f + (curLength / maxLength) + ((float)stayTime * (speedf / maxLength))) * (weaponDef->visuals.texture1->xstart - weaponDef->visuals.texture1->xend);
			texEndOffset   = ((float)stayTime * (speedf / maxLength)) * (weaponDef->visuals.texture1->xstart - weaponDef->visuals.texture1->xend);
		}
		if (validTextures[1]) {
			rb.AddQuadTriangles(
				{ pos1 - (dir1 * size),     weaponDef->visuals.texture1->xstart + texStartOffset, weaponDef->visuals.texture1->ystart, col },
				{ pos1 + (dir1 * size),     weaponDef->visuals.texture1->xstart + texStartOffset, weaponDef->visuals.texture1->yend  , col },
				{ pos2 + (dir1 * size),     weaponDef->visuals.texture1->xend +     texEndOffset, weaponDef->visuals.texture1->yend  , col },
				{ pos2 - (dir1 * size),     weaponDef->visuals.texture1->xend +     texEndOffset, weaponDef->visuals.texture1->ystart, col }
			);

			rb.AddQuadTriangles(
				{ pos1 - (dir1 * coresize), weaponDef->visuals.texture1->xstart + texStartOffset, weaponDef->visuals.texture1->ystart, col2 },
				{ pos1 + (dir1 * coresize), weaponDef->visuals.texture1->xstart + texStartOffset, weaponDef->visuals.texture1->yend  , col2 },
				{ pos2 + (dir1 * coresize), weaponDef->visuals.texture1->xend +     texEndOffset, weaponDef->visuals.texture1->yend  , col2 },
				{ pos2 - (dir1 * coresize), weaponDef->visuals.texture1->xend +     texEndOffset, weaponDef->visuals.texture1->ystart, col2 }
			);
		}
	}
}

int CLaserProjectile::ShieldRepulse(const float3& shieldPos, float shieldForce, float shieldMaxSpeed)
{
	if (luaMoveCtrl)
		return 0;

	const float3 rdir = (pos - shieldPos).Normalize();

	if (rdir.dot(speed) < 0.0f) {
		SetVelocityAndSpeed(speed - (rdir * rdir.dot(speed) * 2.0f));
		return 1;
	}

	return 0;
}

int CLaserProjectile::GetProjectilesCount() const
{
	return
		2 * validTextures[1] +
		4 * validTextures[2];
}
