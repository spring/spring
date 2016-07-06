/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "StarburstProjectile.h"
#include "Game/Camera.h"
#include "Game/GameHelper.h"
#include "Map/Ground.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Rendering/Env/Particles/Classes/SmokeTrailProjectile.h"
#include "Sim/Units/Unit.h"
#include "Sim/Weapons/WeaponDef.h"
#include "System/Sync/SyncTracer.h"
#include "System/Color.h"
#include "System/Matrix44f.h"
#include "System/myMath.h"


// the smokes life-time in frames
static const float SMOKE_TIME = 70.0f;

static const float TRACER_PARTS_STEP = 2.0f;
static const unsigned int MAX_NUM_AGEMODS = 32;

CR_BIND(CStarburstProjectile::TracerPart, )
CR_REG_METADATA_SUB(CStarburstProjectile, TracerPart, (
	CR_MEMBER(pos),
	CR_MEMBER(dir),
	CR_MEMBER(speedf),
	CR_MEMBER(ageMods),
	CR_MEMBER(numAgeMods)
))


CR_BIND_DERIVED(CStarburstProjectile, CWeaponProjectile, )
CR_REG_METADATA(CStarburstProjectile, (
	CR_SETFLAG(CF_Synced),
	CR_MEMBER(tracking),
	CR_MEMBER(ignoreError),
	CR_MEMBER(maxGoodDif),
	CR_MEMBER(maxSpeed),
	CR_MEMBER(acceleration),
	CR_MEMBER(uptime),
	CR_MEMBER(age),
	CR_MEMBER(oldSmoke),
	CR_MEMBER(oldSmokeDir),
	CR_MEMBER(numParts),
	CR_MEMBER(doturn),
	CR_MEMBER(smokeTrail),
	CR_MEMBER(missileAge),
	CR_MEMBER(distanceToTravel),
	CR_MEMBER(aimError),
	CR_MEMBER(curTracerPart),
	CR_MEMBER(tracerParts)
))


CStarburstProjectile::CStarburstProjectile(const ProjectileParams& params): CWeaponProjectile(params)
	, aimError(params.error)
	, tracking(params.tracking)
	, ignoreError(false)
	, doturn(true)
	, maxGoodDif(0.0f)
	, maxSpeed(0.0f)
	, acceleration(0.f)
	, distanceToTravel(params.maxRange)

	, uptime(params.upTime)
	, age(0)

	, oldSmoke(pos)
	, smokeTrail(nullptr)

	, numParts(0)
	, missileAge(0)
	, curTracerPart(0)
{
	projectileType = WEAPON_STARBURST_PROJECTILE;


	if (weaponDef != NULL) {
		maxSpeed = weaponDef->projectilespeed;
		ttl = weaponDef->flighttime;

		// Default uptime is -1. Positive values override the weapondef.
		if (uptime < 0) {
			uptime = weaponDef->uptime * GAME_SPEED;
		}

		if (weaponDef->flighttime == 0) {
			ttl = std::min(3000.0f, uptime + weaponDef->range / maxSpeed + 100);
		}
	}

	maxGoodDif = math::cos(tracking * 0.6f);
	oldSmokeDir = dir;
	drawRadius = maxSpeed * 8.0f;

	for (unsigned int a = 0; a < NUM_TRACER_PARTS; ++a) {
		tracerParts[a].dir = dir;
		tracerParts[a].pos = pos;
		tracerParts[a].speedf = speed.w;

		tracerParts[a].ageMods.resize(MAX_NUM_AGEMODS, 1.0f);
		tracerParts[a].numAgeMods = std::min(MAX_NUM_AGEMODS, static_cast<unsigned int>((speed.w + 0.6f) / TRACER_PARTS_STEP));
	}
	castShadow = true;

#ifdef TRACE_SYNC
	tracefile << "[" << __FUNCTION__ << "] ";
	tracefile << pos.x << " " << pos.y << " " << pos.z << " " << speed.x << " " << speed.y << " " << speed.z << "\n";
#endif
}


void CStarburstProjectile::Collision()
{
	if (weaponDef->visuals.smokeTrail) {
		new CSmokeTrailProjectile(owner(), pos, oldSmoke, dir, oldSmokeDir, false, true, 7, SMOKE_TIME, 0.7f, weaponDef->visuals.texture2);
	}

	oldSmokeDir = dir;
	CWeaponProjectile::Collision();
	oldSmoke = pos;
}

void CStarburstProjectile::Collision(CUnit* unit)
{
	if (weaponDef->visuals.smokeTrail) {
		new CSmokeTrailProjectile(owner(), pos, oldSmoke, dir, oldSmokeDir, false, true, 7, SMOKE_TIME, 0.7f, weaponDef->visuals.texture2);
	}

	oldSmokeDir = dir;
	CWeaponProjectile::Collision(unit);
	oldSmoke = pos;
}

void CStarburstProjectile::Collision(CFeature* feature)
{
	if (weaponDef->visuals.smokeTrail) {
		new CSmokeTrailProjectile(owner(), pos, oldSmoke, dir, oldSmokeDir, false, true, 7, SMOKE_TIME, 0.7f, weaponDef->visuals.texture2);
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

	if (target != nullptr && weaponDef->tracks) {
		const CSolidObject* so = dynamic_cast<const CSolidObject*>(target);

		if (so != NULL) {
			targetPos = so->aimPos;
			if (allyteamID != -1 && !ignoreError) {
				const CUnit* u = dynamic_cast<const CUnit*>(so);

				if (u != nullptr)
					targetPos = u->GetErrorPos(allyteamID, true);
			}
		} else {
			targetPos = target->pos;
		}

		targetPos += aimError;
	}

	if (!luaMoveCtrl) {
		UpdateTrajectory();
	}

	if (ttl > 0) {
		explGenHandler->GenExplosion(cegID, pos, dir, ttl, damages->damageAreaOfEffect, 0.0f, NULL, NULL);
	}


	{
		const unsigned int newTracerPart = (curTracerPart + 1) % NUM_TRACER_PARTS;

		curTracerPart = newTracerPart;
		TracerPart* tracerPart = &tracerParts[curTracerPart];
		tracerPart->pos = pos;
		tracerPart->dir = dir;
		tracerPart->speedf = speed.w;

		unsigned int newsize = 0;

		for (float aa = 0; aa < speed.w + 0.6f && newsize < MAX_NUM_AGEMODS; aa += TRACER_PARTS_STEP, ++newsize) {
			const float ageMod = (missileAge < 20) ? 1.0f : (0.6f + (rand() * 0.8f) / RAND_MAX);
			tracerPart->ageMods[newsize] = ageMod;
		}

		if (tracerPart->numAgeMods != newsize) {
			tracerPart->numAgeMods = newsize;
		}
	}

	age++;
	numParts++;

	if (weaponDef->visuals.smokeTrail) {
		if (smokeTrail) {
			smokeTrail->UpdateEndPos(pos, dir);
			oldSmoke = pos;
			oldSmokeDir = dir;
		}

		if ((age % 8) == 0) {
			smokeTrail = new CSmokeTrailProjectile(
				owner(),
				pos,
				oldSmoke,
				dir,
				oldSmokeDir,
				age == 8,
				false,
				7,
				SMOKE_TIME,
				0.7f,
				weaponDef->visuals.texture2
			);

			numParts = 0;
			useAirLos = smokeTrail->useAirLos;
		}
	}

	UpdateInterception();
}

void CStarburstProjectile::UpdateTrajectory()
{
	if (uptime > 0) {
		// stage 1: going upwards
		speed.w += weaponDef->weaponacceleration;
		speed.w = std::min(speed.w, maxSpeed);
		CWorldObject::SetVelocity(dir * speed.w); // do not need to update dir or speed.w here

	} else if (doturn && ttl > 0 && distanceToTravel > 0.0f) {
		// stage 2: turn to target
		float3 targetErrorVec = (targetPos - pos).Normalize();

		if (targetErrorVec.dot(dir) > 0.99f) {
			dir = targetErrorVec;
			doturn = false;
		} else {
			targetErrorVec = targetErrorVec - dir;
			targetErrorVec = (targetErrorVec - (dir * (targetErrorVec.dot(dir)))).SafeNormalize();

			if (weaponDef->turnrate != 0) {
				dir = (dir + (targetErrorVec * weaponDef->turnrate)).Normalize();
			} else {
				dir = (dir + (targetErrorVec * 0.06f)).Normalize();
			}
		}

		// do not need to update dir or speed.w here
		CWorldObject::SetVelocity(dir * speed.w);

		if (distanceToTravel != MAX_PROJECTILE_RANGE) {
			distanceToTravel -= speed.Length2D();
		}

	} else if (ttl > 0 && distanceToTravel > 0.0f) {
		// stage 3: hit target
		float3 targetErrorVec = (targetPos - pos).Normalize();

		if (targetErrorVec.dot(dir) > maxGoodDif) {
			dir = targetErrorVec;
		} else {
			targetErrorVec = targetErrorVec - dir;
			targetErrorVec = (targetErrorVec - (dir * (targetErrorVec.dot(dir)))).SafeNormalize();

			dir = (dir + (targetErrorVec * tracking)).SafeNormalize();
		}

		speed.w += weaponDef->weaponacceleration;
		speed.w = std::min(speed.w, maxSpeed);
		CWorldObject::SetVelocity(dir * speed.w);

		if (distanceToTravel != MAX_PROJECTILE_RANGE) {
			distanceToTravel -= speed.Length2D();
		}

	} else {
		// stage "out of fuel"
		// changes dir and speed.w, must keep speed-vector in sync
		SetDirectionAndSpeed((dir + (UpVector * mygravity)).Normalize(), speed.w - mygravity);
	}

	SetPosition(pos + speed);
}

void CStarburstProjectile::Draw()
{
	inArray = true;

	unsigned int part = curTracerPart;
	const auto wt3 = weaponDef->visuals.texture3;
	const auto wt1 = weaponDef->visuals.texture1;
	const SColor lightYellow(255, 200, 150, 1);

	for (unsigned int a = 0; a < NUM_TRACER_PARTS; ++a) {
		const TracerPart* tracerPart = &tracerParts[part];
		const float3& opos = tracerPart->pos;
		const float3& odir = tracerPart->dir;
		const float ospeed = tracerPart->speedf;
		float aa = 0;

		unsigned int numAgeMods = tracerPart->numAgeMods;

		for (int num = 0; num < numAgeMods; aa += TRACER_PARTS_STEP, ++num) {
			const float ageMod = tracerPart->ageMods[num];
			const float age2 = (a + (aa / (ospeed + 0.01f))) * 0.2f;
			const float3 interPos = opos - (odir * ((a * 0.5f) + aa));

			float alpha = Square(1.0f - age2);
			if (missileAge >= 20) {
				alpha = (1.0f - age2) * std::max(0.0f, 1.0f - age2);
			}
			SColor col = lightYellow * Clamp(alpha, 0.f, 1.f);
			col.a = 1;

			const float drawsize = 1.0f + age2 * 0.8f * ageMod * 7;
			va->AddVertexTC(interPos - camera->GetRight() * drawsize - camera->GetUp() * drawsize, wt3->xstart, wt3->ystart, col);
			va->AddVertexTC(interPos + camera->GetRight() * drawsize - camera->GetUp() * drawsize, wt3->xend,   wt3->ystart, col);
			va->AddVertexTC(interPos + camera->GetRight() * drawsize + camera->GetUp() * drawsize, wt3->xend,   wt3->yend,   col);
			va->AddVertexTC(interPos - camera->GetRight() * drawsize + camera->GetUp() * drawsize, wt3->xstart, wt3->yend,   col);
		}

		// unsigned, so LHS will wrap around to UINT_MAX
		part = std::min(part - 1, NUM_TRACER_PARTS - 1);
	}

	// draw the engine flare
	const SColor lightRed(255, 180, 180, 1);
	const float fsize = 25.0f;
	va->AddVertexTC(drawPos - camera->GetRight() * fsize - camera->GetUp() * fsize, wt1->xstart, wt1->ystart, lightRed);
	va->AddVertexTC(drawPos + camera->GetRight() * fsize - camera->GetUp() * fsize, wt1->xend,   wt1->ystart, lightRed);
	va->AddVertexTC(drawPos + camera->GetRight() * fsize + camera->GetUp() * fsize, wt1->xend,   wt1->yend,   lightRed);
	va->AddVertexTC(drawPos - camera->GetRight() * fsize + camera->GetUp() * fsize, wt1->xstart, wt1->yend,   lightRed);
}

int CStarburstProjectile::ShieldRepulse(const float3& shieldPos, float shieldForce, float shieldMaxSpeed)
{
	const float3 rdir = (pos - shieldPos).Normalize();

	if (ttl > 0) {
		float3 dif2 = rdir - dir;

		// steer away twice as fast as we can steer toward target
		const float tracking = std::max(shieldForce * 0.05f, weaponDef->turnrate * 2.0f);

		if (dif2.Length() < tracking) {
			dir = rdir;
		} else {
			dif2 = (dif2 - (dir * (dif2.dot(dir)))).Normalize();
			dir = (dir + (dif2 * tracking)).Normalize();
		}

		return 2;
	}

	return 0;
}

int CStarburstProjectile::GetProjectilesCount() const
{
	return numParts + 1;
}
