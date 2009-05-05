#include <sstream>

#include "IncCREG.h"
#include "IncExternAI.h"
#include "IncGlobalAI.h"

#define K_MEANS_ELEVATION 40
#define IDLE_GROUP_ID 0
#define STUCK_GROUP_ID 1
#define AIR_GROUP_ID 2
#define GROUND_GROUP_ID_START 1000
#define SAFE_SPOT_DISTANCE 300
//#define SAFE_SPOT_DISTANCE_SLACK 700
#define KMEANS_ENEMY_MAX_K 32
#define KMEANS_BASE_MAX_K 32
#define KMEANS_MINIMUM_LINE_LENGTH (8 * THREATRES)



CR_BIND(CAttackHandler, (NULL));
CR_REG_METADATA(CAttackHandler, (
	CR_MEMBER(ai),

	CR_MEMBER(attackUnits),
	CR_MEMBER(stuckUnits),
	CR_MEMBER(unarmedAirUnits),
	CR_MEMBER(armedAirUnits),

	CR_MEMBER(airIsAttacking),
	CR_MEMBER(airPatrolOrdersGiven),
	CR_MEMBER(airTarget),

	CR_MEMBER(newGroupID),
	CR_MEMBER(attackGroups),
	// CR_MEMBER(defenseGroups),

	CR_MEMBER(kMeansBase),
	CR_MEMBER(kMeansK),
	CR_MEMBER(kMeansEnemyBase),
	CR_MEMBER(kMeansEnemyK),
	CR_RESERVED(16)
));



CAttackHandler::CAttackHandler(AIClasses* ai) {
	this->ai = ai;

	if (ai) {
		// setting a position to the middle of the map
		float mapWidth = ai->cb->GetMapWidth() * 8.0f;
		float mapHeight = ai->cb->GetMapHeight() * 8.0f;
		newGroupID = GROUND_GROUP_ID_START;

		this->kMeansK = 1;
		this->kMeansBase.push_back(float3(mapWidth / 2.0f, K_MEANS_ELEVATION, mapHeight / 2.0f));
		this->kMeansEnemyK = 1;
		this->kMeansEnemyBase.push_back(float3(mapWidth / 2.0f, K_MEANS_ELEVATION, mapHeight / 2.0f));

		UpdateKMeans();
	}

	airIsAttacking = false;
	airPatrolOrdersGiven = false;
	airTarget = -1;
}

CAttackHandler::~CAttackHandler(void) {
}


void CAttackHandler::AddUnit(int unitID) {
	if ((ai->MyUnits[unitID]->def())->canfly) {
		// the groupID of this "group" is 0, to separate them from other idle units
		ai->MyUnits[unitID]->groupID = AIR_GROUP_ID;
		// this might be a new unit with the same id as an older dead unit
		ai->MyUnits[unitID]->stuckCounter = 0;

		// do some checking then essentially add it to defense group
		if ((ai->MyUnits[unitID]->def())->weapons.size() == 0) {
			unarmedAirUnits.push_back(unitID);
		} else {
			armedAirUnits.push_back(unitID);
		}

		// patrol orders need to be updated
		airPatrolOrdersGiven = false;
	} else {
		// the groupID of this "group" is 0, to separate them from other idle units
		ai->MyUnits[unitID]->groupID = IDLE_GROUP_ID;
		// this might be a new unit with the same id as an older dead unit
		ai->MyUnits[unitID]->stuckCounter = 0;
		// do some checking then essentially add it to defense group
		attackUnits.push_back(unitID);
		// TODO: not that good
		this->PlaceIdleUnit(unitID);
	}
}


void CAttackHandler::UnitDestroyed(int unitID) {
	int attackGroupID = ai->MyUnits[unitID]->groupID;

	// if it's in the defense group
	if (attackGroupID == IDLE_GROUP_ID) {
		bool found_dead_unit_in_attackHandler = false;

		for (std::list<int>::iterator it = attackUnits.begin(); it != attackUnits.end(); it++) {
			if (*it == unitID) {
				attackUnits.erase(it);
				found_dead_unit_in_attackHandler = true;
				break;
			}
		}

		if (found_dead_unit_in_attackHandler) {
			// one of our (idle) attack units died but
			// we somehow have lost track of it before
			std::stringstream msg;
				msg << "[CAttackHandler::UnitDestroyed()] frame " << (ai->cb->GetCurrentFrame()) << "\n";
				msg << "\tidle attack unit " << unitID << " was destroyed but already erased\n";
			L(ai, msg.str());
		}
	}

	else if (attackGroupID >= GROUND_GROUP_ID_START) {
		// unit in an attack-group died
		bool foundGroup = false;
		bool removedDeadUnit = false;
		std::list<CAttackGroup>::iterator it;

		for (it = attackGroups.begin(); it != attackGroups.end(); it++) {
			if (it->GetGroupID() == attackGroupID) {
				removedDeadUnit = it->RemoveUnit(unitID);
				foundGroup = true;
				break;
			}
		}

		assert(foundGroup);
		assert(removedDeadUnit);

		// check if the group is now empty
		if (it->Size() == 0) {
			attackGroups.erase(it);
		}
	}

	else if (attackGroupID == AIR_GROUP_ID) {
		// unit in air-group died
		bool armed = true;
		std::list<int>::iterator unarmedAirIt = unarmedAirUnits.begin();
		std::list<int>::iterator armedAirIt = armedAirUnits.begin();

		for (; unarmedAirIt != unarmedAirUnits.end(); unarmedAirIt++) {
			if (unitID == *unarmedAirIt) {
				unarmedAirUnits.erase(unarmedAirIt);
				armed = false;
				break;
			}
		}
		if (armed) {
			for (; armedAirIt != armedAirUnits.end(); armedAirIt++) {
				if (unitID == *armedAirIt) {
					armedAirUnits.erase(armedAirIt);
					break;
				}
			}
		}
	}

	else {
		// unit in stuck-units group
		bool found_dead_in_stuck_units = false;
		std::list<std::pair<int, float3> >::iterator it;

		for (it = stuckUnits.begin(); it != stuckUnits.end(); it++) {
			if (it->first == unitID) {
				stuckUnits.erase(it);
				found_dead_in_stuck_units = true;
				break;
			}
		}
		assert(found_dead_in_stuck_units);
	}
}


bool CAttackHandler::PlaceIdleUnit(int unit) {
	if (ai->cb->GetUnitDef(unit) != NULL) {
		float3 moo = FindUnsafeArea(ai->cb->GetUnitPos(unit));

		if (moo != ZEROVECTOR && moo != ERRORVECTOR) {
            ai->MyUnits[unit]->Move(moo);
		}
	}

	return false;
}


// is it ok to build at CBS from this position?
bool CAttackHandler::IsSafeBuildSpot(float3 mypos) {
	// TODO: get a subset of the k-means, then
	// iterate the lines along the path that they
	// make with a min distance slightly lower than
	// the area radius definition

	mypos = mypos;
	return false;
}



// returns a safe spot from k-means, adjacent to myPos, safety params are (0..1).
// change to: decide on the random float 0...1 first, then find it (easier)
float3 CAttackHandler::FindSafeSpot(float3 myPos, float minSafety, float maxSafety) {
	// find a safe spot
	myPos = myPos;
	int startIndex = int(minSafety * this->kMeansK);
	if (startIndex < 0)
		startIndex = 0;
//	if (startIndex >= kMeansK)
//		startIndex = kMeansK - 1;

	int endIndex = int(maxSafety * this->kMeansK);
	if (endIndex < 0)
		startIndex = 0;
//	if (endIndex >= kMeansK)
//		endIndex = kMeansK - 1;
	if (startIndex > endIndex)
		startIndex = endIndex;

	if (kMeansK <= 1 || startIndex == endIndex) {
		if (startIndex >= kMeansK)
			startIndex = kMeansK - 1;

		float3 pos = kMeansBase[startIndex] + float3((RANDINT % SAFE_SPOT_DISTANCE), 0, (RANDINT % SAFE_SPOT_DISTANCE));
		pos.y = ai->cb->GetElevation(pos.x, pos.z);

		// SNPRINTF(logMsg, logMsg_maxSize, "AH::FSA1 minS: %3.2f, maxS: %3.2f,", minSafety, maxSafety);
		// PRINTF("%s", logMsg);
		return pos;
	}

	assert(startIndex < endIndex);
	assert(startIndex < kMeansK);
	assert(endIndex <= kMeansK);

	// get a subset of the k-means
	std::vector<float3> subset;

	for (int i = startIndex; i < endIndex; i++) {
		// subset[i] = kMeansBase[startIndex + i];
		assert(i < kMeansK);
		subset.push_back(kMeansBase[i]);
	}

	// then find a position on one of the lines between those points (pather)
	int whichPath;
	if (subset.size() > 1) {
		whichPath = (RANDINT % (int) subset.size());
	} else {
		whichPath = 0;
	}

	assert(whichPath < (int) subset.size());
	assert(subset.size() > 0);

	if ((whichPath + 1) < (int) subset.size() && subset[whichPath].distance2D(subset[whichPath + 1]) > KMEANS_MINIMUM_LINE_LENGTH) {
		std::vector<float3> posPath;

		//TODO: implement one in pathfinder without radius (or unit ID)
		//	if (size > (int)kMeansBase.size())
		//		size = kMeansBase.size();

		float dist = ai->pather->MakePath(posPath, subset[whichPath], subset[whichPath + 1], THREATRES * 8);
		float3 res;

		if (dist > 0) {
			// dist > 0 from path, use res from pather
			int whichPos = RANDINT % (int) posPath.size();
			res = posPath[whichPos];
		} else {
			// dist == 0 from path, using first point
			res = subset[whichPath];
		}

		/*
		SNPRINTF(logMsg, logMsg_maxSize,
				"AH::FSA-2 path:minS: %3.2f, maxS: %3.2f, pos:x: %f5.1 y: %f5.1 z: %f5.1",
				minSafety, maxSafety, res.x, res.y, res.z);
		PRINTF("%s", logMsg);
		*/
		return res;
	}
	else {
		assert(whichPath < (int) subset.size());
		float3 res = subset[whichPath];
		/*
		SNPRINTF(logMsg, logMsg_maxSize,
				"AH::FSA-3 minS: %f, maxS: %f, pos:x: %f y: %f z: %f",
				minSafety, maxSafety, res.x, res.y, res.z);
		PRINTF("%s", logMsg);
		*/
		return res;
	}
}


float3 CAttackHandler::FindSafeArea(float3 pos) {
	if (this->DistanceToBase(pos) < SAFE_SPOT_DISTANCE)
		return pos;

	float min = 0.6;
	float max = 0.95;
	float3 safe = this->FindSafeSpot(pos, min, max);
	// HACK
	safe += pos;
	safe /= 2;

	return safe;
}

float3 CAttackHandler::FindVerySafeArea(float3 pos) {
	float min = 0.9;
	float max = 1.0;
	return (this->FindSafeSpot(pos, min, max));
}

float3 CAttackHandler::FindUnsafeArea(float3 pos) {
	float min = 0.1;
	float max = 0.3;
	return (this->FindSafeSpot(pos, min, max));
}


float CAttackHandler::DistanceToBase(float3 pos) {
	float closestDistance = MY_FLT_MAX;

	for (int i = 0; i < this->kMeansK; i++) {
		float3 mean = this->kMeansBase[i];
		float distance = pos.distance2D(mean);
		closestDistance = std::min(distance, closestDistance);
	}

	return closestDistance;
}


float3 CAttackHandler::GetClosestBaseSpot(float3 pos) {
	float closestDistance = MY_FLT_MAX;
	int index = 0;

	for (int i = 0; i < this->kMeansK; i++) {
		float3 mean = this->kMeansBase[i];
		float distance = pos.distance2D(mean);

		if (distance < closestDistance) {
			closestDistance = distance;
			index = i;
		}
	}

	return kMeansBase[index];
}


std::vector<float3> CAttackHandler::KMeansIteration(std::vector<float3> means, std::vector<float3> unitPositions, int newK) {
	assert(newK > 0 && means.size() > 0);
	int numUnits = unitPositions.size();
	// change the number of means according to newK
	int oldK = means.size();
	means.resize(newK);
	// add a new means, just use one of the positions
	float3 newMeansPosition = unitPositions[0];
	newMeansPosition.y = ai->cb->GetElevation(newMeansPosition.x, newMeansPosition.z) + K_MEANS_ELEVATION;

	for (int i = oldK; i < newK; i++) {
		means[i] = newMeansPosition;
	}

	// check all positions and assign them to means, complexity n*k for one iteration
	std::vector<int> unitsClosestMeanID(numUnits, -1);
	std::vector<int> numUnitsAssignedToMean(newK, 0);

	for (int i = 0; i < numUnits; i++) {
		float3 unitPos = unitPositions.at(i);
		float closestDistance = MY_FLT_MAX;
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
	std::vector<float3> newMeans(newK, float3(0, 0, 0));

	for (int i = 0; i < numUnits; i++) {
		int meanIndex = unitsClosestMeanID[i];
		 // don't divide by 0
		float num = std::max(1, numUnitsAssignedToMean[meanIndex]);
		newMeans[meanIndex] += unitPositions[i] / num;
	}

	// do a check and see if there are any empty means and set the height
	for (int i = 0; i < newK; i++) {
		// if a newmean is unchanged, set it to the new means pos instead of (0, 0, 0)
		if (newMeans[i] == float3(0, 0, 0)) {
			newMeans[i] = newMeansPosition;
		}
		else {
			// get the proper elevation for the y-coord
			newMeans[i].y = ai->cb->GetElevation(newMeans[i].x, newMeans[i].z) + K_MEANS_ELEVATION;
		}
	}

	return newMeans;
}


void CAttackHandler::UpdateKMeans(void) {
	// we want local variable definitions
	{
		// get positions of all friendly units and put them in a vector (completed buildings only)
		std::vector<float3> friendlyPositions;
		int numFriendlies = ai->cb->GetFriendlyUnits(&ai->unitIDs[0]);

		for (int i = 0; i < numFriendlies; i++) {
			int unit = ai->unitIDs[i];
			CUNIT* u = ai->MyUnits[unit];
			// its a building, it has hp, and its mine (0)
			if (this->UnitBuildingFilter(u->def()) && this->UnitReadyFilter(unit) && u->owner() == 0) {
				friendlyPositions.push_back(u->pos());
			}
		}

		// hack to make it at least 1 unit, should only happen when you have no base
		if (friendlyPositions.size() < 1) {
			// it has to be a proper position, unless there are no proper positions
			if (numFriendlies > 0 && ai->cb->GetUnitDef(ai->unitIDs[0]) && ai->MyUnits[ai->unitIDs[0]]->owner() == 0) {
				friendlyPositions.push_back(ai->cb->GetUnitPos(ai->unitIDs[0]));
			}
			else {
			 	// when everything is dead
				friendlyPositions.push_back(float3(RANDINT % (ai->cb->GetMapWidth() * 8), 1000, RANDINT % (ai->cb->GetMapHeight() * 8)));
			}
		}

		// calculate a new K. change the formula to adjust max K, needs to be 1 minimum.
		this->kMeansK = int(std::min((float) (KMEANS_BASE_MAX_K), 1.0f + sqrtf((float) numFriendlies + 0.01f)));
		// iterate k-means algo over these positions and move the means
		this->kMeansBase = KMeansIteration(this->kMeansBase, friendlyPositions, this->kMeansK);
	}
	
	// update enemy position k-means
	// get positions of all enemy units and put them in a vector (completed buildings only)
	std::vector<float3> enemyPositions;
	int numEnemies = ai->ccb->GetEnemyUnits(&ai->unitIDs[0]);

	for (int i = 0; i < numEnemies; i++) {
		const UnitDef* ud = ai->ccb->GetUnitDef(ai->unitIDs[i]);

		if (this->UnitBuildingFilter(ud)) {
			// if (this->UnitReadyFilter(unit)) { ... }
			enemyPositions.push_back(ai->ccb->GetUnitPos(ai->unitIDs[i]));
		}
	}

	// hack to make it at least 1 unit, should only happen when you have no base
	if (enemyPositions.size() < 1) {
		// it has to be a proper position, unless there are no proper positions
		if (numEnemies > 0 && ai->ccb->GetUnitDef(ai->unitIDs[0])) {
			enemyPositions.push_back(ai->ccb->GetUnitPos(ai->unitIDs[0]));
		}
		else {
			// when everything is dead
			enemyPositions.push_back(float3(RANDINT % (ai->cb->GetMapWidth() * 8), 1000, RANDINT % (ai->cb->GetMapHeight() * 8)));
		}
	}

	// calculate a new K. change the formula to adjust max K, needs to be 1 minimum
	this->kMeansEnemyK = int(std::min(float(KMEANS_ENEMY_MAX_K), 1.0f + sqrtf((float) numEnemies + 0.01f)));

	// iterate k-means algo over these positions and move the means
	this->kMeansEnemyBase = KMeansIteration(this->kMeansEnemyBase, enemyPositions, this->kMeansEnemyK);

	// base k-means and enemy base k-means are updated
	// approach: add up (max - distance) to enemies
	std::vector<float> proximity(kMeansK, 0.0000001f);
	const float mapDiagonal = sqrt(pow((float) ai->cb->GetMapHeight() * 8,2) + pow((float) ai->cb->GetMapWidth() * 8, 2) + 1.0f);

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

	// now we have a kMeans list sorted by distance
	// to enemies, 0 being risky and k being safest
}


bool CAttackHandler::UnitGroundAttackFilter(int unit) {
	CUNIT u = *ai->MyUnits[unit];
	bool result = ((u.def() != NULL) && (u.def()->canmove) && (u.category() == CAT_G_ATTACK));
	return result;
}

bool CAttackHandler::UnitBuildingFilter(const UnitDef *ud) {
	bool result = ((ud != NULL) && (ud->speed <= 0));
	return result;
}

bool CAttackHandler::UnitReadyFilter(int unit) {
	CUNIT u = *ai->MyUnits[unit];
	bool result = ((u.def() != NULL) && (!ai->cb->UnitBeingBuilt(unit)) && ((ai->cb->GetUnitHealth(unit)) > (ai->cb->GetUnitMaxHealth(unit) * 0.8f)));
	return result;
}





void CAttackHandler::AirAttack(int currentFrame) {
	int numEnemies = ai->ccb->GetEnemyUnits(&ai->unitIDs[0]);
	int bestTargetID = -1;
	float bestTargetCost = -1.0f;

	for (int i = 0; i < numEnemies; i++) {
		int enemyID = ai->unitIDs[i];
		const UnitDef* udef = (enemyID >= 0)? ai->ccb->GetUnitDef(enemyID): 0;

		if (udef) {
			float mCost = udef->metalCost;
			float eCost = udef->energyCost;
			float baseCost = mCost + eCost * 0.1f;
			bool isStaticTarget = (udef->speed < 0.1f);
			float targetCost = isStaticTarget? baseCost: baseCost * 0.01f;

			if (targetCost > bestTargetCost) {
				bestTargetID = enemyID;
				bestTargetCost = targetCost;
			}
		}
	}

	if (bestTargetID != -1) {
		// attack en-masse, regardless of AA
		for (std::list<int>::iterator it = armedAirUnits.begin(); it != armedAirUnits.end(); it++) {
			CUNIT* u = ai->MyUnits[*it];
			u->Attack(bestTargetID);
		}

		airIsAttacking = true;
		airTarget = bestTargetID;
	}
}

void CAttackHandler::AirPatrol(int currentFrame) {
	// get and make up some outer base perimeter
	// points for air patrol route updates (if we
	// aren't attacking)
	std::vector<float3> outerMeans;
	const unsigned int numClusters = 3;
	outerMeans.reserve(numClusters);

	if (kMeansK > 1) {
		// offset the outermost one
		int counter = (kMeansK / 8);

		for (unsigned int i = 0; i < numClusters; i++) {
			outerMeans.push_back(kMeansBase[counter]);

			if (counter < kMeansK - 1)
				counter++;
		}
	} else {
		// there is just 1 k-means cluster and we need three
		for (unsigned int i = 0; i < numClusters; i++) {
			outerMeans.push_back(kMeansBase[0] + float3(250 * i, 0, 0));
		}
	}

	if (outerMeans.size() < numClusters) {
		// there were two kMeansK clusters?
		return;
	}

	for (std::list<int>::iterator it = armedAirUnits.begin(); it != armedAirUnits.end(); it++) {
		CUNIT* u = ai->MyUnits[*it];
		// do this first in case we are in the enemy base
		u->Move(outerMeans[0] + float3(0, 50, 0));

		for (unsigned int i = 0; i < outerMeans.size(); i++) {
			u->PatrolShift(outerMeans[i]);
		}
	}

	airPatrolOrdersGiven = true;
}


void CAttackHandler::UpdateAir(int currentFrame) {
	if (armedAirUnits.size() == 0)
		return;

	if (airIsAttacking) {
		if (airTarget == -1) {
			// we are attacking an invalid target
			airIsAttacking = false;
		} else {
			// if we are attacking but our target is dead
			if (ai->ccb->GetUnitHealth(airTarget) <= 0.0f) {
				airIsAttacking = false;
				airTarget = -1;
			}
		}
	}

	if (!airIsAttacking) {
		if (armedAirUnits.size() >= 16) {
			// start or continue attacking
			// if we have 16 or more armed
			// planes and no target
			AirAttack(currentFrame);
		} else {
			// return to base
			airIsAttacking = false;
			airTarget = -1;

			if (!airPatrolOrdersGiven) {
				AirPatrol(currentFrame);
			}
		}
	}


	if (currentFrame % 1800 == 0) {
		// clear patrol orders every 60 seconds
		airPatrolOrdersGiven = false;
	}

	if (!airPatrolOrdersGiven && !airIsAttacking) {
		AirPatrol(currentFrame);
	}
}

void CAttackHandler::UpdateSea(int currentFrame) {
	// TODO
}



void CAttackHandler::UpdateNukeSilos(int currentFrame) {
	if ((currentFrame % 300) == 0 && ai->uh->NukeSilos.size() > 0) {
		std::vector<std::pair<int, float> > potentialTargets;
		GetNukeSiloTargets(potentialTargets);

		for (std::list<NukeSilo>::iterator i = ai->uh->NukeSilos.begin(); i != ai->uh->NukeSilos.end(); i++) {
			NukeSilo* silo = &*i;

			if (silo->numNukesReady > 0) {
				int targetID = PickNukeSiloTarget(potentialTargets);

				if (targetID != -1) {
					ai->MyUnits[silo->id]->Attack(targetID);
				}
			}
		}
	}
}

// pick a nuke-silo target from a vector of potential ones
// (if there are more than MAX_NUKE_SILOS/2 targets to choose
// from, pick one of the first <MAX_NUKE_SILOS/2>, else pick
// from the full size of the vector)
int CAttackHandler::PickNukeSiloTarget(std::vector<std::pair<int, float> >& potentialTargets) {
	int s = potentialTargets.size();
	int n = ((s > (MAX_NUKE_SILOS >> 1))? (MAX_NUKE_SILOS >> 1): s);
	return ((s > 0)? potentialTargets[RANDINT % n].first: -1);
}


inline bool ComparePairs(const std::pair<int, float>& l, const std::pair<int, float>& r) {
	return (l.second > r.second);
}

// sort all enemy targets in decreasing order by unit value
void CAttackHandler::GetNukeSiloTargets(std::vector<std::pair<int, float> >& potentialTargets) {
	int numEnemies = ai->ccb->GetEnemyUnits(&ai->unitIDs[0]);
	float minTargetValue = 500.0f;

	std::vector<std::pair<int, float> > staticTargets;
	std::vector<std::pair<int, float> > mobileTargets;

	for (int i = 0; i < numEnemies; i++) {
		int targetID = ai->unitIDs[i];
		const UnitDef* udef = ai->ccb->GetUnitDef(targetID);

		if (udef) {
			float mCost = ai->ccb->GetUnitDef(targetID)->metalCost;
			float eCost = ai->ccb->GetUnitDef(targetID)->energyCost;
			float targetValue = mCost + eCost * 0.1f;
			bool isMobileTarget = (udef->speed > 0.0f);

			if (targetValue > minTargetValue) {
				// don't waste nukes on radar towers
				if (isMobileTarget) {
					mobileTargets.push_back(std::make_pair(targetID, targetValue));
				} else {
					staticTargets.push_back(std::make_pair(targetID, targetValue));
				}
			}
		}
	}

	std::sort(staticTargets.begin(), staticTargets.end(), &ComparePairs);
	std::sort(mobileTargets.begin(), mobileTargets.end(), &ComparePairs);

	// copy over all static targets
	for (unsigned int i = 0; i < staticTargets.size(); i++) {
		potentialTargets.push_back(staticTargets[i]);
	}

	// if there weren't any static targets
	// then copy over all the mobile ones
	if (staticTargets.size() == 0) {
		for (unsigned int i = 0; i < mobileTargets.size(); i++) {
			potentialTargets.push_back(mobileTargets[i]);
		}
	}
}






void CAttackHandler::AssignTarget(CAttackGroup* group_in) {
	int numEnemies = ai->ccb->GetEnemyUnits(&ai->unitIDs[0]);

	if (numEnemies > 0) {
		std::vector<int> allEligibleEnemies;
		allEligibleEnemies.reserve(numEnemies);

		// make a vector with the positions of all
		// non-air and non-cloaked (non-dead) enemies
		for (int i = 0; i < numEnemies; i++) {
			if (ai->unitIDs[i] != -1) {
				const UnitDef* ud = ai->ccb->GetUnitDef(ai->unitIDs[i]);

				if (ud) {
					bool canFly = ud->canfly;
					bool isCloaked = ud->canCloak && ud->startCloaked;
					bool goodPos = !(ai->ccb->GetUnitPos(ai->unitIDs[i]) == ZEROVECTOR);

					if (!canFly && !isCloaked && goodPos) {
						allEligibleEnemies.push_back(ai->unitIDs[i]);
					}
				}
			}
		}

		std::vector<int> availableEnemies;
		std::vector<float3> enemyPositions;
		availableEnemies.reserve(allEligibleEnemies.size());

		// make a list of all enemies already assigned to (non-defending) groups
		std::list<int> takenEnemies;
		for (std::list<CAttackGroup>::iterator groupIt = attackGroups.begin(); groupIt != attackGroups.end(); groupIt++) {
			if ((!groupIt->defending) && (groupIt->GetGroupID() != group_in->GetGroupID())) {
				std::list<int> assignedEnemies = groupIt->GetAssignedEnemies();
				takenEnemies.merge(assignedEnemies);
			}
		}

		// filter out assigned enemies from eligible enemies
		for (std::vector<int>::iterator enemy = allEligibleEnemies.begin(); enemy != allEligibleEnemies.end(); enemy++) {
			int enemyID = *enemy;
			bool taken = false;

			for (std::list<int>::iterator it = takenEnemies.begin(); it != takenEnemies.end(); it++) {
				if (*it == enemyID) {
					taken = true;
					break;
				}
			}

			if (!taken) {
				availableEnemies.push_back(enemyID);
				enemyPositions.push_back(ai->ccb->GetUnitPos(enemyID));
			}
		}

		if (availableEnemies.size() == 0) {
			return;
		}

		// find cheapest (best) target for this group
		std::vector<float3> pathToTarget;
		float3 groupPos = group_in->GetGroupPos();

		ai->pather->micropather->SetMapData(ai->pather->MoveArrays[group_in->GetWorstMoveType()],
											&ai->tm->ThreatArray.front(),
											ai->tm->ThreatMapWidth,
											ai->tm->ThreatMapHeight);

		// pick an enemy position and path to it
		// KLOOTNOTE: should be more like KAI 0.23 by passing group DPS to FindBestPath()
		ai->pather->FindBestPath(pathToTarget, groupPos, THREATRES * 8, enemyPositions);

		if (pathToTarget.size() > 2) {
			const int ATTACKED_AREA_RADIUS = 800;
			int lastIndex = pathToTarget.size() - 1;
			float3 endPos = pathToTarget[lastIndex];

			// get all enemies surrounding endpoint of found path
			int enemiesInArea = ai->ccb->GetEnemyUnits(&ai->unitIDs[0], endPos, ATTACKED_AREA_RADIUS);
			float powerOfEnemies = 0.000001;

			// calculate combined "firepower" of armed enemies near endpoint
			for (int i = 0; i < enemiesInArea; i++) {
				if (ai->ccb->GetUnitDef(ai->unitIDs[i])->weapons.size() > 0) {
					powerOfEnemies += ai->ccb->GetUnitPower(ai->unitIDs[i]);
				}
			}

			if ((enemiesInArea > 0) && group_in->Size() >= 4 && (group_in->Power() > powerOfEnemies * 1.25f)) {
				// assign target to this group
				group_in->AssignTarget(pathToTarget, pathToTarget.back(), ATTACKED_AREA_RADIUS);
			} else {
				// group too weak, forget about this target
				group_in->ClearTarget();
			}
		}
	}
}


void CAttackHandler::AssignTargets(int frameNr) {
	if (frameNr % 120 == 0) {
		// for each attack-group check whether it needs new target, if so assign one
		for (std::list<CAttackGroup>::iterator it = attackGroups.begin(); it != attackGroups.end(); it++) {
			CAttackGroup* group = &(*it);
			// force group target updates every 300 frames
			if (group->NeedsNewTarget() || frameNr % 300 == 0) {
				AssignTarget(group);
			}
		}
	}
}



void CAttackHandler::CombineGroups(void) {
	bool removedSomething = false;

	// pick a group A
	for (std::list<CAttackGroup>::iterator groupA = attackGroups.begin(); groupA != attackGroups.end(); groupA++) {
		// if it is defending
		if (groupA->defending) {
			int groupAid = groupA->GetGroupID();
			float3 groupApos = groupA->GetGroupPos();
			// look for other groups that are defending
			for (std::list<CAttackGroup>::iterator groupB = attackGroups.begin(); groupB != attackGroups.end(); groupB++) {
				// if they are close, combine
				float3 groupBpos = groupB->GetGroupPos();
				int groupBid = groupB->GetGroupID();

				if ((groupB->defending) && (groupAid != groupBid) && (groupApos.distance2D(groupBpos) < 1500)) {
					std::vector<int>* bUnits = groupB->GetAllUnits();

					for (std::vector<int>::iterator groupBunit = bUnits->begin(); groupBunit != bUnits->end(); groupBunit++) {
						groupA->AddUnit(*groupBunit);
					}

					this->attackGroups.erase(groupB);
					removedSomething = true;
					break;
				}
			}
		}

		if (removedSomething)
			break;
	}
}




void CAttackHandler::Update(int frameNr) {
	int frameSpread = 300;

	if (frameNr < 2)
		UpdateKMeans();

	// set map data here so it doesn't have to be done
	// in each group (movement map PATHTOUSE is hack)
	ai->pather->micropather->SetMapData(ai->pather->MoveArrays[PATHTOUSE], &ai->tm->ThreatArray.front(), ai->tm->ThreatMapWidth, ai->tm->ThreatMapHeight);

	// calculate and draw k-means for the base perimeters every 10 seconds
	if (frameNr % frameSpread == 0) {
		UpdateKMeans();

		int num = ai->uh->NumIdleUnits(CAT_G_ATTACK);

		for (int i = 0; i < num; i++) {
			int unit = ai->uh->GetIU(CAT_G_ATTACK);

			if (PlaceIdleUnit(unit) && !ai->cb->GetUnitDef(unit)->canfly)  {
				ai->uh->IdleUnitRemove(unit);
			}
		}
	}

	// check for stuck units in each attack group every second
	if (frameNr % 30 == 0) {
		for (std::list<CAttackGroup>::iterator it = attackGroups.begin(); it != attackGroups.end(); it++) {
			int stuckUnit = it->PopStuckUnit();

			if (stuckUnit != -1 && ai->cb->GetUnitDef(stuckUnit) != NULL) {
				std::pair<int, float3> foo;
					foo.first = stuckUnit;
					foo.second = ai->cb->GetUnitPos(stuckUnit);
				stuckUnits.push_back(foo);
				// popped a stuck unit from attack group it->GetGroupID()
				ai->MyUnits[stuckUnit]->Stop();
				ai->MyUnits[stuckUnit]->groupID = STUCK_GROUP_ID;
			}

			// if attack group now empty then kill it
			if (it->Size() == 0) {
				attackGroups.erase(it);
				break;
			}
		}
	}

	// combine groups that are defending and too weak to attack anything
	if (frameNr % frameSpread == 0) {
		CombineGroups();
	}

	// check if we have any new units, add them to a
	// nearby defending group of less than 16 units
	if (frameNr % 30 == 0 && attackUnits.size() > 0) {
		CAttackGroup* existingGroup = NULL;
		for (std::list<CAttackGroup>::iterator it = attackGroups.begin(); it != attackGroups.end(); it++) {
			if (it->Size() < 16 && it->defending && this->DistanceToBase(it->GetGroupPos()) < 300) {
				// KLOOTNOTE: pick the first valid group, not the last
				existingGroup = &(*it);
				break;
			}
		}

		if (existingGroup != NULL) {
			// add all new units to found group
			for (std::list<int>::iterator it = attackUnits.begin(); it != attackUnits.end(); it++) {
				int unit = *it;

				if (ai->cb->GetUnitDef(unit) != NULL) {
					existingGroup->AddUnit(unit);
				}
			}

			attackUnits.clear();
		}
		else {
			// no suitable group found, make new defending one
			newGroupID++;
			CAttackGroup newGroup(ai, newGroupID);
			newGroup.defending = true;

			for (std::list<int>::iterator it = attackUnits.begin(); it != attackUnits.end(); it++) {
				int unit = *it;
				if (ai->cb->GetUnitDef(unit) != NULL) {
					newGroup.AddUnit(unit);
				}
			}

			attackUnits.clear();
			attackGroups.push_back(newGroup);
		}
	}


	// do basic attack group formation from defense units
	UpdateAir(frameNr);
	UpdateSea(frameNr);
	UpdateNukeSilos(frameNr);
	AssignTargets(frameNr);

	// update current groups
	for (std::list<CAttackGroup>::iterator it = attackGroups.begin(); it != attackGroups.end(); it++) {
		it->Update(frameNr);
	}
}
