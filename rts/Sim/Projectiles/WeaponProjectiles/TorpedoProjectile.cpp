/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "TorpedoProjectile.h"
#include "Game/Camera.h"
#include "Game/GameHelper.h"
#include "Map/Ground.h"
#include "Rendering/ProjectileDrawer.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Projectiles/Unsynced/BubbleProjectile.h"
#include "Sim/Projectiles/Unsynced/SmokeTrailProjectile.h"
#include "Sim/Units/Unit.h"
#include "Sim/Weapons/WeaponDef.h"
#include "System/myMath.h"

#ifdef TRACE_SYNC
	#include "System/Sync/SyncTracer.h"
#endif

CR_BIND_DERIVED(CTorpedoProjectile, CWeaponProjectile, (ProjectileParams()));

CR_REG_METADATA(CTorpedoProjectile,(
	CR_SETFLAG(CF_Synced),
	CR_MEMBER(tracking),
	CR_MEMBER(maxSpeed),
	CR_MEMBER(curSpeed),
	CR_MEMBER(areaOfEffect),
	CR_MEMBER(nextBubble),
	CR_MEMBER(texx),
	CR_MEMBER(texy)
));

CTorpedoProjectile::CTorpedoProjectile(const ProjectileParams& params): CWeaponProjectile(params)
	, tracking(0.0f)
	, maxSpeed(0.0f)
	, curSpeed(0.0f)
	, areaOfEffect(0.0f)

	, nextBubble(4)
	, texx(0.0f)
	, texy(0.0f)
{
	projectileType = WEAPON_TORPEDO_PROJECTILE;

	tracking = params.tracking;
	curSpeed = speed.Length();
	dir = speed / curSpeed;

	if (weaponDef != NULL) {
		maxSpeed = weaponDef->projectilespeed;
		areaOfEffect = weaponDef->damageAreaOfEffect;
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
	if (!weaponDef->submissile && pos.y > -3.0f) {
		// tracking etc only works when we are underwater
		if (!luaMoveCtrl) {
			speed.y += mygravity;
			dir = speed;
			dir.Normalize();
		}
	} else {
		if (!weaponDef->submissile && pos.y-speed.y > -3.0f) {
			// level out torpedo a bit when hitting water
			if (!luaMoveCtrl) {
				dir.y *= 0.5f;
				dir.Normalize();
			}
		}

		if (--ttl > 0) {
			if (!luaMoveCtrl) {
				if (curSpeed < maxSpeed) {
					curSpeed += std::max(0.2f, tracking);
				}

				if (target) {
					CSolidObject* so = dynamic_cast<CSolidObject*>(target);
					CWeaponProjectile* po = dynamic_cast<CWeaponProjectile*>(target);

					targetPos = target->pos;
					float3 targSpeed(ZeroVector);
					if (so) {
						targetPos = so->aimPos;
						targSpeed = so->speed;

						if (pos.SqDistance(so->aimPos) > 150 * 150 && owner()) {
							CUnit* u = dynamic_cast<CUnit*>(so);
							if (u) {
								targetPos = CGameHelper::GetUnitErrorPos(u, owner()->allyteam, true);
							}
						}
					} if (po) {
						targSpeed = po->speed;
					}

					if (!weaponDef->submissile && targetPos.y > 0) {
						targetPos.y = 0;
					}

					const float dist = pos.distance(targetPos);
					float3 dif = (targetPos + targSpeed * (dist / maxSpeed) * 0.7f - pos).Normalize();
					float3 dif2 = dif - dir;

					if (dif2.Length() < tracking) {
						dir = dif;
					} else {
						dif2 -= dir * (dif2.dot(dir));
						dif2.SafeNormalize();
						dir += dif2 * tracking;
						dir.SafeNormalize();
					}
				}

				speed = dir * curSpeed;
			}

			gCEG->Explosion(cegID, pos, ttl, areaOfEffect, NULL, 0.0f, NULL, speed);
		} else {
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

	if (pos.y < -2.0f) {
		--nextBubble;

		if (nextBubble == 0) {
			nextBubble = 1 + (int) (gs->randFloat() * 1.5f);

			const float3 pspeed = (gs->randVector() * 0.1f) + float3(0.0f, 0.2f, 0.0f);

			new CBubbleProjectile(
				pos + gs->randVector(), pspeed, 40 + gs->randFloat() * 30,
				1 + gs->randFloat() * 2, 0.01f, owner(), 0.3f + gs->randFloat() * 0.3f
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

	inArray = true;

	unsigned char col[4];
	col[0] = 60;
	col[1] = 60;
	col[2] = 100;
	col[3] = 255;

	float3 r = dir.cross(UpVector);
	if (r.Length() < 0.001f) {
		r = float3(1.0f, 0.0f, 0.0f);
	}
	r.Normalize();
	const float3 u = dir.cross(r);
	const float h = 12;
	const float w = 2;

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
