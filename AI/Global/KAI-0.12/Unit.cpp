#include "Unit.h"


CUNIT::CUNIT(AIClasses* ai) {
	this ->  ai = ai;
	// added for attackgroup usage
	this ->  groupID = 0;
	this ->  stuckCounter = 0;
	this ->  earlierPosition = float3(0, 0, 0);
	this ->  maneuverCounter = 0;
}
CUNIT::~CUNIT() {
}

const UnitDef* CUNIT::def() {
	return ai ->  cb ->  GetUnitDef(myid);
}

float3 CUNIT::pos() {
	return ai ->  cb ->  GetUnitPos(myid);
}

// 0: mine, 1: allied, 2: enemy -1: non-existant
int CUNIT::owner() {
	if (ai ->  cheat ->  GetUnitDef(myid)) {
		if (def()) {
			if (ai ->  cb ->  GetUnitTeam(myid) == ai ->  cb ->  GetMyTeam()) {
				return 0;
			}
			if (ai ->  cb ->  GetUnitAllyTeam(myid) == ai ->  cb ->  GetMyAllyTeam()) {
				return 1;
			}

			return 2;
		}
	}

	return -1;
}


float CUNIT::Health() {
	return ai ->  cb ->  GetUnitHealth(myid);
}

int CUNIT::category() {
	return GCAT(myid);
}

bool CUNIT::CanAttack(int otherUnit) {
	// currently doesn't see if sending me vs other is a good idea, like peewee vs bomber (not)
	// L("doing CanAttack from " << this -> myid << " to " << otherUnit);
	const UnitDef *ud_mine = ai -> cb -> GetUnitDef(this -> myid);
	const UnitDef *ud_other = ai -> cheat -> GetUnitDef(otherUnit);
	if (ud_mine && ud_other) {
		// L("CanAttack: GUD returned on the second unit: " << ud_other);
		// L("types: " << ud_mine -> humanName << " to " << ud_other -> humanName);
		assert(otherUnit != 0);

		// dps = this -> ai -> ut -> GetDPSvsUnit(ud_mine, ud_other);
		float dps = ai -> ut -> unittypearray[ud_mine -> id].DPSvsUnit[ud_other -> id];

		// L("part-result in CUNIT::CanAttack dps: " << dps);
		return dps > 5.0f;
	}

	// might be a false negative
	return false;
}

bool CUNIT::CanAttackMe(int otherUnit) {
	otherUnit = otherUnit;
	// TODO: the function above, in reverse
	return true;
}



bool CUNIT::Build_ClosestSite(const UnitDef* unitdef, float3 targetpos, int separation, float radius) {
	float3 buildpos;
	buildpos = ai -> cb -> ClosestBuildSite(unitdef, targetpos, radius, separation);

	targetpos.y += 20;
	buildpos.y += 20;

	if (buildpos.x != -1) {
		// L("Site found, building");
		Build(buildpos, unitdef);
		// L("command sent");
        return true;
	}
	else
		Move(ai -> math -> F3Randomize(pos(), 300));

	return false;
}




// tell a factory to build something
bool CUNIT::FactoryBuild(const UnitDef* toBuild) {
	assert(ai -> cb -> GetUnitDef(myid) != NULL);
	Command c;
	c.id = -(toBuild -> id);
	ai -> cb -> GiveOrder(myid, &c);
	ai -> uh -> IdleUnitRemove(myid);

	return true;
}

// added by Kloot
// tell a hub to build something
bool CUNIT::HubBuild(const UnitDef* toBuild) {
	int hub = myid;
	assert(ai -> cb -> GetUnitDef(hub) != NULL);

	float3 hubPos = ai -> cb -> GetUnitPos(hub);
	float3 buildPos = ZeroVector;
	float maxRadius = ai -> cb -> GetUnitDef(hub) -> buildDistance;
	float minRadius = 40.0f;

	int frame = (ai -> cb) -> GetCurrentFrame();
	int mapWidth = (ai -> cb) -> GetMapWidth();
	int mapHeight = (ai -> cb) -> GetMapHeight();
	int mapQuadrant = -1;
	int facing = -1;

	if (hubPos.x < (mapWidth >> 1)) {
		if (hubPos.z < (mapHeight >> 1))
			mapQuadrant = QUADRANT_TOP_LEFT;
		else
			mapQuadrant = QUADRANT_BOT_LEFT;
	}
	else {
		if (hubPos.z < (mapHeight >> 1))
			mapQuadrant = QUADRANT_TOP_RIGHT;
		else
			mapQuadrant = QUADRANT_BOT_RIGHT;
	}


	switch (mapQuadrant) {
		case QUADRANT_TOP_LEFT: {
			facing = (frame % 2)? FACING_DOWN: FACING_RIGHT; break;
		}
		case QUADRANT_TOP_RIGHT: {
			facing = (frame % 2)? FACING_DOWN: FACING_LEFT; break;
		}
		case QUADRANT_BOT_RIGHT: {
			facing = (frame % 2)? FACING_UP: FACING_LEFT; break;
		}
		case QUADRANT_BOT_LEFT: {
			facing = (frame % 2)? FACING_UP: FACING_RIGHT; break;
		}
	}


	// NOTE: this can still go wrong if there is another
	// hub or open factory anywhere inside build radius!
	for (float radius = maxRadius; radius >= minRadius; radius -= 5.0f) {
		for (float angle = 0.0f; angle < 360.0f; angle += 45.0f) {
			buildPos.x = hubPos.x + (radius * cos(angle * DEG2RAD));
			buildPos.y = hubPos.y;
			buildPos.z = hubPos.z + (radius * sin(angle * DEG2RAD));

			float3 closestPos = ai -> cb -> ClosestBuildSite(toBuild, buildPos, minRadius, 1, facing);

			if (closestPos.x >= 0.0f) {
				Command c;
				c.id = -(toBuild -> id);
				c.params.push_back(closestPos.x);
				c.params.push_back(closestPos.y);
				c.params.push_back(closestPos.z);
				c.params.push_back(facing);
				ai -> cb -> GiveOrder(hub, &c);
				ai -> uh -> IdleUnitRemove(hub);

				return true;
			}
		}
	}

	return false;
}






bool CUNIT::ReclaimBest(bool metal, float radius) {
	int features[1000];
	int numfound = ai -> cb -> GetFeatures(features, 1000, pos(),radius);
	float bestscore = 0;
	float myscore = 0;
	float bestdistance = 100000;
	float mydistance = 100000;
	int bestfeature = -1;

	if (metal) {
		for (int i = 0; i < numfound; i++) {
			myscore = ai -> cb -> GetFeatureDef(features[i]) -> metal;
			if (myscore > bestscore && ai -> tm -> ThreatAtThisPoint(ai -> cb -> GetFeaturePos(features[i])) <= ai -> tm -> GetAverageThreat()) {
				bestscore = myscore;
				bestfeature = features[i];
			}
			else if (bestscore == myscore && ai -> tm -> ThreatAtThisPoint(ai -> cb -> GetFeaturePos(features[i])) <= ai -> tm -> GetAverageThreat()) {
				mydistance = ai -> cb -> GetFeaturePos(features[i]).distance2D(ai -> cb -> GetUnitPos(myid));
				if (mydistance < bestdistance) {
					bestfeature = features[i];
					bestdistance = mydistance;
				}
			}
		}
	}
	else {
		for(int i = 0; i < numfound;i++) {
			myscore = ai -> cb -> GetFeatureDef(features[i]) -> energy;
			if (myscore > bestscore && ai -> tm -> ThreatAtThisPoint(ai -> cb -> GetFeaturePos(features[i])) < ai -> tm -> GetAverageThreat()) {
				bestscore = myscore;
				bestfeature = features[i];
			}
			else if (bestscore == myscore && ai -> tm -> ThreatAtThisPoint(ai -> cb -> GetFeaturePos(features[i])) < ai -> tm -> GetAverageThreat()) {
				mydistance = ai -> cb -> GetFeaturePos(features[i]).distance2D(ai -> cb -> GetUnitPos(myid));
				if (mydistance < bestdistance) {
					bestfeature = features[i];
					bestdistance = mydistance;
				}
			}
		}
	}
	if (bestscore > 0) {
		Reclaim(ai -> cb -> GetFeaturePos(bestfeature),10);
		return true;
	}
	return false;
}


Command CUNIT::MakePosCommand(int id, float3 pos, float radius) {
	assert(ai ->  cb ->  GetUnitDef(myid) != NULL);

	if (pos.x > ai ->  cb ->  GetMapWidth() * 8)
		pos.x = ai ->  cb ->  GetMapWidth() * 8;
	if (pos.z  > ai ->  cb ->  GetMapHeight() * 8)
		pos.z = ai ->  cb ->  GetMapHeight() * 8;
	if (pos.x < 0)
		pos.x = 0;
	if (pos.y < 0)
		pos.y = 0;

	Command c;
	c.id = id;
	c.params.push_back(pos.x);
	c.params.push_back(pos.y);
	c.params.push_back(pos.z);

	if (radius)
		c.params.push_back(radius);

	ai ->  uh ->  IdleUnitRemove(myid);
	// L("idle unit removed");
	return c;
}

Command CUNIT::MakeIntCommand(int id, int number, int maxnum) {
	assert(ai ->  cb ->  GetUnitDef(myid) != NULL);

	if (number > maxnum)
		number = maxnum;
	if (number < 0)
		number = 0;

	Command c;
	c.id = id;
	c.params.push_back(number);

	ai ->  uh ->  IdleUnitRemove(myid);
	// L("idle unit removed");
	return c;
}



// Target-based Abilities
bool CUNIT::Attack(int target) {
	assert(ai -> cb -> GetUnitDef(myid) != NULL);
	Command c = MakeIntCommand(CMD_ATTACK, target);

	if (c.id != 0) {
		ai -> cb -> GiveOrder(myid, &c);
		return true;
	}

	return false;
}
bool CUNIT::Capture(int target) {
	assert(ai -> cb -> GetUnitDef(myid) != NULL);
	Command c = MakeIntCommand(CMD_CAPTURE, target);

	if (c.id != 0) {
		ai -> cb -> GiveOrder(myid, &c);
		return true;
	}

	return false;
}

bool CUNIT::Guard(int target) {
	assert(ai -> cb -> GetUnitDef(myid) != NULL);
	Command c = MakeIntCommand(CMD_GUARD, target);

	if (c.id != 0) {
		ai -> cb -> GiveOrder(myid, &c);
		return true;
	}

	return false;
}

bool CUNIT::Load(int target) {
	assert(ai -> cb -> GetUnitDef(myid) != NULL);
	Command c = MakeIntCommand(CMD_LOAD_UNITS, target);

	if (c.id != 0) {
		ai -> cb -> GiveOrder(myid, &c);
		return true;
	}

	return false;
}
bool CUNIT::Reclaim(int target) {
	assert(ai -> cb -> GetUnitDef(myid) != NULL);
	Command c = MakeIntCommand(CMD_RECLAIM, target);

	if (c.id != 0) {
		ai -> cb -> GiveOrder(myid, &c);
		return true;
	}

	return false;
}
bool CUNIT::Repair(int target) {
	assert(ai -> cb -> GetUnitDef(myid) != NULL);
	Command c = MakeIntCommand(CMD_REPAIR, target);

	if (c.id != 0) {
		ai -> cb -> GiveOrder(myid, &c);
		return true;
	}
	return false;
}
bool CUNIT::Ressurect(int target) {
	assert(ai -> cb -> GetUnitDef(myid) != NULL);
	Command c = MakeIntCommand(CMD_RESURRECT, target);

	if (c.id != 0) {
		ai -> cb -> GiveOrder(myid, &c);
		return true;
	}

	return false;
}


// Location Point Abilities
bool CUNIT::Build(float3 pos, const UnitDef* unit) {
	assert(ai -> cb -> GetUnitDef(myid) != NULL);
	Command c = MakePosCommand(-(unit -> id), pos);

	if (c.id != 0) {
		ai -> cb -> GiveOrder(myid, &c);
		ai -> uh -> TaskPlanCreate(myid, pos, unit);
		return true;
	}

	return false;
}

bool CUNIT::Move(float3 pos) {
	assert(ai -> cb -> GetUnitDef(myid) != NULL);
	Command c = MakePosCommand(CMD_MOVE, pos);

	if (c.id != 0) {
		ai -> cb -> GiveOrder(myid, &c);
		return true;
	}

	return false;
}

bool CUNIT::MoveShift(float3 pos) {
	assert(ai -> cb -> GetUnitDef(myid) != NULL);
	Command c = MakePosCommand(CMD_MOVE, pos);

	if (c.id != 0) {
		c.options |= SHIFT_KEY;
		ai -> cb -> GiveOrder(myid, &c);
		return true;
	}

	return false;
}

bool CUNIT::Patrol(float3 pos) {
	assert(ai -> cb -> GetUnitDef(myid) != NULL);
	Command c = MakePosCommand(CMD_PATROL, pos);

	if (c.id != 0) {
		ai -> cb -> GiveOrder(myid, &c);
		return true;
	}
	return false;
}

bool CUNIT::PatrolShift(float3 pos) {
	assert(ai ->  cb ->  GetUnitDef(myid) != NULL);
	Command c = MakePosCommand(CMD_PATROL, pos);

	if (c.id != 0) {
		c.options |= SHIFT_KEY;
		ai ->  cb ->  GiveOrder(myid, &c);
		return true;
	}

	return false;
}


// Radius Abilities
bool CUNIT::Attack(float3 pos, float radius) {
	assert(ai -> cb -> GetUnitDef(myid) != NULL);
	Command c = MakePosCommand(CMD_ATTACK,pos,radius);

	if (c.id != 0) {
		ai -> cb -> GiveOrder(myid, &c);
		return true;
	}

	return false;
}
bool CUNIT::Ressurect(float3 pos, float radius) {
	assert(ai -> cb -> GetUnitDef(myid) != NULL);
	Command c = MakePosCommand(CMD_RESURRECT, pos, radius);

	if (c.id != 0) {
		ai -> cb -> GiveOrder(myid, &c);
		return true;
	}

	return false;
}

bool CUNIT::Reclaim(float3 pos, float radius) {
	assert(ai -> cb -> GetUnitDef(myid) != NULL);
	Command c = MakePosCommand(CMD_RECLAIM, pos, radius);

	if (c.id != 0) {
		ai -> cb -> GiveOrder(myid, &c);
		ai -> uh -> BuilderReclaimOrder(myid, pos);
		return true;
	}

	return false;
}

bool CUNIT::Capture(float3 pos, float radius) {
	assert(ai -> cb -> GetUnitDef(myid) != NULL);
	Command c = MakePosCommand(CMD_CAPTURE, pos, radius);

	if (c.id != 0) {
		ai -> cb -> GiveOrder(myid, &c);
		return true;
	}

	return false;
}

bool CUNIT::Restore(float3 pos, float radius) {
	assert(ai -> cb -> GetUnitDef(myid) != NULL);
	Command c = MakePosCommand(CMD_RESTORE,pos,radius);

	if (c.id != 0) {
		ai -> cb -> GiveOrder(myid, &c);
		return true;
	}

	return false;
}

bool CUNIT::Load(float3 pos, float radius) {
	assert(ai -> cb -> GetUnitDef(myid) != NULL);
	Command c = MakePosCommand(CMD_LOAD_UNITS, pos, radius);

	if (c.id != 0) {
		ai -> cb -> GiveOrder(myid, &c);
		return true;
	}

	return false;
}

bool CUNIT::Unload(float3 pos, float radius) {
	assert(ai -> cb -> GetUnitDef(myid) != NULL);
	Command c = MakePosCommand(CMD_UNLOAD_UNITS, pos, radius);

	if (c.id != 0) {
		ai -> cb -> GiveOrder(myid, &c);
		return true;
	}

	return false;
}


// bool CUNIT::Abilities
bool CUNIT::Cloaking(bool on) {
	assert(ai -> cb -> GetUnitDef(myid) != NULL);
	Command c = MakeIntCommand(CMD_CLOAK, on);

	if (c.id != 0) {
		ai -> cb -> GiveOrder(myid, &c);
		return true;
	}

	return false;
}
bool CUNIT::OnOff(bool on) {
	assert(ai -> cb -> GetUnitDef(myid) != NULL);
	Command c = MakeIntCommand(CMD_ONOFF, on);

	if (c.id != 0) {
		ai -> cb -> GiveOrder(myid, &c);
		return true;
	}

	return false;
}


// Special Abilities
bool CUNIT::SelfDestruct() {
	assert(ai -> cb -> GetUnitDef(myid) != NULL);
	Command c;
	c.id = CMD_SELFD;
	ai -> cb -> GiveOrder(myid, &c);

	return true;
}

bool CUNIT::SetFiringMode(int mode) {
	assert(ai -> cb -> GetUnitDef(myid) != NULL);
	Command c = MakeIntCommand(CMD_FIRE_STATE, mode, 2);
	if (c.id != 0) {
		ai -> cb -> GiveOrder(myid, &c);
		return true;
	}

	return false;
}

bool CUNIT::Stop() {
	assert(ai -> cb -> GetUnitDef(myid) != NULL);
	Command c;
	c.id = CMD_STOP;
	ai -> cb -> GiveOrder(myid, &c);

	return true;
}

bool CUNIT::SetMaxSpeed(float speed) {
	assert(ai -> cb -> GetUnitDef(myid) != NULL);
	Command c;
	c.id = CMD_SET_WANTED_MAX_SPEED;
	c.params.push_back(speed);
	ai -> cb -> GiveOrder(myid, &c);

	return true;
}
