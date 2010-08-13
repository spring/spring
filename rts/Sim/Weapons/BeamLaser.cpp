/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "BeamLaser.h"
#include "Game/GameHelper.h"
#include "Sim/Misc/TeamHandler.h"
#include "Map/Ground.h"
#include "Matrix44f.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/InterceptHandler.h"
#include "Sim/MoveTypes/AirMoveType.h"
#include "Sim/Projectiles/WeaponProjectiles/BeamLaserProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/LargeBeamLaserProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/LaserProjectile.h"
#include "Sim/Units/COB/UnitScript.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitTypes/Building.h"
#include "PlasmaRepulser.h"
#include "WeaponDefHandler.h"
#include "mmgr.h"

CR_BIND_DERIVED(CBeamLaser, CWeapon, (NULL));

CR_REG_METADATA(CBeamLaser,(
	CR_MEMBER(color),
	CR_MEMBER(oldDir),
	CR_MEMBER(damageMul),
	CR_RESERVED(16)
	));

CBeamLaser::CBeamLaser(CUnit* owner)
: CWeapon(owner),
	oldDir(ZeroVector),
	damageMul(0),
	lastFireFrame(0)
{
}

CBeamLaser::~CBeamLaser(void)
{
}

void CBeamLaser::Update(void)
{
	if (targetType != Target_None) {
		weaponPos =
			owner->pos +
			owner->frontdir * relWeaponPos.z +
			owner->updir    * relWeaponPos.y +
			owner->rightdir * relWeaponPos.x;
		weaponMuzzlePos =
			owner->pos +
			owner->frontdir * relWeaponMuzzlePos.z +
			owner->updir    * relWeaponMuzzlePos.y +
			owner->rightdir * relWeaponMuzzlePos.x;

		if (!onlyForward) {
			wantedDir = targetPos - weaponPos;
			wantedDir.Normalize();
		}

		if (!weaponDef->beamburst) {
			predict = salvoSize / 2;
		} else {
 			// beamburst tracks the target during the burst so there's no need to lead
			predict = 0;
		}
	}
	CWeapon::Update();

	if (lastFireFrame > gs->frameNum - 18 && lastFireFrame != gs->frameNum  && weaponDef->sweepFire) {
		if (teamHandler->Team(owner->team)->metal >= metalFireCost &&
			teamHandler->Team(owner->team)->energy >= energyFireCost) {

			owner->UseEnergy(energyFireCost / salvoSize);
			owner->UseMetal(metalFireCost / salvoSize);

			const int piece = owner->script->QueryWeapon(weaponNum);
			const CMatrix44f weaponMat = owner->script->GetPieceMatrix(piece);

			const float3 relWeaponPos = weaponMat.GetPos();
			const float3 dir =
				owner->frontdir * weaponMat[10] +
				owner->updir    * weaponMat[ 6] +
				owner->rightdir * weaponMat[ 2];

			weaponPos =
				owner->pos +
				owner->frontdir * -relWeaponPos.z +
				owner->updir    *  relWeaponPos.y +
				owner->rightdir * -relWeaponPos.x;

			FireInternal(dir, true);
		}
	}
}

bool CBeamLaser::TryTarget(const float3& pos, bool userTarget, CUnit* unit)
{
	if (!CWeapon::TryTarget(pos, userTarget, unit))
		return false;

	if (!weaponDef->waterweapon) {
		if (unit) {
			if (unit->isUnderWater)
				return false;
		} else {
			if (pos.y < 0)
				return false;
		}
	}

	float3 dir = pos - weaponMuzzlePos;
	float length = dir.Length();

	if (length == 0)
		return true;

	dir /= length;

	if (!onlyForward) {
		// skip ground col testing for aircraft
		float g = ground->LineGroundCol(weaponMuzzlePos, pos);
		if (g > 0 && g < length * 0.9f)
			return false;
	}

	const float spread =
		(accuracy + sprayAngle) *
		(1.0f - owner->limExperience * weaponDef->ownerExpAccWeight);

	if (avoidFeature && helper->LineFeatureCol(weaponMuzzlePos, dir, length)) {
		return false;
	}
	if (avoidFriendly) {
		if (helper->TestAllyCone(weaponMuzzlePos, dir, length, spread, owner->allyteam, owner))
			return false;
	}
	if (avoidNeutral) {
		if (helper->TestNeutralCone(weaponMuzzlePos, dir, length, spread, owner))
			return false;
	}

	return true;
}

void CBeamLaser::Init(void)
{
	if (!weaponDef->beamburst) {
		salvoDelay = 0;
		salvoSize = (int) (weaponDef->beamtime * 30);

		if (salvoSize <= 0)
			salvoSize = 1;

		// multiply damage with this on each shot so the total damage done is correct
		damageMul = 1.0f / (float) salvoSize;
	} else {
		damageMul = 1.0f;
	}

	CWeapon::Init();
	muzzleFlareSize = 0;
}

void CBeamLaser::FireImpl(void)
{
	float3 dir;
	if (onlyForward && dynamic_cast<CAirMoveType*>(owner->moveType)) {
		// the taairmovetype can't align itself properly, change back when that is fixed
		dir = owner->frontdir;
	} else {
		if (salvoLeft == salvoSize - 1) {
			dir = targetPos - weaponMuzzlePos;
			dir.Normalize();
			oldDir = dir;
		} else if (weaponDef->beamburst) {
			dir = targetPos - weaponMuzzlePos;
			dir.Normalize();
		} else {
			dir = oldDir;
		}
	}

	dir += ((salvoError) * (1.0f - owner->limExperience * weaponDef->ownerExpAccWeight));
	dir.Normalize();

	FireInternal(dir, false);
}

void CBeamLaser::FireInternal(float3 dir, bool sweepFire)
{
	float rangeMod = 1.0f;

	if (dynamic_cast<CBuilding*>(owner) == NULL) {
		// help units fire while chasing
		rangeMod = 1.3f;
	}

	if (owner->directControl) {
		rangeMod = 0.95f;
	}

	float maxLength = range * rangeMod;
	float curLength = 0.0f;

	float3 curPos = weaponMuzzlePos;
	float3 hitPos;

	dir +=
		((gs->randVector() * sprayAngle *
		(1.0f - owner->limExperience * weaponDef->ownerExpAccWeight)));
	dir.Normalize();

	bool tryAgain = true;

	// increase range if targets are searched for in a cylinder
	if (cylinderTargetting > 0.01f) {
		// const float3 up(0, owner->radius*cylinderTargetting, 0);
		// const float uplen = up.dot(dir);
		const float uplen = owner->radius * cylinderTargetting * dir.y;
		maxLength = math::sqrt(maxLength * maxLength + uplen * uplen);
	}

	// increase range if targetting edge of hitsphere
	if (targetType == Target_Unit && targetUnit && targetBorder != 0) {
		maxLength += targetUnit->radius * targetBorder;
	}

	// unit at the end of the beam
	const CUnit* hit = 0;
	const CFeature* hitfeature = 0;
	for (int tries = 0; tries < 5 && tryAgain; ++tries) {
		tryAgain = false;
		hit = NULL;

		float length = helper->TraceRay(
			curPos,
			dir,
			maxLength - curLength,
			weaponDef->damages[0],
			owner,
			hit,
			collisionFlags,
			&hitfeature
		);

		if (hit && hit->allyteam == owner->allyteam && sweepFire) {
			// never damage friendlies with sweepfire
			lastFireFrame = 0;
			return;
		}

		float3 newDir;
		CPlasmaRepulser* shieldHit = NULL;
		const float shieldLength = interceptHandler.AddShieldInterceptableBeam(this, curPos, dir, length, newDir, shieldHit);

		if (shieldLength < length) {
			length = shieldLength;

			if (shieldHit->BeamIntercepted(this, damageMul)) {
				// repulsed
				tryAgain = true;
			}
		}

		hitPos = curPos + dir * length;

		const float baseAlpha  = weaponDef->intensity * 255.0f;
		const float startAlpha = (1.0f - (curLength         ) / (range * 1.3f)) * baseAlpha;
		const float endAlpha   = (1.0f - (curLength + length) / (range * 1.3f)) * baseAlpha;

		if (weaponDef->largeBeamLaser) {
			new CLargeBeamLaserProjectile(curPos, hitPos, color, weaponDef->visuals.color2, owner, weaponDef);
		} else {
			new CBeamLaserProjectile(
				curPos, hitPos,
				startAlpha, endAlpha,
				color, weaponDef->visuals.color2,
				owner,
				weaponDef->thickness,
				weaponDef->corethickness,
				weaponDef->laserflaresize,
				weaponDef,
				weaponDef->visuals.beamttl,
				weaponDef->visuals.beamdecay
			);
		}

		curPos = hitPos;
		curLength += length;
		dir = newDir;
	}
	CUnit* hitM = (hit == NULL) ? NULL : uh->units[hit->id];
	CFeature* hitF = hitfeature ? featureHandler->GetFeature(hitfeature->id) : 0;

	// fix negative damage when hitting big spheres
	float actualRange = range;
	if (hit) {
		if (hit->unitDef->usePieceCollisionVolumes) {
			// getting the actual piece here is probably overdoing it
			hitM->SetLastAttackedPiece(hit->localmodel->pieces[0], gs->frameNum);
		}

		if (targetBorder > 0) {
			actualRange += hit->radius * targetBorder;
		}
	}

	// make it possible to always hit with some minimal intensity (melee weapons have use for that)
	const float hitIntensity = std::max(minIntensity, 1.0f - (curLength) / (actualRange * 2));

	if (curLength < maxLength) {
		// Dynamic Damage
		DamageArray dynDamages;

		if (weaponDef->dynDamageExp > 0) {
			dynDamages = weaponDefHandler->DynamicDamages(
				weaponDef->damages,
				weaponMuzzlePos,
				curPos,
				weaponDef->dynDamageRange > 0?
					weaponDef->dynDamageRange:
					weaponDef->range,
				weaponDef->dynDamageExp,
				weaponDef->dynDamageMin,
				weaponDef->dynDamageInverted
			);
		}
		
		helper->Explosion(
			hitPos,
			weaponDef->dynDamageExp > 0?
				dynDamages * (hitIntensity * damageMul):
				weaponDef->damages * (hitIntensity * damageMul),
			areaOfEffect,
			weaponDef->edgeEffectiveness,
			weaponDef->explosionSpeed,
			owner,
			true,
			1.0f,
			weaponDef->noExplode || weaponDef->noSelfDamage, /*false*/
			weaponDef->impactOnly,                           /*false*/
			weaponDef->explosionGenerator,
			hitM,
			dir,
			weaponDef->id,
			hitF
		);
	}

	if (targetUnit) {
		lastFireFrame = gs->frameNum;
	}
}
