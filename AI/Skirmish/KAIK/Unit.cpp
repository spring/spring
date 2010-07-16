#include "IncCREG.h"
#include "IncEngine.h"
#include "IncExternAI.h"
#include "IncGlobalAI.h"

CR_BIND(CUNIT, )
CR_REG_METADATA(CUNIT, (
	CR_MEMBER(uid),
	CR_MEMBER(isDead),
	CR_MEMBER(groupID),
	CR_MEMBER(stuckCounter),
	CR_MEMBER(maneuverCounter),
	CR_MEMBER(ai),
	CR_RESERVED(8),
	CR_POSTLOAD(PostLoad)
));

CUNIT::CUNIT(void) {
	ai = NULL;
	isDead = true;
	// added for attackgroup usage
	groupID = -1;
	stuckCounter = 0;
	earlierPosition = float3(0, 0, 0);
	maneuverCounter = 0;
}

CUNIT::CUNIT(AIClasses* _ai) {
	*this = CUNIT();
	ai = _ai;
}

CUNIT::~CUNIT() {
	isDead = true;
}

void CUNIT::PostLoad(void) {
	// TODO
}




bool CUNIT::isHub(void) const {
	// look up the type of this unit via its unitdef ID,
	// then check if it has been flagged as a hub or not
	return (ai->ut->unitTypes[this->def()->id]).isHub;
}

const UnitDef* CUNIT::def() const {
	return ai->cb->GetUnitDef(uid);
}

float3 CUNIT::pos() const {
	return ai->cb->GetUnitPos(uid);
}

// 0: mine, 1: allied, 2: enemy -1: non-existant
int CUNIT::owner() const {
	if (ai->ccb->GetUnitDef(uid)) {
		if (def()) {
			if (ai->cb->GetUnitTeam(uid) == ai->cb->GetMyTeam()) {
				return 0;
			}
			if (ai->cb->GetUnitAllyTeam(uid) == ai->cb->GetMyAllyTeam()) {
				return 1;
			}

			return 2;
		}
	}

	return -1;
}


float CUNIT::Health() const { return ai->cb->GetUnitHealth(uid); }
UnitCategory CUNIT::category() const { return GCAT(uid); }

bool CUNIT::CanAttack(int otherUnit) const {
	// currently doesn't see if sending me vs. other
	// is a good idea or not, like peewee vs bomber
	const UnitDef* ud_mine = ai->cb->GetUnitDef(this->uid);
	const UnitDef* ud_other = ai->ccb->GetUnitDef(otherUnit);

	if (ud_mine != NULL && ud_other != NULL) {
		assert(otherUnit != 0);

		// float dps = this->ai->ut->GetDPSvsUnit(ud_mine, ud_other);
		float dps = ai->ut->unitTypes[ud_mine->id].DPSvsUnit[ud_other->id];
		return (dps > 5.0f);
	}

	// might be a false negative
	return false;
}

bool CUNIT::CanAttackMe(int otherUnit) const {
	otherUnit = otherUnit;
	// TODO: the function above, in reverse
	return true;
}



// tell a nuke-silo to build a missile
// TODO: use this for mobile units too
bool CUNIT::NukeSiloBuild(void) const {
	if (!def()->stockpileWeaponDef)
		return false;

	Command c;
	c.id = CMD_STOCKPILE;
	ai->ct->GiveOrder(uid, &c);
	return true;
}



int CUNIT::GetBestBuildFacing(const float3& pos) const {
	int frame = (ai->cb)->GetCurrentFrame();
	int mapWidth = (ai->cb)->GetMapWidth() * 8;
	int mapHeight = (ai->cb)->GetMapHeight() * 8;
	int mapQuadrant = -1;
	int facing = -1;

	if (pos.x < (mapWidth >> 1)) {
		// left half of map
		if (pos.z < (mapHeight >> 1)) {
			mapQuadrant = QUADRANT_TOP_LEFT;
		} else {
			mapQuadrant = QUADRANT_BOT_LEFT;
		}
	}
	else {
		// right half of map
		if (pos.z < (mapHeight >> 1)) {
			mapQuadrant = QUADRANT_TOP_RIGHT;
		} else {
			mapQuadrant = QUADRANT_BOT_RIGHT;
		}
	}

	switch (mapQuadrant) {
		case QUADRANT_TOP_LEFT: {
			facing = (frame & 1)? FACING_DOWN: FACING_RIGHT;
		} break;
		case QUADRANT_TOP_RIGHT: {
			facing = (frame & 1)? FACING_DOWN: FACING_LEFT;
		} break;
		case QUADRANT_BOT_RIGHT: {
			facing = (frame & 1)? FACING_UP: FACING_LEFT;
		} break;
		case QUADRANT_BOT_LEFT: {
			facing = (frame & 1)? FACING_UP: FACING_RIGHT;
		} break;
	}

	return facing;
}


// called for mobile construction units
bool CUNIT::Build_ClosestSite(const UnitDef* def, const float3& bpos, int separation, float radius) {
	const int buildFacing = GetBestBuildFacing(bpos);
	const float3 cpos = ai->cb->ClosestBuildSite(def, bpos, radius, separation, buildFacing);

	// L(ai, "[CUNIT::Build_ClosestSite()] builder: " << uid << ", def: " << def);
	// L(ai, "\tbpos: <" << bpos.x << ", " << bpos.y << ", " << bpos.z << ">");
	// L(ai, "\tcpos: <" << cpos.x << ", " << cpos.y << ", " << cpos.z << ">");
	// L(ai, "\n");

	if (cpos.x != -1.0f) {
		Build(cpos, def, buildFacing);
		return true;
	}

	Move(ai->math->F3Randomize(pos(), 300));
	return false;
}




// tell a factory to build something
bool CUNIT::FactoryBuild(const UnitDef* toBuild) const {
	assert(ai->cb->GetUnitDef(uid) != NULL);
	Command c;
	c.id = -(toBuild->id);
	ai->ct->GiveOrder(uid, &c);
	ai->uh->IdleUnitRemove(uid);

	return true;
}

// tell a hub to build something
// note: hubs themselves should not be built
// within range of each other, or the newer
// ones may fail to construct anything
bool CUNIT::HubBuild(const UnitDef* toBuild) const {
	int hub = uid;
	assert(ai->cb->GetUnitDef(hub) != NULL);

	float3 hubPos = ai->cb->GetUnitPos(hub);
	float3 buildPos = ZeroVector;
	float maxRadius = ai->cb->GetUnitDef(hub)->buildDistance;
	float minRadius = 40.0f;
	int facing = GetBestBuildFacing(hubPos);

	// CPU usage reduction hack, force
	// a hub to stay idle if the area
	// around it is too crowded
	int numFriends = ai->cb->GetFriendlyUnits(&ai->unitIDs[0], hubPos, maxRadius * 0.5f);

	if (numFriends > 8)
		return false;

	// note: this can still go wrong if there is another
	// hub or open factory anywhere inside build radius
	// (due to the blocking-map issue)
	for (float radius = maxRadius; radius >= minRadius; radius -= 5.0f) {
		for (float angle = 0.0f; angle < 360.0f; angle += 45.0f) {
			buildPos.x = hubPos.x + (radius * cos(angle * DEG2RAD));
			buildPos.y = hubPos.y;
			buildPos.z = hubPos.z + (radius * sin(angle * DEG2RAD));

			float3 closestPos = ai->cb->ClosestBuildSite(toBuild, buildPos, minRadius, 4, facing);

			if (closestPos.x >= 0.0f) {
				Command c;
				c.id = -(toBuild->id);
				c.params.push_back(closestPos.x);
				c.params.push_back(closestPos.y);
				c.params.push_back(closestPos.z);
				c.params.push_back(facing);
				ai->ct->GiveOrder(hub, &c);
				ai->uh->IdleUnitRemove(hub);
				return true;
			}
		}
	}

	return false;
}






bool CUNIT::ReclaimBestFeature(bool metal, float radius) {
	int featureIDs[1000] = {0};

	int   numFeatures   = ai->cb->GetFeatures(featureIDs, 1000, pos(), radius);
	int   bestFeatureID = -1;
	float bestScore     = 0.0f;
	float bestDist      = 1e30f;

	const MoveData* moveData = def()->movedata;

	if (metal) {
		for (int i = 0; i < numFeatures; i++) {
			const FeatureDef* fDef    = ai->cb->GetFeatureDef(featureIDs[i]);
			const float3&     fPos    = ai->cb->GetFeaturePos(featureIDs[i]);
			const float       fDist   = fPos.distance2D(ai->cb->GetUnitPos(uid));
			const float       fThreat = ai->tm->ThreatAtThisPoint(fPos);

			if (fDef == 0)
				continue;

			// not entirely correct, might still be able to get within reclaim-range
			if (!ai->pather->IsPositionReachable(moveData, fPos)) {
				continue;
			}

			if (fDef->metal > bestScore && fThreat <= ai->tm->GetAverageThreat()) {
				bestScore = fDef->metal;
				bestFeatureID = featureIDs[i];
			}
			else if (bestScore == fDef->metal && fThreat <= ai->tm->GetAverageThreat()) {
				if (fDist < bestDist) {
					bestFeatureID = featureIDs[i];
					bestDist = fDist;
				}
			}
		}
	} else {
		for (int i = 0; i < numFeatures;i++) {
			const FeatureDef* fDef    = ai->cb->GetFeatureDef(featureIDs[i]);
			const float3&     fPos    = ai->cb->GetFeaturePos(featureIDs[i]);
			const float       fDist   = fPos.distance2D(ai->cb->GetUnitPos(uid));
			const float       fThreat = ai->tm->ThreatAtThisPoint(fPos);

			if (fDef == 0)
				continue;

			if (!ai->pather->IsPositionReachable(moveData, fPos)) {
				continue;
			}

			if (fDef->energy > bestScore && fThreat < ai->tm->GetAverageThreat()) {
				bestScore = fDef->energy;
				bestFeatureID = featureIDs[i];
			}
			else if (bestScore == fDef->energy && fThreat < ai->tm->GetAverageThreat()) {
				if (fDist < bestDist) {
					bestFeatureID = featureIDs[i];
					bestDist = fDist;
				}
			}
		}
	}

	if (bestScore > 0) {
		Reclaim(ai->cb->GetFeaturePos(bestFeatureID), 128);
		return true;
	}

	return false;
}



Command CUNIT::MakePosCommand(int id, float3 pos, float radius, int facing) const {
	assert(ai->cb->GetUnitDef(uid) != NULL);

	if (pos.x > ai->cb->GetMapWidth() * 8)
		pos.x = ai->cb->GetMapWidth() * 8;
	if (pos.z  > ai->cb->GetMapHeight() * 8)
		pos.z = ai->cb->GetMapHeight() * 8;
	if (pos.x < 0)
		pos.x = 0;
	if (pos.y < 0)
		pos.y = 0;

	Command c;
	c.id = id;
	c.params.push_back(pos.x);
	c.params.push_back(pos.y);
	c.params.push_back(pos.z);

	// for build commands
	if (facing >= 0)
		c.params.push_back(facing);

	if (radius > 0.0f)
		c.params.push_back(radius);

	ai->uh->IdleUnitRemove(uid);
	return c;
}

Command CUNIT::MakeIntCommand(int cmdID, int param) const {
	assert(ai->cb->GetUnitDef(uid) != NULL);

	Command c;
	c.id = cmdID;
	c.params.push_back(param);

	ai->uh->IdleUnitRemove(uid);
	return c;
}



// Target-based Abilities
bool CUNIT::Attack(int target) const {
	assert(ai->cb->GetUnitDef(uid) != NULL);
	Command c = MakeIntCommand(CMD_ATTACK, target);

	if (c.id != 0) {
		ai->ct->GiveOrder(uid, &c);
		return true;
	}

	return false;
}
bool CUNIT::Capture(int target) const {
	assert(ai->cb->GetUnitDef(uid) != NULL);
	Command c = MakeIntCommand(CMD_CAPTURE, target);

	if (c.id != 0) {
		ai->ct->GiveOrder(uid, &c);
		return true;
	}

	return false;
}

bool CUNIT::Guard(int target) const {
	assert(ai->cb->GetUnitDef(uid) != NULL);
	Command c = MakeIntCommand(CMD_GUARD, target);

	if (c.id != 0) {
		ai->ct->GiveOrder(uid, &c);
		return true;
	}

	return false;
}

bool CUNIT::Load(int target) const {
	assert(ai->cb->GetUnitDef(uid) != NULL);
	Command c = MakeIntCommand(CMD_LOAD_UNITS, target);

	if (c.id != 0) {
		ai->ct->GiveOrder(uid, &c);
		return true;
	}

	return false;
}
bool CUNIT::Reclaim(int target) const {
	assert(ai->cb->GetUnitDef(uid) != NULL);
	Command c = MakeIntCommand(CMD_RECLAIM, target);

	if (c.id != 0) {
		ai->ct->GiveOrder(uid, &c);
		ai->uh->BuilderReclaimOrder(uid, ai->cb->GetUnitPos(target));
		return true;
	}

	return false;
}
bool CUNIT::Repair(int target) const {
	assert(ai->cb->GetUnitDef(uid) != NULL);
	Command c = MakeIntCommand(CMD_REPAIR, target);

	if (c.id != 0) {
		ai->ct->GiveOrder(uid, &c);
		return true;
	}
	return false;
}
bool CUNIT::Ressurect(int target) const {
	assert(ai->cb->GetUnitDef(uid) != NULL);
	Command c = MakeIntCommand(CMD_RESURRECT, target);

	if (c.id != 0) {
		ai->ct->GiveOrder(uid, &c);
		return true;
	}

	return false;
}



// Location Point Abilities
bool CUNIT::Build(float3 pos, const UnitDef* unit, int facing) const {
	assert(ai->cb->GetUnitDef(uid) != NULL);
	Command c = MakePosCommand(-(unit->id), pos, -1.0f, facing);

	if (c.id != 0) {
		// c.options |= SHIFT_KEY;
		ai->ct->GiveOrder(uid, &c);
		ai->uh->TaskPlanCreate(uid, pos, unit);
		return true;
	}

	return false;
}

bool CUNIT::Move(float3 pos) const {
	assert(ai->cb->GetUnitDef(uid) != NULL);
	Command c = MakePosCommand(CMD_MOVE, pos);

	if (c.id != 0) {
		ai->ct->GiveOrder(uid, &c);
		return true;
	}

	return false;
}

bool CUNIT::MoveShift(float3 pos) const {
	assert(ai->cb->GetUnitDef(uid) != NULL);
	Command c = MakePosCommand(CMD_MOVE, pos);

	if (c.id != 0) {
		c.options |= SHIFT_KEY;
		ai->ct->GiveOrder(uid, &c);
		return true;
	}

	return false;
}

bool CUNIT::Patrol(float3 pos) const {
	assert(ai->cb->GetUnitDef(uid) != NULL);
	Command c = MakePosCommand(CMD_PATROL, pos);

	if (c.id != 0) {
		ai->ct->GiveOrder(uid, &c);
		return true;
	}
	return false;
}

bool CUNIT::PatrolShift(float3 pos) const {
	assert(ai->cb->GetUnitDef(uid) != NULL);
	Command c = MakePosCommand(CMD_PATROL, pos);

	if (c.id != 0) {
		c.options |= SHIFT_KEY;
		ai->ct->GiveOrder(uid, &c);
		return true;
	}

	return false;
}


// Radius Abilities
bool CUNIT::Attack(float3 pos, float radius) const {
	assert(ai->cb->GetUnitDef(uid) != NULL);
	Command c = MakePosCommand(CMD_ATTACK, pos, radius);

	if (c.id != 0) {
		ai->ct->GiveOrder(uid, &c);
		return true;
	}

	return false;
}
bool CUNIT::Ressurect(float3 pos, float radius) const {
	assert(ai->cb->GetUnitDef(uid) != NULL);
	Command c = MakePosCommand(CMD_RESURRECT, pos, radius);

	if (c.id != 0) {
		ai->ct->GiveOrder(uid, &c);
		return true;
	}

	return false;
}

bool CUNIT::Reclaim(float3 pos, float radius) const {
	assert(ai->cb->GetUnitDef(uid) != NULL);
	Command c = MakePosCommand(CMD_RECLAIM, pos, radius);

	if (c.id != 0) {
		ai->ct->GiveOrder(uid, &c);
		ai->uh->BuilderReclaimOrder(uid, pos);
		return true;
	}

	return false;
}

bool CUNIT::Capture(float3 pos, float radius) const {
	assert(ai->cb->GetUnitDef(uid) != NULL);
	Command c = MakePosCommand(CMD_CAPTURE, pos, radius);

	if (c.id != 0) {
		ai->ct->GiveOrder(uid, &c);
		return true;
	}

	return false;
}

bool CUNIT::Restore(float3 pos, float radius) const {
	assert(ai->cb->GetUnitDef(uid) != NULL);
	Command c = MakePosCommand(CMD_RESTORE, pos, radius);

	if (c.id != 0) {
		ai->ct->GiveOrder(uid, &c);
		return true;
	}

	return false;
}

bool CUNIT::Load(float3 pos, float radius) const {
	assert(ai->cb->GetUnitDef(uid) != NULL);
	Command c = MakePosCommand(CMD_LOAD_UNITS, pos, radius);

	if (c.id != 0) {
		ai->ct->GiveOrder(uid, &c);
		return true;
	}

	return false;
}

bool CUNIT::Unload(float3 pos, float radius) const {
	assert(ai->cb->GetUnitDef(uid) != NULL);
	Command c = MakePosCommand(CMD_UNLOAD_UNITS, pos, radius);

	if (c.id != 0) {
		ai->ct->GiveOrder(uid, &c);
		return true;
	}

	return false;
}


// Toggable Abilities
bool CUNIT::Cloaking(bool on) const {
	assert(ai->cb->GetUnitDef(uid) != NULL);
	Command c = MakeIntCommand(CMD_CLOAK, on);

	if (c.id != 0) {
		ai->ct->GiveOrder(uid, &c);
		return true;
	}

	return false;
}
bool CUNIT::OnOff(bool on) const {
	assert(ai->cb->GetUnitDef(uid) != NULL);
	Command c = MakeIntCommand(CMD_ONOFF, on);

	if (c.id != 0) {
		ai->ct->GiveOrder(uid, &c);
		return true;
	}

	return false;
}


// Special Abilities
bool CUNIT::SelfDestruct() const {
	assert(ai->cb->GetUnitDef(uid) != NULL);
	Command c;
	c.id = CMD_SELFD;
	ai->ct->GiveOrder(uid, &c);

	return true;
}

// state can be 0: hold fire, 1: return fire, 2: fire at will
bool CUNIT::SetFireState(int state) const {
	assert(ai->cb->GetUnitDef(uid) != NULL);
	Command c = MakeIntCommand(CMD_FIRE_STATE, state);

	if (c.id != 0) {
		ai->ct->GiveOrder(uid, &c);
		return true;
	}

	return false;
}

bool CUNIT::Stop() const {
	assert(ai->cb->GetUnitDef(uid) != NULL);
	Command c;
	c.id = CMD_STOP;
	ai->ct->GiveOrder(uid, &c);

	return true;
}

bool CUNIT::SetMaxSpeed(float speed) const {
	assert(ai->cb->GetUnitDef(uid) != NULL);
	Command c;
	c.id = CMD_SET_WANTED_MAX_SPEED;
	c.params.push_back(speed);
	ai->ct->GiveOrder(uid, &c);

	return true;
}
