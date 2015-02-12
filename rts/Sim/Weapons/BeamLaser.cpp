/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "BeamLaser.h"
#include "PlasmaRepulser.h"
#include "WeaponDefHandler.h"
#include "Game/GameHelper.h"
#include "Game/TraceRay.h"
#include "Map/Ground.h"
#include "Sim/Misc/CollisionHandler.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/InterceptHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/MoveTypes/StrafeAirMoveType.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectileFactory.h"
#include "Sim/Units/Scripts/UnitScript.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "System/Matrix44f.h"
#include "System/myMath.h"

#define SWEEPFIRE_ENABLED 1

CR_BIND_DERIVED(CBeamLaser, CWeapon, (NULL, NULL))

CR_REG_METADATA(CBeamLaser,(
	CR_MEMBER(color),
	CR_MEMBER(oldDir),

	CR_MEMBER(salvoDamageMult),
	CR_MEMBER(sweepFireState)
))

CR_BIND(CBeamLaser::SweepFireState, )
CR_REG_METADATA_SUB(CBeamLaser, SweepFireState, (
	CR_MEMBER(sweepInitPos),
	CR_MEMBER(sweepGoalPos),
	CR_MEMBER(sweepInitDir),
	CR_MEMBER(sweepGoalDir),
	CR_MEMBER(sweepCurrDir),
	CR_MEMBER(sweepTempDir),
	CR_MEMBER(sweepInitDst),
	CR_MEMBER(sweepGoalDst),
	CR_MEMBER(sweepCurrDst),
	CR_MEMBER(sweepStartAngle),
	CR_MEMBER(sweepFiring)
))



void CBeamLaser::SweepFireState::Init(const float3& newTargetPos, const float3& muzzlePos)
{
	sweepInitPos = sweepGoalPos;
	sweepInitDst = (sweepInitPos - muzzlePos).Length2D();

	sweepGoalPos = newTargetPos;
	sweepGoalDst = (sweepGoalPos - muzzlePos).Length2D();
	sweepCurrDst = sweepInitDst;

	sweepInitDir = (sweepInitPos - muzzlePos).SafeNormalize();
	sweepGoalDir = (sweepGoalPos - muzzlePos).SafeNormalize();

	sweepStartAngle = math::acosf(Clamp(sweepInitDir.dot(sweepGoalDir), -1.0f, 1.0f));
	sweepFiring = true;
}

float CBeamLaser::SweepFireState::GetTargetDist2D() const {
	if (sweepStartAngle < 0.01f)
		return sweepGoalDst;

	const float sweepCurAngleCos = sweepCurrDir.dot(sweepGoalDir);
	const float sweepCurAngleRad = math::acosf(Clamp(sweepCurAngleCos, -1.0f, 1.0f));

	// goes from 1 to 0 as the angular difference decreases during the sweep
	const float sweepAngleAlpha = (Clamp(sweepCurAngleRad / sweepStartAngle, 0.0f, 1.0f));

	// get the linearly-interpolated beam length for this point of the sweep
	return (mix(sweepInitDst, sweepGoalDst, 1.0f - sweepAngleAlpha));
}



CBeamLaser::CBeamLaser(CUnit* owner, const WeaponDef* def)
	: CWeapon(owner, def)
	, color(def->visuals.color)
	, salvoDamageMult(1.0f)
{
	sweepFireState.SetDamageAllies((collisionFlags & Collision::NOFRIENDLIES) == 0);
}



void CBeamLaser::Init()
{
	if (!weaponDef->beamburst) {
		salvoDelay = 0;
		salvoSize = int(weaponDef->beamtime * GAME_SPEED);
		salvoSize = std::max(salvoSize, 1);

		// multiply damage with this on each shot so the total damage done is correct
		salvoDamageMult = 1.0f / salvoSize;
	}

	CWeapon::Init();

	muzzleFlareSize = 0.0f;
}

void CBeamLaser::UpdatePosAndMuzzlePos()
{
	if (sweepFireState.IsSweepFiring()) {
		const int weaponPiece = owner->script->QueryWeapon(weaponNum);
		const CMatrix44f weaponMat = owner->script->GetPieceMatrix(weaponPiece);

		const float3 relWeaponPos = weaponMat.GetPos();
		const float3 newWeaponDir = owner->GetObjectSpaceVec(float3(weaponMat[2], weaponMat[6], weaponMat[10]));

		relWeaponMuzzlePos = owner->script->GetPiecePos(weaponPiece);

		weaponPos = owner->GetObjectSpacePos(relWeaponPos * float3(-1.0f, 1.0f, -1.0f)); // ??
		weaponMuzzlePos = owner->GetObjectSpacePos(relWeaponMuzzlePos);

		sweepFireState.SetSweepTempDir(newWeaponDir);
	} else {
		weaponPos = owner->GetObjectSpacePos(relWeaponPos);
		weaponMuzzlePos = owner->GetObjectSpacePos(relWeaponMuzzlePos);

		if (weaponDef->sweepFire) {
			// needed for first call to GetFireDir() when new sweep starts after inactivity
			sweepFireState.SetSweepTempDir((weaponMuzzlePos - weaponPos).SafeNormalize());
		}
	}
}

void CBeamLaser::UpdateWantedDir()
{
	if (targetType == Target_None)
		return;

	if (!onlyForward) {
		wantedDir = (targetPos - weaponPos).SafeNormalize();
	}

	if (!weaponDef->beamburst) {
		predict = salvoSize / 2;
	} else {
		// beamburst tracks the target during the burst so there's no need to lead
		predict = 0;
	}
}

void CBeamLaser::UpdateSweep()
{
	// sweeping always happens between targets
	if (targetType == Target_None) {
		sweepFireState.SetSweepFiring(false);
		return;
	}
	if (!weaponDef->sweepFire)
		return;

	#if (!SWEEPFIRE_ENABLED)
	return;
	#endif

	// if current target position changed, start sweeping through a new arc
	if (sweepFireState.StartSweep(targetPos))
		sweepFireState.Init(targetPos, weaponMuzzlePos);

	if (sweepFireState.IsSweepFiring())
		sweepFireState.Update(GetFireDir(true, false));

	// TODO:
	//   also stop sweep if angle no longer changes, spawn
	//   more intermediate beams for large angle and range?
	if (sweepFireState.StopSweep())
		sweepFireState.SetSweepFiring(false);

	if (!sweepFireState.IsSweepFiring())
		return;
	if (reloadStatus > gs->frameNum)
		return;

	if (teamHandler->Team(owner->team)->res.metal < metalFireCost) { return; }
	if (teamHandler->Team(owner->team)->res.energy < energyFireCost) { return; }

	owner->UseEnergy(energyFireCost / salvoSize);
	owner->UseMetal(metalFireCost / salvoSize);

	FireInternal(sweepFireState.GetSweepCurrDir());

	// FIXME:
	//   reloadStatus is normally only set in UpdateFire and only if CanFire
	//   (which is not true during sweeping, the integration should be better)
	reloadStatus = gs->frameNum + int(reloadTime / owner->reloadSpeed);
}

void CBeamLaser::Update()
{
	UpdatePosAndMuzzlePos();
	UpdateWantedDir();

	CWeapon::Update();
	UpdateSweep();
}

float3 CBeamLaser::GetFireDir(bool sweepFire, bool scriptCall)
{
	float3 dir = targetPos - weaponMuzzlePos;

	if (!sweepFire) {
		if (scriptCall) {
			dir = dir.SafeNormalize();
		} else {
			if (onlyForward && owner->unitDef->IsStrafingAirUnit()) {
				// [?] StrafeAirMovetype cannot align itself properly, change back when that is fixed
				dir = owner->frontdir;
			} else {
				if (salvoLeft == salvoSize - 1) {
					oldDir = (dir = dir.SafeNormalize());
				} else if (weaponDef->beamburst) {
					dir = dir.SafeNormalize();
				} else {
					dir = oldDir;
				}
			}

			dir += SalvoErrorExperience();
			dir.SafeNormalize();

			// NOTE:
			//  on units with (extremely) long weapon barrels the muzzle
			//  can be on the far side of the target unit such that <dir>
			//  would point away from it
			if ((targetPos - weaponMuzzlePos).dot(targetPos - owner->aimPos) < 0.0f) {
				dir = -dir;
			}
		}
	} else {
		// need to emit the sweeping beams from the right place
		// NOTE:
		//   this way of implementing sweep-fire is extremely bugged
		//   the intersection points traced by rays from the turning
		//   weapon piece do NOT describe a fluid arc segment between
		//   old and new target positions (nor even a straight line)
		//   --> animation scripts cannot be relied upon to smoothly
		//   vary pitch of the weapon muzzle piece so use workaround
		//
		dir = sweepFireState.GetSweepTempDir();
		dir.Normalize2D();

		const float3 tgtPos = float3(dir.x * sweepFireState.GetTargetDist2D(), 0.0f, dir.z * sweepFireState.GetTargetDist2D());
		const float tgtHgt = CGround::GetHeightReal(weaponMuzzlePos.x + tgtPos.x, weaponMuzzlePos.z + tgtPos.z);

		// NOTE: INTENTIONALLY NOT NORMALIZED HERE
		dir = (tgtPos + UpVector * (tgtHgt - weaponMuzzlePos.y));
	}

	return dir;
}

void CBeamLaser::FireImpl(bool scriptCall)
{
	// sweepfire must exclude regular fire (!)
	if (sweepFireState.IsSweepFiring())
		return;

	FireInternal(GetFireDir(false, scriptCall));
}

void CBeamLaser::FireInternal(float3 curDir)
{
	float actualRange = range;
	float rangeMod = 1.0f;

	if (!owner->unitDef->IsImmobileUnit()) {
		// help units fire while chasing
		rangeMod = 1.3f;
	}
	if (owner->UnderFirstPersonControl()) {
		rangeMod = 0.95f;
	}

	bool tryAgain = true;
	bool doDamage = true;

	float maxLength = range * rangeMod;
	float curLength = 0.0f;

	float3 curPos = weaponMuzzlePos;
	float3 hitPos;
	float3 newDir;

	// objects at the end of the beam
	CUnit* hitUnit = NULL;
	CFeature* hitFeature = NULL;
	CPlasmaRepulser* hitShield = NULL;
	CollisionQuery hitColQuery;

	if (!sweepFireState.IsSweepFiring()) {
		curDir += (gs->randVector() * SprayAngleExperience());
		curDir.SafeNormalize();

		// increase range if targets are searched for in a cylinder
		if (cylinderTargeting > 0.01f) {
			const float verticalDist = owner->radius * cylinderTargeting * curDir.y;
			const float maxLengthModSq = maxLength * maxLength + verticalDist * verticalDist;

			maxLength = math::sqrt(maxLengthModSq);
		}

		// adjust range if targetting edge of hitsphere
		if (targetType == Target_Unit && targetUnit != NULL && targetBorder != 0.0f) {
			maxLength += (targetUnit->radius * targetBorder);
		}
	} else {
		// restrict the range when sweeping
		maxLength = std::min(maxLength, sweepFireState.GetTargetDist3D() * 1.125f);
	}

	for (int tries = 0; tries < 5 && tryAgain; ++tries) {
		float beamLength = TraceRay::TraceRay(curPos, curDir, maxLength - curLength, collisionFlags, owner, hitUnit, hitFeature, &hitColQuery);

		if (hitUnit != NULL && teamHandler->AlliedTeams(hitUnit->team, owner->team)) {
			if (sweepFireState.IsSweepFiring() && !sweepFireState.DamageAllies()) {
				doDamage = false; break;
			}
		}

		if (!weaponDef->waterweapon) {
			// terminate beam at water surface if necessary
			if ((curDir.y < 0.0f) && ((curPos.y + curDir.y * beamLength) <= 0.0f)) {
				beamLength = curPos.y / -curDir.y;
			}
		}

		// if the beam gets intercepted, this modifies newDir
		//
		// we do more than one trace-iteration and set dir to
		// newDir only in the case there is a shield in our way
		const float shieldLength = interceptHandler.AddShieldInterceptableBeam(this, curPos, curDir, beamLength, newDir, hitShield);

		if (shieldLength < beamLength) {
			beamLength = shieldLength;
			tryAgain = hitShield->BeamIntercepted(this, curPos, salvoDamageMult);
		} else {
			tryAgain = false;
		}

		// same as hitColQuery.GetHitPos() if no water or shield in way
		hitPos = curPos + curDir * beamLength;

		{
			const float baseAlpha  = weaponDef->intensity * 255.0f;
			const float startAlpha = (1.0f - (curLength             ) / maxLength);
			const float endAlpha   = (1.0f - (curLength + beamLength) / maxLength);

			ProjectileParams pparams = GetProjectileParams();
			pparams.pos = curPos;
			pparams.end = hitPos;
			pparams.ttl = weaponDef->beamLaserTTL;
			pparams.startAlpha = Clamp(startAlpha * baseAlpha, 0.0f, 255.0f);
			pparams.endAlpha = Clamp(endAlpha * baseAlpha, 0.0f, 255.0f);

			WeaponProjectileFactory::LoadProjectile(pparams);
		}

		curPos = hitPos;
		curDir = newDir;
		curLength += beamLength;
	}

	if (!doDamage)
		return;

	if (hitUnit != NULL) {
		hitUnit->SetLastAttackedPiece(hitColQuery.GetHitPiece(), gs->frameNum);

		if (targetBorder > 0.0f) {
			actualRange += (hitUnit->radius * targetBorder);
		}
	}

	if (curLength < maxLength) {
		// make it possible to always hit with some minimal intensity (melee weapons have use for that)
		const float hitIntensity = std::max(minIntensity, 1.0f - curLength / (actualRange * 2.0f));

		const DamageArray& baseDamages = CWeaponDefHandler::DynamicDamages(weaponDef, weaponMuzzlePos, curPos);
		const DamageArray damages = baseDamages * (hitIntensity * salvoDamageMult);
		const CGameHelper::ExplosionParams params = {
			hitPos,
			curDir,
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
			true,                                             // damageGround
			-1u                                               // projectileID
		};

		helper->Explosion(params);
	}
}
