/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cassert>


#include "MoveType.h"
#include "Map/Ground.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/RadarHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "System/myMath.h"
#include "System/Sync/HsiehHash.h"

CR_BIND_DERIVED_INTERFACE(AMoveType, CObject)
CR_REG_METADATA(AMoveType, (
	CR_MEMBER(owner),
	CR_MEMBER(goalPos),
	CR_MEMBER(oldPos),
	CR_MEMBER(oldSlowUpdatePos),

	CR_MEMBER(maxSpeed),
	CR_MEMBER(maxSpeedDef),
	CR_MEMBER(maxWantedSpeed),
	CR_MEMBER(repairBelowHealth),
	CR_MEMBER(maneuverLeash),

	CR_MEMBER(useHeading),
	CR_MEMBER(progressState)
))

AMoveType::AMoveType(CUnit* owner):
	owner(owner),

	goalPos(owner? owner->pos: ZeroVector),
	oldPos(owner? owner->pos: ZeroVector),
	oldSlowUpdatePos(oldPos),

	useHeading(true),

	progressState(Done),

	maxSpeed(owner? owner->unitDef->speed / GAME_SPEED : 0.0f),
	maxSpeedDef(owner? owner->unitDef->speed / GAME_SPEED : 0.0f),
	maxWantedSpeed(owner? owner->unitDef->speed / GAME_SPEED : 0.0f),

	repairBelowHealth(0.3f),
	maneuverLeash(500.0f)
{
}



void AMoveType::SlowUpdate()
{
	if (owner->pos != oldSlowUpdatePos) {
		oldSlowUpdatePos = owner->pos;

		const int newMapSquare = CGround::GetSquare(owner->pos);

		const float losHeight = owner->losHeight;
		const float radarHeight = owner->radarHeight;

		if (newMapSquare != owner->mapSquare) {
			owner->mapSquare = newMapSquare;

			// if owner is an aircraft, temporarily add current altitude to emit-heights
			// otherwise leave them as-is unless owner floats on water and is over water
			if (!owner->UsingScriptMoveType()) {
				const float agh = CGround::GetApproximateHeight(owner->pos.x, owner->pos.z);
				const float alt = owner->pos.y - agh;

				if (owner->IsInAir() && owner->unitDef->canfly) {
					owner->losHeight += alt;
					owner->radarHeight += alt;
				}
				if (owner->IsInWater() && owner->unitDef->floatOnWater) {
					// agh is (should be) <= 0 here
					owner->losHeight -= agh;
					owner->radarHeight -= agh;
				}

				if ((owner->IsOnGround() || owner->IsInWater()) && owner->unitDef->IsGroundUnit()) {
					// always (re-)add us to occupation map if we moved
					// (since our last SlowUpdate) and are on the ground
					// NOTE: ships are ground units but not on the ground
					owner->Block();
				}
			}

			losHandler->MoveUnit(owner, false);
			radarHandler->MoveUnit(owner);

			// restore emit-heights
			owner->losHeight = losHeight;
			owner->radarHeight = radarHeight;
		}

		quadField->MovedUnit(owner);
	}
}

void AMoveType::KeepPointingTo(CUnit* unit, float distance, bool aggressive)
{
	KeepPointingTo(float3(unit->pos), distance, aggressive);
}

float AMoveType::CalcStaticTurnRadius() const {
	// calculate a rough turn radius (not based on current speed)
	const float turnFrames = SPRING_CIRCLE_DIVS / std::max(owner->unitDef->turnRate, 1.0f);
	const float turnRadius = (maxSpeedDef * turnFrames) / (PI + PI);

	return turnRadius;
}



bool AMoveType::WantsRepair() const { return (owner->health      < (repairBelowHealth * owner->maxHealth)); }
bool AMoveType::WantsRefuel() const { return (owner->currentFuel < (repairBelowHealth * owner->unitDef->maxFuel)); }

bool AMoveType::SetMemberValue(unsigned int memberHash, void* memberValue) {
	#define MEMBER_CHARPTR_HASH(memberName) HsiehHash(memberName, strlen(memberName),     0)
	#define MEMBER_LITERAL_HASH(memberName) HsiehHash(memberName, sizeof(memberName) - 1, 0)

	#define          MAXSPEED_MEMBER_IDX 0
	#define    MAXWANTEDSPEED_MEMBER_IDX 1
	#define REPAIRBELOWHEALTH_MEMBER_IDX 2
	#define     MANEUVERLEASH_MEMBER_IDX 3

	static const unsigned int floatMemberHashes[] = {
		MEMBER_LITERAL_HASH(         "maxSpeed"),
		MEMBER_LITERAL_HASH(   "maxWantedSpeed"),
		MEMBER_LITERAL_HASH("repairBelowHealth"),
		MEMBER_LITERAL_HASH(    "maneuverLeash"),
	};

	#undef MEMBER_CHARPTR_HASH
	#undef MEMBER_LITERAL_HASH

	/*
	// unordered_map etc. perform dynallocs, so KISS here
	float* floatMemberPtrs[] = {
		&maxSpeed,
		&maxWantedSpeed,
		&repairBelowHealth,
	};
	*/

	// special cases
	if (memberHash == floatMemberHashes[MAXSPEED_MEMBER_IDX]) {
		SetMaxSpeed((*reinterpret_cast<float*>(memberValue)) / GAME_SPEED);
		return true;
	}
	if (memberHash == floatMemberHashes[MAXWANTEDSPEED_MEMBER_IDX]) {
		SetWantedMaxSpeed((*reinterpret_cast<float*>(memberValue)) / GAME_SPEED);
		return true;
	}
	if (memberHash == floatMemberHashes[REPAIRBELOWHEALTH_MEMBER_IDX]) {
		SetRepairBelowHealth(*reinterpret_cast<float*>(memberValue));
		return true;
	}
	if (memberHash == floatMemberHashes[MANEUVERLEASH_MEMBER_IDX]) {
		SetManeuverLeash(*reinterpret_cast<float*>(memberValue));
		return true;
	}

	return false;
}

