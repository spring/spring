/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "TorpedoProjectile.h"
#include "Game/Camera.h"
#include "Game/GameHelper.h"
#include "Game/GlobalUnsynced.h"
#include "Map/Ground.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Rendering/Env/Particles/Classes/BubbleProjectile.h"
#include "Rendering/Env/Particles/Classes/SmokeTrailProjectile.h"
#include "Sim/Units/Unit.h"
#include "Sim/Weapons/WeaponDef.h"
#include "System/myMath.h"

#ifdef TRACE_SYNC
	#include "System/Sync/SyncTracer.h"
#endif

CR_BIND_DERIVED(CTorpedoProjectile, CWeaponProjectile, )

CR_REG_METADATA(CTorpedoProjectile,(
	CR_SETFLAG(CF_Synced),
	CR_MEMBER(tracking),
	CR_MEMBER(ignoreError),
	CR_MEMBER(maxSpeed),
	CR_MEMBER(nextBubble),
	CR_MEMBER(texx),
	CR_MEMBER(texy)
))


CTorpedoProjectile::CTorpedoProjectile(const ProjectileParams& params): CWeaponProjectile(params)
	, tracking(0.0f)
	, ignoreError(false)
	, maxSpeed(0.0f)

	, nextBubble(4)
	, texx(0.0f)
	, texy(0.0f)
{
	projectileType = WEAPON_TORPEDO_PROJECTILE;

	tracking = params.tracking;

	if (weaponDef != NULL) {
		maxSpeed = weaponDef->projectilespeed;
	}

	drawRadius = maxSpeed * 8;

	texx = projectileDrawer->torpedotex->xstart - (projectileDrawer->torpedotex->xend - projectileDrawer->torpedotex->xstart) * 0.5f;
	texy = projectileDrawer->torpedotex->ystart - (projectileDrawer->torpedotex->yend - projectileDrawer->torpedotex->ystart) * 0.5f;

#ifdef TRACE_SYNC
	tracefile << "New projectile: ";
	tracefile << pos.x << " " << pos.y << " " << pos.z << " " << speed.x << " " << speed.y << " " << speed.z << "\n";
#endif
}

void CTorpedoProjectile::Update()
{
	// tracking only works when we are underwater
	if (!weaponDef->submissile && pos.y > 0.0f) {
		if (!luaMoveCtrl) {
			// must update dir and speed.w here
			SetVelocityAndSpeed(speed + (UpVector * mygravity));
		}
	} else {
		if (--ttl > 0) {
			if (!luaMoveCtrl) {
				float3 targetVel;

				if (speed.w < maxSpeed)
					speed.w += std::max(0.2f, tracking);

				if (target != nullptr) {
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

				if (!weaponDef->submissile && targetPos.y > 0.0f) {
					targetPos.y = 0.0f;
				}

				const float3 targetLeadVec = targetVel * (pos.distance(targetPos) / maxSpeed) * 0.7f;
				const float3 targetLeadDir = (targetPos + targetLeadVec - pos).Normalize();

				float3 targetDirDif = targetLeadDir - dir;

				if (targetDirDif.Length() < tracking) {
					dir = targetLeadDir;
				} else {
					// <tracking> is the projectile's turn-rate
					targetDirDif = (targetDirDif - (dir * targetDirDif.dot(dir))).SafeNormalize();
					dir = (dir + (targetDirDif * tracking)).SafeNormalize();
				}

				// do not need to update dir or speed.w here
				CWorldObject::SetVelocity(dir * speed.w);
			}

			explGenHandler->GenExplosion(cegID, pos, speed, ttl, damages->damageAreaOfEffect, 0.0f, NULL, NULL);
		} else {
			if (!luaMoveCtrl) {
				// must update dir and speed.w here
				SetVelocityAndSpeed((speed * 0.98f) + (UpVector * mygravity));
			}
		}
	}

	if (!luaMoveCtrl) {
		SetPosition(pos + speed);
	}

	if (pos.y < -2.0f) {
		--nextBubble;

		if (nextBubble == 0) {
			nextBubble = 1 + (int) (gs->randFloat() * 1.5f);

			const float3 pspeed = (gu->RandVector() * 0.1f) + float3(0.0f, 0.2f, 0.0f);

			// Spawn unsynced bubble projectile
			new CBubbleProjectile(
				owner(),
				pos + gu->RandVector(), pspeed, 40 + gu->RandFloat() * GAME_SPEED,
				1 + gu->RandFloat() * 2, 0.01f, 0.3f + gu->RandFloat() * 0.3f
			);
		}
	}

	UpdateGroundBounce();
	UpdateInterception();
}

void CTorpedoProjectile::Draw()
{
	if (model) {
		// do not draw if a 3D model has been defined for us
		return;
	}

	SColor col(60, 60, 100, 255);
	float3 r = dir.cross(UpVector);
	if (r.SqLength() < 0.001f) {
		r = float3(RgtVector);
	}
	r.Normalize();
	const float3 u = dir.cross(r);
	const float h = 12;
	const float w = 2;

	inArray = true;
	va->EnlargeArrays(32, 0, VA_SIZE_TC);

	va->AddVertexQTC(drawPos + (r * w),             texx, texy, col);
	va->AddVertexQTC(drawPos + (u * w),             texx, texy, col);
	va->AddVertexQTC(drawPos + (u * w) + (dir * h), texx, texy, col);
	va->AddVertexQTC(drawPos + (r * w) + (dir * h), texx, texy, col);

	va->AddVertexQTC(drawPos + (u * w),             texx, texy, col);
	va->AddVertexQTC(drawPos - (r * w),             texx, texy, col);
	va->AddVertexQTC(drawPos - (r * w) + (dir * h), texx, texy, col);
	va->AddVertexQTC(drawPos + (u * w) + (dir * h), texx, texy, col);

	va->AddVertexQTC(drawPos - (r * w),             texx, texy, col);
	va->AddVertexQTC(drawPos - (u * w),             texx, texy, col);
	va->AddVertexQTC(drawPos - (u * w) + (dir * h), texx, texy, col);
	va->AddVertexQTC(drawPos - (r * w) + (dir * h), texx, texy, col);

	va->AddVertexQTC(drawPos - (u * w),             texx, texy, col);
	va->AddVertexQTC(drawPos + (r * w),             texx, texy, col);
	va->AddVertexQTC(drawPos + (r * w) + (dir * h), texx, texy, col);
	va->AddVertexQTC(drawPos - (u * w) + (dir * h), texx, texy, col);


	va->AddVertexQTC(drawPos + (r * w) + (dir * h), texx, texy, col);
	va->AddVertexQTC(drawPos + (u * w) + (dir * h), texx, texy, col);
	va->AddVertexQTC(drawPos + (dir * h * 1.2f),    texx, texy, col);
	va->AddVertexQTC(drawPos + (dir * h * 1.2f),    texx, texy, col);

	va->AddVertexQTC(drawPos + (u * w) + (dir * h), texx, texy, col);
	va->AddVertexQTC(drawPos - (r * w) + (dir * h), texx, texy, col);
	va->AddVertexQTC(drawPos + (dir * h * 1.2f),    texx, texy, col);
	va->AddVertexQTC(drawPos + (dir * h * 1.2f),    texx, texy, col);

	va->AddVertexQTC(drawPos - (r * w) + (dir * h), texx, texy, col);
	va->AddVertexQTC(drawPos - (u * w) + (dir * h), texx, texy, col);
	va->AddVertexQTC(drawPos + (dir * h * 1.2f),    texx, texy, col);
	va->AddVertexQTC(drawPos + (dir * h * 1.2f),    texx, texy, col);

	va->AddVertexQTC(drawPos - (u * w) + (dir * h), texx, texy, col);
	va->AddVertexQTC(drawPos + (r * w) + (dir * h), texx, texy, col);
	va->AddVertexQTC(drawPos + (dir * h * 1.2f),    texx, texy, col);
	va->AddVertexQTC(drawPos + (dir * h * 1.2f),    texx, texy, col);
}

int CTorpedoProjectile::GetProjectilesCount() const
{
	return 8;
}
