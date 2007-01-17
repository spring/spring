#include "AttackHandler.h"

#define K_MEANS_ELEVATION 40
#define IDLE_GROUP_ID 0
#define STUCK_GROUP_ID 1
#define AIR_GROUP_ID 2
#define GROUND_GROUP_ID_START 1000
#define SAFE_SPOT_DISTANCE 300
//#define SAFE_SPOT_DISTANCE_SLACK 700
#define KMEANS_ENEMY_MAX_K 32
#define KMEANS_BASE_MAX_K 32
#define KMEANS_MINIMUM_LINE_LENGTH 8*THREATRES

CAttackHandler::CAttackHandler(AIClasses* ai) {
	this -> ai = ai;
	// setting a position to the middle of the map
	float mapWidth = ai -> cb -> GetMapWidth() * 8.0f;
	float mapHeight = ai -> cb -> GetMapHeight() * 8.0f;
	newGroupID = GROUND_GROUP_ID_START;
	this -> kMeansK = 1;
	this -> kMeansBase.push_back(float3(mapWidth / 2.0f, K_MEANS_ELEVATION, mapHeight / 2.0f));
	this -> kMeansEnemyK = 1;
	this -> kMeansEnemyBase.push_back(float3(mapWidth / 2.0f, K_MEANS_ELEVATION, mapHeight / 2.0f));
	// L("constructor of CAttackHandler");
	UpdateKMeans();
	airIsAttacking = false;
	airPatrolOrdersGiven = false;
	airTarget = -1;
}

CAttackHandler::~CAttackHandler() {
}


void CAttackHandler::AddUnit(int unitID) {
	if ((ai -> MyUnits[unitID] -> def()) -> canfly) {
		// the groupID of this "group" is 0, to separate them from other idle units
		ai -> MyUnits[unitID] -> groupID = AIR_GROUP_ID;
		// this might be a new unit with the same id as an older dead unit
		ai -> MyUnits[unitID] -> stuckCounter = 0;
		// do some checking then essentially add it to defense group
		airUnits.push_back(unitID);
		// patrol orders need to be updated
		airPatrolOrdersGiven = false;
	}
	else {
		// the groupID of this "group" is 0, to separate them from other idle units
		ai -> MyUnits[unitID] -> groupID = IDLE_GROUP_ID;
		// this might be a new unit with the same id as an older dead unit
		ai -> MyUnits[unitID] -> stuckCounter = 0;
		// do some checking then essentially add it to defense group
		units.push_back(unitID);
		// TODO: this aint that good tbh
		this -> PlaceIdleUnit(unitID);
	}
}


void CAttackHandler::UnitDestroyed(int unitID) {
	int attackGroupID = ai -> MyUnits[unitID] -> groupID;
	// L("AttackHandler: unitDestroyed id:" << unitID << " groupID:" << attackGroupID << " numGroups:" << attackGroups.size());

	// if its in the defense group:
	if (attackGroupID == IDLE_GROUP_ID) {
		bool found_dead_unit_in_attackHandler = false;

		for (list<int>::iterator it = units.begin(); it != units.end(); it++) {
			if (*it == unitID) {
				units.erase(it);
				found_dead_unit_in_attackHandler = true;
				break;
			}
		}
		assert(found_dead_unit_in_attackHandler);
	}
	else if (attackGroupID >= GROUND_GROUP_ID_START) {
		// its in an attackgroup
		bool foundGroup = false;
		bool removedDeadUnit = false;
		list<CAttackGroup>::iterator it;

		for (it = attackGroups.begin(); it != attackGroups.end(); it++) {
			if (it -> GetGroupID() == attackGroupID) {
				removedDeadUnit = it -> RemoveUnit(unitID);
				foundGroup = true;
				break;
			}
		}

		// TODO: this has failed before. and again.
		assert(foundGroup);
		assert(removedDeadUnit);

		// check if the group is now empty
		// L("AH: about to check if a group needs to be deleted entirely");
		int groupSize = it -> Size();

		if (groupSize == 0) {
			// L("AH: yes, its ID is " << it -> GetGroupID());
			attackGroups.erase(it);
		}
	}
	else if(attackGroupID == AIR_GROUP_ID) {
		// L("AH: unit destroyed and its in the air group, trying to remove");
		bool found_dead_unit_in_airUnits = false;
		for (list<int>::iterator it = airUnits.begin(); it != airUnits.end(); it++) {
			if (*it == unitID) {
				airUnits.erase(it);
				found_dead_unit_in_airUnits = true;
				break;
			}
		}
		assert(found_dead_unit_in_airUnits);
	}
	else {
		// its in stuckunits
		bool found_dead_in_stuck_units = false;
		list<pair<int,float3> >::iterator it;

		for (it = stuckUnits.begin(); it != stuckUnits.end(); it++) {
			if (it -> first == unitID) {
				stuckUnits.erase(it);
				found_dead_in_stuck_units = true;
				break;
			}
		}
		assert(found_dead_in_stuck_units);
	}

	// L("AH: deletion done");
}


bool CAttackHandler::PlaceIdleUnit(int unit) {
	if (ai -> cb -> GetUnitDef(unit) != NULL) {
		float3 moo = FindUnsafeArea(ai -> cb -> GetUnitPos(unit));

		if (moo != ZEROVECTOR && moo != ERRORVECTOR) {
            ai -> MyUnits[unit] -> Move(moo);
		}
	}

	return false;
}


// is it ok to build at CBS from this position?
bool CAttackHandler::IsSafeBuildSpot(float3 mypos) {
	// get a subset of the kmeans
	// iterate the lines along the path that they make with a min distance slightly lower than the area radius definition

	mypos = mypos;
	return false;
}

// is it ok to build at CBS from this position?
bool CAttackHandler::IsVerySafeBuildSpot(float3 mypos) {
	mypos = mypos;
	return false;
}


// returns a safe spot from k-means, adjacent to myPos, safety params are (0..1).
// this is going away or changing.
// change to: decide on the random float 0...1 first, then find it. waaaay easier.
float3 CAttackHandler::FindSafeSpot(float3 myPos, float minSafety, float maxSafety) {
	// L("FindSafeSpot called at " << ai -> cb -> GetCurrentFrame());
	// L("kMeansK:" << kMeansK << " myPos.x:" << myPos.x << " minSafety:" << minSafety << " maxSafety:" << maxSafety);

	// find a safe spot
	myPos = myPos;
	int startIndex = int(minSafety * this -> kMeansK);
	if (startIndex < 0)
		startIndex = 0;
//	if (startIndex >= kMeansK)
//		startIndex = kMeansK - 1;

	int endIndex = int(maxSafety * this -> kMeansK);
	if (endIndex < 0)
		startIndex = 0;
//	if (endIndex >= kMeansK)
//		endIndex = kMeansK - 1;
	if (startIndex > endIndex)
		startIndex = endIndex;

	// L("startIndex:" << startIndex << " endIndex:" << endIndex);

	char text[512];

	if (kMeansK <= 1 || startIndex == endIndex) {
		if (startIndex >= kMeansK)
			startIndex = kMeansK - 1;

		float3 pos = kMeansBase[startIndex] + float3((RANDINT % SAFE_SPOT_DISTANCE), 0, (RANDINT % SAFE_SPOT_DISTANCE));
		pos.y = ai -> cb -> GetElevation(pos.x, pos.z);

		sprintf(text, "AH::FSA1 minS: %3.2f, maxS: %3.2f,", minSafety, maxSafety);

		AIHCAddMapPoint amp;
		amp.label = text;
		amp.pos = pos;

		return pos;
	}

	// L("CAttackHandler::FindSafeSpot - about to get the subset");

	assert(startIndex < endIndex);
	assert(startIndex < kMeansK);
	assert(endIndex <= kMeansK);

	// get a subset of the kmeans
	vector<float3> subset;

	// L("CAttackHandler::FindSafeSpot - before the for loop. size:" << size);
	for (int i = startIndex; i < endIndex; i++) {
		// subset[i] = kMeansBase[startIndex + i];
		// L("i:" << i << " startIndex:" << startIndex << " endIndex:" << endIndex << " kMeansK:" << kMeansK);
		assert(i < kMeansK);
		subset.push_back(kMeansBase[i]);
		// L("pushed kMeansBase[startIndex + i] to subset, it was " << kMeansBase[startIndex + i].x << " " << kMeansBase[startIndex + i].y << " " << kMeansBase[startIndex + i].z);
	}

	// then find a position on one of the lines between those points (pather)
//	// L("CAttackHandler::FindSafeSpot - before the for random and modulo thing. subset.size:" << subset.size());
	int whichPath;
	if (subset.size() > 1)
		whichPath = (RANDINT % (int) subset.size());
	else
		whichPath = 0;

	assert(whichPath < (int) subset.size());
	assert(subset.size() > 0);

	// L("CAttackHandler::AH-FSA - before the if, whichPath is " << whichPath << " subset size is " << subset.size());
	if ((whichPath + 1) < (int) subset.size() && subset[whichPath].distance2D(subset[whichPath + 1]) > KMEANS_MINIMUM_LINE_LENGTH) {
        vector<float3> posPath;
		 //TODO: implement one in pathfinder without radius (or unit ID)
	//	if (size > (int)kMeansBase.size())
	//		size = kMeansBase.size();

		float dist = ai -> pather -> MakePath(&posPath, &subset[whichPath], &subset[whichPath + 1], THREATRES * 8);
		float3 res;
		if (dist > 0) {
			// L("attackhandler:findsafespot #1 dist > 0 from path, using res from pather. dist:" << dist);
			int whichPos = RANDINT % (int) posPath.size();
			res = posPath[whichPos];
		}
		else {
			// L("attackhandler:findsafespot #2 dist == 0 from path, using first point. dist:" << dist);
			res = subset[whichPath];
		}

		sprintf(text, "AH::FSA-2 path:minS: %3.2f, maxS: %3.2f, pos:x: %f5.1 y: %f5.1 z: %f5.1", minSafety, maxSafety, res.x, res.y, res.z);
		AIHCAddMapPoint amp;
		amp.label = text;
		amp.pos = res;

		return res;
	}
	else {
		assert(whichPath < (int) subset.size());
		float3 res = subset[whichPath];
		sprintf(text, "AH::FSA-3 minS: %f, maxS: %f, pos:x: %f y: %f z: %f", minSafety, maxSafety, res.x, res.y, res.z);

		AIHCAddMapPoint amp;
		amp.label = text;
		amp.pos = res;
		return res;
	}
}


float3 CAttackHandler::FindSafeArea(float3 pos) {
	if (this -> DistanceToBase(pos) < SAFE_SPOT_DISTANCE)
		return pos;

	float min = 0.6;
	float max = 0.95;
	float3 safe = this -> FindSafeSpot(pos, min, max);
	// HACK
	safe += pos;
	safe /= 2;

	return safe;
}

float3 CAttackHandler::FindVerySafeArea(float3 pos) {
	float min = 0.9;
	float max = 1.0;
	return (this -> FindSafeSpot(pos, min, max));
}

float3 CAttackHandler::FindUnsafeArea(float3 pos) {
	float min = 0.1;
	float max = 0.3;
	return (this -> FindSafeSpot(pos, min, max));
}


float CAttackHandler::DistanceToBase(float3 pos) {
	float closestDistance = FLT_MAX;

	for (int i = 0; i < this -> kMeansK; i++) {
		float3 mean = this -> kMeansBase[i];
		float distance = pos.distance2D(mean);
		closestDistance = min(distance, closestDistance);
	}

	return closestDistance;
}


float3 CAttackHandler::GetClosestBaseSpot(float3 pos) {
	float closestDistance = FLT_MAX;
	int index = 0;

	for (int i = 0; i < this -> kMeansK; i++) {
		float3 mean = this -> kMeansBase[i];
		float distance = pos.distance2D(mean);

		if (distance < closestDistance) {
			closestDistance = distance;
			index = i;
		}
	}

	return kMeansBase[index];
}


vector<float3> CAttackHandler::KMeansIteration(vector<float3> means, vector<float3> unitPositions, int newK) {
	assert(newK > 0 && means.size() > 0);
	int numUnits = unitPositions.size();
	// change the number of means according to newK
	int oldK = means.size();
	means.resize(newK);
	// add a new means, just use one of the positions
	float3 newMeansPosition = unitPositions[0];
	newMeansPosition.y = ai -> cb -> GetElevation(newMeansPosition.x, newMeansPosition.z) + K_MEANS_ELEVATION;

	for (int i = oldK; i < newK; i++) {
		means[i] = newMeansPosition;
	}

	// check all positions and assign them to means, complexity n*k for one iteration
	vector<int> unitsClosestMeanID;
	unitsClosestMeanID.resize(numUnits, -1);
	vector<int> numUnitsAssignedToMean;
	numUnitsAssignedToMean.resize(newK, 0);

	for (int i = 0; i < numUnits; i++) {
		float3 unitPos = unitPositions.at(i);
		float closestDistance = FLT_MAX;
		int closestIndex = -1;

		for (int m = 0; m < newK; m++) {
			float3 mean = means[m];
			float distance = unitPos.distance2D(mean);

			if (distance < closestDistance) {
				closestDistance = distance;
				closestIndex = m;
			}
		}

		// position i is closest to the mean at closestIndex
		unitsClosestMeanID[i] = closestIndex;
		numUnitsAssignedToMean[closestIndex]++;
	}

	// change the means according to which positions are assigned to them
	// use meanAverage for indexes with 0 pos'es assigned
	// make a new means list
	vector<float3> newMeans;
	newMeans.resize(newK, float3(0, 0, 0));

	for (int i = 0; i < numUnits; i++) {
		int meanIndex = unitsClosestMeanID[i];
		 // dont want to divide by 0
		float num = max(1, numUnitsAssignedToMean[meanIndex]);
		newMeans[meanIndex] += unitPositions[i] / num;
	}

	// do a check and see if there are any empty means and set the height
	for (int i = 0; i < newK; i++) {
		// if a newmean is unchanged, set it to the new means pos instead of (0, 0, 0)
		if (newMeans[i] == float3(0, 0, 0)) {
			newMeans[i] = newMeansPosition;
		}
		else {
			//get the proper elevation for the y coord
			newMeans[i].y = ai -> cb -> GetElevation(newMeans[i].x, newMeans[i].z) + K_MEANS_ELEVATION;
		}
	}

	return newMeans;
}


void CAttackHandler::UpdateKMeans() {
	// we want local variable definitions
	{
		// get positions of all friendly units and put them in a vector (completed buildings only)
		int numFriendlies = 0;
		vector<float3> friendlyPositions;
		int friendlies[MAXUNITS];
		numFriendlies = ai -> cb -> GetFriendlyUnits(friendlies);

		for (int i = 0; i < numFriendlies; i++) {
			int unit = friendlies[i];
			CUNIT* u = ai -> MyUnits[unit];
			// its a building, it has hp, and its mine (0)
			if (this -> UnitBuildingFilter(u -> def()) && this -> UnitReadyFilter(unit) && u -> owner() == 0) {
				friendlyPositions.push_back(u -> pos());
			}
		}
		// hack to make it at least 1 unit, should only happen when you have no base
		if (friendlyPositions.size() < 1) {
			// it has to be a proper position, unless there are no proper positions
			if (numFriendlies > 0 && ai -> cb -> GetUnitDef(friendlies[0]) && ai -> MyUnits[friendlies[0]] -> owner() == 0) {
				friendlyPositions.push_back(ai -> cb -> GetUnitPos(friendlies[0]));
			}
			else {
			 	// when everything is dead
				friendlyPositions.push_back(float3(RANDINT % (ai -> cb -> GetMapWidth() * 8), 1000, RANDINT % (ai -> cb -> GetMapHeight() * 8)));
			}
		}

		// calculate a new K. change the formula to adjust max K, needs to be 1 minimum.
		this -> kMeansK = int(min((float) (KMEANS_BASE_MAX_K), 1.0f + sqrtf((float)numFriendlies + 0.01f)));
		// iterate k-means algo over these positions and move the means
		this -> kMeansBase = KMeansIteration(this -> kMeansBase, friendlyPositions, this -> kMeansK);
	}
	
	//update enemy position k-means
	//get positions of all enemy units and put them in a vector (completed buildings only)
	int numEnemies = 0;
	vector<float3> enemyPositions;
	int enemies[MAXUNITS];
	numEnemies = ai -> cheat -> GetEnemyUnits(enemies);

	for (int i = 0; i < numEnemies; i++) {
		const UnitDef* ud = ai -> cheat -> GetUnitDef(enemies[i]);
		if (this -> UnitBuildingFilter(ud)) {
//		if (this -> UnitBuildingFilter(ud) && this -> UnitReadyFilter(unit)) {
			enemyPositions.push_back(ai -> cheat -> GetUnitPos(enemies[i]));
			// L("AttackHandler debug: added enemy building position for k-means " << i);
		}
	}

	// hack to make it at least 1 unit, should only happen when you have no base
	if (enemyPositions.size() < 1) {
		// it has to be a proper position, unless there are no proper positions
		if (numEnemies > 0 && ai -> cheat -> GetUnitDef(enemies[0])) {
			enemyPositions.push_back(ai -> cheat -> GetUnitPos(enemies[0]));
		}
		else {
			// when everything is dead
			enemyPositions.push_back(float3(RANDINT % (ai -> cb -> GetMapWidth() * 8), 1000, RANDINT % (ai -> cb -> GetMapHeight() * 8)));
		}
	}

	// calculate a new K. change the formula to adjust max K, needs to be 1 minimum
	this -> kMeansEnemyK = int(min(float(KMEANS_ENEMY_MAX_K), 1.0f + sqrtf((float) numEnemies + 0.01f)));

	// L("AttackHandler: doing k-means k:" << kMeansK << " numPositions=" << numFriendlies);
	// iterate k-means algo over these positions and move the means
	this -> kMeansEnemyBase = KMeansIteration(this -> kMeansEnemyBase, enemyPositions, this -> kMeansEnemyK);

	// base k-means and enemy base k-means are updated
	// approach: add up (max - distance) to enemies
	vector<float> proximity;
	proximity.resize(kMeansK, 0.0000001f);
	const float mapDiagonal = sqrt(pow((float) ai -> cb -> GetMapHeight() * 8,2) + pow((float) ai -> cb -> GetMapWidth() * 8, 2) + 1.0f);

	for (int f = 0; f < kMeansK; f++) {
		for (int e = 0; e < kMeansEnemyK; e++) {
			proximity[f] += mapDiagonal - kMeansBase[f].distance2D(kMeansEnemyBase[e]);
		}
	}

	// sort kMeans by the proximity score
	float3 tempPos;
	float temp;

	for (int i = 1; i < kMeansK; i++) {
		for (int j = 0; j < i; j++) {
			// switch
			if (proximity[i] > proximity[j]) {
				tempPos = kMeansBase[i];
				kMeansBase[i] = kMeansBase[j];
				kMeansBase[j] = tempPos;
				temp = proximity[i];
				proximity[i] = proximity[j];
				proximity[j] = temp;
			}
		}
	}

	// okay, so now we have a kMeans list sorted by distance to enemies, 0 being risky and k being safest.
}


bool CAttackHandler::UnitGroundAttackFilter(int unit) {
	CUNIT u = *ai -> MyUnits[unit];
	bool result = ((u.def() != NULL) && (u.def() -> canmove) && (u.category() == CAT_G_ATTACK));
	return result;
}

bool CAttackHandler::UnitBuildingFilter(const UnitDef *ud) {
	bool result = ((ud != NULL) && (ud -> speed <= 0));
	return result;
}

bool CAttackHandler::UnitReadyFilter(int unit) {
	CUNIT u = *ai -> MyUnits[unit];
	bool result = ((u.def() != NULL) && (!ai -> cb -> UnitBeingBuilt(unit)) && ((ai -> cb -> GetUnitHealth(unit)) > (ai -> cb -> GetUnitMaxHealth(unit) * 0.8f)));
	return result;
}



void CAttackHandler::UpdateAir() {
	if (airUnits.size() == 0)
		return;

	/*
	 * if enemy is dead, attacking = false
	 * every blue moon, do an air raid on most valuable thingy
	 */
	assert(!(airIsAttacking && airTarget == -1));

	if (airIsAttacking && (airUnits.size() == 0 || ai -> cheat -> GetUnitDef(airTarget) == NULL)) {
		airTarget = -1;
		airIsAttacking = false;
	}

 	// 5 mins || 30 secs && 8+ units
	if ((ai -> cb -> GetCurrentFrame() % (60 * 30 * 5) == 0) || ((ai -> cb -> GetCurrentFrame() % (30 * 30) == 0) && (airUnits.size() > 8))) {
		// L("AH: trying to attack with air units.");
		int numOfEnemies = ai -> cheat -> GetEnemyUnits(unitArray);
		int bestID = -1;
		float bestFound = -1.0;

		for (int i = 0; i < numOfEnemies; i++) {
			int enemy = unitArray[i];
			if ((enemy != -1) && (ai -> cheat -> GetUnitDef(enemy) != NULL) && ((ai -> cheat -> GetUnitDef(enemy) -> metalCost) > bestFound)) {
				bestID = enemy;
				bestFound = ai -> cheat -> GetUnitDef(enemy) -> metalCost;
			}
		}

		// L("AH: selected the enemy: " << bestID);
		if ((bestID != -1) && (ai -> cheat -> GetUnitDef(bestID))) {
			// give the order
			for (list<int>::iterator it = airUnits.begin(); it != airUnits.end(); it++) {
				CUNIT* u = ai -> MyUnits[*it];
				u -> Attack(bestID);
			}

			airIsAttacking = true;
			airTarget = bestID;
			ai -> cb -> SendTextMsg("AH: air group is attacking", 0);
		}
	}

	if (ai -> cb -> GetCurrentFrame() % 1800 == 0) {
		airPatrolOrdersGiven = false;
	}

	if (!airPatrolOrdersGiven && !airIsAttacking) {
		// L("AH: updating air patrol routes");
		// get and make up some outer base perimeter points
		vector<float3> outerMeans;
		const int num = 3;
		outerMeans.reserve(num);

		if (kMeansK > 1) {
			int counter = 0;
			// offsetting the outermost one
			counter += kMeansK / 8;

			for (int i = 0; i < num; i++) {
				outerMeans.push_back(kMeansBase[counter]);

				if (counter < kMeansK - 1)
					counter++;
			}
		}
		else {
			// there is just 1 kmeans and we need three
			for (int i = 0; i < num; i++) {
				outerMeans.push_back(kMeansBase[0] + float3(250 * i, 0, 0));
			}
		}

		assert(outerMeans.size() == (unsigned int) num);

		// give the patrol orders to the outer means
		for (list<int>::iterator it = airUnits.begin(); it != airUnits.end(); it++) {
			CUNIT* u = ai -> MyUnits[*it];
			// do this first in case they are in the enemy base
			u -> Move(outerMeans[0] + float3(0, 50, 0));

			for (int i = 0; i < num; i++) {
				u -> PatrolShift(outerMeans[i]);
			}
		}

		airPatrolOrdersGiven = true;
	}
}



void CAttackHandler::AssignTarget(CAttackGroup* group_in) {
	// L("AH: assign target to group " << group_in -> GetGroupID());

	// GET ENEMIES, FILTER THEM BY WHATS TAKEN, PATH TO THEM AND GIVE THE PATH TO THE GROUP
	int numOfEnemies = ai -> cheat -> GetEnemyUnits(unitArray);

	// L("AH: initial num of enemies: "  << numOfEnemies);
	if (numOfEnemies) {
		// build vector of enemies
		vector<int> allEligibleEnemies;
		allEligibleEnemies.reserve(numOfEnemies);
		// make a vector with the positions of all enemies
		for (int i = 0; i < numOfEnemies; i++) {
			if (unitArray[i] != -1) {
				const UnitDef* ud = ai -> cheat -> GetUnitDef(unitArray[i]);

				// filter out cloaked and air enemies
				if ((ud != NULL) && (!ud -> canfly) && !((ud -> canCloak) && (ud -> startCloaked) && (ai -> cb -> GetUnitPos(unitArray[i]) == ZEROVECTOR))) {
					allEligibleEnemies.push_back(unitArray[i]);
				}
			}
		}

		// L("AH: num of enemies of acceptable type: "  << allEligibleEnemies.size());

		vector<int> availableEnemies;
		availableEnemies.reserve(allEligibleEnemies.size());

		// now, filter out enemies currently in locations that existing groups are attacking

		// make a list of all assigned enemies
		list<int> takenEnemies;
		for (list<CAttackGroup>::iterator groupIt = attackGroups.begin(); groupIt != attackGroups.end(); groupIt++) {
			// TODO: when caching, fix this
			if ((!groupIt -> defending) && (groupIt -> GetGroupID() != group_in -> GetGroupID())) {
				list<int> assignedEnemies = groupIt -> GetAssignedEnemies();
				takenEnemies.merge(assignedEnemies);
			}
		}

		for (vector<int>::iterator enemy = allEligibleEnemies.begin(); enemy != allEligibleEnemies.end(); enemy++) {
			int enemyID = *enemy;
			bool found = false;

			for (list<int>::iterator it = takenEnemies.begin(); it != takenEnemies.end(); it++) {
				if (*it == enemyID) {
					found = true;
					break;
				}
			}

			if (!found)
				availableEnemies.push_back(enemyID);
		}

		// L("AH: num of enemies that are not taken: "  << availableEnemies.size());

		// make a list of the positions of these available enemies' positions
		// TODO: cache availableEnemies
		vector<float3> enemyPositions;
		enemyPositions.reserve(availableEnemies.size());

		for (vector<int>::iterator it = availableEnemies.begin(); it != availableEnemies.end(); it++) {
			// eligible target filtering has been done earlier
			enemyPositions.push_back(ai -> cheat -> GetUnitPos(*it));
		}

		// L("AH: num of positions of enemies sent to pathfinder: " << enemyPositions.size());
		// L("AH form: done adding enemies");

		// find cheapest target
		vector<float3> pathToTarget;

		float3 groupPos = group_in -> GetGroupPos();
		// L("AH form: running pather");
		const int ATTACKED_AREA_RADIUS = 800;
		ai -> pather -> micropather -> SetMapData(	ai -> pather -> MoveArrays[group_in -> GetWorstMoveType()],
													ai -> tm -> ThreatArray,
													ai -> tm -> ThreatMapWidth,
													ai -> tm -> ThreatMapHeight);

		// L("AH form: checking if we found someone");
		if (pathToTarget.size() > 2) {
			int lastIndex = pathToTarget.size() - 1;
			float3 endPos = pathToTarget[lastIndex];

			// L("AH form: getting enemies in radius of endPos");
			int enemiesInArea = ai -> cheat -> GetEnemyUnits(unitArray, endPos, ATTACKED_AREA_RADIUS);
			float powerOfEnemies = 0.000001;

			for (int i = 0; i < enemiesInArea; i++) {
				if (ai -> cheat -> GetUnitDef(unitArray[i]) -> weapons.size() > 0) {
					powerOfEnemies += ai -> cheat -> GetUnitDef(unitArray[i]) -> power;
				}
			}

			// L("AH: during group formation, numEnemies in sector:" << enemiesInArea << " the combined power of enemies at cheapest pos:" << powerOfEnemies << " my Power:" << group_in -> Power());
			if ((enemiesInArea > 0) && ((group_in -> Power()) > powerOfEnemies * 1.2f)) {
				// ATTACK !
				group_in -> AssignTarget(pathToTarget, pathToTarget.back(), ATTACKED_AREA_RADIUS);
			}
			else {
				group_in -> ClearTarget();
				// L("AH: during a power check, defense group was too weak, num:" << group_in -> Size() << " mypow:" << group_in -> Power() << " enemypow:" << powerOfEnemies);
			}
		}
	}
}


void CAttackHandler::AssignTargets() {
	int frameNr = ai -> cb -> GetCurrentFrame();

	// assign targets
	if (frameNr % 120 == 0) {
		// L("AH: start: attack group change");
		// L("AH: Iterating group list and assigning targets.");
		for(list<CAttackGroup>::iterator it = attackGroups.begin(); it != attackGroups.end(); it++) {
			CAttackGroup *group = &(*it);
			// force updates every 300frames
			if (group -> NeedsNewTarget() || frameNr % 300 == 0) {
				AssignTarget(group);
			}
		}
	}
}



void CAttackHandler::CombineGroups() {
	bool removedSomething = false;

	// pick a group A
	for (list<CAttackGroup>::iterator groupA = attackGroups.begin(); groupA != attackGroups.end(); groupA++) {
		// if it is defending
		if (groupA -> defending) {
			int groupAid = groupA -> GetGroupID();
			float3 groupApos = groupA -> GetGroupPos();
			// look for other groups that are defending
			for (list<CAttackGroup>::iterator groupB = attackGroups.begin(); groupB != attackGroups.end(); groupB++) {
				// if they are close, combine
				float3 groupBpos = groupB -> GetGroupPos();
				int groupBid = groupB -> GetGroupID();

				if ((groupB -> defending) && (groupAid != groupBid) && (groupApos.distance2D(groupBpos) < 1500)) {
					// L("AH:CombineGroups():: adding group " << groupB -> GetGroupID() << " to group " << groupA -> GetGroupID());
					vector<int>* bUnits = groupB -> GetAllUnits();

					for (vector<int>::iterator groupBunit = bUnits -> begin(); groupBunit != bUnits -> end(); groupBunit++) {
						groupA -> AddUnit(*groupBunit);
					}

					this -> attackGroups.erase(groupB);
					removedSomething = true;
					break;
				}
			}
		}

		if (removedSomething)
			break;
	}
}




void CAttackHandler::Update() {
//	std::cout << "CAttackHandler::Update()" << std::endl;

	int frameNr = ai -> cb -> GetCurrentFrame();

	if (frameNr < 2)
		UpdateKMeans();
	
	// set the map data here so i dont have to do it in each group or whatever
	// TODO: movement map PATHTOUSE = hack
	ai -> pather -> micropather -> SetMapData(ai -> pather -> MoveArrays[PATHTOUSE], ai -> tm -> ThreatArray, ai -> tm -> ThreatMapWidth, ai -> tm -> ThreatMapHeight);
	
	// update the k-means
	int frameSpread = 300;
	// calculate and draw k-means for the base perimeters
	if (frameNr % frameSpread == 0) {
		UpdateKMeans();
	}

	if (frameNr % frameSpread == 0) {
		int num = ai -> uh -> NumIdleUnits(CAT_G_ATTACK);
		for (int i = 0; i < num; i++) {
			int unit = ai -> uh -> GetIU(CAT_G_ATTACK);
			if (this -> PlaceIdleUnit(unit) && !ai -> cb -> GetUnitDef(unit) -> canfly)  {
				// L("AH: moved idle cat_g_attack unit. unit:" << unit << " groupid:" << ai -> MyUnits[unit] -> groupID);
				ai -> uh -> IdleUnitRemove(unit);
			}
		}
		// L("finished updating position for defense units");
	}

	// check for stuck units in the groups
	if (frameNr % 30 == 0) {
		for (list<CAttackGroup>::iterator it = attackGroups.begin(); it != attackGroups.end(); it++) {
			int stuckUnit = it -> PopStuckUnit();
			if (stuckUnit != -1 && ai -> cb -> GetUnitDef(stuckUnit) != NULL) {
				pair<int, float3> foo;
				foo.first = stuckUnit;
				foo.second = ai -> cb -> GetUnitPos(stuckUnit);
				stuckUnits.push_back(foo);
				// L("AttackHandler: popped a stuck unit from attack group " << it -> GetGroupID() << " and put it in the stuckUnits list which is now of size " << stuckUnits.size());
				ai -> MyUnits[stuckUnit] -> Stop();
				ai -> MyUnits[stuckUnit] -> groupID = STUCK_GROUP_ID;
			}

			// check if the group is now empty
			int groupSize = it -> Size();
			if (groupSize == 0) {
				attackGroups.erase(it);
				break;
			}
		}
	}

	//combine groups that are defending and too weak to attack anything
	if (frameNr % 300 == 0) {
		CombineGroups();
	}

	// check if we have any new units, add them to a group.
	if (frameNr % 30 == 0 && units.size() > 0) {
		// L("AH: add units to group");
		CAttackGroup* existingGroup = NULL;
		// find a defending group:
		for (list<CAttackGroup>::iterator it = attackGroups.begin(); it != attackGroups.end(); it++) {
			if (it -> Size() < 16 && it -> defending && this -> DistanceToBase(it -> GetGroupPos()) < 300) {
				existingGroup = &(*it);
			}
		}

		if (existingGroup != NULL) {
			// L("AH: adding to existing, num groups:" << attackGroups.size());
			for (list<int>::iterator it = units.begin(); it != units.end(); it++) {
				int unit = *it;
				if (ai -> cb -> GetUnitDef(unit) != NULL) {
					existingGroup -> AddUnit(unit);
					// L("added unit:" << unit << " type:" << ai -> cb -> GetUnitDef(unit) -> humanName);
				}
			}

			units.clear();
		}
		else {
			// we dont have a good group, make one
			newGroupID++;
			// L("creating new att group: group " << newGroupID);
			CAttackGroup newGroup(ai, newGroupID);
			newGroup.defending = true;

			for (list<int>::iterator it = units.begin(); it != units.end(); it++) {
				int unit = *it;
				if (ai -> cb -> GetUnitDef(unit) != NULL) {
					newGroup.AddUnit(unit);
					// L("added unit:" << unit << " type:" << ai -> cb -> GetUnitDef(unit) -> humanName);
				}
			}

			units.clear();
			attackGroups.push_back(newGroup);
		}
	}

	// update all the air units:
	UpdateAir();
	// basic attack group formation from defense units:
	this -> AssignTargets();

	//update current groups
	for (list<CAttackGroup>::iterator it = attackGroups.begin(); it != attackGroups.end(); it++) {
		it -> Update();
	}

	// print out the totals
	frameSpread = 7200;
	if (ai -> cb -> GetCurrentFrame() % frameSpread == 0) {
		// L("AttackHandler: writing out the number of total units");
		int sum = 0;
		float powerSum = 0;

		for (list<CAttackGroup>::iterator it = attackGroups.begin(); it != attackGroups.end(); it++) {
			int size = it -> Size();
			sum += size;
			// L("an attack group had " << size << " units and power:" << it -> Power() << ":");
			powerSum += it -> Power();
			it -> Log();
		}

		float airPower = 0;

		for (list<int>::iterator it = airUnits.begin(); it != airUnits.end(); it++) {
			airPower +=  ai -> cb -> GetUnitPower(*it);
			// L("" << counter++ << ":" << ai -> MyUnits[*it] -> def() -> humanName);
		}

		// L("airUnits: " << airUnits.size() << " units and power:" << airPower);
		sum += airUnits.size();
		powerSum += airPower;

		// L("total units: " << sum << " total power:" << powerSum);
		// L("in inactive group: " << units.size());
		// L("in stuck group: " << stuckUnits.size());
	}
}
