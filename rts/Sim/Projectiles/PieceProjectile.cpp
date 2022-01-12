/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Sim/Projectiles/PieceProjectile.h"
#include "Game/Camera.h"
#include "Game/GameHelper.h"
#include "Game/GlobalUnsynced.h"
#include "Map/Ground.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "Rendering/Textures/TextureAtlas.h"
#include "Rendering/Colors.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Rendering/Env/Particles/Classes/SmokeTrailProjectile.h"
#include "Rendering/Models/3DModel.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Projectiles/ProjectileMemPool.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "System/Matrix44f.h"
#include "System/SpringMath.h"




CR_BIND_DERIVED(CPieceProjectile, CProjectile, )
CR_REG_METADATA(CPieceProjectile,(
	CR_SETFLAG(CF_Synced),

	CR_MEMBER(age),
	CR_MEMBER(explFlags),
	CR_IGNORED(modelPiece),
	CR_IGNORED(smokeTrail),
	CR_IGNORED(fireTrailPoints),
	CR_MEMBER(spinVector),
	CR_MEMBER(spinParams),
	CR_MEMBER(oldSmokePos),
	CR_MEMBER(oldSmokeDir)
))

CPieceProjectile::CPieceProjectile(
	CUnit* owner, // never null
	LocalModelPiece* lmp, // never null
	const float3& pos,
	const float3& speed,
	int flags,
	float radius
):
	CProjectile(pos, speed, owner, true, false, true),
	age(0),
	explFlags(flags),
	modelPiece(lmp->original),
	smokeTrail(nullptr)
{
	model = owner->model;

	{
		const UnitDef* ud = owner->unitDef;

		if ((explFlags & PF_NoCEGTrail) == 0 && ud->GetPieceExpGenCount() > 0) {
			cegID = guRNG.NextInt(ud->GetPieceExpGenCount());
			cegID = ud->GetPieceExpGenID(cegID);

			explGenHandler.GenExplosion(cegID, pos, speed, 100, 0.0f, 0.0f, nullptr, nullptr);
		}

		explFlags |= (PF_NoCEGTrail * (cegID == -1u));
	}
	{
		// neither spinVector nor spinParams technically need to be
		// synced, but since instances of this class are themselves
		// synced and have LuaSynced{Ctrl, Read} exposure we treat
		// them that way for consistency
		spinVector = gsRNG.NextVector();
		spinParams = {gsRNG.NextFloat() * 20.0f, 0.0f};

		oldSmokePos = pos;
		oldSmokeDir = speed;

		spinVector.Normalize();
		oldSmokeDir.Normalize();
	}

	for (auto& ftp: fireTrailPoints) {
		ftp = {pos, 0.0f};
	}

	checkCol = false;
	castShadow = true;
	useAirLos = ((pos.y - CGround::GetApproximateHeight(pos.x, pos.z)) > 10.0f);

	SetRadiusAndHeight(radius, 0.0f);

	projectileHandler.AddProjectile(this);
	assert(!detached);
}


void CPieceProjectile::Collision()
{
	Collision(nullptr, nullptr);
	if (gsRNG.NextFloat() < 0.666f) { // give it a small chance to `ground bounce`
		CProjectile::Collision();
		return;
	}

	// ground bounce
	const float3& norm = CGround::GetNormal(pos.x, pos.z);
	const float ns = speed.dot(norm);

	SetVelocityAndSpeed(speed - (norm * ns * 1.6f));
	SetPosition(pos + (norm * 0.1f));
}


void CPieceProjectile::Collision(CFeature* f)
{
	Collision(nullptr, f);
	CProjectile::Collision(f);
}


void CPieceProjectile::Collision(CUnit* unit)
{
	Collision(unit, nullptr);
	CProjectile::Collision(unit);
}


void CPieceProjectile::Collision(CUnit* unit, CFeature* feature)
{
	if (unit != nullptr && (unit == owner()))
		return;

	if ((explFlags & PF_Explode) != 0 && (unit != nullptr || feature != nullptr )) {
		const DamageArray damageArray(50.0f);
		const CExplosionParams params = {
			pos,
			ZeroVector,
			damageArray,
			nullptr,           // weaponDef
			owner(),
			unit,              // hitUnit
			feature,           // hitFeature
			0.0f,              // craterAreaOfEffect
			5.0f,              // damageAreaOfEffect
			0.0f,              // edgeEffectiveness
			10.0f,             // explosionSpeed
			1.0f,              // gfxMod
			true,              // impactOnly
			false,             // ignoreOwner
			false,             // damageGround
			static_cast<unsigned int>(id)
		};

		helper->Explosion(params);
	}

	if ((explFlags & PF_Smoke) != 0) {
		if ((explFlags & PF_NoCEGTrail) != 0) {
			const unsigned int smokeTime = TRAIL_SMOKE_TIME;

			smokeTrail = projMemPool.alloc<CSmokeTrailProjectile>(
				owner(),
				projectileDrawer->smoketrailtex,
				pos, oldSmokePos,
				dir, oldSmokeDir,
				smokeTime,
				14.0f,
				0.5f,
				false,
				true
			);
		}
	}

	oldSmokePos = pos;
}



float CPieceProjectile::GetDrawAngle() const
{
	return (spinParams.y + spinParams.x * globalRendering->timeOffset);
}


void CPieceProjectile::Update()
{
	if (!luaMoveCtrl) {
		SetVelocityAndSpeed((speed += (UpVector * mygravity)) * 0.997f);
		SetPosition(pos + speed);
	}

	spinParams.y += spinParams.x;
	age += 1;
	checkCol |= (age > 10);

	if ((explFlags & PF_NoCEGTrail) == 0) {
		// TODO: pass a more sensible ttl to the CEG (age-related?)
		explGenHandler.GenExplosion(cegID, pos, speed, 100, 0.0f, 0.0f, nullptr, nullptr);
		return;
	}

	if ((explFlags & PF_Fire) != 0) {
		for (int a = NUM_TRAIL_PARTS - 2; a >= 0; --a) {
			fireTrailPoints[a + 1] = fireTrailPoints[a];
		}

		CMatrix44f m(pos);

		m.Rotate(spinParams.y * math::DEG_TO_RAD, spinVector);
		m.Translate(mix(modelPiece->mins, modelPiece->maxs, float3(guRNG.NextFloat(), guRNG.NextFloat(), guRNG.NextFloat())));

		fireTrailPoints[0] = {m.GetPos(), 1.0f + guRNG.NextFloat()};
	}

	if ((explFlags & PF_Smoke) != 0) {
		if (smokeTrail != nullptr)
			smokeTrail->UpdateEndPos(oldSmokePos = pos, oldSmokeDir = dir);

		if ((age % 8) == 0) {
			// need the temporary to avoid an undefined ref
			const unsigned int smokeTime = TRAIL_SMOKE_TIME;

			smokeTrail = projMemPool.alloc<CSmokeTrailProjectile>(
				owner(),
				projectileDrawer->smoketrailtex,
				pos, oldSmokePos,
				dir, oldSmokeDir,
				smokeTime,
				14.0f,
				0.5f,
				age == (NUM_TRAIL_PARTS - 1),
				false
			);

			useAirLos = smokeTrail->useAirLos;
		}
	}
}


void CPieceProjectile::DrawOnMinimap(GL::RenderDataBufferC* va)
{
	va->SafeAppend({pos        , color4::red});
	va->SafeAppend({pos + speed, color4::red});
}


void CPieceProjectile::Draw(GL::RenderDataBufferTC* va) const
{
	if ((explFlags & PF_Fire) == 0)
		return;

	const SColor lightOrange(1.0f, 0.78f, 0.59f, 0.2f);
	const auto eft = projectileDrawer->explofadetex;

	for (unsigned int age = 0; age < NUM_TRAIL_PARTS; ++age) {
		const float3 interPos = fireTrailPoints[age];

		const float alpha = 1.0f - (age * (1.0f / NUM_TRAIL_PARTS));
		const float drawsize = (1.0f + age) * fireTrailPoints[age].w;

		const SColor col = lightOrange * alpha;

		va->SafeAppend({interPos - camera->GetRight() * drawsize - camera->GetUp() * drawsize, eft->xstart, eft->ystart, col});
		va->SafeAppend({interPos + camera->GetRight() * drawsize - camera->GetUp() * drawsize, eft->xend,   eft->ystart, col});
		va->SafeAppend({interPos + camera->GetRight() * drawsize + camera->GetUp() * drawsize, eft->xend,   eft->yend,   col});

		va->SafeAppend({interPos + camera->GetRight() * drawsize + camera->GetUp() * drawsize, eft->xend,   eft->yend,   col});
		va->SafeAppend({interPos - camera->GetRight() * drawsize + camera->GetUp() * drawsize, eft->xstart, eft->yend,   col});
		va->SafeAppend({interPos - camera->GetRight() * drawsize - camera->GetUp() * drawsize, eft->xstart, eft->ystart, col});
	}
}

