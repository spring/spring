/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "StarburstProjectile.h"
#include "Game/Camera.h"
#include "Game/GameHelper.h"
#include "Map/Ground.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ProjectileDrawer.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Projectiles/Unsynced/SmokeTrailProjectile.h"
#include "Sim/Units/Unit.h"
#include "Sim/Weapons/WeaponDef.h"
#include "System/Sync/SyncTracer.h"
#include "System/Color.h"
#include "System/Matrix44f.h"
#include "System/myMath.h"
#include "System/TimeProfiler.h"

// the smokes life-time in frames
static const float SMOKE_TIME = 70.0f;

static const float TRACER_PARTS_STEP = 2.0f;

static const size_t MAX_NUM_AGEMODS = 32;

CR_BIND(CStarburstProjectile::TracerPart, )
CR_REG_METADATA_SUB(CStarburstProjectile, TracerPart, (
	CR_MEMBER(pos),
	CR_MEMBER(dir),
	CR_MEMBER(speedf),
	CR_MEMBER(ageMods),
	CR_MEMBER(numAgeMods)
));


CR_BIND_DERIVED(CStarburstProjectile, CWeaponProjectile, (ProjectileParams()));
CR_REG_METADATA(CStarburstProjectile, (
	CR_SETFLAG(CF_Synced),
	CR_MEMBER(tracking),
	CR_MEMBER(maxGoodDif),
	CR_MEMBER(maxSpeed),
	CR_MEMBER(curSpeed),
	CR_MEMBER(acceleration),
	CR_MEMBER(uptime),
	CR_MEMBER(areaOfEffect),
	CR_MEMBER(age),
	CR_MEMBER(oldSmoke),
	CR_MEMBER(oldSmokeDir),
	CR_MEMBER(drawTrail),
	CR_MEMBER(numParts),
	CR_MEMBER(doturn),
	CR_MEMBER(curCallback),
	CR_MEMBER(missileAge),
	CR_MEMBER(distanceToTravel),
	CR_MEMBER(aimError),
	CR_MEMBER(curTracerPart),
	CR_MEMBER(tracerParts)
));


CStarburstProjectile::CStarburstProjectile(const ProjectileParams& params): CWeaponProjectile(params)
	, tracking(0.0f)
	, maxGoodDif(0.0f)
	, maxSpeed(0.0f)
	, curSpeed(0.0f)
	, acceleration(0.f)
	, areaOfEffect(0.0f)
	, distanceToTravel(0.0f)

	, uptime(0)
	, age(0)

	, oldSmoke(pos)

	, drawTrail(true)
	, doturn(true)
	, curCallback(NULL)

	, numParts(0)
	, missileAge(0)
	, curTracerPart(0)
{
	projectileType = WEAPON_STARBURST_PROJECTILE;

	tracking = params.tracking;
	distanceToTravel = params.maxRange;
	aimError = params.error;

	if (weaponDef != NULL) {
		maxSpeed = weaponDef->projectilespeed;
		areaOfEffect = weaponDef->damageAreaOfEffect;
		uptime = weaponDef->uptime * GAME_SPEED;

		if (weaponDef->flighttime == 0) {
			ttl = std::min(3000.0f, uptime + weaponDef->range / maxSpeed + 100);
		} else {
			ttl = weaponDef->flighttime;
		}
	}

	maxGoodDif = math::cos(tracking * 0.6f);
	curSpeed = speed.Length();
	dir = speed / curSpeed;
	oldSmokeDir = dir;

	const float3 camDir = (pos - camera->pos).ANormalize();
	const float camDist = (camera->pos.distance(pos) * 0.2f) + ((1.0f - math::fabs(camDir.dot(dir))) * 3000);

	drawTrail = (camDist >= 200.0f);
	drawRadius = maxSpeed * 8.0f;

	for (int a = 0; a < NUM_TRACER_PARTS; ++a) {
		tracerParts[a].dir = dir;
		tracerParts[a].pos = pos;
		tracerParts[a].speedf = curSpeed;

		tracerParts[a].ageMods.resize(MAX_NUM_AGEMODS, 1.0f);
		tracerParts[a].numAgeMods = std::min(MAX_NUM_AGEMODS, (size_t)std::floor((curSpeed + 0.6f) / TRACER_PARTS_STEP));
	}
	castShadow = true;

#ifdef TRACE_SYNC
	tracefile << "[" << __FUNCTION__ << "] ";
	tracefile << pos.x << " " << pos.y << " " << pos.z << " " << speed.x << " " << speed.y << " " << speed.z << "\n";
#endif
}

void CStarburstProjectile::Detach()
{
	// SYNCED
	if (curCallback) // this is unsynced, but it prevents some callback crash on exit
		curCallback->drawCallbacker = 0;

	CProjectile::Detach();
}

CStarburstProjectile::~CStarburstProjectile()
{
	// UNSYNCED
	for (int a = 0; a < NUM_TRACER_PARTS; ++a) {
		tracerParts[a].ageMods.clear();
	}
}


void CStarburstProjectile::Collision()
{
	if (weaponDef->visuals.smokeTrail) {
		new CSmokeTrailProjectile(pos, oldSmoke, dir, oldSmokeDir, owner(), false, true, 7, SMOKE_TIME, 0.7f, drawTrail, 0, weaponDef->visuals.texture2);
	}

	oldSmokeDir = dir;
	CWeaponProjectile::Collision();
	oldSmoke = pos;
}

void CStarburstProjectile::Collision(CUnit* unit)
{
	if (weaponDef->visuals.smokeTrail) {
		new CSmokeTrailProjectile(pos, oldSmoke, dir, oldSmokeDir, owner(), false, true, 7, SMOKE_TIME, 0.7f, drawTrail, 0, weaponDef->visuals.texture2);
	}

	oldSmokeDir = dir;
	CWeaponProjectile::Collision(unit);
	oldSmoke = pos;
}

void CStarburstProjectile::Collision(CFeature* feature)
{
	if (weaponDef->visuals.smokeTrail) {
		new CSmokeTrailProjectile(pos, oldSmoke, dir, oldSmokeDir, owner(), false, true, 7, SMOKE_TIME, 0.7f, drawTrail, 0, weaponDef->visuals.texture2);
	}

	oldSmokeDir = dir;
	CWeaponProjectile::Collision(feature);
	oldSmoke = pos;
}


void CStarburstProjectile::Update()
{
	ttl--;
	uptime--;
	missileAge++;

	if (target && weaponDef->tracks && owner()) {
		targetPos = target->pos;
		CUnit* u = dynamic_cast<CUnit*>(target);
		if (u) {
			targetPos = CGameHelper::GetUnitErrorPos(u, owner()->allyteam, true);
		}
	}

	if (uptime > 0) {
		if (!luaMoveCtrl) {
			if (curSpeed < maxSpeed) {
				curSpeed += weaponDef->weaponacceleration;
			}

			speed = dir * curSpeed;
		}
	} else if (doturn && ttl > 0 && distanceToTravel > 0.0f) {
		if (!luaMoveCtrl) {
			float3 dif(targetPos - pos);
			dif.Normalize();
			dif += aimError;
			dif.Normalize();

			if (dif.dot(dir) > 0.99f) {
				dir = dif;
				doturn = false;
			} else {
				dif = dif - dir;
				dif -= dir * (dif.dot(dir));
				dif.Normalize();

				if (weaponDef->turnrate != 0) {
					dir += dif * weaponDef->turnrate;
				} else {
					dir += dif * 0.06;
				}

				dir.Normalize();
			}
			speed = dir * curSpeed;
			if (distanceToTravel != MAX_PROJECTILE_RANGE) {
				distanceToTravel -= speed.Length2D();
			}
		}
	} else if (ttl > 0 && distanceToTravel > 0.0f) {
		if (!luaMoveCtrl) {
			if (curSpeed < maxSpeed) {
				curSpeed += weaponDef->weaponacceleration;
			}

			float3 dif = (targetPos - pos).Normalize();

			if (dif.dot(dir) > maxGoodDif) {
				dir = dif;
			} else {
				dif = dif - dir;
				dif -= dir * (dif.dot(dir));
				dif.SafeNormalize();
				dir += dif * tracking;
				dir.SafeNormalize();
			}

			speed = dir * curSpeed;

			if (distanceToTravel != MAX_PROJECTILE_RANGE) {
				distanceToTravel -= speed.Length2D();
			}
		}
	} else {
		if (!luaMoveCtrl) {
			dir.y += mygravity;
			dir.Normalize();
			curSpeed -= mygravity;
			speed = dir * curSpeed;
		}
	}

	if (!luaMoveCtrl) {
		pos += speed;
	}

	if (ttl > 0) {
		gCEG->Explosion(cegID, pos, ttl, areaOfEffect, NULL, 0.0f, NULL, dir);
	}


	{
		size_t newTracerPart = (curTracerPart + 1) % NUM_TRACER_PARTS;
		curTracerPart = GML::SimEnabled() ? *(volatile size_t*)&newTracerPart : newTracerPart;
		TracerPart* tracerPart = &tracerParts[curTracerPart];
		tracerPart->pos = pos;
		tracerPart->dir = dir;
		tracerPart->speedf = curSpeed;

		size_t newsize = 0;
		for (float aa = 0; aa < curSpeed + 0.6f && newsize < MAX_NUM_AGEMODS; aa += TRACER_PARTS_STEP, ++newsize) {
			const float ageMod = (missileAge < 20) ? 1.0f : (0.6f + (rand() * 0.8f) / RAND_MAX);
			tracerPart->ageMods[newsize] = ageMod;
		}

		if (tracerPart->numAgeMods != newsize) {
			tracerPart->numAgeMods = GML::SimEnabled() ? *(volatile size_t*)&newsize : newsize;
		}
	}

	age++;
	numParts++;

	if (weaponDef->visuals.smokeTrail && !(age & 7)) {
		if (curCallback) {
			curCallback->drawCallbacker = 0;
		}

		curCallback = new CSmokeTrailProjectile(pos, oldSmoke, dir, oldSmokeDir,
				owner(), age == 8, false, 7, SMOKE_TIME, 0.7f, drawTrail, this,
				weaponDef->visuals.texture2);

		oldSmoke = pos;
		oldSmokeDir = dir;
		numParts = 0;
		useAirLos = curCallback->useAirLos;

		if (!drawTrail) {
			const float3 camDir = (pos - camera->pos).ANormalize();
			const float camDist = (camera->pos.distance(pos) * 0.2f + (1 - math::fabs(camDir.dot(dir))) * 3000);

			drawTrail = (camDist > 300.0f);
		}
	}

	UpdateInterception();
}

void CStarburstProjectile::Draw()
{
	inArray = true;

	if (weaponDef->visuals.smokeTrail) {
		const int curNumParts = GML::SimEnabled() ? *(volatile int*) &numParts : numParts;

		va->EnlargeArrays(4 + (4 * curNumParts), 0, VA_SIZE_TC);

		const float age2 = (age & 7) + globalRendering->timeOffset;
		const float color = 0.7f;

		if (drawTrail) {
			// draw the trail as a single quad
			const float3 dif1 = (drawPos - camera->pos).ANormalize();
			const float3 dir1 = (dif1.cross(dir)).ANormalize();
			const float3 dif2 = (oldSmoke - camera->pos).ANormalize();
			const float3 dir2 = (dif2.cross(oldSmokeDir)).ANormalize();

			const float a1 =
				((1.0f - (0.0f / SMOKE_TIME))) *
				(0.7f + math::fabs(dif1.dot(dir)));
			const float a2 =
				(age < 8)? 0.0f:
				((1.0f - (age2 / SMOKE_TIME))) *
				(0.7f + math::fabs(dif2.dot(oldSmokeDir)));
			const float alpha1 = Clamp(a1, 0.0f, 1.0f);
			const float alpha2 = Clamp(a2, 0.0f, 1.0f);

			const float size1 = 1.0f;
			const float size2 = (1.0f + age2 * (1.0f / SMOKE_TIME) * 7.0f);

			const float txs =
				weaponDef->visuals.texture2->xend -
				(weaponDef->visuals.texture2->xend - weaponDef->visuals.texture2->xstart) *
				(age2 / 8.0f);

			SColor col (color * alpha1, color * alpha1, color * alpha1, alpha1);
			SColor col2(color * alpha2, color * alpha2, color * alpha2, alpha2);

			va->AddVertexQTC(drawPos  - dir1 * size1, txs,                               weaponDef->visuals.texture2->ystart, col);
			va->AddVertexQTC(drawPos  + dir1 * size1, txs,                               weaponDef->visuals.texture2->yend,   col);
			va->AddVertexQTC(oldSmoke + dir2 * size2, weaponDef->visuals.texture2->xend, weaponDef->visuals.texture2->yend,   col2);
			va->AddVertexQTC(oldSmoke - dir2 * size2, weaponDef->visuals.texture2->xend, weaponDef->visuals.texture2->ystart, col2);
		} else {
			// draw the trail as particles
			const float dist = pos.distance(oldSmoke);
			const float3 dirpos1 = pos - dir * dist * 0.33f;
			const float3 dirpos2 = oldSmoke + oldSmokeDir * dist * 0.33f;
			const SColor col(color, color, color);

			for (int a = 0; a < curNumParts; ++a) {
				// CAUTION: loop count must match EnlargeArrays above
				const float size = 1 + (a * (1.0f / SMOKE_TIME) * 7.0f);
				const float3 pos1 = CalcBeizer((float)a / curNumParts, pos, dirpos1, dirpos2, oldSmoke);

				#define st projectileDrawer->smoketex[0]
				va->AddVertexQTC(pos1 + ( camera->up + camera->right) * size, st->xstart, st->ystart, col);
				va->AddVertexQTC(pos1 + ( camera->up - camera->right) * size, st->xend,   st->ystart, col);
				va->AddVertexQTC(pos1 + (-camera->up - camera->right) * size, st->xend,   st->ystart, col);
				va->AddVertexQTC(pos1 + (-camera->up + camera->right) * size, st->xstart, st->ystart, col);
				#undef st
			}
		}
	}

	DrawCallback();
}

void CStarburstProjectile::DrawCallback()
{
	inArray = true;

	unsigned char col[4];

	size_t part = GML::SimEnabled() ? *(volatile size_t*)&curTracerPart : curTracerPart;

	for (int a = 0; a < NUM_TRACER_PARTS; ++a) {
		const TracerPart* tracerPart = &tracerParts[part];
		const float3& opos = tracerPart->pos;
		const float3& odir = tracerPart->dir;
		const float ospeed = tracerPart->speedf;
		float aa = 0;

		size_t numAgeMods = GML::SimEnabled() ? *(volatile size_t*)&tracerPart->numAgeMods : tracerPart->numAgeMods;
		for (int num = 0; num < numAgeMods; aa += TRACER_PARTS_STEP, ++num) {
			const float ageMod = tracerPart->ageMods[num];
			const float age2 = (a + (aa / (ospeed + 0.01f))) * 0.2f;
			const float3 interPos = opos - (odir * ((a * 0.5f) + aa));

			col[3] = 1;

			if (missileAge < 20) {
				const float alpha = std::max(0.0f, (1.0f - age2) * (1.0f - age2));
				col[0] = (255 * alpha);
				col[1] = (200 * alpha);
				col[2] = (150 * alpha);
			} else {
				const float alpha = std::max(0.0f, ((1.0f - age2) * std::max(0.0f, 1.0f - age2)));
				col[0] = (255 * alpha);
				col[1] = (200 * alpha);
				col[2] = (150 * alpha);
			}

			const float drawsize = 1.0f + age2 * 0.8f * ageMod * 7;

			#define wt3 weaponDef->visuals.texture3
			va->AddVertexTC(interPos - camera->right * drawsize - camera->up * drawsize, wt3->xstart, wt3->ystart, col);
			va->AddVertexTC(interPos + camera->right * drawsize - camera->up * drawsize, wt3->xend,   wt3->ystart, col);
			va->AddVertexTC(interPos + camera->right * drawsize + camera->up * drawsize, wt3->xend,   wt3->yend,   col);
			va->AddVertexTC(interPos - camera->right * drawsize + camera->up * drawsize, wt3->xstart, wt3->yend,   col);
			#undef wt3
		}

		part = (part == 0) ? NUM_TRACER_PARTS - 1 : part - 1;
	}

	// draw the engine flare
	col[0] = 255;
	col[1] = 180;
	col[2] = 180;
	col[3] =   1;

	const float fsize = 25.0f;

	#define wt1 weaponDef->visuals.texture1
	va->AddVertexTC(drawPos - camera->right * fsize - camera->up * fsize, wt1->xstart, wt1->ystart, col);
	va->AddVertexTC(drawPos + camera->right * fsize - camera->up * fsize, wt1->xend,   wt1->ystart, col);
	va->AddVertexTC(drawPos + camera->right * fsize + camera->up * fsize, wt1->xend,   wt1->yend,   col);
	va->AddVertexTC(drawPos - camera->right * fsize + camera->up * fsize, wt1->xstart, wt1->yend,   col);
	#undef wt1
}

int CStarburstProjectile::ShieldRepulse(CPlasmaRepulser* shield, float3 shieldPos, float shieldForce, float shieldMaxSpeed)
{
	const float3 rdir = (pos - shieldPos).Normalize();

	if (ttl > 0) {
		float3 dif2 = rdir - dir;

		// steer away twice as fast as we can steer toward target
		const float tracking = std::max(shieldForce * 0.05f, weaponDef->turnrate * 2.0f);

		if (dif2.Length() < tracking) {
			dir = rdir;
		} else {
			dif2 -= (dir * (dif2.dot(dir)));
			dif2.Normalize();
			dir += dif2 * tracking;
			dir.Normalize();
		}
		return 2;
	}
	return 0;
}
