/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "Game/Camera.h"
#include "Game/GameHelper.h"
#include "Game/TraceRay.h"
#include "Map/Ground.h"
#include "MissileProjectile.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ProjectileDrawer.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Models/3DModel.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Misc/GeometricObjects.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Projectiles/Unsynced/SmokeTrailProjectile.h"
#include "Sim/Units/Unit.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "System/Matrix44f.h"
#include "System/myMath.h"
#include "System/Sync/SyncTracer.h"

const float CMissileProjectile::SMOKE_TIME = 60.0f;

CR_BIND_DERIVED(CMissileProjectile, CWeaponProjectile, (ProjectileParams()));

CR_REG_METADATA(CMissileProjectile,(
	CR_SETFLAG(CF_Synced),
	CR_MEMBER(maxSpeed),
	// CR_MEMBER(ttl),
	CR_MEMBER(areaOfEffect),
	CR_MEMBER(age),
	CR_MEMBER(oldSmoke),
	CR_MEMBER(oldDir),
	CR_MEMBER(drawTrail),
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
	CR_MEMBER(extraHeightTime)
));

CMissileProjectile::CMissileProjectile(const ProjectileParams& params): CWeaponProjectile(params)
	, maxSpeed(0.0f)
	, areaOfEffect(0.0f)
	, extraHeight(0.0f)
	, extraHeightDecay(0.0f)

	, age(0)
	, numParts(0)
	, extraHeightTime(0)

	, drawTrail(true)
	, isDancing(false)
	, isWobbling(false)

	, danceTime(1)
	, wobbleTime(1)

	, oldSmoke(pos)
{
	projectileType = WEAPON_MISSILE_PROJECTILE;

	oldDir = dir;

	if (model != NULL) {
		SetRadiusAndHeight(model);
	}
	if (weaponDef != NULL) {
		maxSpeed = weaponDef->projectilespeed;
		areaOfEffect = weaponDef->damageAreaOfEffect;

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

	float3 camDir = (pos - camera->GetPos()).ANormalize();

	if ((camera->GetPos().distance(pos) * 0.2f + (1 - math::fabs(camDir.dot(dir))) * 3000) < 200) {
		drawTrail = false;
	}

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
		new CSmokeTrailProjectile(owner(), pos, oldSmoke, dir, oldDir, false, true, 7, SMOKE_TIME, 0.6f, drawTrail, 0, weaponDef->visuals.texture2);
	}

	CWeaponProjectile::Collision();
	oldSmoke = pos;
}

void CMissileProjectile::Collision(CUnit* unit)
{
	if (weaponDef->visuals.smokeTrail) {
		new CSmokeTrailProjectile(owner(), pos, oldSmoke, dir, oldDir, false, true, 7, SMOKE_TIME, 0.6f, drawTrail, 0, weaponDef->visuals.texture2);
	}

	CWeaponProjectile::Collision(unit);
	oldSmoke = pos;
}

void CMissileProjectile::Collision(CFeature* feature)
{
	if (weaponDef->visuals.smokeTrail) {
		new CSmokeTrailProjectile(owner(), pos, oldSmoke, dir, oldDir, false, true, 7, SMOKE_TIME, 0.6f, drawTrail, 0, weaponDef->visuals.texture2);
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
				const CWeaponProjectile* po = dynamic_cast<const CWeaponProjectile*>(target);

				targetPos = target->pos;

				if (so != NULL) {
					targetPos = so->aimPos;
					targetVel = so->speed;

					if (own != NULL && pos.SqDistance(so->aimPos) > Square(150.0f)) {
						// if we have an owner and our target is a unit,
						// set target-position to its error-position for
						// our owner's allyteam
						const CUnit* tgt = dynamic_cast<const CUnit*>(so);

						if (tgt != NULL) {
							targetPos = tgt->GetErrorPos(own->allyteam, true);
						}
					}
				}
				if (po != NULL) {
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

		explGenHandler->GenExplosion(cegID, pos, dir, ttl, areaOfEffect, 0.0f, NULL, NULL);
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

	if (weaponDef->visuals.smokeTrail && !(age & 7)) {
		CSmokeTrailProjectile* tp = new CSmokeTrailProjectile(
			own,
			pos, oldSmoke,
			dir, oldDir,
			age == 8,
			false,
			7,
			SMOKE_TIME,
			0.6f,
			drawTrail,
			NULL,
			weaponDef->visuals.texture2
		);

		oldSmoke = pos;
		oldDir = dir;
		numParts = 0;
		useAirLos = tp->useAirLos;

		if (!drawTrail) {
			const float3 camDir = (pos - camera->GetPos()).ANormalize();

			if ((camera->GetPos().distance(pos) * 0.2f + (1 - math::fabs(camDir.dot(dir))) * 3000) > 300) {
				drawTrail = true;
			}
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
	const float age2 = (age & 7) + globalRendering->timeOffset;

	unsigned char col[4];

	va->EnlargeArrays(8 + (4 * numParts), 0, VA_SIZE_TC);
	if (weaponDef->visuals.smokeTrail) {
		const float color = 0.6f;
		if (drawTrail) {
			// draw the trail as a single quad
			const float3 dif = (drawPos - camera->GetPos()).ANormalize();
			const float3 dir1 = (dif.cross(dir)).ANormalize();
			const float3 dif2 = (oldSmoke - camera->GetPos()).ANormalize();
			const float3 dir2 = (dif2.cross(oldDir)).ANormalize();

			const float a1 = ((1.0f / (SMOKE_TIME)) * 255) * (0.7f + math::fabs(dif.dot(dir)));
			const float alpha1 = std::min(255.0f, std::max(0.0f, a1));
			col[0] = (unsigned char) (color * alpha1);
			col[1] = (unsigned char) (color * alpha1);
			col[2] = (unsigned char) (color * alpha1);
			col[3] = (unsigned char) alpha1;

			unsigned char col2[4];
			float a2 = (1 - float(age2) / (SMOKE_TIME)) * 255;

			if (age < 8)
				a2 = 0.0f;

			a2 *= (0.7f + math::fabs(dif2.dot(oldDir)));
			const float alpha2 = std::min(255.0f, std::max(0.0f, a2));
			col2[0] = (unsigned char) (color * alpha2);
			col2[1] = (unsigned char) (color * alpha2);
			col2[2] = (unsigned char) (color * alpha2);
			col2[3] = (unsigned char) alpha2;

			const float size = 1.0f;
			const float size2 = (1 + (age2 * (1 / SMOKE_TIME)) * 7);
			const float txs = weaponDef->visuals.texture2->xend - (weaponDef->visuals.texture2->xend - weaponDef->visuals.texture2->xstart) * (age2 / 8.0f);

			va->AddVertexQTC(drawPos  - dir1 * size,  txs,                               weaponDef->visuals.texture2->ystart, col);
			va->AddVertexQTC(drawPos  + dir1 * size,  txs,                               weaponDef->visuals.texture2->yend,   col);
			va->AddVertexQTC(oldSmoke + dir2 * size2, weaponDef->visuals.texture2->xend, weaponDef->visuals.texture2->yend,   col2);
			va->AddVertexQTC(oldSmoke - dir2 * size2, weaponDef->visuals.texture2->xend, weaponDef->visuals.texture2->ystart, col2);
		} else {
			// draw the trail as particles
			const float dist = pos.distance(oldSmoke);
			const float3 dirpos1 = pos - dir * dist * 0.33f;
			const float3 dirpos2 = oldSmoke + oldDir * dist * 0.33f;

			for (int a = 0; a < numParts; ++a) { //! CAUTION: loop count must match EnlargeArrays above
				const float alpha = 255.0f;
				col[0] = (unsigned char) (color * alpha);
				col[1] = (unsigned char) (color * alpha);
				col[2] = (unsigned char) (color * alpha);
				col[3] = (unsigned char) alpha;

				const float size = (1 + (a * (1 / SMOKE_TIME)) * 7);
				float3 pos1 = CalcBeizer(float(a) / (numParts), pos, dirpos1, dirpos2, oldSmoke);

				#define st projectileDrawer->smoketex[0]
				va->AddVertexQTC(pos1 + ( camera->up + camera->right) * size, st->xstart, st->ystart, col);
				va->AddVertexQTC(pos1 + ( camera->up - camera->right) * size, st->xend,   st->ystart, col);
				va->AddVertexQTC(pos1 + (-camera->up - camera->right) * size, st->xend,   st->ystart, col);
				va->AddVertexQTC(pos1 + (-camera->up + camera->right) * size, st->xstart, st->ystart, col);
				#undef st
			}
		}
	}

	// rocket flare
	col[0] = 255;
	col[1] = 210;
	col[2] = 180;
	col[3] = 1;
	const float fsize = radius * 0.4f;
	va->AddVertexQTC(drawPos - camera->right * fsize-camera->up * fsize, weaponDef->visuals.texture1->xstart, weaponDef->visuals.texture1->ystart, col);
	va->AddVertexQTC(drawPos + camera->right * fsize-camera->up * fsize, weaponDef->visuals.texture1->xend,   weaponDef->visuals.texture1->ystart, col);
	va->AddVertexQTC(drawPos + camera->right * fsize+camera->up * fsize, weaponDef->visuals.texture1->xend,   weaponDef->visuals.texture1->yend,   col);
	va->AddVertexQTC(drawPos - camera->right * fsize+camera->up * fsize, weaponDef->visuals.texture1->xstart, weaponDef->visuals.texture1->yend,   col);
}

int CMissileProjectile::ShieldRepulse(
	CPlasmaRepulser* shield,
	float3 shieldPos,
	float shieldForce,
	float shieldMaxSpeed
) {
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
