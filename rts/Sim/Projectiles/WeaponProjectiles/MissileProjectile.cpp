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
	CR_MEMBER(curSpeed),
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
	, curSpeed(0.0f)
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

	curSpeed = speed.Length();
	dir = (curSpeed > 0.0f) ? speed / curSpeed : ZeroVector;
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

	float3 camDir = (pos - camera->pos).ANormalize();

	if ((camera->pos.distance(pos) * 0.2f + (1 - math::fabs(camDir.dot(dir))) * 3000) < 200) {
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
		new CSmokeTrailProjectile(pos, oldSmoke, dir, oldDir, owner(), false, true, 7, SMOKE_TIME, 0.6f, drawTrail, 0, weaponDef->visuals.texture2);
	}

	CWeaponProjectile::Collision();
	oldSmoke = pos;
}

void CMissileProjectile::Collision(CUnit* unit)
{
	if (weaponDef->visuals.smokeTrail) {
		new CSmokeTrailProjectile(pos, oldSmoke, dir, oldDir, owner(), false, true, 7, SMOKE_TIME, 0.6f, drawTrail, 0, weaponDef->visuals.texture2);
	}

	CWeaponProjectile::Collision(unit);
	oldSmoke = pos;
}

void CMissileProjectile::Collision(CFeature* feature)
{
	if (weaponDef->visuals.smokeTrail) {
		new CSmokeTrailProjectile(pos, oldSmoke, dir, oldDir, owner(), false, true, 7, SMOKE_TIME, 0.6f, drawTrail, 0, weaponDef->visuals.texture2);
	}

	CWeaponProjectile::Collision(feature);
	oldSmoke = pos;
}

void CMissileProjectile::Update()
{
	if (--ttl > 0) {
		if (!luaMoveCtrl) {
			if (curSpeed < maxSpeed) {
				curSpeed += weaponDef->weaponacceleration;
			}

			float3 targSpeed(ZeroVector);

			if (weaponDef->tracks && target) {
				CSolidObject* so = dynamic_cast<CSolidObject*>(target);
				CWeaponProjectile* po = dynamic_cast<CWeaponProjectile*>(target);

				targetPos = target->pos;
				if (so) {
					targetPos = so->aimPos;
					targSpeed = so->speed;

					if (owner()) {
						CUnit* u = dynamic_cast<CUnit*>(so);
						if (u) {
							targetPos = CGameHelper::GetUnitErrorPos(u, owner()->allyteam, true);
						}
					}
				} if (po) {
					targSpeed = po->speed;
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

				dir += wobbleDir * weaponDef->wobble * (owner()? (1.0f - owner()->limExperience * 0.5f): 1);
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


			float3 dif = (targetPos + targSpeed * (dist / maxSpeed) * 0.7f - pos).SafeNormalize();
			float3 dif2 = dif - dir;

			if (dif2.SqLength() < Square(weaponDef->turnrate)) {
				dir = dif;
			} else {
				dif2 -= (dir * (dif2.dot(dir)));
				dif2.SafeNormalize();
				dir += (dif2 * weaponDef->turnrate);
				dir.SafeNormalize();
			}

			targetPos = orgTargPos;
			speed = dir * curSpeed;
		}

		gCEG->Explosion(cegID, pos, ttl, areaOfEffect, NULL, 0.0f, NULL, dir);
	} else {
		if (weaponDef->selfExplode) {
			Collision();
		} else {
			// only when TTL <= 0 do we (missiles)
			// get influenced by gravity and drag
			if (!luaMoveCtrl) {
				speed *= 0.98f;
				speed.y += mygravity;
				dir = speed;
				dir.SafeNormalize();
			}
		}
	}

	if (!luaMoveCtrl) {
		pos += speed;
	}

	age++;
	numParts++;
	if (weaponDef->visuals.smokeTrail && !(age & 7)) {
		CSmokeTrailProjectile* tp = new CSmokeTrailProjectile(
			pos, oldSmoke,
			dir, oldDir, owner(), age == 8, false, 7, SMOKE_TIME, 0.6f, drawTrail, 0,
			weaponDef->visuals.texture2
		);

		oldSmoke = pos;
		oldDir = dir;
		numParts = 0;
		useAirLos = tp->useAirLos;

		if (!drawTrail) {
			const float3 camDir = (pos - camera->pos).ANormalize();

			if ((camera->pos.distance(pos) * 0.2f + (1 - math::fabs(camDir.dot(dir))) * 3000) > 300) {
				drawTrail = true;
			}
		}
	}

	UpdateInterception();
	UpdateGroundBounce();
}

void CMissileProjectile::UpdateGroundBounce() {
	if (luaMoveCtrl) {
		return;
	}

	const float3 tempSpeed = speed;
	CWeaponProjectile::UpdateGroundBounce();

	if (tempSpeed != speed) {
		curSpeed = speed.Length();
		dir = speed;
		dir.SafeNormalize();
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
			float3 dif(drawPos - camera->pos);
			dif.ANormalize();
			float3 dir1(dif.cross(dir));
			dir1.ANormalize();
			float3 dif2(oldSmoke - camera->pos);
			dif2.ANormalize();
			float3 dir2(dif2.cross(oldDir));
			dir2.ANormalize();

			float a1 = (1.0f / (SMOKE_TIME)) * 255;
			a1 *= 0.7f + math::fabs(dif.dot(dir));
			const float alpha1 = std::min(255.0f, std::max(0.0f, a1));
			col[0] = (unsigned char) (color * alpha1);
			col[1] = (unsigned char) (color * alpha1);
			col[2] = (unsigned char) (color * alpha1);
			col[3] = (unsigned char) alpha1;

			unsigned char col2[4];
			float a2 = (1 - float(age2) / (SMOKE_TIME)) * 255;

			if (age < 8) {
				a2 = 0;
			}

			a2 *= 0.7f + math::fabs(dif2.dot(oldDir));
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

int CMissileProjectile::ShieldRepulse(CPlasmaRepulser* shield, float3 shieldPos, float shieldForce, float shieldMaxSpeed)
{
	if (!luaMoveCtrl) {
		if (ttl > 0) {
			const float3 sdir = (pos - shieldPos).SafeNormalize();
			// steer away twice as fast as we can steer toward target
			float3 dif2 = sdir - dir;
			float tracking = std::max(shieldForce * 0.05f, weaponDef->turnrate * 2);

			if (dif2.SqLength() < Square(tracking)) {
				dir = sdir;
			} else {
				dif2 -= dir * (dif2.dot(dir));
				dif2.SafeNormalize();
				dir += dif2 * tracking;
				dir.SafeNormalize();
			}

			return 2;
		}
	}

	return 0;
}
