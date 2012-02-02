/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "BeamLaser.h"
#include "Game/GameHelper.h"
#include "Game/TraceRay.h"
#include "Sim/Misc/TeamHandler.h"
#include "Map/Ground.h"
#include "System/Matrix44f.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/InterceptHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/MoveTypes/StrafeAirMoveType.h"
#include "Sim/Projectiles/WeaponProjectiles/BeamLaserProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/LargeBeamLaserProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/LaserProjectile.h"
#include "Sim/Units/Scripts/UnitScript.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitTypes/Building.h"
#include "PlasmaRepulser.h"
#include "WeaponDefHandler.h"
#include "System/mmgr.h"

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
			wantedDir.SafeNormalize();
		}

		if (!weaponDef->beamburst) {
			predict = salvoSize / 2;
		} else {
 			// beamburst tracks the target during the burst so there's no need to lead
			predict = 0;
		}
	}
	CWeapon::Update();

	if (lastFireFrame > gs->frameNum - 18 && lastFireFrame != gs->frameNum && weaponDef->sweepFire) {
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

	if (!weaponDef->waterweapon && TargetUnitOrPositionInWater(pos, unit))
		return false;

	float3 dir = pos - weaponMuzzlePos;
	float length = dir.Length();

	if (length == 0.0f)
		return true;

	dir /= length;

	if (!onlyForward) {
		if (!HaveFreeLineOfFire(weaponMuzzlePos, dir, length, unit)) {
			return false;
		}
	}

	const float spread =
		(accuracy + sprayAngle) *
		(1.0f - owner->limExperience * weaponDef->ownerExpAccWeight);

	if (avoidFeature && TraceRay::LineFeatureCol(weaponMuzzlePos, dir, length)) {
		return false;
	}
	if (avoidFriendly && TraceRay::TestCone(weaponMuzzlePos, dir, length, spread, owner->allyteam, true, false, false, owner)) {
		return false;
	}
	if (avoidNeutral && TraceRay::TestCone(weaponMuzzlePos, dir, length, spread, owner->allyteam, false, true, false, owner)) {
		return false;
	}

	return true;
}

void CBeamLaser::Init(void)
{
	if (!weaponDef->beamburst) {
		salvoDelay = 0;
		salvoSize = int(weaponDef->beamtime * GAME_SPEED);
		salvoSize = std::max(salvoSize, 1);

		// multiply damage with this on each shot so the total damage done is correct
		damageMul = 1.0f / salvoSize;
	} else {
		damageMul = 1.0f;
	}

	CWeapon::Init();
	muzzleFlareSize = 0;
}

void CBeamLaser::FireImpl(void)
{
	float3 dir;
	if (onlyForward && dynamic_cast<CStrafeAirMoveType*>(owner->moveType)) {
		// [?] HoverAirMoveType cannot align itself properly, change back when that is fixed
		dir = owner->frontdir;
	} else {
		if (salvoLeft == salvoSize - 1) {
			dir = targetPos - weaponMuzzlePos;
			dir.SafeNormalize();
			oldDir = dir;
		} else if (weaponDef->beamburst) {
			dir = targetPos - weaponMuzzlePos;
			dir.SafeNormalize();
		} else {
			dir = oldDir;
		}
	}

	dir += ((salvoError) * (1.0f - owner->limExperience * weaponDef->ownerExpAccWeight));
	dir.SafeNormalize();

	FireInternal(dir, false);
}

void CBeamLaser::FireInternal(float3 dir, bool sweepFire)
{
	// fix negative damage when hitting big spheres
	float actualRange = range;
	float rangeMod = 1.0f;

	if (dynamic_cast<CBuilding*>(owner) == NULL) {
		// help units fire while chasing
		rangeMod = 1.3f;
	}

	if (owner->fpsControlPlayer != NULL) {
		rangeMod = 0.95f;
	}

	float maxLength = range * rangeMod;
	float curLength = 0.0f;

	float3 curPos = weaponMuzzlePos;
	float3 hitPos;
	float3 newDir;

	dir +=
		((gs->randVector() * sprayAngle *
		(1.0f - owner->limExperience * weaponDef->ownerExpAccWeight)));
	dir.SafeNormalize();

	bool tryAgain = true;
	bool doDamage = true;

	// increase range if targets are searched for in a cylinder
	if (cylinderTargetting > 0.01f) {
		const float verticalDist = owner->radius * cylinderTargetting * dir.y;
		const float maxLengthModSq = maxLength * maxLength + verticalDist * verticalDist;

		maxLength = math::sqrt(maxLengthModSq);
	}


	// increase range if targetting edge of hitsphere
	if (targetType == Target_Unit && targetUnit && targetBorder != 0) {
		maxLength += (targetUnit->radius * targetBorder);
	}


	// unit at the end of the beam
	CUnit* hitUnit = NULL;
	CFeature* hitFeature = NULL;
	CPlasmaRepulser* hitShield = NULL;

	for (int tries = 0; tries < 5 && tryAgain; ++tries) {
		float length = TraceRay::TraceRay(curPos, dir, maxLength - curLength, collisionFlags, owner, hitUnit, hitFeature);

		if (hitUnit && teamHandler->AlliedTeams(hitUnit->team, owner->team) && sweepFire) {
			// never damage friendlies with sweepfire
			lastFireFrame = 0; doDamage = false; break;
		}

		// if the beam gets intercepted, this modifies newDir
		//
		// we do more than one trace-iteration and set dir to
		// newDir only in the case there is a shield in our way
		const float shieldLength = interceptHandler.AddShieldInterceptableBeam(this, curPos, dir, length, newDir, hitShield);

		if (shieldLength < length) {
			length = shieldLength;
			tryAgain = hitShield->BeamIntercepted(this, damageMul); // repulsed
		} else {
			tryAgain = false;
		}

		if (!weaponDef->waterweapon) {
			// terminate beam at water surface if necessary
			if ((dir.y < 0.0f) && ((curPos.y + dir.y * length) <= 0.0f)) {
				length = curPos.y / -dir.y;
			}
		}

		hitPos = curPos + dir * length;

		const float baseAlpha  = weaponDef->intensity * 255.0f;
		const float startAlpha = (1.0f - (curLength         ) / (range * 1.3f)) * baseAlpha;
		const float endAlpha   = (1.0f - (curLength + length) / (range * 1.3f)) * baseAlpha;

		if (weaponDef->largeBeamLaser) {
			new CLargeBeamLaserProjectile(curPos, hitPos, color, weaponDef->visuals.color2, owner, weaponDef);
		} else {
			new CBeamLaserProjectile(curPos, hitPos, startAlpha, endAlpha, color, owner, weaponDef);
		}

		curPos = hitPos;
		curLength += length;
		dir = newDir;
	}

	if (!doDamage)
		return;

	if (hitUnit) {
		if (hitUnit->unitDef->usePieceCollisionVolumes) {
			// getting the actual piece here is probably overdoing it
			// TODO change this if we really need proper flanking bonus support
			// for beam-lasers
			hitUnit->SetLastAttackedPiece(hitUnit->localmodel->GetRoot(), gs->frameNum);
		}

		if (targetBorder > 0.0f) {
			actualRange += (hitUnit->radius * targetBorder);
		}
	}

	// make it possible to always hit with some minimal intensity (melee weapons have use for that)
	const float hitIntensity = std::max(minIntensity, 1.0f - (curLength) / (actualRange * 2));

	if (curLength < maxLength) {
		const DamageArray& baseDamages = (weaponDef->dynDamageExp <= 0.0f)?
			weaponDef->damages:
			weaponDefHandler->DynamicDamages(
				weaponDef->damages,
				weaponMuzzlePos,
				curPos,
				(weaponDef->dynDamageRange > 0.0f)?
					weaponDef->dynDamageRange:
					weaponDef->range,
				weaponDef->dynDamageExp,
				weaponDef->dynDamageMin,
				weaponDef->dynDamageInverted
			);

		const DamageArray damages = baseDamages * (hitIntensity * damageMul);
		const CGameHelper::ExplosionParams params = {
			hitPos,
			dir,
			damages,
			weaponDef,
			owner,
			hitUnit,
			hitFeature,
			craterAreaOfEffect,
			damageAreaOfEffect,
			weaponDef->edgeEffectiveness,
			weaponDef->explosionSpeed,
			1.0f,                                             // gfxMod
			weaponDef->impactOnly,
			weaponDef->noExplode || weaponDef->noSelfDamage,  // ignoreOwner
			true                                              // damageGround
		};

		helper->Explosion(params);
	}

	if (targetUnit) {
		lastFireFrame = gs->frameNum;
	}
}
