#include "AttackHandler.h"
#include "GlobalAI.h"
#include "creg/STL_List.h"
#include "creg/STL_Map.h"

#undef assert
#define assert(a) if (!(a)) throw "Assertion " #a "faild";

#define K_MEANS_ELEVATION 40
#define IDLE_GROUP_ID 0
#define STUCK_GROUP_ID 1
#define AIR_GROUP_ID 2
#define GROUND_GROUP_ID_START 1000
#define SAFE_SPOT_DISTANCE 300
#define KMEANS_ENEMY_MAX_K 32
#define KMEANS_BASE_MAX_K 32
#define KMEANS_MINIMUM_LINE_LENGTH 8*THREATRES
#define GROUP_MAX_NUM_UNITS 24
#define ATTACK_MAX_THREAT_DIFFERENCE 0.65f
#define ATTACKED_AREA_RADIUS  600

CR_BIND(CAttackHandler ,(NULL))

CR_REG_METADATA(CAttackHandler,(
				CR_MEMBER(ai),
				CR_MEMBER(units),
				CR_MEMBER(stuckUnits),
				CR_MEMBER(airUnits),
				CR_MEMBER(airIsAttacking),
				CR_MEMBER(airPatrolOrdersGiven),
				CR_MEMBER(airTarget),
				CR_MEMBER(newGroupID),
				CR_MEMBER(attackGroups),
				CR_MEMBER(unitArray),
				CR_MEMBER(kMeansBase),
				CR_MEMBER(kMeansK),
				CR_MEMBER(kMeansEnemyBase),
				CR_MEMBER(kMeansEnemyK),
				CR_RESERVED(256),
				CR_POSTLOAD(PostLoad)
				));


CAttackHandler::CAttackHandler(AIClasses* ai) {
	this -> ai = ai;

	if (ai) {
		// test: setting a position to the middle of the map
		float mapWidth = ai -> cb -> GetMapWidth() * 8.0f;
		float mapHeight = ai -> cb -> GetMapHeight() * 8.0f;

		newGroupID = GROUND_GROUP_ID_START + GROUND_GROUP_ID_START * (ai -> cb -> GetMyTeam());

		this -> kMeansK = 1;
		this -> kMeansBase.push_back(float3(mapWidth / 2.0f, K_MEANS_ELEVATION, mapHeight / 2.0f));
		this -> kMeansEnemyK = 1;
		this -> kMeansEnemyBase.push_back(float3(mapWidth / 2.0f, K_MEANS_ELEVATION, mapHeight / 2.0f));

		// timers
		this -> ah_timer_totalTime = ai -> math -> GetNewTimerGroupNumber("CAttackHandler and CAttackGroup");
		this -> ah_timer_totalTimeMinusPather = ai -> math -> GetNewTimerGroupNumber("CAttackHandler and CAttackGroup, everything except pather calls");
		this -> ah_timer_MicroUpdate = ai -> math -> GetNewTimerGroupNumber("CAttackGroup::MicroUpdate()");
		this -> ah_timer_Defend = ai -> math -> GetNewTimerGroupNumber("CAttackGroup::Defend()");
		this -> ah_timer_Flee = ai -> math -> GetNewTimerGroupNumber("CAttackGroup::Flee()");
		this -> ah_timer_MoveOrderUpdate = ai -> math -> GetNewTimerGroupNumber("CAttackGroup::MoveOrderUpdate()");
		this -> ah_timer_NeedsNewTarget = ai -> math -> GetNewTimerGroupNumber("CAttackGroup::NeedsNewTarget()");

		UpdateKMeans();
	}
	airIsAttacking = false;
	airPatrolOrdersGiven = false;
	airTarget = -1;

	this -> debug = true;
	this -> debugDraw = false;

//	if (debug)
//		L("constructor of CAttackHandler");
}

CAttackHandler::~CAttackHandler() {
}

void CAttackHandler::PostLoad()
{
	// timers
	this -> ah_timer_totalTime = ai -> math -> GetNewTimerGroupNumber("CAttackHandler and CAttackGroup");
	this -> ah_timer_totalTimeMinusPather = ai -> math -> GetNewTimerGroupNumber("CAttackHandler and CAttackGroup, everything except pather calls");
	this -> ah_timer_MicroUpdate = ai -> math -> GetNewTimerGroupNumber("CAttackGroup::MicroUpdate()");
	this -> ah_timer_Defend = ai -> math -> GetNewTimerGroupNumber("CAttackGroup::Defend()");
	this -> ah_timer_Flee = ai -> math -> GetNewTimerGroupNumber("CAttackGroup::Flee()");
	this -> ah_timer_MoveOrderUpdate = ai -> math -> GetNewTimerGroupNumber("CAttackGroup::MoveOrderUpdate()");
	this -> ah_timer_NeedsNewTarget = ai -> math -> GetNewTimerGroupNumber("CAttackGroup::NeedsNewTarget()");
}

void CAttackHandler::AddUnit(int unitID) {
	const UnitDef* ud = ai -> cb -> GetUnitDef(unitID);
	if (debug) L("CAttackHandler::AddUnit frame:"<<ai -> cb -> GetCurrentFrame()<<" unit:"<<unitID<<" name:"<<ud -> humanName<<" movetypething:"<< ai -> MyUnits[unitID] -> GetMoveType());
	ai -> math -> StartTimer(ai -> ah -> ah_timer_totalTime);
	ai -> math -> StartTimer(ai -> ah -> ah_timer_totalTimeMinusPather);

	{
		//the groupID of this "group" is 0, to separate them from other idle units
		ai -> MyUnits[unitID] -> groupID = IDLE_GROUP_ID;
		//this might be a new unit with the same id as an older dead unit
		ai -> MyUnits[unitID] -> stuckCounter = 0;
		//do some checking then essentially add it to defense group
		units.push_back(unitID);
		//TODO: this aint that good tbh, but usually does move it away from the factory
		this -> PlaceIdleUnit(unitID);
	}

	ai -> math -> StopTimer(ai -> ah -> ah_timer_totalTime);
	ai -> math -> StopTimer(ai -> ah -> ah_timer_totalTimeMinusPather);
}

void CAttackHandler::UnitDestroyed(int unitID)
{
	ai -> math -> StartTimer(ai -> ah -> ah_timer_totalTime);
	ai -> math -> StartTimer(ai -> ah -> ah_timer_totalTimeMinusPather);
	int attackGroupID = ai -> MyUnits[unitID] -> groupID;
//	L("AttackHandler: unitDestroyed id:" << unitID << " groupID:" << attackGroupID << " numGroups:" << attackGroups.size());
	//if its in the defense group:
	if (attackGroupID == IDLE_GROUP_ID) {
		bool found_dead_unit_in_attackHandler = false;
		for (list<int>::iterator it = units.begin(); it != units.end(); it++) {
			if (*it == unitID) {
				units.erase(it);
				found_dead_unit_in_attackHandler = true;
				break;
			}
		}
		if (!found_dead_unit_in_attackHandler) 
			L("ERROR: cannot find dead unit in attack handler idle list "<<unitID);
//		assert(found_dead_unit_in_attackHandler);
	} else if (attackGroupID >= GROUND_GROUP_ID_START) {
		//its in an attackgroup
		bool foundGroup = false;
		bool removedDeadUnit = false;
		list<CAttackGroup>::iterator it;
		//bool itWasAttGroup = false;
		for (it = attackGroups.begin(); it != attackGroups.end(); it++) {
			if (it -> GetGroupID() == attackGroupID) {
				removedDeadUnit = it -> RemoveUnit(unitID);
				foundGroup = true;
				//itWasAttGroup = true;
				break;
			}
		}
		L("ERROR: cannot find dead unit in attack handler group list "<<unitID);
//		assert(foundGroup); // TODO: this has failed before. and again.
//		assert(removedDeadUnit);
		//check if the group is now empty
//		L("AH: about to check if a group needs to be deleted entirely");
		int groupSize = it -> Size();
		if (groupSize == 0) {
//			L("AH: yes, its ID is " << it -> GetGroupID());
			attackGroups.erase(it);
		}
	} else if(attackGroupID == AIR_GROUP_ID) {
//		L("AH: unit destroyed and its in the air group, trying to remove");
		bool found_dead_unit_in_airUnits = false;
		for (list<int>::iterator it = airUnits.begin(); it != airUnits.end(); it++) {
			if (*it == unitID) {
				airUnits.erase(it);
				found_dead_unit_in_airUnits = true;
				break;
			}
		}
		L("ERROR: cannot find dead unit in attack handler air list "<<unitID);
//		assert(found_dead_unit_in_airUnits);
	}else {
		//its in stuckunits
		bool found_dead_in_stuck_units = false;
		list<pair<int,float3> >::iterator it;
		for (it = stuckUnits.begin(); it != stuckUnits.end(); it++) {
			if (it -> first == unitID) {
				stuckUnits.erase(it);
				found_dead_in_stuck_units = true;
				break;
			}
		}
		L("ERROR: cannot find dead unit in attack handler stuck list "<<unitID);
//		assert(found_dead_in_stuck_units);
	}
	if (debug) L("AH: unit deletion done");
	ai -> math -> StopTimer(ai -> ah -> ah_timer_totalTime);
	ai -> math -> StopTimer(ai -> ah -> ah_timer_totalTimeMinusPather);
}

vector<float3>* CAttackHandler::GetKMeansBase()
{
	return &this -> kMeansBase;
}

vector<float3>* CAttackHandler::GetKMeansEnemyBase()
{
	return &this -> kMeansEnemyBase;
}

//if something is on this position, can it drive to any parts of our base?
bool CAttackHandler::CanTravelToBase(float3 pos) //TODO add movetype
{
	ai -> math -> StartTimer(ai -> ah -> ah_timer_totalTime);
	ai -> pather -> micropather -> SetMapData(&ai -> pather -> canMoveIntMaskArray.front(),&ai -> tm -> ThreatArray.front(),ai -> tm -> ThreatMapWidth,ai -> tm -> ThreatMapHeight, PATHTOUSE);
	bool res = ai -> pather -> PathExistsToAny(pos, *this -> GetKMeansBase());
	ai -> math -> StopTimer(ai -> ah -> ah_timer_totalTime);
	return res;
}

//if something is on this position, can it drive to any parts of the enemy base?
bool CAttackHandler::CanTravelToEnemyBase(float3 pos) //TODO add movetype
{
	ai -> math -> StartTimer(ai -> ah -> ah_timer_totalTime);
	ai -> pather -> micropather -> SetMapData(&ai -> pather -> canMoveIntMaskArray.front(),&ai -> tm -> ThreatArray.front(),ai -> tm -> ThreatMapWidth,ai -> tm -> ThreatMapHeight, PATHTOUSE);
   	bool res = ai -> pather -> PathExistsToAny(pos, *this -> GetKMeansEnemyBase());
	if (debugDraw) {
		AIHCAddMapPoint amp;
		if (res) {
			amp.label = "CanTravelToEnemyBase";
		} else {
			amp.label = "NOT CanTravelToEnemyBase";
		}
		amp.pos = pos;
		ai -> cb -> HandleCommand(AIHCAddMapPointId, &amp);
	}
	ai -> math -> StopTimer(ai -> ah -> ah_timer_totalTime);
	return res;
}

bool CAttackHandler::PlaceIdleUnit(int unit)
{
	if (ai -> cb -> GetUnitDef(unit) != NULL) {
		float3 moo = FindUnsafeArea(ai -> cb -> GetUnitPos(unit));
		if (moo != ZEROVECTOR && moo != ERRORVECTOR) {
            ai -> MyUnits[unit] -> Move(moo);
		}
	}
	return false;
}

//returns a safe spot from k-means, adjacent to myPos, safety params are (0..1).
//this is going away or changing.
//change to: decide on the random float 0...1 first, then find it. waaaay easier.
float3 CAttackHandler::FindSafeSpot(float3 myPos, float minSafety, float maxSafety) {
	myPos = myPos;
	//find a safe spot
	int startIndex = int(minSafety * this -> kMeansK);
	if (startIndex < 0) startIndex = 0;
	int endIndex = int(maxSafety * this -> kMeansK);
	if (endIndex < 0) startIndex = 0;
	if (startIndex > endIndex) startIndex = endIndex;
	if (kMeansK <= 1 || startIndex == endIndex) {
		if (startIndex >= kMeansK) startIndex = kMeansK-1;
		float3 pos = kMeansBase[startIndex] + float3((RANDINT%SAFE_SPOT_DISTANCE), 0, (RANDINT%SAFE_SPOT_DISTANCE));
		pos.y = ai -> cb -> GetElevation(pos.x, pos.z);
		return pos;
	}
	assert(startIndex < endIndex);
	assert(startIndex < kMeansK);
	assert(endIndex <= kMeansK);
	//get a subset of the kmeans

	vector<float3> subset;
	for(int i = startIndex; i < endIndex; i++) {
		assert ( i < kMeansK);
		subset.push_back(kMeansBase[i]);
	}
	//then find a position on one of the lines between those points (pather)
	int whichPath;
	if (subset.size() > 1) whichPath = RANDINT % (int)subset.size();
	else whichPath = 0;

	assert (whichPath < (int)subset.size());
	assert (subset.size() > 0);
	if (whichPath+1 < (int)subset.size() && subset[whichPath].distance2D(subset[whichPath+1]) > KMEANS_MINIMUM_LINE_LENGTH) {
        vector<float3> posPath;
		ai -> pather -> micropather -> SetMapData(&ai -> pather -> canMoveIntMaskArray.front(),&ai -> tm -> ThreatArray.front(),ai -> tm -> ThreatMapWidth,ai -> tm -> ThreatMapHeight, PATHTOUSE);
		float cost = ai -> pather -> PathToPos(&posPath, subset[whichPath], subset[whichPath+1]);
		float3 res;
		if (cost > 0) {
			int whichPos = RANDINT % (int)posPath.size();
			res = posPath[whichPos];
		} else {
			res = subset[whichPath];
		}
		return res;
	} else {
		assert (whichPath < (int)subset.size());
		float3 res = subset[whichPath];
		return res;
	}
}

float3 CAttackHandler::FindSafeArea(float3 pos) {
	if(this -> DistanceToBase(pos) < SAFE_SPOT_DISTANCE) return pos;
	float min = 0.6;
	float max = 0.95;
	float3 safe = this -> FindSafeSpot(pos, min, max);
	//TODO: this hack makes me cry. blood. from my ears. while dying. in a fire.
	safe += pos;
	safe /= 2;
	return safe;
}

float3 CAttackHandler::FindVerySafeArea(float3 pos) {
	float min = 0.9;
	float max = 1.0;
	return this -> FindSafeSpot(pos, min, max);
}

float3 CAttackHandler::FindUnsafeArea(float3 pos) {
	float min = 0.1;
	float max = 0.3;
	return this -> FindSafeSpot(pos, min, max);
}

float CAttackHandler::DistanceToBase(float3 pos) {
	float closestDistance = FLT_MAX;
	for(int i = 0; i < this -> kMeansK; i++) {
		float3 mean = this -> kMeansBase[i];
		float distance = pos.distance2D(mean);
		closestDistance = min(distance, closestDistance);
	}
	return closestDistance;
}

float3 CAttackHandler::GetClosestBaseSpot(float3 pos) {
	float closestDistance = FLT_MAX;
	int index = 0;
	for(int i = 0; i < this -> kMeansK; i++) {
		float3 mean = this -> kMeansBase[i];
		float distance = pos.distance2D(mean);
		if (distance < closestDistance) {
			closestDistance = distance;
			index = i;
		}
	}
	return kMeansBase[index];
}

vector<float3> CAttackHandler::KMeansIteration(vector<float3> means, vector<float3> unitPositions, int newK)
{
	assert(newK > 0 && means.size() > 0);
	int numUnits = unitPositions.size();
	//change the number of means according to newK
	int oldK = means.size();
	means.resize(newK);
	//add a new means, just use one of the positions
	float3 newMeansPosition = unitPositions[0];
	newMeansPosition.y = ai -> cb -> GetElevation(newMeansPosition.x, newMeansPosition.z) + K_MEANS_ELEVATION;
	for (int i = oldK; i < newK; i++) {
		means[i] = newMeansPosition;
	}
	//check all positions and assign them to means, complexity n*k for one iteration
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
		//position nr i is closest to the mean at closestIndex
		unitsClosestMeanID[i] = closestIndex;
		numUnitsAssignedToMean[closestIndex]++;
	}
	//change the means according to which positions are assigned to them
	//use meanAverage for indexes with 0 pos'es assigned
	//make a new means list
	vector<float3> newMeans;
	newMeans.resize(newK, float3(0,0,0));
	for(int i = 0; i < numUnits; i++) {
		int meanIndex = unitsClosestMeanID[i];
		float num = max(1, numUnitsAssignedToMean[meanIndex]); //dont want to divide by 0
		newMeans[meanIndex] += unitPositions[i] / num;
	}
	//do a check and see if there are any empty means //, and set the height.
	for(int i = 0; i < newK; i++) {
		//if a newmean is unchanged, set it to the new means pos instead of 0,0,0
		if (newMeans[i] == float3(0,0,0)) {
			newMeans[i] = newMeansPosition;
		} else {
			//get the proper elevation for the y coord
			newMeans[i].y = ai -> cb -> GetElevation(newMeans[i].x, newMeans[i].z) + K_MEANS_ELEVATION;
		}
	}
	return newMeans;
}

//TODO: remove cloaked things (like perimeter cameras) so they wont be a defense priority
void CAttackHandler::UpdateKMeans() {
	const int arrowDuration = 300;
	{//want local variable definitions
		//get positions of all friendly units and put them in a vector (completed buildings only)
		int numFriendlies = 0;
		vector<float3> friendlyPositions;
		int friendlies[MAXUNITS];
		numFriendlies = ai -> cb -> GetFriendlyUnits(friendlies);
		for (int i = 0; i < numFriendlies; i++) {
			int unit = friendlies[i];
			CUNIT* u = ai -> MyUnits[unit];
			//its a building, it has hp, and its mine (0)
			if (this -> UnitBuildingFilter(u -> def()) && this -> UnitReadyFilter(unit) && u -> owner() == 0) {
				friendlyPositions.push_back(u -> pos());
			}
		}
		//hack to make it at least 1 unit, should only happen when you have no base:
		if (friendlyPositions.size() < 1) {
			//it has to be a proper position, unless there are no proper positions.
			if (numFriendlies > 0 && ai -> cb -> GetUnitDef(friendlies[0]) && ai -> MyUnits[friendlies[0]] -> owner() == 0) friendlyPositions.push_back(ai -> cb -> GetUnitPos(friendlies[0]));
			else friendlyPositions.push_back(float3(RANDINT % (ai -> cb -> GetMapWidth()*8), 1000, RANDINT % (ai -> cb -> GetMapHeight()*8))); //when everything is dead
		}
		//calculate a new K. change the formula to adjust max K, needs to be 1 minimum.
		this -> kMeansK = int(min((float)(KMEANS_BASE_MAX_K), 1.0f + sqrtf((float)numFriendlies+0.01f)));
		//iterate k-means algo over these positions and move the means
		this -> kMeansBase = KMeansIteration(this -> kMeansBase, friendlyPositions, this -> kMeansK);
	}
	//update enemy position k-means
	//get positions of all enemy units and put them in a vector (completed buildings only) TODO: buildingslist from surveillance
	int numEnemies = 0;
	vector<float3> enemyPositions;
	const int* enemies = ai -> sh -> GetEnemiesList();
	numEnemies = ai -> sh -> GetNumberOfEnemies(); // ai -> cheat -> GetEnemyUnits(enemies);
	for (int i = 0; i < numEnemies; i++) {
		const UnitDef *ud = ai -> cheat -> GetUnitDef(enemies[i]);
		if (this -> UnitBuildingFilter(ud)) { // && this -> UnitReadyFilter(unit)) {
			enemyPositions.push_back(ai -> cheat -> GetUnitPos(enemies[i]));
		}
	}
	//hack to make it at least 1 unit, should only happen when you have no base:
	if (enemyPositions.size() < 1) {
		//it has to be a proper position, unless there are no proper positions.
		if (numEnemies > 0 && ai -> cheat -> GetUnitDef(enemies[0])) enemyPositions.push_back(ai -> cheat -> GetUnitPos(enemies[0]));
		else enemyPositions.push_back(float3(RANDINT % (ai -> cb -> GetMapWidth()*8), 1000, RANDINT % (ai -> cb -> GetMapHeight()*8))); //when everything is dead
	}
	//calculate a new K. change the formula to adjust max K, needs to be 1 minimum.
	this -> kMeansEnemyK = int(min((float)(KMEANS_ENEMY_MAX_K), 1.0f + sqrtf((float)numEnemies+0.01f)));
//		L("AttackHandler: doing k-means k:" << kMeansK << " numPositions=" << numFriendlies);
	//iterate k-means algo over these positions and move the means
	this -> kMeansEnemyBase = KMeansIteration(this -> kMeansEnemyBase, enemyPositions, this -> kMeansEnemyK);
	//base k-means and enemy base k-means are updated.	
	//approach: add up (max - distance) to enemies
	vector<float> proximity;
	proximity.resize(kMeansK, 0.0000001f);
	const float mapDiagonal = sqrt(pow((float)ai -> cb -> GetMapHeight()*8,2) + pow((float)ai -> cb -> GetMapWidth()*8,2) + 1.0f);

	for (int f = 0; f < kMeansK; f++) {
		for (int e = 0; e < kMeansEnemyK; e++) {
			proximity[f] += mapDiagonal - kMeansBase[f].distance2D(kMeansEnemyBase[e]);
		}
	}
	//sort kMeans by the proximity score
	float3 tempPos;
	float temp;
	for (int i = 1; i < kMeansK; i++) { //how many are completed
		for (int j = 0; j < i; j++) { //compare to / switch with
			if (proximity[i] > proximity[j]) { //switch
				tempPos = kMeansBase[i];
				kMeansBase[i] = kMeansBase[j];
				kMeansBase[j] = tempPos;
				temp = proximity[i];
				proximity[i] = proximity[j];
				proximity[j] = temp;
			}
		}
	}
	//okay, so now we have a kMeans list sorted by distance to enemies, 0 being risky and k being safest.
	if (debugDraw) {
		int lineWidth = 25;
		//now, draw these means on the map
		for (int i = 1; i < kMeansK; i++) {
			int figureGroup = (RANDINT % 59549847);
			ai -> cb -> CreateLineFigure(kMeansBase[i-1]+float3(0,100,0), kMeansBase[i]+float3(0,100,0), lineWidth, 1, arrowDuration, figureGroup);
			ai -> cb -> SetFigureColor(figureGroup, 160, 32, 240, 0.3);
		}
	}
}

bool CAttackHandler::UnitGroundAttackFilter(int unit) {
	CUNIT u = *ai -> MyUnits[unit];
	bool result = u.def() != NULL && u.def() -> canmove && (u.category() == CAT_ATTACK || u.category() == CAT_ARTILLERY || u.category() == CAT_ASSAULT);
	return result;
}

bool CAttackHandler::UnitBuildingFilter(const UnitDef *ud) {
	bool result = ud != NULL && ud -> speed <= 0;
	return result;
}

bool CAttackHandler::CanHandleThisUnit(int unit) {
	const UnitDef *ud = ai -> cb -> GetUnitDef(unit);
	bool result = false;
	if (ud != NULL) {
		result = ud -> speed > 0 && !ud -> builder;
	}
	return result;
}

bool CAttackHandler::UnitReadyFilter(int unit) {
	CUNIT u = *ai -> MyUnits[unit];
	bool result = u.def() != NULL 
			&& !ai -> cb -> UnitBeingBuilt(unit) 
			&& ai -> cb -> GetUnitHealth(unit) > ai -> cb -> GetUnitMaxHealth(unit)*0.5f;
	return result;
}

void CAttackHandler::UpdateAir() {
	if (airUnits.size() == 0) return;
	/*
	if enemy is dead, attacking = false
	every blue moon, do an air raid on most valuable thingy
	*/
	assert(!(airIsAttacking && airTarget == -1));
	if (airIsAttacking &&  (airUnits.size() == 0 || ai -> cheat -> GetUnitDef(airTarget) == NULL)) {
		airTarget = -1;
		airIsAttacking = false;
	}
	if (ai -> cb -> GetCurrentFrame() % (60*30*5) == 0  //5 mins
			|| (ai -> cb -> GetCurrentFrame() % (30*30) == 0 && airUnits.size() > 8)) { //30 secs && 8+ units
		if (debug) L("AH: trying to attack with air units.");
		int numOfEnemies = ai -> cheat -> GetEnemyUnits(unitArray);
		int bestID = -1;
		float bestFound = -1.0;
		for (int i = 0; i < numOfEnemies; i++) {
			int enemy = unitArray[i];
			if (enemy != -1 && ai -> cheat -> GetUnitDef(enemy) != NULL && ai -> cheat -> GetUnitDef(enemy) -> metalCost > bestFound) {
				bestID = enemy;
				bestFound = ai -> cheat -> GetUnitDef(enemy) -> metalCost;
			}
		}
		if (debug) L("AH: selected the enemy: " << bestID);
		if (bestID != -1 && ai -> cheat -> GetUnitDef(bestID)) {
			//give the order
			for (list<int>::iterator it = airUnits.begin(); it != airUnits.end(); it++) {
				CUNIT* u = ai -> MyUnits[*it];
				u -> Attack(bestID);
			}
			airIsAttacking = true;
			airTarget = bestID;
			ai -> cb -> SendTextMsg("AH: air group is attacking", 0);
		}
	}
	//TODO: units currently being built. (while the others are off attacking)
	if (ai -> cb -> GetCurrentFrame() % 1800 == 0) {
		airPatrolOrdersGiven = false;
	}
	if (!airPatrolOrdersGiven && !airIsAttacking) {
		if (debug) L("AH: updating air patrol routes");
		//get / make up some outer base perimeter points
		vector<float3> outerMeans;
		const int num = 3;
		outerMeans.reserve(num);
		if (kMeansK > 1) {
			int counter = 0;
			counter += kMeansK / 8; //offsetting the outermost one
			for (int i = 0; i < num; i++) {
				outerMeans.push_back(kMeansBase[counter]);
				if (counter < kMeansK-1) counter++;
			}
		} else {
			//theres just 1 kmeans and we need three
			for (int i = 0; i < num; i++) {
				outerMeans.push_back(kMeansBase[0] + float3(250*i, 0, 0));
			}
		}
		assert(outerMeans.size() == (unsigned) num);
		//give the patrol orders to the outer means
		for (list<int>::iterator it = airUnits.begin(); it != airUnits.end(); it++) {
			CUNIT* u = ai -> MyUnits[*it];
			u -> Move(outerMeans[0] + float3(0,50,0)); //do this first in case theyre in the enemy base
			for (int i = 0; i < num; i++) {
				u -> PatrolShift(outerMeans[i]);
			}
		}
		airPatrolOrdersGiven = true;
	}
}

void CAttackHandler::AssignTarget(CAttackGroup* group_in) {
	
	if (debug) L("AH: assign target to group " << group_in -> GetGroupID());
	//do a target finding check
	//GET ENEMIES, FILTER THEM BY WHATS TAKEN, PATH TO THEM THEN GIVE THE PATH TO THE GROUP
	int numOfEnemies = ai -> sh -> GetNumberOfEnemies(); //ai -> cheat -> GetEnemyUnits(unitArray);
	const int* allEnemies = ai -> sh -> GetEnemiesList();
//	L("AH: initial num of enemies: "  << numOfEnemies);
	if (numOfEnemies) {
		//build vector of enemies
		static vector<int> allEligibleEnemies;
		allEligibleEnemies.reserve(numOfEnemies);
		allEligibleEnemies.clear();
		//make a vector with the positions of all enemies
		for (int i = 0; i < numOfEnemies; i++) {
			if(allEnemies[i] != -1) {
				const UnitDef * ud = ai -> cheat -> GetUnitDef(allEnemies[i]);
				//filter out cloaked and air enemies
				if (ud != NULL && !ud -> canfly 
						&& !(ud -> canCloak 
							//&& ud -> startCloaked 
							&& ai -> cb -> GetUnitPos(allEnemies[i]) == ZEROVECTOR)) {
					allEligibleEnemies.push_back(allEnemies[i]);
				}
			}
		}
//		L("AH: num of enemies of acceptable type: "  << allEligibleEnemies.size());
		static vector<int> availableEnemies;
		availableEnemies.reserve(allEligibleEnemies.size()); // its fewer though, really
		availableEnemies.clear();
		//now, filter out enemies currently in locations that existing groups are attacking
		//make a list of all assigned enemies:
		list<int> takenEnemies;
		for (list<CAttackGroup>::iterator groupIt = attackGroups.begin(); groupIt != attackGroups.end(); groupIt++) {
			if (!groupIt -> Defending() && groupIt -> GetGroupID() != group_in -> GetGroupID()) { // TODO when caching, fix this
				list<int> theGroup = *groupIt -> GetAssignedEnemies();
				//list<int> hack = *theGroup;
				takenEnemies.splice(takenEnemies.end(), theGroup);
//				L("AH debug: added to takenEnemies, size is now " << takenEnemies.size());
			}
		}
		//put in availableEnemies those allEligibleEnemies that are not in takenEnemies
		for (vector<int>::iterator enemy = allEligibleEnemies.begin(); enemy != allEligibleEnemies.end(); enemy++) {
			int enemyID = *enemy;
			bool found = false;
			for (list<int>::iterator it = takenEnemies.begin(); it != takenEnemies.end(); it++) {
				if (*it == enemyID) {
					found = true;
					break;
				}
			}
			if (!found) availableEnemies.push_back(enemyID);
		}
//		L("AH: num of enemies that arent taken: "  << availableEnemies.size());
		//TODO: cache availableEnemies
		//make a list of the positions of these available enemies' positions
		static vector<float3> enemyPositions;
		enemyPositions.reserve(availableEnemies.size());
		enemyPositions.clear();
		for (vector<int>::iterator it = availableEnemies.begin(); it != availableEnemies.end(); it++) {
			//eligible target filtering has been done earlier
			enemyPositions.push_back(ai -> cheat -> GetUnitPos(*it));
		}
//		L("AH: num of positions of enemies sent to pathfinder: " << enemyPositions.size());
		//find cheapest target
		static vector<float3> pathToTarget;
		pathToTarget.clear();
		float3 groupPos = group_in -> GetGroupPos();
		//ai -> pather -> micropather -> SetMapData(ai -> pather -> MoveArrays[group_in -> GetWorstMoveType()],ai -> tm -> ThreatArray,ai -> tm -> ThreatMapWidth,ai -> tm -> ThreatMapHeight);
//		float costToTarget = ai -> pather -> PathToSet(&pathToTarget, groupPos, &enemyPositions);
		float myGroupDPS = group_in -> DPS();		

ai -> math -> StopTimer(ai -> ah -> ah_timer_totalTimeMinusPather);
		ai -> pather -> micropather -> SetMapData(&ai -> pather -> canMoveIntMaskArray.front(),&ai -> tm -> ThreatArray.front(),ai -> tm -> ThreatMapWidth,ai -> tm -> ThreatMapHeight, group_in -> GetWorstMoveType());
		ai -> pather -> PathToSet(&pathToTarget,group_in->GetGroupPos(),&enemyPositions,myGroupDPS);

ai -> math -> StartTimer(ai -> ah -> ah_timer_totalTimeMinusPather);
		if (pathToTarget.size() > 2) { //if it found something below max threat
			int lastIndex = pathToTarget.size()-1;
			float3 endPos = pathToTarget[lastIndex];//the position at the end
			//TODO: add move type of the threat map when support for that is added
			float targetThreat = ai -> tm -> ThreatAtThisPoint(endPos) - ai -> tm -> GetUnmodifiedAverageThreat();
			L("AH: AssignTarget() group:" << group_in -> GetGroupID() << " groupdps:" << myGroupDPS << " targetThreat:" << targetThreat << " num units:" << group_in -> Size() <<" - -> ATTACK!");
			//ATTACK !
//			L("AH: during a power check, defense group is strong enough: attacking !");
			group_in -> AssignTarget(pathToTarget, pathToTarget.back(), ATTACKED_AREA_RADIUS);
		}//endif nopath
		else { //TODO dont re-give the same order and so on
			//TODO: maybe combine groups here
			L("AH: AssignTarget() group:" << group_in -> GetGroupID() << " groupdps:" << myGroupDPS << " num units:" << group_in -> Size() <<" - -> continue defending...");
			group_in -> Defend();
		}

	}//endif noenemies
}

void CAttackHandler::AssignTargets() {
	int frameNr = ai -> cb -> GetCurrentFrame();
	//assign targets
//	if(frameNr % 600 == 0) {
		//L("AH debug: supposedly a full update cycle of targets");
//	}
	for(list<CAttackGroup>::iterator it = attackGroups.begin(); it != attackGroups.end(); it++) {
		CAttackGroup *group = &*it;
		if ((group -> NeedsNewTarget() &&  frameNr % 60 == 0)) {//  || frameNr % 300 == 0) {
			AssignTarget(group);
		}
//		if (group -> NeedsNewTarget()) {
//			ai -> cb -> SendTextMsg("AH: target no longer needed? o_O or no targets...", 0);
//		}
	}
}

void CAttackHandler::CombineGroups() {
	bool removedSomething = false;
	//pick a group A
	for (list<CAttackGroup>::iterator groupA = attackGroups.begin(); groupA != attackGroups.end(); groupA++) {
		//if it is defending
		if (groupA -> Defending()) {
			int groupAid = groupA -> GetGroupID();
			float3 groupApos = groupA -> GetGroupPos();
			unsigned groupAMoveType = groupA -> GetWorstMoveType();
			//look for other groups that are defending.
			for (list<CAttackGroup>::iterator groupB = attackGroups.begin(); groupB != attackGroups.end(); groupB++) {
				//if they are close, combine.
				float3 groupBpos = groupB -> GetGroupPos();
				int groupBid = groupB -> GetGroupID();
				unsigned groupBMoveType = groupB -> GetWorstMoveType();
				if (groupB -> Defending()
						&& groupAid != groupBid 
						&& groupApos.distance2D(groupBpos) < 1500
						&& groupAMoveType == groupBMoveType
						&& groupA -> Size()+groupB -> Size() < GROUP_MAX_NUM_UNITS) { //TODO get a better pragmatic filter
					if (debug) L("AH:CombineGroups():: adding group " << groupB -> GetGroupID() << " to group " << groupA -> GetGroupID());
					//deeeeeeeeeeeeestroooooooooooooy
					vector<int>* bUnits = groupB -> GetAllUnits();
					for (vector<int>::iterator groupBunit = bUnits -> begin(); groupBunit != bUnits -> end(); groupBunit++) {
						groupA -> AddUnit(*groupBunit);
					}
					this -> attackGroups.erase(groupB); //TODO uh-oh
					removedSomething = true;
					break;
				}
			}
		}
		if (removedSomething) break; //both for loops iterate the same list
	}

}

void CAttackHandler::Update()
{
ai -> math -> StartTimer(ai -> ah -> ah_timer_totalTime);
ai -> math -> StartTimer(ai -> ah -> ah_timer_totalTimeMinusPather);
//	ai -> math -> TimerStart();
	int frameNr = ai -> cb -> GetCurrentFrame();
	if (frameNr < 2) UpdateKMeans();
	//set the map data here so i dont have to do it in each group or whatever
	//TODO: movement map PATHTOUSE = hack
	ai -> pather -> micropather -> SetMapData(&ai -> pather -> canMoveIntMaskArray.front(),&ai -> tm -> ThreatArray.front(),ai -> tm -> ThreatMapWidth,ai -> tm -> ThreatMapHeight, PATHTOUSE);
	//update the k-means
	int frameSpread = 300; //frames between each update
	//calculate and draw k-means for the base perimeters
	if (frameNr % frameSpread == 0) {
		UpdateKMeans();
	}

	//check for stuck units in the groups
	if (frameNr % 30 == 0) {
		for (list<CAttackGroup>::iterator it = attackGroups.begin(); it != attackGroups.end(); it++) {
			int stuckUnit = it -> PopStuckUnit();
			if (stuckUnit != -1 && ai -> cb -> GetUnitDef(stuckUnit) != NULL) {
				//this -> AddUnit(stuckUnit);
				pair<int, float3> foo;
				foo.first = stuckUnit;
				foo.second = ai -> cb -> GetUnitPos(stuckUnit);
				stuckUnits.push_back(foo);
				if (debug) L("AttackHandler: popped a stuck unit from attack group " << it -> GetGroupID() << " and put it in the stuckUnits list which is now of size " << stuckUnits.size());
				ai -> MyUnits[stuckUnit] -> Stop();
				ai -> MyUnits[stuckUnit] -> groupID = STUCK_GROUP_ID;
			}
			//check if the group is now empty
			int groupSize = it -> Size();
			if (groupSize == 0) {
				attackGroups.erase(it);
				break;
			}
		}
	}
	//attempt to combine groups that are defending and too weak to attack anything
	if (frameNr % 30 == 0) {
		CombineGroups();
	}
	//check if we have any new units, add them to a group.
	if (frameNr % 30 == 0 && units.size() > 0) {
//		L("AH: add units to group");
		for (list<int>::iterator unitIt = units.begin(); unitIt != units.end(); unitIt++) {
			int unit = *unitIt;
			const UnitDef* ud = ai -> cb -> GetUnitDef(unit);
			if (ud != NULL) {
				unsigned unitMoveType = ai -> MyUnits[unit] -> GetMoveType();//ai -> ut -> unittypearray[ud -> id].moveSlopeType | ai -> ut -> unittypearray[ud -> id].moveDepthType;
				//TODO: add the water thing and maybe crush thing
				if (debug) L("CAttackHandler::Update() adding unit:" << unit << " type:" << ud -> humanName << " unitMoveType:" << unitMoveType);
				CAttackGroup* existingGroup = NULL;
				//find a defending group:
				const int JOIN_GROUP_DISTANCE = 300;
				const float MAX_SPEED_DIFFERENCE_TO_JOIN = 1.5;
				for (list<CAttackGroup>::iterator it = attackGroups.begin(); it != attackGroups.end(); it++) {
					if (it -> Size() < GROUP_MAX_NUM_UNITS 
							&& it -> Defending()
							&& this -> DistanceToBase(it -> GetGroupPos()) < JOIN_GROUP_DISTANCE
							&& ai -> MyUnits[unit] -> def() -> speed <= it -> GetHighestUnitSpeed() * MAX_SPEED_DIFFERENCE_TO_JOIN
							&& ai -> MyUnits[unit] -> def() -> speed * MAX_SPEED_DIFFERENCE_TO_JOIN >= it -> GetHighestUnitSpeed() 
							&& it -> GetWorstMoveType() == unitMoveType) {
						existingGroup = &*it;
						break;
					}
				}
				if (existingGroup != NULL) {
					if (debug) L("CAttackHandler::Update() adding unit to existing group:" << existingGroup -> GetGroupID() << " num groups:" << attackGroups.size() << " added unit:" << unit << " type:" << ai -> cb -> GetUnitDef(unit) -> humanName);
					existingGroup -> AddUnit(unit);
				} else {
					if (debug) L("CAttackHandler::Update() creating new group for unit, num groups:" << attackGroups.size() << " added unit:" << unit << " type:" << ai -> cb -> GetUnitDef(unit) -> humanName);
					//we dont have a good group, make one
					newGroupID++;
		//			L("creating new att group: group " << newGroupID);
					CAttackGroup newGroup(ai, newGroupID);
					//newGroup.defending = true;
					newGroup.AddUnit(unit);
		//					L("added unit:" << unit << " type:" << ai -> cb -> GetUnitDef(unit) -> humanName);
					attackGroups.push_back(newGroup);
				}
			}
		}
		units.clear();
	}
	//update all the air units:
	UpdateAir();
	//basic attack group formation from defense units:
	this -> AssignTargets();
	//update current groups
	int counter = 0;
	//ai -> cb -> GetSelectedUnits()

	//adding extremely basic cyborg play support:
	int numSelectedUnits = ai -> cb -> GetSelectedUnits(unitArray);
	for (list<CAttackGroup>::iterator it = attackGroups.begin(); it != attackGroups.end(); it++) {
		int thisGroupID = it -> GetGroupID();
		bool thisGroupTaken = false;
		for (int i = 0; i < numSelectedUnits; i++) {
			if (ai -> cb -> GetUnitDef(unitArray[i]) && ai -> MyUnits[unitArray[i]] -> groupID == thisGroupID) {
				thisGroupTaken = true;
				L("AH::Update for group " << thisGroupID << " frame:" << frameNr << " unit in group taken by player");
				break;
			}
		}
		if (!thisGroupTaken) it -> Update();
	}
	//print out the totals
	frameSpread = 7200; //frames between each update
	if (debug && ai -> cb -> GetCurrentFrame() % frameSpread == 0) {
		L("---------------------------------------------------------------------------");
		L("AttackHandler: writing out the number of total units");
		int sum = 0;
		float powerSum = 0;
		for (list<CAttackGroup>::iterator it = attackGroups.begin(); it != attackGroups.end(); it++) {
			int size = it -> Size();
			sum += size; 
			L("an attack group had " << size << " units and DPS:" << it -> DPS() << ":");
			powerSum += it -> DPS();
			it -> Log();
		}
		float airPower = 0;

		for (list<int>::iterator it = airUnits.begin(); it != airUnits.end(); it++) {
			airPower += ai -> ut -> unittypearray[ai -> MyUnits[*it] -> def() -> id].AverageDPS;
			L("" << counter++ << ":" << ai -> MyUnits[*it] -> def() -> humanName);
		}
		L("airUnits: " << airUnits.size() << " units and dps:" << airPower);
		sum += airUnits.size();
		powerSum += airPower;
		L("total units: " << sum << " total dps:" << powerSum);
		L("in inactive group: " << units.size());
		L("in stuck group: " << stuckUnits.size());
		L("---------------------------------------------------------------------------");
	}
ai -> math -> StopTimer(ai -> ah -> ah_timer_totalTime);
ai -> math -> StopTimer(ai -> ah -> ah_timer_totalTimeMinusPather);
}
