/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "Game/Camera.h"
#include "Game/GameHelper.h"
#include "Map/Ground.h"
#include "MissileProjectile.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Misc/GeometricObjects.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Rendering/Env/Particles/Classes/SmokeTrailProjectile.h"
#include "Sim/Units/Unit.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "System/Matrix44f.h"
#include "System/myMath.h"
#include "System/Sync/SyncTracer.h"

const float CMissileProjectile::SMOKE_TIME = 60.0f;

CR_BIND_DERIVED(CMissileProjectile, CWeaponProjectile, )

CR_REG_METADATA(CMissileProjectile,(
	CR_SETFLAG(CF_Synced),
	CR_MEMBER(ignoreError),
	CR_MEMBER(maxSpeed),
	// CR_MEMBER(ttl),
	CR_MEMBER(age),
	CR_MEMBER(oldSmoke),
	CR_MEMBER(oldDir),
	CR_MEMBER(numParts),
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
	CR_IGNORED(smokeTrail)
))


CMissileProjectile::CMissileProjectile(const ProjectileParams& params): CWeaponProjectile(params)
	, ignoreError(false)
	, maxSpeed(0.0f)
	, extraHeight(0.0f)
	, extraHeightDecay(0.0f)

	, age(0)
	, numParts(0)
	, extraHeightTime(0)

	, isDancing(false)
	, isWobbling(false)

	, danceTime(1)
	, wobbleTime(1)

	, oldSmoke(pos)
	, oldDir(dir)
	, smokeTrail(nullptr)
{
	projectileType = WEAPON_MISSILE_PROJECTILE;


	if (model != NULL) {
		SetRadiusAndHeight(model);
	}
	if (weaponDef != NULL) {
		maxSpeed = weaponDef->projectilespeed;
		isDancing = (weaponDef->dance > 0);
		isWobbling = (weaponDef->wobble > 0);

		if (weaponDef->trajectoryHeight > 0.0f) {
			const float dist = pos.distance(targetPos);

			assert(maxSpeed > 0.0f);
			assert((std::max(dist, maxSpeed) / maxSpeed) >= 1.0f);

			extraHeight = (dist * weaponDef->trajectoryHeight);
			extraHeightTime = int(std::max(dist, maxSpeed) / maxSpeed);
			extraHeightDecay = extraHeight / extraHeightTime;
		}
	}

	drawRadius = radius + maxSpeed * 8;
	castShadow = true;

#ifdef TRACE_SYNC
	tracefile << "New missile: ";
	tracefile << pos.x << " " << pos.y << " " << pos.z << " " << speed.x << " " << speed.y << " " << speed.z << "\n";
#endif

	CUnit* u = dynamic_cast<CUnit*>(target);
	if (u != NULL) {
		u->IncomingMissile(this);
	}
}

void CMissileProjectile::Collision()
{
	if (weaponDef->visuals.smokeTrail) {
		new CSmokeTrailProjectile(owner(), pos, oldSmoke, dir, oldDir, false, true, 7, SMOKE_TIME, 0.6f, weaponDef->visuals.texture2);
	}

	CWeaponProjectile::Collision();
	oldSmoke = pos;
}

void CMissileProjectile::Collision(CUnit* unit)
{
	if (weaponDef->visuals.smokeTrail) {
		new CSmokeTrailProjectile(owner(), pos, oldSmoke, dir, oldDir, false, true, 7, SMOKE_TIME, 0.6f, weaponDef->visuals.texture2);
	}

	CWeaponProjectile::Collision(unit);
	oldSmoke = pos;
}

void CMissileProjectile::Collision(CFeature* feature)
{
	if (weaponDef->visuals.smokeTrail) {
		new CSmokeTrailProjectile(owner(), pos, oldSmoke, dir, oldDir, false, true, 7, SMOKE_TIME, 0.6f, weaponDef->visuals.texture2);
	}

	CWeaponProjectile::Collision(feature);
	oldSmoke = pos;
}

void CMissileProjectile::Update()
{
	const CUnit* own = owner();

	if (--ttl > 0) {
		if (!luaMoveCtrl) {
			float3 targetVel;

			if (speed.w < maxSpeed)
				speed.w += weaponDef->weaponacceleration;

			if (weaponDef->tracks && target != NULL) {
				const CSolidObject* so = dynamic_cast<const CSolidObject*>(target);

				if (so != nullptr) {
					targetPos = so->aimPos;
					targetVel = so->speed;


					if (allyteamID != -1 && !ignoreError) {
						const CUnit* u = dynamic_cast<const CUnit*>(so);

						if (u != nullptr)
							targetPos = u->GetErrorPos(allyteamID, true);

					}
				} else {
					targetPos = target->pos;
					const CWeaponProjectile* po = dynamic_cast<const CWeaponProjectile*>(target);
					if (po != nullptr)
						targetVel = po->speed;

				}
			}


			UpdateWobble();
			UpdateDance();

			const float3 orgTargPos = targetPos;
			const float3 targetDir = (targetPos - pos).SafeNormalize();
			const float dist = pos.distance(targetPos) + 0.1f;

			if (extraHeightTime > 0) {
				extraHeight -= extraHeightDecay;
				--extraHeightTime;

				targetPos.y += extraHeight;

				if (dir.y <= 0.0f) {
					// missile has reached apex, smoothly transition
					// to targetDir (can still overshoot when target
					// is too close or height difference too large)
					const float horDiff = (targetPos - pos).Length2D() + 0.01f;
					const float verDiff = (targetPos.y - pos.y) + 0.01f;
					const float dirDiff = math::fabs(targetDir.y - dir.y);
					const float ratio = math::fabs(verDiff / horDiff);

					dir.y -= (dirDiff * ratio);
				} else {
					// missile is still ascending
					dir.y -= (extraHeightDecay / dist);
				}
			}

			const float3 targetLeadVec = targetVel * (dist / maxSpeed) * 0.7f;
			const float3 targetLeadDir = (targetPos + targetLeadVec - pos).Normalize();

			float3 targetDirDif = targetLeadDir - dir;

			if (targetDirDif.SqLength() < Square(weaponDef->turnrate)) {
				dir = targetLeadDir;
			} else {
				targetDirDif = (targetDirDif - (dir * (targetDirDif.dot(dir)))).SafeNormalize();
				dir = (dir + (targetDirDif * weaponDef->turnrate)).SafeNormalize();
			}

			targetPos = orgTargPos;

			// dir and speed.w have changed, keep speed-vector in sync
			SetDirectionAndSpeed(dir, speed.w);
		}

		explGenHandler->GenExplosion(cegID, pos, dir, ttl, damages->damageAreaOfEffect, 0.0f, NULL, NULL);
	} else {
		if (weaponDef->selfExplode) {
			Collision();
		} else {
			// only when TTL <= 0 do we (missiles)
			// get influenced by gravity and drag
			if (!luaMoveCtrl) {
				SetVelocityAndSpeed((speed * 0.98f) + (UpVector * mygravity));
			}
		}
	}

	if (!luaMoveCtrl) {
		SetPosition(pos + speed);
	}

	age++;
	numParts++;

	if (weaponDef->visuals.smokeTrail) {
		if (smokeTrail) {
			smokeTrail->UpdateEndPos(pos, dir);
			oldSmoke = pos;
			oldDir = dir;
		}

		if ((age % 8) == 0) {
			smokeTrail = new CSmokeTrailProjectile(
				own,
				pos, oldSmoke,
				dir, oldDir,
				age == 8,
				false,
				7,
				SMOKE_TIME,
				0.6f,
				weaponDef->visuals.texture2
			);

			numParts = 0;
			useAirLos = smokeTrail->useAirLos;
		}
	}

	UpdateInterception();
	UpdateGroundBounce();
}

void CMissileProjectile::UpdateWobble() {
	if (!isWobbling)
		return;

	if ((--wobbleTime) <= 0) {
		wobbleDif = (gs->randVector() - wobbleDir) * (1.0f / 16);
		wobbleTime = 16;
	}

	float wobbleFact = weaponDef->wobble;

	if (owner() != NULL)
		wobbleFact *= CUnit::ExperienceScale(owner()->limExperience, weaponDef->ownerExpAccWeight);

	wobbleDir += wobbleDif;
	dir = (dir + wobbleDir * wobbleFact).Normalize();
}

void CMissileProjectile::UpdateDance() {
	if (!isDancing)
		return;

	if ((--danceTime) <= 0) {
		danceMove = gs->randVector() * weaponDef->dance - danceCenter;
		danceCenter += danceMove;
		danceTime = 8;
	}

	SetPosition(pos + danceMove);
}


void CMissileProjectile::UpdateGroundBounce() {
	if (luaMoveCtrl)
		return;

	const float4 oldSpeed = speed;

	CWeaponProjectile::UpdateGroundBounce();

	if (oldSpeed != speed) {
		SetVelocityAndSpeed(speed);
	}
}



void CMissileProjectile::Draw()
{
	inArray = true;

	// rocket flare
	const SColor lightYellow(255, 210, 180, 1);
	const float fsize = radius * 0.4f;
	va->EnlargeArrays(4, 0, VA_SIZE_TC);
	va->AddVertexQTC(drawPos - camera->GetRight() * fsize-camera->GetUp() * fsize, weaponDef->visuals.texture1->xstart, weaponDef->visuals.texture1->ystart, lightYellow);
	va->AddVertexQTC(drawPos + camera->GetRight() * fsize-camera->GetUp() * fsize, weaponDef->visuals.texture1->xend,   weaponDef->visuals.texture1->ystart, lightYellow);
	va->AddVertexQTC(drawPos + camera->GetRight() * fsize+camera->GetUp() * fsize, weaponDef->visuals.texture1->xend,   weaponDef->visuals.texture1->yend,   lightYellow);
	va->AddVertexQTC(drawPos - camera->GetRight() * fsize+camera->GetUp() * fsize, weaponDef->visuals.texture1->xstart, weaponDef->visuals.texture1->yend,   lightYellow);
}

int CMissileProjectile::ShieldRepulse(const float3& shieldPos, float shieldForce, float shieldMaxSpeed)
{
	if (luaMoveCtrl)
		return 0;

	if (ttl > 0) {
		const float3 sdir = (pos - shieldPos).SafeNormalize();
		// steer away twice as fast as we can steer toward target
		float3 dif2 = sdir - dir;

		const float tracking = std::max(shieldForce * 0.05f, weaponDef->turnrate * 2);

		if (dif2.SqLength() < Square(tracking)) {
			dir = sdir;
		} else {
			dif2 = (dif2 - (dir * (dif2.dot(dir)))).SafeNormalize();
			dir = (dir + (dif2 * tracking)).SafeNormalize();
		}

		return 2;
	}

	return 0;
}

int CMissileProjectile::GetProjectilesCount() const
{
	return 1;
}
