#include "Unit.h"
#include "GlobalAI.h"

#include "Sim/Misc/GlobalConstants.h"

CR_BIND(CUNIT, )
CR_REG_METADATA(CUNIT, (
	CR_MEMBER(myid),
	CR_MEMBER(groupID),
	CR_MEMBER(stuckCounter),
	CR_MEMBER(maneuverCounter),
	CR_MEMBER(ai),
	CR_RESERVED(8),
	CR_POSTLOAD(PostLoad)
));


CUNIT::CUNIT(void) {
	this->ai = 0;
	// added for attackgroup usage
	this->groupID = 0;
	this->stuckCounter = 0;
	this->earlierPosition = float3(0, 0, 0);
	this->maneuverCounter = 0;
}

CUNIT::CUNIT(AIClasses* ai) {
	this->ai = ai;
	// added for attackgroup usage
	this->groupID = 0;
	this->stuckCounter = 0;
	this->earlierPosition = float3(0, 0, 0);
	this->maneuverCounter = 0;
}
CUNIT::~CUNIT() {
}

void CUNIT::PostLoad(void) {
	// TODO
}




bool CUNIT::isHub(void) {
	// look up the type of this unit via its unitdef ID,
	// then check if it has been flagged as a hub or not
	return (ai->ut->unitTypes[this->def()->id]).isHub;
}

const UnitDef* CUNIT::def() {
	return ai->cb->GetUnitDef(myid);
}

float3 CUNIT::pos() {
	return ai->cb->GetUnitPos(myid);
}

// 0: mine, 1: allied, 2: enemy -1: non-existant
int CUNIT::owner() {
	if (ai->cheat->GetUnitDef(myid)) {
		if (def()) {
			if (ai->cb->GetUnitTeam(myid) == ai->cb->GetMyTeam()) {
				return 0;
			}
			if (ai->cb->GetUnitAllyTeam(myid) == ai->cb->GetMyAllyTeam()) {
				return 1;
			}

			return 2;
		}
	}

	return -1;
}


float CUNIT::Health() {
	return ai->cb->GetUnitHealth(myid);
}

int CUNIT::category() {
	return GCAT(myid);
}

bool CUNIT::CanAttack(int otherUnit) {
	// currently doesn't see if sending me vs other is a good idea, like peewee vs bomber (not)
	const UnitDef *ud_mine = ai->cb->GetUnitDef(this->myid);
	const UnitDef *ud_other = ai->cheat->GetUnitDef(otherUnit);

	if (ud_mine && ud_other) {
		assert(otherUnit != 0);

		// float dps = this->ai->ut->GetDPSvsUnit(ud_mine, ud_other);
		float dps = ai->ut->unitTypes[ud_mine->id].DPSvsUnit[ud_other->id];
		return (dps > 5.0f);
	}

	// might be a false negative
	return false;
}

bool CUNIT::CanAttackMe(int otherUnit) {
	otherUnit = otherUnit;
	// TODO: the function above, in reverse
	return true;
}



// tell a nuke-silo to build a missile
bool CUNIT::NukeSiloBuild(void) {
	if (!def()->stockpileWeaponDef)
		return false;

	Command c;
	c.id = CMD_STOCKPILE;
	ai->cb->GiveOrder(myid, &c);
	return true;
}



int CUNIT::GetBestBuildFacing(float3& pos) {
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
bool CUNIT::Build_ClosestSite(const UnitDef* unitdef, float3 targetpos, int separation, float radius) {
	int buildFacing = GetBestBuildFacing(targetpos);
	float3 buildpos = ai->cb->ClosestBuildSite(unitdef, targetpos, radius, separation, buildFacing);

	targetpos.y += 20;
	buildpos.y += 20;

	if (buildpos.x != -1) {
		Build(buildpos, unitdef, buildFacing);
		return true;
	}

	Move(ai->math->F3Randomize(pos(), 300));
	return false;
}




// tell a factory to build something
bool CUNIT::FactoryBuild(const UnitDef* toBuild) {
	assert(ai->cb->GetUnitDef(myid) != NULL);
	Command c;
	c.id = -(toBuild->id);
	ai->cb->GiveOrder(myid, &c);
	ai->uh->IdleUnitRemove(myid);

	return true;
}

// tell a hub to build something
bool CUNIT::HubBuild(const UnitDef* toBuild) {
	int hub = myid;
	assert(ai->cb->GetUnitDef(hub) != NULL);

	float3 hubPos = ai->cb->GetUnitPos(hub);
	float3 buildPos = ZeroVector;
	float maxRadius = ai->cb->GetUnitDef(hub)->buildDistance;
	float minRadius = 40.0f;
	int facing = GetBestBuildFacing(hubPos);

	// CPU usage reduction hack, force
	// a hub to stay idle if the area
	// around it is too crowded
	int friends[MAX_UNITS];
	int numFriends = ai->cb->GetFriendlyUnits(friends, hubPos, maxRadius * 0.5f);

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
				ai->cb->GiveOrder(hub, &c);
				ai->uh->IdleUnitRemove(hub);
				return true;
			}
		}
	}

	return false;
}






bool CUNIT::ReclaimBestFeature(bool metal, float radius) {
	int features[1000];
	int numfound = ai->cb->GetFeatures(features, 1000, pos(), radius);
	float bestscore = 0.0f;
	float myscore = 0.0f;
	float bestdistance = 100000.0f;
	float mydistance = 100000.0f;
	int bestfeature = -1;

	if (metal) {
		for (int i = 0; i < numfound; i++) {
			myscore = ai->cb->GetFeatureDef(features[i])->metal;
			if (myscore > bestscore && ai->tm->ThreatAtThisPoint(ai->cb->GetFeaturePos(features[i])) <= ai->tm->GetAverageThreat()) {
				bestscore = myscore;
				bestfeature = features[i];
			}
			else if (bestscore == myscore && ai->tm->ThreatAtThisPoint(ai->cb->GetFeaturePos(features[i])) <= ai->tm->GetAverageThreat()) {
				mydistance = ai->cb->GetFeaturePos(features[i]).distance2D(ai->cb->GetUnitPos(myid));
				if (mydistance < bestdistance) {
					bestfeature = features[i];
					bestdistance = mydistance;
				}
			}
		}
	}
	else {
		for (int i = 0; i < numfound;i++) {
			myscore = ai->cb->GetFeatureDef(features[i])->energy;
			if (myscore > bestscore && ai->tm->ThreatAtThisPoint(ai->cb->GetFeaturePos(features[i])) < ai->tm->GetAverageThreat()) {
				bestscore = myscore;
				bestfeature = features[i];
			}
			else if (bestscore == myscore && ai->tm->ThreatAtThisPoint(ai->cb->GetFeaturePos(features[i])) < ai->tm->GetAverageThreat()) {
				mydistance = ai->cb->GetFeaturePos(features[i]).distance2D(ai->cb->GetUnitPos(myid));
				if (mydistance < bestdistance) {
					bestfeature = features[i];
					bestdistance = mydistance;
				}
			}
		}
	}
	if (bestscore > 0) {
		Reclaim(ai->cb->GetFeaturePos(bestfeature), 10);
		return true;
	}
	return false;
}


Command CUNIT::MakePosCommand(int id, float3 pos, float radius, int facing) {
	assert(ai->cb->GetUnitDef(myid) != NULL);

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

	ai->uh->IdleUnitRemove(myid);
	return c;
}

Command CUNIT::MakeIntCommand(int cmdID, int param, int) {
	assert(ai->cb->GetUnitDef(myid) != NULL);

	Command c;
	c.id = cmdID;
	c.params.push_back(param);

	ai->uh->IdleUnitRemove(myid);
	return c;
}



// Target-based Abilities
bool CUNIT::Attack(int target) {
	assert(ai->cb->GetUnitDef(myid) != NULL);
	Command c = MakeIntCommand(CMD_ATTACK, target);

	if (c.id != 0) {
		ai->cb->GiveOrder(myid, &c);
		return true;
	}

	return false;
}
bool CUNIT::Capture(int target) {
	assert(ai->cb->GetUnitDef(myid) != NULL);
	Command c = MakeIntCommand(CMD_CAPTURE, target);

	if (c.id != 0) {
		ai->cb->GiveOrder(myid, &c);
		return true;
	}

	return false;
}

bool CUNIT::Guard(int target) {
	assert(ai->cb->GetUnitDef(myid) != NULL);
	Command c = MakeIntCommand(CMD_GUARD, target);

	if (c.id != 0) {
		ai->cb->GiveOrder(myid, &c);
		return true;
	}

	return false;
}

bool CUNIT::Load(int target) {
	assert(ai->cb->GetUnitDef(myid) != NULL);
	Command c = MakeIntCommand(CMD_LOAD_UNITS, target);

	if (c.id != 0) {
		ai->cb->GiveOrder(myid, &c);
		return true;
	}

	return false;
}
bool CUNIT::Reclaim(int target) {
	assert(ai->cb->GetUnitDef(myid) != NULL);
	Command c = MakeIntCommand(CMD_RECLAIM, target);

	if (c.id != 0) {
		ai->cb->GiveOrder(myid, &c);
		return true;
	}

	return false;
}
bool CUNIT::Repair(int target) {
	assert(ai->cb->GetUnitDef(myid) != NULL);
	Command c = MakeIntCommand(CMD_REPAIR, target);

	if (c.id != 0) {
		ai->cb->GiveOrder(myid, &c);
		return true;
	}
	return false;
}
bool CUNIT::Ressurect(int target) {
	assert(ai->cb->GetUnitDef(myid) != NULL);
	Command c = MakeIntCommand(CMD_RESURRECT, target);

	if (c.id != 0) {
		ai->cb->GiveOrder(myid, &c);
		return true;
	}

	return false;
}


bool CUNIT::Upgrade(int target, const UnitDef* newTarget) {
	int facing = ai->cb->GetBuildingFacing(target);
	float3 pos = ai->cb->GetUnitPos(target) + float3(60.0f, 0.0f, 60.0f);

	pos = ai->cb->ClosestBuildSite(newTarget, pos, 60.0f, 2, facing);

	bool b1 = Reclaim(target);
	bool b2 = BuildShift(pos, newTarget, facing);
	return (b1 && b2);
}




// Location Point Abilities
bool CUNIT::Build(float3 pos, const UnitDef* unit, int facing) {
	assert(ai->cb->GetUnitDef(myid) != NULL);
	Command c = MakePosCommand(-(unit->id), pos, -1.0f, facing);

	if (c.id != 0) {
		ai->cb->GiveOrder(myid, &c);
		ai->uh->TaskPlanCreate(myid, pos, unit);
		return true;
	}

	return false;
}

bool CUNIT::BuildShift(float3 pos, const UnitDef* unit, int facing) {
	assert(ai->cb->GetUnitDef(myid) != NULL);
	Command c = MakePosCommand(-(unit->id), pos, -1.0f, facing);

	if (c.id != 0) {
		c.options |= SHIFT_KEY;
		ai->cb->GiveOrder(myid, &c);
		ai->uh->TaskPlanCreate(myid, pos, unit);
		return true;
	}

	return false;
}


bool CUNIT::Move(float3 pos) {
	assert(ai->cb->GetUnitDef(myid) != NULL);
	Command c = MakePosCommand(CMD_MOVE, pos);

	if (c.id != 0) {
		ai->cb->GiveOrder(myid, &c);
		return true;
	}

	return false;
}

bool CUNIT::MoveShift(float3 pos) {
	assert(ai->cb->GetUnitDef(myid) != NULL);
	Command c = MakePosCommand(CMD_MOVE, pos);

	if (c.id != 0) {
		c.options |= SHIFT_KEY;
		ai->cb->GiveOrder(myid, &c);
		return true;
	}

	return false;
}

bool CUNIT::Patrol(float3 pos) {
	assert(ai->cb->GetUnitDef(myid) != NULL);
	Command c = MakePosCommand(CMD_PATROL, pos);

	if (c.id != 0) {
		ai->cb->GiveOrder(myid, &c);
		return true;
	}
	return false;
}

bool CUNIT::PatrolShift(float3 pos) {
	assert(ai->cb->GetUnitDef(myid) != NULL);
	Command c = MakePosCommand(CMD_PATROL, pos);

	if (c.id != 0) {
		c.options |= SHIFT_KEY;
		ai->cb->GiveOrder(myid, &c);
		return true;
	}

	return false;
}


// Radius Abilities
bool CUNIT::Attack(float3 pos, float radius) {
	assert(ai->cb->GetUnitDef(myid) != NULL);
	Command c = MakePosCommand(CMD_ATTACK, pos, radius);

	if (c.id != 0) {
		ai->cb->GiveOrder(myid, &c);
		return true;
	}

	return false;
}
bool CUNIT::Ressurect(float3 pos, float radius) {
	assert(ai->cb->GetUnitDef(myid) != NULL);
	Command c = MakePosCommand(CMD_RESURRECT, pos, radius);

	if (c.id != 0) {
		ai->cb->GiveOrder(myid, &c);
		return true;
	}

	return false;
}

bool CUNIT::Reclaim(float3 pos, float radius) {
	assert(ai->cb->GetUnitDef(myid) != NULL);
	Command c = MakePosCommand(CMD_RECLAIM, pos, radius);

	if (c.id != 0) {
		ai->cb->GiveOrder(myid, &c);
		ai->uh->BuilderReclaimOrder(myid, pos);
		return true;
	}

	return false;
}

bool CUNIT::Capture(float3 pos, float radius) {
	assert(ai->cb->GetUnitDef(myid) != NULL);
	Command c = MakePosCommand(CMD_CAPTURE, pos, radius);

	if (c.id != 0) {
		ai->cb->GiveOrder(myid, &c);
		return true;
	}

	return false;
}

bool CUNIT::Restore(float3 pos, float radius) {
	assert(ai->cb->GetUnitDef(myid) != NULL);
	Command c = MakePosCommand(CMD_RESTORE, pos, radius);

	if (c.id != 0) {
		ai->cb->GiveOrder(myid, &c);
		return true;
	}

	return false;
}

bool CUNIT::Load(float3 pos, float radius) {
	assert(ai->cb->GetUnitDef(myid) != NULL);
	Command c = MakePosCommand(CMD_LOAD_UNITS, pos, radius);

	if (c.id != 0) {
		ai->cb->GiveOrder(myid, &c);
		return true;
	}

	return false;
}

bool CUNIT::Unload(float3 pos, float radius) {
	assert(ai->cb->GetUnitDef(myid) != NULL);
	Command c = MakePosCommand(CMD_UNLOAD_UNITS, pos, radius);

	if (c.id != 0) {
		ai->cb->GiveOrder(myid, &c);
		return true;
	}

	return false;
}


// Toggable Abilities
bool CUNIT::Cloaking(bool on) {
	assert(ai->cb->GetUnitDef(myid) != NULL);
	Command c = MakeIntCommand(CMD_CLOAK, on);

	if (c.id != 0) {
		ai->cb->GiveOrder(myid, &c);
		return true;
	}

	return false;
}
bool CUNIT::OnOff(bool on) {
	assert(ai->cb->GetUnitDef(myid) != NULL);
	Command c = MakeIntCommand(CMD_ONOFF, on);

	if (c.id != 0) {
		ai->cb->GiveOrder(myid, &c);
		return true;
	}

	return false;
}


// Special Abilities
bool CUNIT::SelfDestruct() {
	assert(ai->cb->GetUnitDef(myid) != NULL);
	Command c;
	c.id = CMD_SELFD;
	ai->cb->GiveOrder(myid, &c);

	return true;
}

// state can be 0: hold fire, 1: return fire, 2: fire at will
bool CUNIT::SetFireState(int state) {
	assert(ai->cb->GetUnitDef(myid) != NULL);
	Command c = MakeIntCommand(CMD_FIRE_STATE, state);

	if (c.id != 0) {
		ai->cb->GiveOrder(myid, &c);
		return true;
	}

	return false;
}

bool CUNIT::Stop() {
	assert(ai->cb->GetUnitDef(myid) != NULL);
	Command c;
	c.id = CMD_STOP;
	ai->cb->GiveOrder(myid, &c);

	return true;
}

bool CUNIT::SetMaxSpeed(float speed) {
	assert(ai->cb->GetUnitDef(myid) != NULL);
	Command c;
	c.id = CMD_SET_WANTED_MAX_SPEED;
	c.params.push_back(speed);
	ai->cb->GiveOrder(myid, &c);

	return true;
}
