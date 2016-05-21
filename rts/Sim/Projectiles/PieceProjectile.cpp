/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Sim/Projectiles/PieceProjectile.h"
#include "Game/Camera.h"
#include "Game/GameHelper.h"
#include "Game/GlobalUnsynced.h"
#include "Map/Ground.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Rendering/Colors.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Rendering/Models/3DModel.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Rendering/Env/Particles/Classes/SmokeTrailProjectile.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "System/Matrix44f.h"
#include "System/myMath.h"

#define SMOKE_TIME 40

CR_BIND_DERIVED(CPieceProjectile, CProjectile, )
CR_REG_METADATA(CPieceProjectile,(
	CR_SETFLAG(CF_Synced),

	CR_MEMBER(age),
	CR_MEMBER(explFlags),
	CR_IGNORED(dispList),
	CR_IGNORED(omp),
	CR_IGNORED(smokeTrail),
	CR_IGNORED(fireTrailPoints),
	CR_MEMBER(spinVec),
	CR_MEMBER(spinSpeed),
	CR_MEMBER(spinAngle),
	CR_MEMBER(oldSmokePos),
	CR_MEMBER(oldSmokeDir)
))

CPieceProjectile::CPieceProjectile(
	CUnit* owner,
	LocalModelPiece* lmp,
	const float3& pos,
	const float3& speed,
	int flags,
	float radius
):
	CProjectile(pos, speed, owner, true, false, true),
	age(0),
	explFlags(flags),
	dispList((lmp != nullptr) ? lmp->dispListID : 0),
	omp((lmp != nullptr) ? lmp->original : nullptr),
	smokeTrail(NULL)
{
	checkCol = false;

	if (owner != NULL) {
		if ((explFlags & PF_NoCEGTrail) == 0) {
			explGenHandler->GenExplosion((cegID = owner->unitDef->GetPieceExplosionGeneratorID(gs->randInt())), pos, speed, 100, 0.0f, 0.0f, NULL, NULL);
		}

		model = owner->model;
		explFlags |= (PF_NoCEGTrail * (cegID == -1u));
	}

	oldSmokePos = pos;
	oldSmokeDir = speed;
	oldSmokeDir.Normalize();

	// neither spinVec nor spinSpeed technically
	// needs to be synced, but since instances of
	// this class are themselves synced and have
	// LuaSynced{Ctrl, Read} exposure we treat
	// them that way for consistency
	spinAngle = 0.0f;
	spinVec = gs->randVector().Normalize();
	spinSpeed = gs->randFloat() * 20;

	for (auto& ftp: fireTrailPoints) {
		ftp.pos = pos;
		ftp.size = 0.f;
	}

	SetRadiusAndHeight(radius, 0.0f);
	castShadow = true;

	if (pos.y - CGround::GetApproximateHeight(pos.x, pos.z) > 10) {
		useAirLos = true;
	}

	projectileHandler->AddProjectile(this);
	assert(!detached);
}


void CPieceProjectile::Collision()
{
	Collision(nullptr, nullptr);
	if (gs->randFloat() < 0.666f) { // give it a small chance to `ground bounce`
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
	if (unit && (unit == owner()))
		return;

	if ((explFlags & PF_Explode) && (unit || feature)) {
		const DamageArray damageArray(50.0f);
		const CExplosionParams params = {
			pos,
			ZeroVector,
			damageArray,
			NULL,              // weaponDef
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

	if (explFlags & PF_Smoke) {
		if (explFlags & PF_NoCEGTrail) {
			smokeTrail = new CSmokeTrailProjectile(
				owner(),
				pos, oldSmokePos,
				dir, oldSmokeDir,
				false,
				true,
				14,
				SMOKE_TIME,
				0.5f,
				projectileDrawer->smoketrailtex
			);
		}
	}

	oldSmokePos = pos;
}



float3 CPieceProjectile::RandomVertexPos() const
{
	if (omp == nullptr)
		return ZeroVector;
	#define rf gu->RandFloat()
	return mix(omp->mins, omp->maxs, float3(rf,rf,rf));
}


float CPieceProjectile::GetDrawAngle() const
{
	return spinAngle + spinSpeed * globalRendering->timeOffset;
}


void CPieceProjectile::Update()
{
	if (!luaMoveCtrl) {
		speed.y += mygravity;
		SetVelocityAndSpeed(speed * 0.997f);
		SetPosition(pos + speed);
	}

	spinAngle += spinSpeed;
	age += 1;
	checkCol |= (age > 10);

	if ((explFlags & PF_NoCEGTrail) == 0) {
		// TODO: pass a more sensible ttl to the CEG (age-related?)
		explGenHandler->GenExplosion(cegID, pos, speed, 100, 0.0f, 0.0f, NULL, NULL);
		return;
	}

	if (explFlags & PF_Fire) {
		for (int a = NUM_TRAIL_PARTS - 2; a >= 0; --a) {
			fireTrailPoints[a + 1] = fireTrailPoints[a];
		}

		CMatrix44f m(pos);
		m.Rotate(spinAngle * (PI / 180.0f), spinVec);
		m.Translate(RandomVertexPos());

		fireTrailPoints[0].pos  = m.GetPos();
		fireTrailPoints[0].size = 1 + gu->RandFloat();
	}

	if (explFlags & PF_Smoke) {
		if (smokeTrail) {
			smokeTrail->UpdateEndPos(pos, dir);
			oldSmokePos = pos;
			oldSmokeDir = dir;
		}

		if ((age % 8) == 0) {
			smokeTrail = new CSmokeTrailProjectile(
				owner(),
				pos, oldSmokePos,
				dir, oldSmokeDir,
				age == (NUM_TRAIL_PARTS - 1),
				false,
				14,
				SMOKE_TIME,
				0.5f,
				projectileDrawer->smoketrailtex
			);

			useAirLos = smokeTrail->useAirLos;
		}
	}
}


void CPieceProjectile::DrawOnMinimap(CVertexArray& lines, CVertexArray& points)
{
	points.AddVertexQC(pos, color4::red);
}


void CPieceProjectile::Draw()
{
	if (explFlags & PF_Fire) {
		inArray = true;
		va->EnlargeArrays(NUM_TRAIL_PARTS * 4, 0, VA_SIZE_TC);
		static const SColor lightOrange(1.f, 0.78f, 0.59f, 0.2f);

		for (unsigned int age = 0; age < NUM_TRAIL_PARTS; ++age) {
			const float3 interPos = fireTrailPoints[age].pos;
			const float size = fireTrailPoints[age].size;

			const float alpha = 1.0f - (age * (1.0f / NUM_TRAIL_PARTS));
			const float drawsize = (1.0f + age) * size;
			const SColor col = lightOrange * alpha;

			const auto eft = projectileDrawer->explofadetex;
			va->AddVertexQTC(interPos - camera->GetRight() * drawsize-camera->GetUp() * drawsize, eft->xstart, eft->ystart, col);
			va->AddVertexQTC(interPos + camera->GetRight() * drawsize-camera->GetUp() * drawsize, eft->xend,   eft->ystart, col);
			va->AddVertexQTC(interPos + camera->GetRight() * drawsize+camera->GetUp() * drawsize, eft->xend,   eft->yend,   col);
			va->AddVertexQTC(interPos - camera->GetRight() * drawsize+camera->GetUp() * drawsize, eft->xstart, eft->yend,   col);
		}
	}
}


int CPieceProjectile::GetProjectilesCount() const
{
	return NUM_TRAIL_PARTS;
}
