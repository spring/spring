/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/mmgr.h"
#include "System/creg/STL_Map.h"
#include "WeaponDefHandler.h"
#include "Weapon.h"
#include "Game/GameHelper.h"
#include "Game/Player.h"
#include "Game/TraceRay.h"
#include "Lua/LuaRules.h"
#include "Map/Ground.h"
#include "Sim/Misc/CollisionHandler.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/GeometricObjects.h"
#include "Sim/Misc/InterceptHandler.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/MoveTypes/AAirMoveType.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectile.h"
#include "Sim/Units/Scripts/CobInstance.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/Unit.h"
#include "System/EventHandler.h"
#include "System/float3.h"
#include "System/myMath.h"
#include "System/Sync/SyncTracer.h"
#include "System/Sound/SoundChannels.h"

CR_BIND_DERIVED(CWeapon, CObject, (NULL));

CR_REG_METADATA(CWeapon, (
	CR_MEMBER(owner),
	CR_MEMBER(range),
	CR_MEMBER(heightMod),
	CR_MEMBER(reloadTime),
	CR_MEMBER(reloadStatus),
	CR_MEMBER(salvoLeft),
	CR_MEMBER(salvoDelay),
	CR_MEMBER(salvoSize),
	CR_MEMBER(nextSalvo),
	CR_MEMBER(predict),
	CR_MEMBER(targetUnit),
	CR_MEMBER(accuracy),
	CR_MEMBER(projectileSpeed),
	CR_MEMBER(predictSpeedMod),
	CR_MEMBER(metalFireCost),
	CR_MEMBER(energyFireCost),
	CR_MEMBER(fireSoundId),
	CR_MEMBER(fireSoundVolume),
	CR_MEMBER(hasBlockShot),
	CR_MEMBER(hasTargetWeight),
	CR_MEMBER(angleGood),
	CR_MEMBER(avoidTarget),
	CR_MEMBER(haveUserTarget),
	CR_MEMBER(subClassReady),
	CR_MEMBER(onlyForward),
	CR_MEMBER(muzzleFlareSize),
	CR_MEMBER(craterAreaOfEffect),
	CR_MEMBER(damageAreaOfEffect),

	CR_MEMBER(badTargetCategory),
	CR_MEMBER(onlyTargetCategory),
	CR_MEMBER(incomingProjectiles),
//	CR_MEMBER(weaponDef),
	CR_MEMBER(stockpileTime),
	CR_MEMBER(buildPercent),
	CR_MEMBER(numStockpiled),
	CR_MEMBER(numStockpileQued),
	CR_MEMBER(interceptTarget),
	CR_MEMBER(salvoError),
	CR_ENUM_MEMBER(targetType),
	CR_MEMBER(sprayAngle),
	CR_MEMBER(useWeaponPosForAim),

	CR_MEMBER(lastRequest),
	CR_MEMBER(lastTargetRetry),
	CR_MEMBER(lastErrorVectorUpdate),

	CR_MEMBER(slavedTo),
	CR_MEMBER(maxForwardAngleDif),
	CR_MEMBER(maxMainDirAngleDif),
	CR_MEMBER(hasCloseTarget),
	CR_MEMBER(avoidFriendly),
	CR_MEMBER(avoidFeature),
	CR_MEMBER(avoidNeutral),
	CR_MEMBER(targetBorder),
	CR_MEMBER(cylinderTargetting),
	CR_MEMBER(minIntensity),
	CR_MEMBER(heightBoostFactor),
	CR_MEMBER(collisionFlags),
	CR_MEMBER(fuelUsage),
	CR_MEMBER(weaponNum),

	CR_MEMBER(relWeaponPos),
	CR_MEMBER(weaponPos),
	CR_MEMBER(relWeaponMuzzlePos),
	CR_MEMBER(weaponMuzzlePos),
	CR_MEMBER(weaponDir),
	CR_MEMBER(mainDir),
	CR_MEMBER(wantedDir),
	CR_MEMBER(lastRequestedDir),
	CR_MEMBER(salvoError),
	CR_MEMBER(errorVector),
	CR_MEMBER(errorVectorAdd),
	CR_MEMBER(targetPos),
	CR_RESERVED(64)
));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CWeapon::CWeapon(CUnit* owner):
	owner(owner),
	weaponDef(0),
	weaponNum(-1),
	haveUserTarget(false),
	craterAreaOfEffect(1.0f),
	damageAreaOfEffect(1.0f),
	muzzleFlareSize(1),
	useWeaponPosForAim(0),
	hasCloseTarget(false),
	reloadTime(1),
	reloadStatus(0),
	range(1),
	heightMod(0),
	projectileSpeed(1),
	accuracy(0),
	sprayAngle(0),
	salvoDelay(0),
	salvoSize(1),
	projectilesPerShot(1),
	nextSalvo(0),
	salvoLeft(0),
	targetType(Target_None),
	targetUnit(0),
	predict(0),
	predictSpeedMod(1),
	metalFireCost(0),
	energyFireCost(0),
	fireSoundId(0),
	fireSoundVolume(0),
	hasBlockShot(false),
	hasTargetWeight(false),
	angleGood(false),
	avoidTarget(false),
	subClassReady(true),
	onlyForward(false),
	badTargetCategory(0),
	onlyTargetCategory(0xffffffff),

	interceptTarget(0),
	stockpileTime(1),
	buildPercent(0),
	numStockpiled(0),
	numStockpileQued(0),

	lastRequest(0),
	lastTargetRetry(-100),
	lastErrorVectorUpdate(0),

	slavedTo(0),
	maxForwardAngleDif(0.0f),
	maxMainDirAngleDif(-1.0f),
	avoidFriendly(true),
	avoidFeature(true),
	avoidNeutral(true),
	targetBorder(0.f),
	cylinderTargetting(0.f),
	minIntensity(0.f),
	heightBoostFactor(-1.f),
	collisionFlags(0),
	fuelUsage(0),

	relWeaponPos(UpVector),
	weaponPos(ZeroVector),
	relWeaponMuzzlePos(UpVector),
	weaponMuzzlePos(ZeroVector),
	weaponDir(ZeroVector),
	mainDir(0.0f, 0.0f, 1.0f),
	wantedDir(UpVector),
	lastRequestedDir(-UpVector),
	salvoError(ZeroVector),
	errorVector(ZeroVector),
	errorVectorAdd(ZeroVector),
	targetPos(1.0f, 1.0f, 1.0f)
{
}


CWeapon::~CWeapon()
{
	if (weaponDef->interceptor)
		interceptHandler.RemoveInterceptorWeapon(this);
}


void CWeapon::SetWeaponNum(int num)
{
	weaponNum = num;

	hasBlockShot = owner->script->HasBlockShot(weaponNum);
	hasTargetWeight = owner->script->HasTargetWeight(weaponNum);
}


inline bool CWeapon::CobBlockShot(const CUnit* targetUnit)
{
	if (!hasBlockShot) {
		return false;
	}

	return owner->script->BlockShot(weaponNum, targetUnit, haveUserTarget);
}


float CWeapon::TargetWeight(const CUnit* targetUnit) const
{
	return owner->script->TargetWeight(weaponNum, targetUnit);
}



static inline bool isBeingServicedOnPad(CUnit* u)
{
	const AAirMoveType *a = dynamic_cast<AAirMoveType*>(u->moveType);
	return (a != NULL && a->GetPadStatus() != 0);
}

void CWeapon::Update()
{
	if (hasCloseTarget) {
		int weaponPiece = -1;
		bool weaponAimed = (useWeaponPosForAim == 0);

		// if we couldn't get a line of fire from the
		// muzzle, try if we can get it from the aim
		// piece
		if (!weaponAimed) {
			weaponPiece = owner->script->QueryWeapon(weaponNum);
		} else {
			weaponPiece = owner->script->AimFromWeapon(weaponNum);
		}

		relWeaponMuzzlePos = owner->script->GetPiecePos(weaponPiece);

		if (!weaponAimed) {
			weaponPiece = owner->script->AimFromWeapon(weaponNum);
		}

		relWeaponPos = owner->script->GetPiecePos(weaponPiece);
	}

	if (targetType == Target_Unit) {
		if (lastErrorVectorUpdate < gs->frameNum - UNIT_SLOWUPDATE_RATE) {
			float3 newErrorVector(gs->randVector());
			errorVectorAdd = (newErrorVector - errorVector) * (1.0f / UNIT_SLOWUPDATE_RATE);
			lastErrorVectorUpdate = gs->frameNum;
		}
		errorVector += errorVectorAdd;
		if (predict > 50000) {
			// to prevent runaway prediction (happens sometimes when a missile
			// is moving *away* from its target), we may need to disable missiles
			// in case they fly around too long
			predict = 50000;
		}

		float3 lead = targetUnit->speed * (weaponDef->predictBoost+predictSpeedMod * (1.0f - weaponDef->predictBoost)) * predict;

		if (weaponDef->leadLimit >= 0.0f && lead.SqLength() > Square(weaponDef->leadLimit + weaponDef->leadBonus * owner->experience)) {
			lead *= (weaponDef->leadLimit + weaponDef->leadBonus*owner->experience) / (lead.Length() + 0.01f);
		}

		targetPos =
			helper->GetUnitErrorPos(targetUnit, owner->allyteam) + lead +
			errorVector * (weaponDef->targetMoveError * GAME_SPEED * targetUnit->speed.Length() * (1.0f - owner->limExperience));

		const float appHeight = ground->GetApproximateHeight(targetPos.x, targetPos.z) + 2.0f;

		if (targetPos.y < appHeight)
			targetPos.y = appHeight;

		if (!weaponDef->waterweapon && targetPos.y < 1.0f)
			targetPos.y = 1.0f;
	}

	if (weaponDef->interceptor) {
		// keep track of the closest projectile heading our way (if any)
		UpdateInterceptTarget();
	}

	if (targetType != Target_None) {
		if (onlyForward) {
			const float3 goalDir = (targetPos - owner->pos).Normalize();
			const float goalDot = owner->frontdir.dot(goalDir);

			angleGood = (goalDot > maxForwardAngleDif);
		} else if (gs->frameNum >= (lastRequest + (GAME_SPEED >> 1))) {
			// NOTE:
			//   if AimWeapon sets angleGood immediately (ie. before it returns),
			//   the weapon can continuously fire at its maximum rate once every
			//   int(reloadTime / owner->reloadSpeed) frames
			//
			//   if it does not (eg. because AimWeapon always spawns a thread to
			//   aim the weapon and defers setting angleGood to it) then this can
			//   lead to irregular/stuttering firing behavior, even in scenarios
			//   when the weapon does not have to re-aim --> detecting this case
			//   is the responsibility of the script
			angleGood = false;

			lastRequestedDir = wantedDir;
			lastRequest = gs->frameNum;

			const float heading = GetHeadingFromVectorF(wantedDir.x, wantedDir.z);
			const float pitch = math::asin(Clamp(wantedDir.dot(owner->updir), -1.0f, 1.0f));

			// call AimWeapon every N=15 frames regardless of current angleGood state
			// for COB, this sets anglegood to return value of AimWeapon when it finished,
			// for Lua, there exists a callout to set the angleGood member.
			// FIXME: convert CSolidObject::heading to radians too.
			owner->script->AimWeapon(weaponNum, ClampRad(heading - owner->heading * TAANG2RAD), pitch);
		}
	}
	if (weaponDef->stockpile && numStockpileQued) {
		const float p = 1.0f / stockpileTime;

		if (teamHandler->Team(owner->team)->metal >= metalFireCost*p && teamHandler->Team(owner->team)->energy >= energyFireCost*p) {
			owner->UseEnergy(energyFireCost * p);
			owner->UseMetal(metalFireCost * p);
			buildPercent += p;
		} else {
			// update the energy and metal required counts
			teamHandler->Team(owner->team)->energyPull += energyFireCost * p;
			teamHandler->Team(owner->team)->metalPull += metalFireCost * p;
		}
		if (buildPercent >= 1) {
			const int oldCount = numStockpiled;
			buildPercent=0;
			numStockpileQued--;
			numStockpiled++;
			owner->commandAI->StockpileChanged(this);
			eventHandler.StockpileChanged(owner, this, oldCount);
		}
	}

	bool canFire = true;
	const CPlayer* fpsPlayer = owner->fpsControlPlayer;

	canFire = canFire && angleGood;
	canFire = canFire && subClassReady;
	canFire = canFire && (salvoLeft == 0);
	canFire = canFire && (targetType != Target_None);
	canFire = canFire && (reloadStatus <= gs->frameNum);
	canFire = canFire && (!weaponDef->stockpile || numStockpiled);
	canFire = canFire && (weaponDef->fireSubmersed || (weaponMuzzlePos.y > 0));
	canFire = canFire && ((fpsPlayer == NULL)
		 || fpsPlayer->fpsController.mouse1
		 || fpsPlayer->fpsController.mouse2);
	canFire = canFire && ((owner->unitDef->maxFuel == 0) || (owner->currentFuel > 0) || (fuelUsage == 0));
	canFire = canFire && !isBeingServicedOnPad(owner);

	if (canFire) {
		if ((weaponDef->stockpile ||
		     (teamHandler->Team(owner->team)->metal >= metalFireCost &&
		      teamHandler->Team(owner->team)->energy >= energyFireCost)))
		{
			const int piece = owner->script->QueryWeapon(weaponNum);
			owner->script->GetEmitDirPos(piece, relWeaponMuzzlePos, weaponDir);
			weaponMuzzlePos = owner->pos + owner->frontdir * relWeaponMuzzlePos.z +
			                               owner->updir    * relWeaponMuzzlePos.y +
			                               owner->rightdir * relWeaponMuzzlePos.x;
			useWeaponPosForAim = reloadTime / 16 + 8;
			weaponDir = owner->frontdir * weaponDir.z +
			            owner->updir    * weaponDir.y +
			            owner->rightdir * weaponDir.x;

			weaponDir.SafeNormalize();

			if (TryTarget(targetPos, haveUserTarget, targetUnit) && !CobBlockShot(targetUnit)) {
				if (weaponDef->stockpile) {
					const int oldCount = numStockpiled;
					numStockpiled--;
					owner->commandAI->StockpileChanged(this);
					eventHandler.StockpileChanged(owner, this, oldCount);
				} else {
					owner->UseEnergy(energyFireCost);
					owner->UseMetal(metalFireCost);
					owner->currentFuel = std::max(0.0f, owner->currentFuel - fuelUsage);
				}

				reloadStatus = gs->frameNum + int(reloadTime / owner->reloadSpeed);

				salvoLeft = salvoSize;
				nextSalvo = gs->frameNum;
				salvoError = gs->randVector() * (owner->isMoving? weaponDef->movingAccuracy: accuracy);

				if (targetType == Target_Pos || (targetType == Target_Unit && !(targetUnit->losStatus[owner->allyteam] & LOS_INLOS))) {
					// area firing stuff is too effective at radar firing...
					salvoError *= 1.3f;
				}

				owner->lastMuzzleFlameSize = muzzleFlareSize;
				owner->lastMuzzleFlameDir = wantedDir;
				owner->script->FireWeapon(weaponNum);
			}
		} else {
			// FIXME  -- never reached?
			if (TryTarget(targetPos, haveUserTarget, targetUnit) && !weaponDef->stockpile) {
				// update the energy and metal required counts
				const int minPeriod = std::max(1, (int)(reloadTime / owner->reloadSpeed));
				const float averageFactor = 1.0f / (float)minPeriod;
				teamHandler->Team(owner->team)->energyPull += averageFactor * energyFireCost;
				teamHandler->Team(owner->team)->metalPull += averageFactor * metalFireCost;
			}
		}
	}

	if (salvoLeft && nextSalvo <= gs->frameNum) {
		salvoLeft--;
		nextSalvo = gs->frameNum + salvoDelay;
		owner->lastFireWeapon = gs->frameNum;

		int projectiles = projectilesPerShot;

		while (projectiles > 0) {
			--projectiles;

			// add to the commandShotCount if this is the last salvo,
			// and it is being directed towards the current target
			// (helps when deciding if a queued ground attack order has been completed)
			if (((salvoLeft == 0) && (owner->commandShotCount >= 0) &&
			    ((targetType == Target_Pos) && (targetPos == owner->userAttackPos))) ||
					((targetType == Target_Unit) && (targetUnit == owner->userTarget))) {
				owner->commandShotCount++;
			}

			owner->script->Shot(weaponNum);

			int piece = owner->script->AimFromWeapon(weaponNum);
			relWeaponPos = owner->script->GetPiecePos(piece);

			piece = owner->script->/*AimFromWeapon*/QueryWeapon(weaponNum);
			owner->script->GetEmitDirPos(piece, relWeaponMuzzlePos, weaponDir);

			weaponPos=owner->pos+owner->frontdir*relWeaponPos.z+owner->updir*relWeaponPos.y+owner->rightdir*relWeaponPos.x;

			weaponMuzzlePos=owner->pos+owner->frontdir*relWeaponMuzzlePos.z+owner->updir*relWeaponMuzzlePos.y+owner->rightdir*relWeaponMuzzlePos.x;
			weaponDir = owner->frontdir * weaponDir.z + owner->updir * weaponDir.y + owner->rightdir * weaponDir.x;
			weaponDir.SafeNormalize();

			if (owner->unitDef->decloakOnFire && (owner->scriptCloak <= 2)) {
				if (owner->isCloaked) {
					owner->isCloaked = false;
					eventHandler.UnitDecloaked(owner);
				}
				owner->curCloakTimeout = gs->frameNum + owner->cloakTimeout;
			}

			Fire();
		}

		//Rock the unit in the direction of the fireing
		if (owner->script->HasRockUnit()) {
			float3 rockDir = wantedDir;
			rockDir.y = 0.0f;
			rockDir = -rockDir.SafeNormalize();
			owner->script->RockUnit(rockDir);
		}

		owner->commandAI->WeaponFired(this);

		if (salvoLeft == 0) {
			owner->script->EndBurst(weaponNum);
		}

#ifdef TRACE_SYNC
	tracefile << "Weapon fire: ";
	tracefile << weaponPos.x << " " << weaponPos.y << " " << weaponPos.z << " " << targetPos.x << " " << targetPos.y << " " << targetPos.z << "\n";
#endif
	}
}

bool CWeapon::AttackGround(float3 pos, bool userTarget)
{
	if (!userTarget && weaponDef->noAutoTarget) {
		return false;
	}
	if (weaponDef->interceptor || !weaponDef->canAttackGround) {
		return false;
	}

	if (!weaponDef->waterweapon && (pos.y < 1.0f)) {
		pos.y = 1.0f;
	}

	weaponMuzzlePos =
		owner->pos +
		owner->frontdir * relWeaponMuzzlePos.z +
		owner->updir    * relWeaponMuzzlePos.y +
		owner->rightdir * relWeaponMuzzlePos.x;

	if (weaponMuzzlePos.y < ground->GetHeightReal(weaponMuzzlePos.x, weaponMuzzlePos.z)) {
		// hope that we are underground because we are a popup weapon and will come above ground later
		weaponMuzzlePos = owner->pos + UpVector * 10;
	}

	if (!TryTarget(pos, userTarget, 0))
		return false;
	if (targetUnit) {
		DeleteDeathDependence(targetUnit, DEPENDENCE_TARGETUNIT);
		targetUnit = NULL;
	}

	haveUserTarget = userTarget;
	targetType = Target_Pos;
	targetPos = pos;

	return true;
}

bool CWeapon::AttackUnit(CUnit* unit, bool userTarget)
{
	if ((!userTarget && weaponDef->noAutoTarget))
		return false;
	if (weaponDef->interceptor)
		return false;

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

	if (weaponMuzzlePos.y < ground->GetHeightReal(weaponMuzzlePos.x, weaponMuzzlePos.z)) {
		// hope that we are underground because we are a popup weapon and will come above ground later
		weaponMuzzlePos = owner->pos + UpVector * 10;
	}

	if (!unit) {
		if (targetType != Target_Unit) {
			// make the unit be more likely to keep the current target if user starts to move it
			targetType = Target_None;
		}

		haveUserTarget = false;
		return false;
	}

	float3 tempTargetPos =
		helper->GetUnitErrorPos(unit, owner->allyteam) +
		errorVector * (weaponDef->targetMoveError * GAME_SPEED * unit->speed.Length() * (1.0f - owner->limExperience));

	const float appHeight = ground->GetApproximateHeight(tempTargetPos.x, tempTargetPos.z) + 2.0f;

	if (tempTargetPos.y < appHeight)
		tempTargetPos.y = appHeight;

	if (!TryTarget(tempTargetPos, userTarget, unit))
		return false;

	if (targetUnit) {
		DeleteDeathDependence(targetUnit, DEPENDENCE_TARGETUNIT);
		targetUnit = NULL;
	}

	haveUserTarget = userTarget;
	targetType = Target_Unit;
	targetUnit = unit;
	targetPos = tempTargetPos;

	AddDeathDependence(targetUnit, DEPENDENCE_TARGETUNIT);
	avoidTarget = false;
	return true;
}


void CWeapon::HoldFire()
{
	if (targetUnit) {
		DeleteDeathDependence(targetUnit, DEPENDENCE_TARGETUNIT);
		targetUnit = NULL;
	}
	targetType = Target_None;
	haveUserTarget = false;
}



inline bool CWeapon::AllowWeaponTargetCheck() const
{
	if (luaRules != NULL) {
		const int checkAllowed = luaRules->AllowWeaponTargetCheck(owner->id, weaponNum, weaponDef->id);

		if (checkAllowed >= 0) {
			return checkAllowed;
		}
	}

	if (weaponDef->noAutoTarget)                 { return false; }
	if (owner->fireState < FIRESTATE_FIREATWILL) { return false; }
	if (haveUserTarget)                          { return false; }

	if (targetType == Target_None) { return true; }
	if (avoidTarget)             { return true; }

	if (targetType == Target_Unit) {
		if (targetUnit->category & badTargetCategory) {
			return true;
		}
	}

	if (gs->frameNum > (lastTargetRetry + 65)) {
		return true;
	}

	return false;
}



void CWeapon::SlowUpdate()
{
	SlowUpdate(false);
}

void CWeapon::SlowUpdate(bool noAutoTargetOverride)
{
#ifdef TRACE_SYNC
	tracefile << "Weapon slow update: ";
	tracefile << owner->id << " " << weaponNum <<  "\n";
#endif

	// If we can't get a line of fire from the muzzle, try
	// the aim piece instead (since the weapon may just be
	// turned in a wrong way)
	int weaponPiece = -1;
	bool weaponAimed = (useWeaponPosForAim == 0);

	if (!weaponAimed) {
		weaponPiece = owner->script->QueryWeapon(weaponNum);

		if (useWeaponPosForAim > 1)
			useWeaponPosForAim--;
	} else {
		weaponPiece = owner->script->AimFromWeapon(weaponNum);
	}

	relWeaponMuzzlePos = owner->script->GetPiecePos(weaponPiece);
	weaponMuzzlePos =
		owner->pos +
		owner->frontdir * relWeaponMuzzlePos.z +
		owner->updir    * relWeaponMuzzlePos.y +
		owner->rightdir * relWeaponMuzzlePos.x;
	weaponPos =
		owner->pos +
		owner->frontdir * relWeaponPos.z +
		owner->updir    * relWeaponPos.y +
		owner->rightdir * relWeaponPos.x;

	if (!weaponAimed) {
		weaponPiece = owner->script->AimFromWeapon(weaponNum);
	}

	relWeaponPos = owner->script->GetPiecePos(weaponPiece);

	if (weaponMuzzlePos.y < ground->GetHeightReal(weaponMuzzlePos.x, weaponMuzzlePos.z)) {
		// hope that we are underground because we are a popup weapon and will come above ground later
		weaponMuzzlePos = owner->pos + UpVector * 10;
	}

	predictSpeedMod = 1.0f + (gs->randFloat() - 0.5f) * 2 * (1.0f - owner->limExperience);
	hasCloseTarget = ((targetPos - weaponPos).SqLength() < relWeaponPos.SqLength() * 16);


	if (targetType != Target_None && !TryTarget(targetPos, haveUserTarget, targetUnit)) {
		HoldFire();
	}

	if (targetType == Target_Unit) {
		// stop firing at cloaked targets
		if (targetUnit != NULL && targetUnit->isCloaked && !(targetUnit->losStatus[owner->allyteam] & (LOS_INLOS | LOS_INRADAR)))
			HoldFire();

		if (!haveUserTarget) {
			// stop firing at neutral targets (unless in FAW mode)
			// note: HoldFire sets targetUnit to NULL, so recheck
			if (targetUnit != NULL && targetUnit->neutral && owner->fireState <= FIRESTATE_FIREATWILL)
				HoldFire();

			// stop firing at allied targets
			//
			// this situation (unit keeps attacking its target if the
			// target or the unit switches to an allied team) should
			// be handled by /ally processing now
			if (targetUnit != NULL && teamHandler->Ally(owner->allyteam, targetUnit->allyteam))
				HoldFire();
		}
	}

	if (slavedTo) {
		// use targets from the thing we are slaved to
		if (targetUnit) {
			DeleteDeathDependence(targetUnit, DEPENDENCE_TARGETUNIT);
			targetUnit = NULL;
		}
		targetType = Target_None;

		if (slavedTo->targetType == Target_Unit) {
			const float3 tp =
				helper->GetUnitErrorPos(slavedTo->targetUnit, owner->allyteam) +
				errorVector * (weaponDef->targetMoveError * GAME_SPEED * slavedTo->targetUnit->speed.Length() * (1.0f - owner->limExperience));

			if (TryTarget(tp, false, slavedTo->targetUnit)) {
				targetType = Target_Unit;
				targetUnit = slavedTo->targetUnit;
				targetPos = tp;

				AddDeathDependence(targetUnit, DEPENDENCE_TARGETUNIT);
			}
		} else if (slavedTo->targetType == Target_Pos) {
			if (TryTarget(slavedTo->targetPos, false, 0)) {
				targetType = Target_Pos;
				targetPos = slavedTo->targetPos;
			}
		}
		return;
	}


	if (!noAutoTargetOverride && AllowWeaponTargetCheck()) {
		lastTargetRetry = gs->frameNum;

		std::multimap<float, CUnit*> targets;
		std::multimap<float, CUnit*>::const_iterator nextTargetIt;
		std::multimap<float, CUnit*>::const_iterator lastTargetIt;

		helper->GenerateWeaponTargets(this, targetUnit, targets);

		if (!targets.empty())
			lastTargetIt = --targets.end();

		for (nextTargetIt = targets.begin(); nextTargetIt != targets.end(); ++nextTargetIt) {
			CUnit* nextTargetUnit = nextTargetIt->second;

			if (nextTargetUnit->neutral && (owner->fireState <= FIRESTATE_FIREATWILL)) {
				continue;
			}

			// when only one target is available, <nextTarget> can equal <targetUnit>
			// and we want to attack whether it is in our bad target category or not
			// (if only bad targets are available and this is the last, just pick it)
			if (nextTargetUnit != targetUnit && (nextTargetUnit->category & badTargetCategory)) {
				if (nextTargetIt != lastTargetIt) {
					continue;
				}
			}

			const float weaponLead = weaponDef->targetMoveError * GAME_SPEED * nextTargetUnit->speed.Length();
			const float weaponError = weaponLead * (1.0f - owner->limExperience);

			float3 nextTargetPos = nextTargetUnit->midPos + (errorVector * weaponError);

			const float appHeight = ground->GetApproximateHeight(nextTargetPos.x, nextTargetPos.z) + 2.0f;

			if (nextTargetPos.y < appHeight) {
				nextTargetPos.y = appHeight;
			}

			if (TryTarget(nextTargetPos, false, nextTargetUnit)) {
				if (targetUnit) {
					DeleteDeathDependence(targetUnit, DEPENDENCE_TARGETUNIT);
				}

				targetType = Target_Unit;
				targetUnit = nextTargetUnit;
				targetPos = nextTargetPos;

				AddDeathDependence(targetUnit, DEPENDENCE_TARGETUNIT);
				break;
			}
		}
	}

	if (targetType != Target_None) {
		owner->haveTarget = true;
		if (haveUserTarget) {
			owner->haveUserTarget = true;
		}
	} else {
		// if we can't target anything, try switching aim point
		if (useWeaponPosForAim == 1) {
			useWeaponPosForAim = 0;
		} else {
			useWeaponPosForAim = 1;
		}
	}
}

void CWeapon::DependentDied(CObject* o)
{
	if (o == targetUnit) {
		targetUnit = NULL;
		if (targetType == Target_Unit) {
			targetType = Target_None;
			haveUserTarget = false;
		}
	}

	// NOTE: DependentDied is called from ~CObject-->Detach, object is just barely valid
	if (weaponDef->interceptor || weaponDef->isShield) {
		incomingProjectiles.erase(((CWeaponProjectile*) o)->id);
	}

	if (o == interceptTarget) {
		interceptTarget = NULL;
	}
}

bool CWeapon::TargetUnitOrPositionInWater(const float3& targetPos, const CUnit* targetUnit) const
{
	if (targetUnit != NULL) {
		if (targetUnit->isUnderWater) {
			// target-unit underwater
			return true;
		}
	} else {
		if (targetPos.y < 0.0f) {
			// target-position underwater
			return true;
		}
	}

	return false;
}

bool CWeapon::HaveFreeLineOfFire(const float3& pos, const float3& dir, float length, const CUnit* target) const
{
	CUnit* unit = NULL;
	CFeature* feature = NULL;

	// any non-ballistic turreted weapon by default ignores everything BUT the ground; if
	// the weapon is also set not to collide with the ground, then it ignores everything
	//
	// NOTE:
	//     ballistic weapons (Cannon / Missile icw. trajectoryHeight) do not call this,
	//     they use TrajectoryGroundCol with an external check for the NOGROUND flag
	if ((collisionFlags & Collision::NOGROUND) != 0)
		return true;

	const float g = TraceRay::TraceRay(pos, dir, length, ~Collision::NOGROUND, owner, unit, feature);

	// true iff ground does not block the ray of length <length> from <pos> along <dir>
	return (g <= 0.0f || g >= (length * 0.9f));
}

bool CWeapon::AdjustTargetVectorLength(
	CUnit* targetUnit,
	float3& targetPos,
	float3& targetVec,
	float3& targetDir)
const {
	bool retCode = false;
	const float tbScale = math::fabsf(targetBorder);

	CollisionVolume* cvOld = targetUnit->collisionVolume;
	CollisionVolume  cvNew = CollisionVolume(targetUnit->collisionVolume);
	CollisionQuery   cq;

	// test for "collision" with a temporarily volume
	// (scaled uniformly by the absolute target-border
	// factor)
	cvNew.RescaleAxes(tbScale, tbScale, tbScale);
	cvNew.SetTestType(CollisionVolume::COLVOL_HITTEST_DISC);

	targetUnit->collisionVolume = &cvNew;

	if (CCollisionHandler::DetectHit(targetUnit, weaponMuzzlePos, ZeroVector, NULL)) {
		// our weapon muzzle is inside the target unit's volume; this
		// means we do not need to make any adjustments to targetVec
		targetVec = ZeroVector;
	} else {
		targetDir.SafeNormalize();

		// otherwise, perform a raytrace to find the proper length correction
		// factor for non-spherical coldet volumes based on the ray's ingress
		// (for positive TB values) or egress (for negative TB values) position;
		// this either increases or decreases the length of <targetVec> but does
		// not change its direction
		cvNew.SetTestType(CollisionVolume::COLVOL_HITTEST_CONT);

		// make the ray-segment long enough so it can reach the far side of the
		// scaled collision volume (helps to ensure a ray-intersection is found)
		//
		// note: ray-intersection is NOT guaranteed if the volume itself has a
		// non-zero offset, since here we are "shooting" at the target UNIT's
		// midpoint
		const float3 targetOffset = targetDir * (cvNew.GetBoundingRadius() * 2.0f);
		const float3 targetRayPos = targetPos + targetOffset;

		if (CCollisionHandler::DetectHit(targetUnit, weaponMuzzlePos, targetRayPos, &cq)) {
			if (targetBorder > 0.0f) { targetVec -= (targetDir * ((targetPos - cq.p0).Length())); }
			if (targetBorder < 0.0f) { targetVec += (targetDir * ((cq.p1 - targetPos).Length())); }
		}

		retCode = true;
	}

	targetUnit->collisionVolume = cvOld;

	// true indicates we took the else-branch and targetDir is now normalized
	return retCode;
}

// if targetUnit != NULL, this checks our onlyTargetCategory against unit->category
// etc. as well as range, otherwise the only concern is range and angular difference
// (terrain is NOT checked here, subclasses do that)
bool CWeapon::TryTarget(const float3& tgtPos, bool /*userTarget*/, CUnit* targetUnit)
{
	if (targetUnit && !(onlyTargetCategory & targetUnit->category)) {
		return false;
	}
	if (targetUnit && ((targetUnit->isDead   && (modInfo.fireAtKilled   == 0)) ||
	                   (targetUnit->crashing && (modInfo.fireAtCrashing == 0)))) {
		return false;
	}
	if (weaponDef->stockpile && !numStockpiled) {
		return false;
	}

	float3 targetPos = tgtPos;
	float3 targetVec = targetPos - weaponMuzzlePos;
	float3 targetDir = targetVec;

	float heightDiff = 0.0f; // negative when target below owner
	float weaponRange = 0.0f; // range modified by heightDiff and cylinderTargetting
	bool targetDirNormalized = false;

	if (targetBorder != 0.0f && targetUnit != NULL) {
		// adjust the length of <targetVec> based on the targetBorder factor
		targetDirNormalized = AdjustTargetVectorLength(targetUnit, targetPos, targetVec, targetDir);
		targetPos.y = weaponPos.y + targetVec.y;
	}

	heightDiff = targetPos.y - owner->pos.y;

	if (targetUnit == NULL || cylinderTargetting < 0.01f) {
		// check range in a sphere (with extra radius <heightDiff * heightMod>)
		weaponRange = GetRange2D(heightDiff * heightMod);
	} else {
		// check range in a cylinder (with height <cylinderTargetting * range>)
		if ((cylinderTargetting * range) > (math::fabsf(heightDiff) * heightMod)) {
			weaponRange = GetRange2D(0.0f);
		}
	}

	if (targetVec.SqLength2D() >= (weaponRange * weaponRange))
		return false;

	if (maxMainDirAngleDif > -1.0f) {
		// NOTE: mainDir is in unit-space, worldMainDir is in world-space
		const float3 targetNormDir = targetDirNormalized? targetDir: targetDir.SafeNormalize();
		const float3 worldMainDir =
			owner->frontdir * mainDir.z +
			owner->rightdir * mainDir.x +
			owner->updir    * mainDir.y;

		if (worldMainDir.dot(targetNormDir) < maxMainDirAngleDif)
			return false;
	}

	return true;
}

bool CWeapon::TryTarget(CUnit* unit, bool userTarget) {
	float3 tempTargetPos =
		helper->GetUnitErrorPos(unit, owner->allyteam) +
		errorVector * (weaponDef->targetMoveError * GAME_SPEED * unit->speed.Length() * (1.0f - owner->limExperience));

	const float appHeight = ground->GetApproximateHeight(tempTargetPos.x, tempTargetPos.z) + 2.0f;

	if (tempTargetPos.y < appHeight) {
		tempTargetPos.y = appHeight;
	}
	return TryTarget(tempTargetPos, userTarget, unit);
}

bool CWeapon::TryTargetRotate(CUnit* unit, bool userTarget) {
	float3 tempTargetPos =
		helper->GetUnitErrorPos(unit, owner->allyteam) +
		errorVector * (weaponDef->targetMoveError * GAME_SPEED * unit->speed.Length() * (1.0f - owner->limExperience));

	const float appHeight = ground->GetApproximateHeight(tempTargetPos.x, tempTargetPos.z) + 2.0f;

	if (tempTargetPos.y < appHeight) {
		tempTargetPos.y = appHeight;
	}

	const short weaponHeading = GetHeadingFromVector(mainDir.x, mainDir.z);
	const short enemyHeading = GetHeadingFromVector(tempTargetPos.x - weaponPos.x, tempTargetPos.z - weaponPos.z);

	return TryTargetHeading(enemyHeading - weaponHeading, tempTargetPos, userTarget, unit);
}

bool CWeapon::TryTargetRotate(float3 pos, bool userTarget) {
	if (!userTarget && weaponDef->noAutoTarget) {
		return false;
	}
	if (weaponDef->interceptor || !weaponDef->canAttackGround) {
		return false;
	}

	if (!weaponDef->waterweapon && pos.y < 1) {
		pos.y = 1;
	}

	const short weaponHeading = GetHeadingFromVector(mainDir.x, mainDir.z);
	const short enemyHeading = GetHeadingFromVector(
		pos.x - weaponPos.x, pos.z - weaponPos.z);

	return TryTargetHeading(enemyHeading - weaponHeading, pos, userTarget, 0);
}

bool CWeapon::TryTargetHeading(short heading, float3 pos, bool userTarget, CUnit* unit) {
	const float3 tempfrontdir(owner->frontdir);
	const float3 temprightdir(owner->rightdir);
	const short tempHeading = owner->heading;

	owner->heading = heading;
	owner->frontdir = GetVectorFromHeading(owner->heading);
	owner->rightdir = owner->frontdir.cross(owner->updir);

	weaponPos = owner->pos +
		owner->frontdir * relWeaponPos.z +
		owner->updir    * relWeaponPos.y +
		owner->rightdir * relWeaponPos.x;
	weaponMuzzlePos = owner->pos +
		owner->frontdir * relWeaponMuzzlePos.z +
		owner->updir    * relWeaponMuzzlePos.y +
		owner->rightdir * relWeaponMuzzlePos.x;

	const bool val = TryTarget(pos, userTarget, unit);

	owner->frontdir = tempfrontdir;
	owner->rightdir = temprightdir;
	owner->heading = tempHeading;

	weaponPos = owner->pos +
		owner->frontdir * relWeaponPos.z +
		owner->updir    * relWeaponPos.y +
		owner->rightdir * relWeaponPos.x;
	weaponMuzzlePos = owner->pos +
		owner->frontdir * relWeaponMuzzlePos.z +
		owner->updir    * relWeaponMuzzlePos.y +
		owner->rightdir * relWeaponMuzzlePos.x;

	return val;

}

void CWeapon::Init(void)
{
	int piece = owner->script->AimFromWeapon(weaponNum);
	relWeaponPos = owner->script->GetPiecePos(piece);
	weaponPos = owner->pos + owner->frontdir * relWeaponPos.z + owner->updir * relWeaponPos.y + owner->rightdir * relWeaponPos.x;
	piece = owner->script->QueryWeapon(weaponNum);
	relWeaponMuzzlePos = owner->script->GetPiecePos(piece);
	weaponMuzzlePos = owner->pos + owner->frontdir * relWeaponMuzzlePos.z + owner->updir * relWeaponMuzzlePos.y + owner->rightdir * relWeaponMuzzlePos.x;

	if (range > owner->maxRange) {
		owner->maxRange = range;
	}

	muzzleFlareSize = std::min(damageAreaOfEffect * 0.2f, std::min(1500.f, weaponDef->damages[0]) * 0.003f);

	if (weaponDef->interceptor)
		interceptHandler.AddInterceptorWeapon(this);

	if(weaponDef->stockpile){
		owner->stockpileWeapon = this;
		owner->commandAI->AddStockpileWeapon(this);
	}

	if (weaponDef->isShield) {
		if ((owner->shieldWeapon == NULL) ||
		    (owner->shieldWeapon->weaponDef->shieldRadius < weaponDef->shieldRadius)) {
			owner->shieldWeapon = this;
		}
	}
}

void CWeapon::Fire()
{
#ifdef TRACE_SYNC
	tracefile << weaponDef->name.c_str() << " fire: ";
	tracefile << owner->pos.x << " " << owner->frontdir.x << " " << targetPos.x << " " << targetPos.y << " " << targetPos.z;
	tracefile << sprayAngle << " " <<  " " << salvoError.x << " " << salvoError.z << " " << owner->limExperience << " " << projectileSpeed << "\n";
#endif
	FireImpl();
	if (fireSoundId && (!weaponDef->soundTrigger || salvoLeft == salvoSize - 1))
		Channels::Battle.PlaySample(fireSoundId, owner, fireSoundVolume);
}

void CWeapon::UpdateInterceptTarget(void)
{
	targetType = Target_None;

	float minInterceptTargetDistSq = std::numeric_limits<float>::max();
	float curInterceptTargetDistSq = std::numeric_limits<float>::min();

	for (std::map<int, CWeaponProjectile*>::iterator pi = incomingProjectiles.begin(); pi != incomingProjectiles.end(); ++pi) {
		CWeaponProjectile* p = pi->second;

		// set by CWeaponProjectile's ctor when the interceptor fires
		if (p->targeted)
			continue;
		if ((curInterceptTargetDistSq = (p->pos - weaponPos).SqLength()) >= minInterceptTargetDistSq)
			continue;

		minInterceptTargetDistSq = curInterceptTargetDistSq;

		// NOTE:
		//     <incomingProjectiles> is sorted by increasing projectile ID
		//     however projectiles launched later in time (which are still
		//     likely out of range) can be assigned *lower* ID's than older
		//     projectiles (which might be almost in range already), so if
		//     we already have an interception target we should not replace
		//     it unless another incoming projectile <p> is closer
		//
		//     this is still not optimal (closer projectiles should receive
		//     higher priority), so just always look for the overall closest
		// if ((interceptTarget != NULL) && ((p->pos - weaponPos).SqLength() >= (interceptTarget->pos - weaponPos).SqLength()))
		//     continue;

		// keep targetPos in sync with the incoming projectile's position
		interceptTarget = p;
		targetType = Target_Intercept;
		targetPos = p->pos;
	}
}

float CWeapon::GetRange2D(float yDiff) const
{
	const float root1 = range * range - yDiff * yDiff;
	if (root1 < 0) {
		return 0;
	} else {
		return sqrt(root1);
	}
}


void CWeapon::StopAttackingAllyTeam(int ally)
{
	if (targetUnit && targetUnit->allyteam == ally) {
		HoldFire();
	}
}
