/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "StarburstProjectile.h"
#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Map/Ground.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Projectiles/ProjectileMemPool.h"
#include "Rendering/Env/Particles/Classes/SmokeTrailProjectile.h"
#include "Sim/Units/Unit.h"
#include "Sim/Weapons/WeaponDef.h"
#include "System/Color.h"
#include "System/Matrix44f.h"
#include "System/SpringMath.h"


// the smokes life-time in frames
static constexpr float SMOKE_LIFETIME = 70.0f;
static constexpr float TRACER_PARTS_STEP = 2.0f;

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
	CR_MEMBER(aimError),
	CR_MEMBER(oldSmoke),
	CR_MEMBER(oldSmokeDir),

	CR_MEMBER(tracking),
	CR_MEMBER(maxGoodDif),
	CR_MEMBER(maxSpeed),
	CR_MEMBER(acceleration),
	CR_MEMBER(distanceToTravel),

	CR_MEMBER(uptime),
	CR_MEMBER(trailAge),
	CR_MEMBER(numParts),
	CR_MEMBER(missileAge),
	CR_MEMBER(curTracerPart),

	CR_MEMBER(ignoreError),
	CR_MEMBER(turnToTarget),

	CR_MEMBER(tracerParts),
	CR_MEMBER(smokeTrail)
))


CStarburstProjectile::CStarburstProjectile(const ProjectileParams& params): CWeaponProjectile(params)
	, aimError(params.error)
	, oldSmoke(pos)

	, tracking(params.tracking)
	, distanceToTravel(params.maxRange)

	, uptime(params.upTime)
{
	projectileType = WEAPON_STARBURST_PROJECTILE;


	if (weaponDef != nullptr) {
		maxSpeed = weaponDef->projectilespeed;
		ttl = weaponDef->flighttime;

		// Default uptime is -1. Positive values override the weapondef.
		if (uptime < 0)
			uptime = weaponDef->uptime * GAME_SPEED;

		if (weaponDef->flighttime == 0)
			ttl = std::min(3000.0f, uptime + myrange / maxSpeed + 100);
	}

	oldSmokeDir = dir;

	maxGoodDif = math::cos(tracking * 0.6f);
	drawRadius = maxSpeed * 8.0f;

	castShadow = true;
	leaveSmokeTrail = (weaponDef != nullptr && weaponDef->visuals.smokeTrail);

	InitTracerParts();
}


void CStarburstProjectile::Collision()
{
	if (leaveSmokeTrail)
		projMemPool.alloc<CSmokeTrailProjectile>(owner(), weaponDef->visuals.texture2, pos, oldSmoke, dir, oldSmokeDir, SMOKE_LIFETIME, 7.0f, 0.7f, false, true);

	CWeaponProjectile::Collision();

	oldSmokeDir = dir;
	oldSmoke = pos;
}

void CStarburstProjectile::Collision(CUnit* unit)
{
	if (leaveSmokeTrail)
		projMemPool.alloc<CSmokeTrailProjectile>(owner(), weaponDef->visuals.texture2, pos, oldSmoke, dir, oldSmokeDir, SMOKE_LIFETIME, 7.0f, 0.7f, false, true);

	CWeaponProjectile::Collision(unit);

	oldSmokeDir = dir;
	oldSmoke = pos;
}

void CStarburstProjectile::Collision(CFeature* feature)
{
	if (leaveSmokeTrail)
		projMemPool.alloc<CSmokeTrailProjectile>(owner(), weaponDef->visuals.texture2, pos, oldSmoke, dir, oldSmokeDir, SMOKE_LIFETIME, 7.0f, 0.7f, false, true);

	CWeaponProjectile::Collision(feature);

	oldSmokeDir = dir;
	oldSmoke = pos;
}


void CStarburstProjectile::Update()
{
	ttl--;
	uptime--;
	missileAge++;

	UpdateTargeting();

	if (!luaMoveCtrl) {
		UpdateTrajectory();
		SetPosition(pos + speed);
	}

	if (ttl > 0)
		explGenHandler.GenExplosion(cegID, pos, dir, ttl, damages->damageAreaOfEffect, 0.0f, nullptr, nullptr);

	UpdateTracerPart();
	UpdateSmokeTrail();
	UpdateInterception();
}

void CStarburstProjectile::UpdateTargeting()
{
	if (target == nullptr)
		return;

	if (!weaponDef->tracks)
		return;

	const CSolidObject* so = dynamic_cast<const CSolidObject*>(target);
	const CUnit* u = nullptr;

	if (so == nullptr) {
		targetPos = target->pos + aimError;
		return;
	}

	if (!ignoreError && allyteamID != -1 && (u = dynamic_cast<const CUnit*>(so)) != nullptr)
		targetPos = u->GetErrorPos(allyteamID, true);
	else
		targetPos = so->aimPos;

	targetPos.y = std::max(targetPos.y, targetPos.y * weaponDef->waterweapon);
	targetPos += aimError;
}

void CStarburstProjectile::UpdateTrajectory()
{
	if (uptime > 0) {
		// stage 1: going upwards
		speed.w += weaponDef->weaponacceleration;
		speed.w = std::min(speed.w, maxSpeed);
		CWorldObject::SetVelocity(dir * speed.w); // do not need to update dir or speed.w here
		return;
	}

	if (turnToTarget && ttl > 0 && distanceToTravel > 0.0f) {
		// stage 2: turn to target
		float3 targetErrorVec = (targetPos - pos).Normalize();

		if (targetErrorVec.dot(dir) > 0.99f) {
			dir = targetErrorVec;
			turnToTarget = false;
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

		if (distanceToTravel != MAX_PROJECTILE_RANGE)
			distanceToTravel -= speed.Length2D();

		return;
	}

	if (ttl > 0 && distanceToTravel > 0.0f) {
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

		if (distanceToTravel != MAX_PROJECTILE_RANGE)
			distanceToTravel -= speed.Length2D();

		return;
	}

	// stage "out of fuel"
	// changes dir and speed.w, must keep speed-vector in sync
	SetDirectionAndSpeed((dir + (UpVector * mygravity)).Normalize(), speed.w - mygravity);
}


void CStarburstProjectile::InitTracerParts()
{
	const unsigned int maxAgeMods = MAX_NUM_AGEMODS;
	const unsigned int numAgeMods = static_cast<unsigned int>((speed.w + 0.6f) / TRACER_PARTS_STEP);

	for (unsigned int i = 0; i < NUM_TRACER_PARTS; ++i) {
		TracerPart& tracerPart = tracerParts[i];

		tracerPart.pos = pos;
		tracerPart.dir = dir;

		tracerPart.speedf = speed.w;
		tracerPart.numAgeMods = std::min(maxAgeMods, numAgeMods);

		for (unsigned int j = 0; j < maxAgeMods; j++) {
			tracerPart.ageMods[j] = 1.0f;
		}
	}
}

void CStarburstProjectile::UpdateTracerPart()
{
	TracerPart& tracerPart = tracerParts[curTracerPart = (curTracerPart + 1) % NUM_TRACER_PARTS];
	tracerPart.pos = pos;
	tracerPart.dir = dir;
	tracerPart.speedf = speed.w;

	unsigned int i = 0;

	for (float curStep = 0.0f, maxStep = speed.w + 0.6f; curStep < maxStep && i < MAX_NUM_AGEMODS; curStep += TRACER_PARTS_STEP, ++i) {
		tracerPart.ageMods[i] = (missileAge < 20) ? 1.0f : (0.6f + (guRNG.NextFloat() * 0.8f));
	}

	tracerPart.numAgeMods = i;
}

void CStarburstProjectile::UpdateSmokeTrail()
{
	trailAge++;
	numParts++;

	if (!leaveSmokeTrail)
		return;

	if (smokeTrail != nullptr)
		smokeTrail->UpdateEndPos(oldSmoke = pos, oldSmokeDir = dir);

	if ((trailAge % SMOKE_INTERVAL) != 0)
		return;

	smokeTrail = projMemPool.alloc<CSmokeTrailProjectile>(
		owner(),
		weaponDef->visuals.texture2,
		pos, oldSmoke,
		dir, oldSmokeDir,
		SMOKE_LIFETIME,
		7.0f,
		0.7f,
		trailAge == SMOKE_INTERVAL,
		false
	);

	numParts = 0;
	useAirLos = smokeTrail->useAirLos;
}

void CStarburstProjectile::Draw(GL::RenderDataBufferTC* va) const
{
	const auto wt3 = weaponDef->visuals.texture3;
	const auto wt1 = weaponDef->visuals.texture1;

	const SColor lightYellow(255, 200, 150, 1);
	const SColor lightRed(255, 180, 180, 1);

	unsigned int partNum = curTracerPart;

	for (unsigned int a = 0; a < NUM_TRACER_PARTS; ++a) {
		const TracerPart* tracerPart = &tracerParts[partNum];
		const float3& opos = tracerPart->pos;
		const float3& odir = tracerPart->dir;
		const float ospeed = tracerPart->speedf;

		float curStep = 0.0f;

		for (int ageModIdx = 0, numAgeMods = tracerPart->numAgeMods; ageModIdx < numAgeMods; curStep += TRACER_PARTS_STEP, ++ageModIdx) {
			const float ageMod = tracerPart->ageMods[ageModIdx];
			const float age2 = (a + (curStep / (ospeed + 0.01f))) * 0.2f;
			const float drawsize = 1.0f + age2 * 0.8f * ageMod * 7;
			const float alpha = (missileAge >= 20)? ((1.0f - age2) * std::max(0.0f, 1.0f - age2)): Square(1.0f - age2);

			const float3 interPos = opos - (odir * ((a * 0.5f) + curStep));

			SColor col = lightYellow * Clamp(alpha, 0.0f, 1.0f);
			col.a = 1;

			va->SafeAppend({interPos - camera->GetRight() * drawsize - camera->GetUp() * drawsize, wt3->xstart, wt3->ystart, col});
			va->SafeAppend({interPos + camera->GetRight() * drawsize - camera->GetUp() * drawsize, wt3->xend,   wt3->ystart, col});
			va->SafeAppend({interPos + camera->GetRight() * drawsize + camera->GetUp() * drawsize, wt3->xend,   wt3->yend,   col});

			va->SafeAppend({interPos + camera->GetRight() * drawsize + camera->GetUp() * drawsize, wt3->xend,   wt3->yend,   col});
			va->SafeAppend({interPos - camera->GetRight() * drawsize + camera->GetUp() * drawsize, wt3->xstart, wt3->yend,   col});
			va->SafeAppend({interPos - camera->GetRight() * drawsize - camera->GetUp() * drawsize, wt3->xstart, wt3->ystart, col});
		}

		// unsigned, so LHS will wrap around to UINT_MAX
		partNum = std::min(partNum - 1, NUM_TRACER_PARTS - 1);
	}
	{
		// draw the engine flare
		constexpr float fsize = 25.0f;

		va->SafeAppend({drawPos - camera->GetRight() * fsize - camera->GetUp() * fsize, wt1->xstart, wt1->ystart, lightRed});
		va->SafeAppend({drawPos + camera->GetRight() * fsize - camera->GetUp() * fsize, wt1->xend,   wt1->ystart, lightRed});
		va->SafeAppend({drawPos + camera->GetRight() * fsize + camera->GetUp() * fsize, wt1->xend,   wt1->yend,   lightRed});

		va->SafeAppend({drawPos + camera->GetRight() * fsize + camera->GetUp() * fsize, wt1->xend,   wt1->yend,   lightRed});
		va->SafeAppend({drawPos - camera->GetRight() * fsize + camera->GetUp() * fsize, wt1->xstart, wt1->yend,   lightRed});
		va->SafeAppend({drawPos - camera->GetRight() * fsize - camera->GetUp() * fsize, wt1->xstart, wt1->ystart, lightRed});
	}
}

int CStarburstProjectile::ShieldRepulse(const float3& shieldPos, float shieldForce, float shieldMaxSpeed)
{
	if (ttl <= 0)
		return 0;

	const float3 rdir = (pos - shieldPos).Normalize();
	float3 dif2 = rdir - dir;

	// steer away twice as fast as we can steer toward target
	const float tracking = std::max(shieldForce * 0.05f, weaponDef->turnrate * 2.0f);

	if (dif2.SqLength() < Square(tracking)) {
		dir = rdir;
	} else {
		dif2 = (dif2 - (dir * (dif2.dot(dir)))).Normalize();
		dir = (dir + (dif2 * tracking)).Normalize();
	}

	return 2;
}

