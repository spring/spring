#include <cassert>
#include <ctime>
#include <vector>

#include "IncCREG.h"
#include "IncExternAI.h"
#include "IncGlobalAI.h"

#include "KAIK.h"

extern CKAIK* KAIKStateExt;

CR_BIND(CDGunController, (NULL));
CR_REG_METADATA(CDGunController, (
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
	CR_MEMBER(captureOrderFrame),
	CR_MEMBER(targetSelectionFrame),
	CR_MEMBER(targetID),
	CR_MEMBER(oldTargetPos),
	CR_RESERVED(16)
));

CR_BIND(CDGunControllerHandler, (NULL));
CR_REG_METADATA(CDGunControllerHandler, (
	CR_MEMBER(ai),
	CR_MEMBER(controllers),
	CR_POSTLOAD(PostLoad)
));



CDGunController::CDGunController(AIClasses* aic) {
	ai = aic;

	srand((unsigned) time(0));
}

void CDGunController::PostLoad() {
	ai = KAIKStateExt->GetAi();
}



void CDGunController::Init(int unitID) {
	commanderID  = unitID;
	commanderUD  = ai->cb->GetUnitDef(unitID);
	commanderWD  = NULL;
	state.inited = true;

	// set the commander to hold fire (we need this since
	// FAW and RF interfere with dgun and reclaim orders)
	ai->GetUnit(commanderID)->SetFireState(0);

	std::vector<UnitDef::UnitDefWeapon>::const_iterator i = commanderUD->weapons.begin();

	for (; i != commanderUD->weapons.end(); i++) {
		if (i->def->type == "DGun") {
			commanderWD = i->def;
			break;
		}
	}

	assert(commanderWD != NULL);
}




void CDGunController::HandleEnemyDestroyed(int attackerID, int targetID) {
	// if we were dgunning or reclaiming this unit and it died
	// (if an enemy unit is reclaimed, given attackerID is 0)
	if (attackerID == 0 || state.targetID == targetID) {
		// hack to avoid wiggle deadlocks
		Stop();
		state.Reset(0, true);
	}
}



// problems: giving reclaim order on moving target causes commander
// to walk (messing up subsequent dgun order if target still moving)
// and does not take commander torso rotation time into account
//
void CDGunController::TrackAttackTarget(unsigned int currentFrame) {
	if (currentFrame - state.targetSelectionFrame == 5) {
		// five sim-frames have passed since selecting target, attack
		const UnitDef* udef = ai->cb->GetUnitDef(state.targetID);

		const float3 curTargetPos = ai->cb->GetUnitPos(state.targetID);              // current target position
		const float3 commanderPos = ai->cb->GetUnitPos(commanderID);                 // current commander position

		const float3 targetDif    = (commanderPos - curTargetPos);
		const float  targetDist   = targetDif.Length();                              // distance to target
		const float3 targetVel    = (curTargetPos - state.oldTargetPos);

		float3 targetMoveDir   = targetVel;
		float  targetMoveSpeed = 0.0f;

		if (targetVel != ZeroVector) {
			targetMoveSpeed = targetVel.Length() / 5.0f;                             // target speed per sim-frame during tracking interval
			targetMoveDir   = targetVel / (targetMoveSpeed * 5.0f);                  // target direction of movement
		}

		const float  dgunDelay    = targetDist / commanderWD->projectilespeed;       // sim-frames needed for dgun to reach target position
		const float3 leadPos      = targetMoveDir * (targetMoveSpeed * dgunDelay);
		const float3 dgunPos      = curTargetPos + leadPos;                          // position where target will be in <dgunDelay> frames
		const float  maxRange     = ai->cb->GetUnitMaxRange(commanderID);

		bool haveClearShot = true;
		int numSegments = 0;


		AIHCTraceRay rayData;
		float  rayLen = maxRange * 1.1f;
		float3 rayPos = commanderPos;
		float3 rayDir = targetDif / targetDist;
			rayDir.y = 0.0f;

		while (haveClearShot && (rayLen >= 1.0f)) {
			rayData.rayPos = rayPos;
			rayData.rayDir = targetDif / targetDist;
			rayData.rayLen = rayLen;
			rayData.srcUID = commanderID;
			rayData.hitUID = -1;
			rayData.flags  = 0;

			// modifies hitUID and rayLen
			ai->ccb->HandleCommand(AIHCTraceRayId, &rayData);

			// note: even if hitUnitID is -1, we can still end up destroying
			// our own units because dgun projectiles keep hugging the ground
			// rather than being absorbed by it on impact (which is assumed by
			// the trace)
			if (rayData.hitUID != -1) {
				if (ai->cb->GetUnitAllyTeam(rayData.hitUID) == ai->cb->GetMyAllyTeam()) {
					// hitUID is an allied unit
					haveClearShot = false;
				} else {
					// hitUID is an enemy unit
					rayPos = ai->ccb->GetUnitPos(rayData.hitUID);
				}
			}

			// make sure we always terminate
			rayLen -= rayData.rayLen;
		}


		// multiply by 0.9 to ensure commander does not have to walk
		if ((commanderPos - dgunPos).Length() < maxRange * 0.9f) {
			bool canDGun =
				(ai->cb->GetEnergy() >= DGUN_MIN_ENERGY_LEVEL)
				&& haveClearShot
				&& (udef != NULL && !udef->weapons.empty());

			if (canDGun) {
				IssueOrder(dgunPos, CMD_DGUN, 0);
			} else {
				bool bDanger = ai->tm->ThreatAtThisPoint(commanderPos/*curTargetPos*/) > ai->tm->GetAverageThreat();

				if (bDanger) {
					state.Reset(currentFrame, true);
				} else {
					if (ai->cb->GetUnitHealth(state.targetID) < ai->cb->GetUnitMaxHealth(state.targetID) * 0.5f) {
						IssueOrder(state.targetID, CMD_RECLAIM, 0);
					} else {
						IssueOrder(state.targetID, CMD_CAPTURE, 0);
					}
				}
			}
		} else {
			state.Reset(currentFrame, true);
		}
	}

	state.Reset(currentFrame, false);
}

void CDGunController::SelectTarget(unsigned int currentFrame) {
	float3 commanderPos = ai->cb->GetUnitPos(commanderID);

	// if our commander is dead, its position will be (0, 0, 0)
	if (commanderPos.x <= 0.0f && commanderPos.z <= 0.0f) {
		return;
	}

	// get all units within immediate (non-walking) dgun range
	const float maxRange = ai->cb->GetUnitMaxRange(commanderID);
	const int numUnits = ai->cb->GetEnemyUnits(&ai->unitIDs[0], commanderPos, maxRange * 0.9f);

	for (int i = 0; i < numUnits; i++) {
		// if enemy unit with valid ID found in array
		if (ai->unitIDs[i] > 0) {
			// check if unit still alive (needed since when UnitDestroyed()
			// triggered GetEnemyUnits() is not immediately updated as well)
			if (ai->cb->GetUnitHealth(ai->unitIDs[i]) > 0.0f) {
				const UnitDef* attackerDef = ai->cb->GetUnitDef(ai->unitIDs[i]);

				// don't directly pop enemy commanders
				if (attackerDef && !attackerDef->isCommander && !attackerDef->canDGun) {
					state.targetSelectionFrame = currentFrame;
					state.targetID = ai->unitIDs[i];
					state.oldTargetPos = ai->cb->GetUnitPos(ai->unitIDs[i]);
					return;
				}
			}
		}
	}
}

void CDGunController::Update(unsigned int currentFrame) {
	if (!state.inited)
		return;

	if (state.targetID != -1) {
		// we selected a target, track and attack it
		TrackAttackTarget(currentFrame);
	} else {
		// we have no target yet, pick one
		SelectTarget(currentFrame);
	}
}



void CDGunController::Stop(void) const {
	Command c; c.id = CMD_STOP; ai->ct->GiveOrder(commanderID, &c);
}

void CDGunController::IssueOrder(const float3& pos, int orderType, int keyMod) {
	Command c;
		c.id = orderType;
		c.options |= keyMod;
		c.params.push_back(pos.x);
		c.params.push_back(pos.y);
		c.params.push_back(pos.z);

	ai->ct->GiveOrder(commanderID, &c);

	if (orderType == CMD_DGUN   ) { state.dgunOrderFrame    = ai->cb->GetCurrentFrame(); }
	if (orderType == CMD_RECLAIM) { state.reclaimOrderFrame = ai->cb->GetCurrentFrame(); }
	if (orderType == CMD_CAPTURE) { state.captureOrderFrame = ai->cb->GetCurrentFrame(); }
}

void CDGunController::IssueOrder(int target, int orderType, int keyMod) {
	Command c;
		c.id = orderType;
		c.options |= keyMod;
		c.params.push_back(target);

	ai->ct->GiveOrder(commanderID, &c);

	if (orderType == CMD_DGUN   ) { state.dgunOrderFrame    = ai->cb->GetCurrentFrame(); }
	if (orderType == CMD_RECLAIM) { state.reclaimOrderFrame = ai->cb->GetCurrentFrame(); }
	if (orderType == CMD_CAPTURE) { state.captureOrderFrame = ai->cb->GetCurrentFrame(); }
}

bool CDGunController::IsBusy(void) const {
	return (state.dgunOrderFrame > 0 || state.reclaimOrderFrame > 0 || state.captureOrderFrame > 0);
}






void CDGunControllerHandler::PostLoad() {
	ai = KAIKStateExt->GetAi();
}

bool CDGunControllerHandler::AddController(int unitID) {
	if (controllers.find(unitID) == controllers.end()) {
		controllers[unitID] = new CDGunController(ai);
		controllers[unitID]->Init(unitID);
		return true;
	}

	return false;
}

bool CDGunControllerHandler::DelController(int unitID) {
	std::map<int, CDGunController*>::iterator it = controllers.find(unitID);

	if (it != controllers.end()) {
		delete it->second;
		controllers.erase(it);
		return true;
	}

	return false;
}

CDGunController* CDGunControllerHandler::GetController(int unitID) const {
	std::map<int, CDGunController*>::const_iterator it = controllers.find(unitID);

	if (it == controllers.end()) {
		return NULL;
	}

	return it->second;
}



void CDGunControllerHandler::NotifyEnemyDestroyed(int enemy, int attacker) {
	std::map<int, CDGunController*>::iterator conIt;

	for (conIt = controllers.begin(); conIt != controllers.end(); conIt++) {
		conIt->second->HandleEnemyDestroyed(attacker, enemy);
	}
}

void CDGunControllerHandler::Update(int frame) {
	std::list<int> deadCons;
	std::list<int>::iterator deadConIt;

	std::map<int, CDGunController*>::iterator conIt;

	for (conIt = controllers.begin(); conIt != controllers.end(); conIt++) {
		const UnitDef* udef = ai->cb->GetUnitDef(conIt->first);

		if (udef == NULL || !udef->canDGun) {
			deadCons.push_back(conIt->first);
		} else {
			conIt->second->Update(frame);
		}
	}

	for (deadConIt = deadCons.begin(); deadConIt != deadCons.end(); deadConIt++) {
		// probably should tie this into [Notify]UnitDestroyed()
		DelController(*deadConIt);
	}
}
