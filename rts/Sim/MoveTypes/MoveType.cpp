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

CR_BIND_DERIVED_INTERFACE(AMoveType, CObject);
CR_REG_METADATA(AMoveType, (
	CR_MEMBER(owner),
	CR_MEMBER(goalPos),
	CR_MEMBER(oldPos),
	CR_MEMBER(oldSlowUpdatePos),

	CR_MEMBER(maxSpeed),
	CR_MEMBER(maxSpeedDef),
	CR_MEMBER(maxWantedSpeed),
	CR_MEMBER(repairBelowHealth),

	CR_MEMBER(useHeading),
	CR_ENUM_MEMBER(progressState)
));

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

	repairBelowHealth(0.3f)
{
}



void AMoveType::SlowUpdate()
{
	if (owner->pos != oldSlowUpdatePos) {
		oldSlowUpdatePos = owner->pos;

		const int newMapSquare = ground->GetSquare(owner->pos);

		const float losHeight = owner->losHeight;
		const float radarHeight = owner->radarHeight;

		if (newMapSquare != owner->mapSquare) {
			owner->mapSquare = newMapSquare;

			// if owner is an aircraft, temporarily add current altitude to emit-heights
			// otherwise leave them as-is unless owner floats on water and is over water
			if (!owner->UsingScriptMoveType()) {
				const float agh = ground->GetApproximateHeight(owner->pos.x, owner->pos.z);
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
				if (owner->IsOnGround() && owner->unitDef->IsGroundUnit()) {
					// always (re-)add us to occupation map if we moved
					// (since our last SlowUpdate) and are on the ground
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



bool AMoveType::WantsRepair() const { return (owner->health      < (repairBelowHealth * owner->maxHealth)); }
bool AMoveType::WantsRefuel() const { return (owner->currentFuel < (repairBelowHealth * owner->unitDef->maxFuel)); }

