// #include "ExternalAI/aibase.h"
// #include "Sim/Misc/GlobalConstants.h"

#include "IncCREG.h"
#include "IncExternAI.h"
#include "IncGlobalAI.h"

#define UNIT_STUCK_MOVE_DISTANCE 2.0f
// not moving for 5 * 60 frames = stuck
#define UNIT_STUCK_COUNTER_MANEUVER_LIMIT 5
// gives it (STUCK_COUNTER_LIMIT - STUCK_COUNTER_MANEUVER_LIMIT) * 60 seconds to move (longer than UNIT_STUCK_MOVE_DISTANCE (7 sec))
#define UNIT_STUCK_COUNTER_LIMIT 15
// stuck maneuver distance should be the movement map res since threat is not really relevant when maneuvering from a stuck pos
#define UNIT_STUCK_MANEUVER_DISTANCE (THREATRES * 8)
// the max amount of difference in height there may be at the position to maneuver to (don't retreat into a hole)
#define UNIT_MAX_MANEUVER_HEIGHT_DIFFERENCE_UP 20
// minimum of offset between my maxrange and the enemys position before considering moving
#define UNIT_MIN_MANEUVER_RANGE_DELTA (THREATRES * 8)
// minimum amount to maneuver when getting to max range
#define UNIT_MIN_MANEUVER_DISTANCE (THREATRES * 4)
// second requirement, minimum percentage of my range to move when getting to max range (too stop slow long range from dancing around)
#define UNIT_MIN_MANEUVER_RANGE_PERCENTAGE 0.2f
// minimum time to maneuver (frames), used for setting maneuvercounter (in case the speed / dist formula is weird)
#define UNIT_MIN_MANEUVER_TIME 15

#define UNIT_DESTINATION_SLACK (THREATRES * 4.0f * 1.4f)
#define GROUP_DESTINATION_SLACK THREATRES * 8
#define GROUP_MEDIAN_UNIT_SELECTION_SLACK 10.0f



CR_BIND(CAttackGroup, (NULL, 0));
CR_REG_METADATA(CAttackGroup, (
	CR_MEMBER(ai),
	CR_MEMBER(units),
	CR_MEMBER(groupID),
	CR_MEMBER(isMoving),
	CR_MEMBER(pathIterator),
	CR_MEMBER(lowestAttackRange),
	CR_MEMBER(highestAttackRange),
	CR_MEMBER(isShooting),
	CR_MEMBER(movementCounterForStuckChecking),
	CR_RESERVED(16)
));



CAttackGroup::CAttackGroup() {
	this->ai = NULL;
	this->groupID = 0;
	this->pathIterator = 0;
	this->lowestAttackRange = 100000;
	this->highestAttackRange = 1;
	this->movementCounterForStuckChecking = 0;
	this->defending = false;
	this->isMoving = false;
	this->isShooting = false;
	this->attackPosition = ZEROVECTOR;
	this->attackRadius = 1;
}

CAttackGroup::CAttackGroup(AIClasses* ai, int groupID_in) {
	this->ai = ai;
	this->groupID = groupID_in;
	this->pathIterator = 0;
	this->lowestAttackRange = 100000;
	this->highestAttackRange = 1;
	this->movementCounterForStuckChecking = 0;
	this->defending = false;
	this->isMoving = false;
	this->isShooting = false;
	this->attackPosition = ZEROVECTOR;
	this->attackRadius = 1;
}

CAttackGroup::~CAttackGroup() {
}


void CAttackGroup::Log() {
}


void CAttackGroup::AddUnit(int unitID) {
	if (ai->cb->GetUnitDef(unitID)) {
		// add to my structure
		units.push_back(unitID);
		// set its group ID:
		ai->MyUnits[unitID]->groupID = this->groupID;
		// update the attack range properties of this group
		this->lowestAttackRange = std::min(this->lowestAttackRange, this->ai->ut->GetMaxRange(ai->cb->GetUnitDef(unitID)));
		this->highestAttackRange = std::max(this->highestAttackRange, this->ai->ut->GetMaxRange(ai->cb->GetUnitDef(unitID)));
	}
	else {
		assert(false);
	}
}

bool CAttackGroup::RemoveUnit(int unitID) {
	bool found = false;
	std::vector<int>::iterator it;

	for (it = units.begin(); it != units.end(); it++) {
		if (*it == unitID) {
			found = true;
			break;
		}
	}

	if (found) {
		units.erase(it);

		// attempt to reset the group ID of a removed unit
		// for debugging, check ai->MyUnits[unitID]->stuckCounter
		if (ai->cb->GetUnitDef(unitID) != NULL) {
			ai->MyUnits[unitID]->groupID = 0;
		}
	}

	assert(found);

	// update lowestAttackRange and highestAttackRange
	this->lowestAttackRange = 10000.0f;
	this->highestAttackRange = 1.0f;

	for (unsigned int i = 0; i < units.size(); i++) {
		int unitID = units[i];
		if (ai->cb->GetUnitDef(unitID) != NULL) {
			this->lowestAttackRange = std::min(this->lowestAttackRange, this->ai->ut->GetMaxRange(ai->cb->GetUnitDef(unitID)));
			this->highestAttackRange = std::max(this->highestAttackRange, this->ai->ut->GetMaxRange(ai->cb->GetUnitDef(unitID)));
		}
	}

	return found;
}



void CAttackGroup::MoveTo(float3 newPosition) {
	newPosition = newPosition;
}

int CAttackGroup::Size() {
	// LONG DEAD-UNIT TEST
	int unitCounter = 0;
	int numUnits = units.size();
	int invalid = -2;

	for (int i = 0; i < numUnits; i++) {
		int unit = units[i];

		if (ai->cb->GetUnitDef(unit) != NULL) {
			unitCounter++;
		}
		else {
			invalid = unit;
		}
	}

	if (numUnits != unitCounter) {
		// size mismatch in group
	}

	return units.size();
}


int CAttackGroup::GetGroupID() {
	return groupID;
}

int CAttackGroup::GetWorstMoveType() {
	return PATHTOUSE;
}

std::vector<int>* CAttackGroup::GetAllUnits() {
	return &(this->units);
}


// combined unit power of the group
float CAttackGroup::Power() {
	float sum = 0.00001f;

	for (std::vector<int>::iterator it = units.begin(); it != units.end(); it++) {
		if (ai->cb->GetUnitDef(*it) != NULL) {
			sum += ai->cb->GetUnitPower(*it);
		}
	}

	return sum;
}



int CAttackGroup::PopStuckUnit() {
	// removes a stuck unit from the group if there is one, and puts a marker on the map
	for (std::vector<int>::iterator it = units.begin(); it != units.end(); it++) {
		if (ai->MyUnits[*it]->stuckCounter > UNIT_STUCK_COUNTER_LIMIT) {
			int id = *it;
			// mark it
			/*
			SNPRINTF(logMsg, logMsg_maxSize,
					"stuck %i: %i, dropping from group: %i. isMoving = %i",
					id, ai->MyUnits[*it]->stuckCounter, groupID, isMoving);
			PRINTF("%s", logMsg);
			SNPRINTF(logMsg, logMsg_maxSize, "humanName: %s",
					ai->MyUnits[*it]->def()->humanName.c_str());
			PRINTF("%s", logMsg);
			*/
			ai->MyUnits[*it]->stuckCounter = 0;
			units.erase(it);
			return id;
		}
	}

	return -1;
}



bool CAttackGroup::CloakedFix(int enemy) {
	const UnitDef* ud = ai->ccb->GetUnitDef(enemy);

	return ((ud != NULL) && !(ud->canCloak && ud->startCloaked && (ai->cb->GetUnitPos(enemy) == ZEROVECTOR)));
}



float3 CAttackGroup::GetGroupPos() {
	// what's the group's position (for distance checking when selecting targets)
	int unitCounter = 0;
	float3 groupPosition = float3(0, 0, 0);
	int numUnits = units.size();

	for (int i = 0; i < numUnits; i++) {
		int unit = units[i];

		if (ai->cb->GetUnitDef(unit) != NULL) {
			unitCounter++;
			groupPosition += ai->cb->GetUnitPos(unit);
		}
	}

	if (unitCounter > 0) {
		groupPosition /= unitCounter;
		// find the unit closest to the center (since the actual center might be on a hill or something)
		float closestSoFar = MY_FLT_MAX;
		int closestUnitID = -1;
		float temp;
		int unit;

		for (int i = 0; i < numUnits; i++) {
			unit = units[i];
			// is it closer. consider also low unit counts, then the first will be used since it's < and not <= (assuming sufficient float accuracy)
			if (ai->cb->GetUnitDef(unit) != NULL && (temp = groupPosition.distance2D(ai->cb->GetUnitPos(unit))) < closestSoFar - GROUP_MEDIAN_UNIT_SELECTION_SLACK) {
				closestSoFar = temp;
				closestUnitID = unit;
			}
		}

		assert(closestUnitID != -1);
		groupPosition = ai->cb->GetUnitPos(closestUnitID);
	}
	else {
		// L("AttackGroup: empty attack group when calcing group pos!");
		return ERRORVECTOR;
	}

	return groupPosition;
}



// returns enemies in my attack area
std::list<int> CAttackGroup::GetAssignedEnemies() {
	std::list<int> takenEnemies;

	if (!defending) {
		int numTaken = ai->ccb->GetEnemyUnits(&ai->unitIDs[0], attackPosition, attackRadius);
	
		for (int i = 0; i < numTaken; i++) {
			takenEnemies.push_back(ai->unitIDs[i]);
		}
	}

	return takenEnemies;
}

void CAttackGroup::AssignTarget(std::vector<float3> path, float3 position, float radius) {
	this->attackPosition = position;
	this->attackRadius = radius;
	this->pathToTarget = path;
	this->isMoving = true;
	this->isShooting = false;
	this->pathIterator = 0;
	this->defending = false;
}




// attack routine (the "find new enemy" part)
void CAttackGroup::FindDefenseTarget(float3 groupPosition, int frameNr) {
	/*
	static const unsigned int logMsg_maxSize = 512;
	char logMsg[logMsg_maxSize];
	SNPRINTF(logMsg, logMsg_maxSize,
			"AG: FindDefenseTarget(), group %i, frame %i, numUnits %i",
			this->groupID, frameNr, this->units.size());
	PRINTF("%s", logMsg);
	*/

	// KLOOTNOTE: numEnemies will be zero if no enemies in LOS or radar when
	// non-ccb callback used, rely on AttackHandler to pick "global" targets
	// and on this function for "local" ones
	// int numEnemies = ai->ccb->GetEnemyUnits(unitArray);
	int numEnemies = ai->cb->GetEnemyUnitsInRadarAndLos(&ai->unitIDs[0]);

	if (numEnemies > 0) {
		std::vector<float3> enemyPositions;
		enemyPositions.reserve(numEnemies);

		// make a vector with the positions of all enemies
		for (int i = 0; i < numEnemies; i++) {
			if (ai->unitIDs[i] != -1) {
				const UnitDef* enemy_ud = ai->ccb->GetUnitDef(ai->unitIDs[i]);
				float3 enemyPos = ai->ccb->GetUnitPos(ai->unitIDs[i]);

				// store enemy position if unit not cloaked and not an aircraft
				if (ai->cb->GetUnitDef(ai->unitIDs[i]) != NULL && CloakedFix(ai->unitIDs[i]) && !enemy_ud->canfly) {
					// TODO: remove currently cloaked units
					// TODO: remove units not reachable by my unit type and position
					enemyPositions.push_back(enemyPos);
				}
			}
		}

		// if ALL units are cloaked or aircraft, get their positions anyway
		if (enemyPositions.size() == 0) {
			for (int i = 0; i < numEnemies; i++) {
				if (ai->unitIDs[i] != -1) {
					float3 enemyPos = ai->ccb->GetUnitPos(ai->unitIDs[i]);
					enemyPositions.push_back(enemyPos);
				}
			}
		}

		// find path to general enemy position
		pathToTarget.clear();
		float costToTarget = ai->pather->FindBestPath(pathToTarget, groupPosition, lowestAttackRange, enemyPositions);

		if (costToTarget < 0.001f && pathToTarget.size() <= 2) {
			// cost of zero means something is in range, isShooting will take care of it
			isMoving = false;
		} else {
			isMoving = true;
			this->pathIterator = 0;
		}
	}
	else {
		// attempt to path back to base if there are no targets
		// KLOOTNOTE: this branch is now purposely never taken
		// (might not be the best idea to leave units lingering
		// around however)
		return;

        pathToTarget.clear();
		float3 closestBaseSpot = ai->ah->GetClosestBaseSpot(groupPosition);
		float costToTarget = ai->pather->FindBestPathToRadius(pathToTarget, groupPosition, THREATRES * 8, closestBaseSpot);

		// TODO: GetKBaseMeans() for support of multiple islands/movetypes
		// TODO: this doesn't need to be to radius

		if (costToTarget == 0 && pathToTarget.size() <= 2) {
			isMoving = false;

			if (ai->ah->DistanceToBase(groupPosition) > 500) {
				// we could not path back to closest base spot
			}
		} else {
			isMoving = true;
			this->pathIterator = 0;
		}
	}

	if (!isShooting && !isMoving) {
		// no accessible enemies and we're idling
	}
}



bool CAttackGroup::NeedsNewTarget() {
	return (defending && !isShooting && !isMoving);
}

void CAttackGroup::ClearTarget() {
	this->isMoving = false;
	this->defending = true;
	this->attackPosition = ZEROVECTOR;
	this->attackRadius = 0.0f;
	this->pathToTarget.clear();
	// to avoid getting a new target the first frame after arrival
	this->isShooting = true;
}




// called from CAttackHandler::Update()
void CAttackGroup::Update(int frameNr) {
	int frameSpread = 30;
	unsigned int numUnits = units.size();

	if (!numUnits) {
		// empty attack group, nothing to update
		return;
	}

	float3 groupPosition = GetGroupPos();

	if (groupPosition == ERRORVECTOR)
		return;


	// this part of the code checks for nearby enemies and does focus fire/maneuvering
	if ((frameNr % frameSpread) == ((groupID * 4) % frameSpread)) {
		isShooting = false;

		// get all enemies within attack range
		float range = highestAttackRange + 100.0f;
		int numEnemies = ai->ccb->GetEnemyUnits(&ai->unitIDs[0], groupPosition, range);

		if (numEnemies > 0) {
			// select one of the enemies
			int enemySelected = SelectEnemy(numEnemies, groupPosition);

			// for every unit, pathfind to enemy perifery (with personal range - 10) then
			// if distance to that last point in the path is < 10 or cost is 0, attack

			if (enemySelected != -1) {
				AttackEnemy(enemySelected, numUnits, range, frameSpread);
			}
		}
	}


	if (pathToTarget.size() >= 2) {
		// AssignToTarget() was called for this group so
		// we have an attack position and path, just move
		// (movement may be slow due to spreading of orders)
		if (!isShooting && isMoving && (frameNr % 60 == (groupID * 5) % frameSpread)) {
			MoveAlongPath(groupPosition, numUnits);
		}
	} else {
		// find something to attack within visual and radar LOS if AssignToTarget() wasn't called
		if ((defending && !isShooting && !isMoving) && (frameNr % 60 == groupID % 60)) {
			FindDefenseTarget(groupPosition, frameNr);
		}
	}
}




int CAttackGroup::SelectEnemy(int numEnemies, const float3& groupPos) {
	int enemySelected = -1;
	float shortestDistanceFound = MY_FLT_MAX;
	float temp;

	for (int i = 0; i < numEnemies; i++) {
		// my range not considered in picking the closest one
		// TODO: is it air? is it cloaked?
		bool b1 = ((temp = groupPos.distance2D(ai->ccb->GetUnitPos(ai->unitIDs[i]))) < shortestDistanceFound);
		bool b2 = (ai->ccb->GetUnitDef(ai->unitIDs[i]) != NULL);
		bool b3 = CloakedFix(ai->unitIDs[i]);
		bool b4 = ai->ccb->GetUnitDef(ai->unitIDs[i])->canfly;

		if (b1 && b2 && b3 && !b4) {
			enemySelected = i;
			shortestDistanceFound = temp;
		}
	}

	return enemySelected;
}


void CAttackGroup::AttackEnemy(int enemySelected, int numUnits, float range, int frameSpread) {
	float3 enemyPos = ai->ccb->GetUnitPos(ai->unitIDs[enemySelected]);
	assert(CloakedFix(ai->unitIDs[enemySelected]));
	isShooting = true;

	assert(numUnits >= 0);
	for (unsigned int i = 0; i < (unsigned int)numUnits; i++) {
		int unit = units[i];
		const UnitDef* udef = ai->cb->GetUnitDef(unit);

		// does our unit exist and is it not currently maneuvering?
		if (udef && (ai->MyUnits[unit]->maneuverCounter-- <= 0)) {
			// TODO: add a routine finding best (not just closest) target
			// TODO: in some cases, force-fire on position
			// TODO: add canAttack
			ai->MyUnits[unit]->Attack(ai->unitIDs[enemySelected]);

			// TODO: this should be the max-range of the lowest-ranged weapon
			// the unit has assuming you want to rush in with the heavy stuff
			assert(range >= ai->cb->GetUnitMaxRange(unit));

			// SINGLE UNIT MANEUVERING: testing the possibility of retreating to max
			// range if target is too close, EXCEPT FOR FLAMETHROWER-EQUIPPED units
			float3 myPos = ai->cb->GetUnitPos(unit);
			float maxRange = ai->ut->GetMaxRange(udef);
			float losDiff = (maxRange - udef->losRadius);
		//	float myRange = (losDiff > 0.0f)? (maxRange + udef->losRadius) * 0.5f: maxRange;
			float myRange = (losDiff > 0.0f)? maxRange * 0.75f: maxRange;

			bool b5 = udef->canfly;
			bool b6 = myPos.y < (ai->cb->GetElevation(myPos.x, myPos.z) + 25);
			bool b7 = (myRange - UNIT_MIN_MANEUVER_RANGE_DELTA) > myPos.distance2D(enemyPos);

			// is it air, or air that's landed
			if (!b5 || (b6 && b7)) {
				bool debug1 = true;
				bool debug2 = false;

				std::vector<float3> tempPath;

				// note 1: we don't need a path, just a position
				// note 2: should avoid other immediate friendly units and/or immediate enemy units + radius
				// maybe include the height parameter in the search? probably not possible
				// doesn't this mean pathing might happen every second? outer limit should harsher than inner
				float3 unitPos = ai->ccb->GetUnitPos(ai->unitIDs[enemySelected]);
				float dist = ai->pather->FindBestPathToRadius(tempPath, myPos, myRange, unitPos);

				if (tempPath.size() > 0) {
					float3 moveHere = tempPath.back();
					dist = myPos.distance2D(moveHere);

					// TODO: Penetrators are now broken
					// is the position between the proposed destination and the
					// enemy higher than the average of mine and his height?
					float v1 = ((moveHere.y + enemyPos.y) / 2.0f) + UNIT_MAX_MANEUVER_HEIGHT_DIFFERENCE_UP;
					float v2 = ai->cb->GetElevation((moveHere.x + enemyPos.x) / 2, (moveHere.z + enemyPos.z) / 2);
					bool losHack = v1 > v2;
					float a = (float) UNIT_MIN_MANEUVER_TIME / frameSpread;
					float b = (dist / ai->MyUnits[unit]->def()->speed);
					float c = ceilf(std::max(a, b));

					// assume the pathfinder returns correct Y values
					// REMEMBER that this will suck for planes
					if (dist > std::max((UNIT_MIN_MANEUVER_RANGE_PERCENTAGE * myRange), float(UNIT_MIN_MANEUVER_DISTANCE)) && losHack) {
						debug2 = true;
						ai->MyUnits[unit]->maneuverCounter = int(c);
						ai->MyUnits[unit]->Move(moveHere);
					}
				}
				if (debug1 && !debug2) {
					// pathfinder run but path not used?
				}
			}
			else if (!udef->canfly || myPos.y < (ai->cb->GetElevation(myPos.x, myPos.z) + 25)) {
				// this unit is an air unit
			}
		}
		else {
			// OUR unit is dead?
		}
	}
}






// give move orders to units along previously generated pathToTarget
void CAttackGroup::MoveAlongPath(float3& groupPosition, int numUnits) {
	const int maxStepsAhead = 8;
	int pathMaxIndex = (int) pathToTarget.size() - 1;
	int step1 = std::min(pathIterator + maxStepsAhead / 2, pathMaxIndex);
	int step2 = std::min(pathIterator + maxStepsAhead, pathMaxIndex);
	float3 moveToHereFirst = pathToTarget[step1];
	float3 moveToHere = pathToTarget[step2];

	// if we aren't there yet
	if (groupPosition.distance2D(pathToTarget[pathMaxIndex]) > GROUP_DESTINATION_SLACK) {
		// TODO: give a group the order instead of each unit
		assert(numUnits >= 0);
		for (unsigned int i = 0; i < (unsigned int)numUnits; i++) {
			int unit = units[i];

			if (ai->cb->GetUnitDef(unit) != NULL) {
				// TODO: when they are near target, change this so they eg. line up
				// while some are here and some aren't, there's also something that
				// should be done with the units in front that are given the same
				// order+shiftorder and skittle around back and forth meanwhile if
				// the single unit isn't there yet
				if (ai->cb->GetUnitPos(unit).distance2D(pathToTarget[pathMaxIndex]) > UNIT_DESTINATION_SLACK) {
					ai->MyUnits[unit]->Move(moveToHereFirst);

					if (moveToHere != moveToHereFirst) {
						ai->MyUnits[unit]->MoveShift(moveToHere);
					}
				}
			}
		}

		// if group is as close as the pathiterator-indicated target
		// is to the end of the path, increase pathIterator

		pathIterator = 0;
		float3 endOfPathPos = pathToTarget[pathMaxIndex];
		float groupDistanceToEnemy = groupPosition.distance2D(endOfPathPos);
		float pathIteratorTargetDistanceToEnemy = pathToTarget[pathIterator].distance2D(endOfPathPos);
		int increment = maxStepsAhead / 2;

		while (groupDistanceToEnemy <= pathIteratorTargetDistanceToEnemy && pathIterator < pathMaxIndex) {
			pathIterator = std::min(pathIterator + increment, pathMaxIndex);
			pathIteratorTargetDistanceToEnemy = pathToTarget[pathIterator].distance2D(endOfPathPos);
		}

		pathIterator = std::min(pathIterator, pathMaxIndex);
	}
	else {
		// group thinks it has arrived at the destination
		this->ClearTarget();
	}
}
