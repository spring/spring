#include <cstdlib>
#include <ctime>

#include "DGunController.hpp"


CR_BIND(DGunController, (NULL));
CR_REG_METADATA(DGunController, (
	CR_MEMBER(ai),
	CR_MEMBER(state),
	CR_MEMBER(commanderID),
	CR_RESERVED(16),
	CR_POSTLOAD(PostLoad)
));

CR_BIND(ControllerState, );
CR_REG_METADATA(ControllerState, (
	CR_MEMBER(inited),
	CR_MEMBER(dgunOrderFrame),
	CR_MEMBER(reclaimOrderFrame),
	CR_MEMBER(targetSelectionFrame),
	CR_MEMBER(targetID),
	CR_MEMBER(oldTargetPos),
	CR_RESERVED(16)
));



DGunController::DGunController(AIClasses* ai) {
	this->ai = ai;

	if (ai)
		CALLOUT = ai->cb;

	units.resize(MAX_UNITS, 0);
	srand((unsigned) time(0));
}

DGunController::~DGunController(void) {
}

void DGunController::PostLoad() {
	CALLOUT = ai->cb;
}


void DGunController::init(int commID) {
	commanderID  = commID;
	commanderUD  = CALLOUT->GetUnitDef(commID);
	state.inited = true;

	// set the commander to hold fire (we need this since
	// FAW and RF interfere with dgun and reclaim orders)
	CUNIT* commander = ai->MyUnits[commanderID];
	commander->SetFireState(0);

	std::vector<UnitDef::UnitDefWeapon>::const_iterator i = commanderUD->weapons.begin();

	for (; i != commanderUD->weapons.end(); i++) {
		if (i->def->type == "DGun") {
			commanderWD = i->def;
			break;
		}
	}
}




void DGunController::handleDestroyEvent(int attackerID, int targetID) {
	// if we were dgunning or reclaiming this unit and it died
	// (if commander reclaims unit then attackerID of 0 is passed)
	if (attackerID == 0 || state.targetID == targetID) {
		// hack to avoid wiggle deadlocks
		stop();
		state.reset(0, true);
	}
}




// problems: giving reclaim order on moving target causes commander
// to walk (messing up subsequent dgun order if target still moving)
// and does not take commander torso rotation time into account
//
void DGunController::trackAttackTarget(unsigned int currentFrame) {
	if (commanderWD && currentFrame - state.targetSelectionFrame == 5) {
		// five sim-frames have passed since selecting target, attack
		float3 newTargetPos = CALLOUT->GetUnitPos(state.targetID);					// current target position
		float3 commanderPos = CALLOUT->GetUnitPos(commanderID);						// current commander position
		float targetDist = (commanderPos - newTargetPos).Length();					// distance to target
		float3 targetDir = (newTargetPos - state.oldTargetPos).Normalize();			// target direction of movement
		float targetSpeed = (newTargetPos - state.oldTargetPos).Length() / 5;		// target speed per sim-frame during tracking interval
		float dgunDelay = targetDist / commanderWD->projectilespeed;				// sim-frames needed for dgun to reach target position
		float3 attackPos = newTargetPos + targetDir * (targetSpeed * dgunDelay);	// position where target will be in <dgunDelay> frames
		float maxRange = CALLOUT->GetUnitMaxRange(commanderID);
		// CALLOUT->CreateLineFigure(commanderPos, attackPos, 48, 1, 3600, 0);

		if ((commanderPos - attackPos).Length() < maxRange * 0.9f) {
			// multiply by 0.9 to ensure commander does not have to walk
			if ((CALLOUT->GetEnergy()) >= DGUN_MIN_ENERGY_LEVEL) {
				state.dgunOrderFrame = currentFrame;
				issueOrder(attackPos, CMD_DGUN, 0);
			} else {
				state.reclaimOrderFrame = currentFrame;
				issueOrder(state.targetID, CMD_RECLAIM, 0);
			}
		} else {
			state.reset(currentFrame, true);
		}
	}

	state.reset(currentFrame, false);
}


void DGunController::selectTarget(unsigned int currentFrame) {
	float3 commanderPos = CALLOUT->GetUnitPos(commanderID);

	// if our commander is dead then position will be (0, 0, 0)
	if (commanderPos.x <= 0.0f && commanderPos.z <= 0.0f) {
		return;
	}

	// get all units within immediate (non-walking) dgun range
	float maxRange = CALLOUT->GetUnitMaxRange(commanderID);
	int numUnits = CALLOUT->GetEnemyUnits(&units[0], commanderPos, maxRange * 0.9f);

	for (int i = 0; i < numUnits; i++) {
		// if enemy unit with valid ID found in array
		if (units[i] > 0) {
			// check if unit still alive (needed since when UnitDestroyed()
			// triggered GetEnemyUnits() is not immediately updated as well)
			if (CALLOUT->GetUnitHealth(units[i]) > 0) {
				const UnitDef* attackerDef = CALLOUT->GetUnitDef(units[i]);

				// don't directly pop enemy commanders
				if (attackerDef && !attackerDef->isCommander && !attackerDef->canDGun) {
					state.targetSelectionFrame = currentFrame;
					state.targetID = units[i];
					state.oldTargetPos = CALLOUT->GetUnitPos(units[i]);
					return;
				}
			}
		}
	}
}


void DGunController::update(unsigned int currentFrame) {
	if (!state.inited)
		return;

	if (state.targetID != -1) {
		// we selected a target, track and attack it
		trackAttackTarget(currentFrame);
	} else {
		// we have no target yet, pick one
		selectTarget(currentFrame);
	}
}




void DGunController::stop(void) {
	Command c; c.id = CMD_STOP; CALLOUT->GiveOrder(commanderID, &c);
}

void DGunController::issueOrder(float3 targetPos, int orderType, int keyMod) {
	Command c;
	c.id = orderType;
	c.options |= keyMod;
	c.params.push_back(targetPos.x);
	c.params.push_back(targetPos.y);
	c.params.push_back(targetPos.z);

	CALLOUT->GiveOrder(commanderID, &c);
}

void DGunController::issueOrder(int target, int orderType, int keyMod) {
	Command c;
	c.id = orderType;
	c.options |= keyMod;
	c.params.push_back(target);

	CALLOUT->GiveOrder(commanderID, &c);
}

bool DGunController::isBusy(void) {
	return (state.dgunOrderFrame > 0 || state.reclaimOrderFrame > 0);
}
