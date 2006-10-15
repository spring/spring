#include "AttackGroup.h"

#define UNIT_STUCK_MOVE_DISTANCE 2.0f
//not moving for 5*60 frames = stuck :
#define UNIT_STUCK_COUNTER_MANEUVER_LIMIT 5
//gives it (STUCK_COUNTER_LIMIT - STUCK_COUNTER_MANEUVER_LIMIT)*60 seconds
// to move longer than UNIT_STUCK_MOVE_DISTANCE (7 sec)
#define UNIT_STUCK_COUNTER_LIMIT 15
//stuck maneuver distance should be the movement map res since threat isnt really relevant when maneuvering from a stuck pos
#define UNIT_STUCK_MANEUVER_DISTANCE (THREATRES*8)
//the max amount of difference in height there may be at the position to maneuver to (dont retreat into a hole)
#define UNIT_MAX_MANEUVER_HEIGHT_DIFFERENCE_UP 20
//minimum of offset between my maxrange and the enemys position before considering moving
#define UNIT_MIN_MANEUVER_RANGE_DELTA (THREATRES*8)
//minimum amount to maneuver when getting to max range
#define UNIT_MIN_MANEUVER_DISTANCE (THREATRES*4)
//second requirement, minimum percentage of my range to move when getting to max range (to stop slow long range from dancing around)
#define UNIT_MIN_MANEUVER_RANGE_PERCENTAGE 0.2f
//minimum time to maneuver (frames), used for setting maneuvercounter (in case the speed/dist formula is weird)
#define UNIT_MIN_MANEUVER_TIME 15

#define UNIT_DESTINATION_SLACK (THREATRES*4.0f*1.4f)

#define GROUP_DESTINATION_SLACK THREATRES*8

#define GROUP_MEDIAN_UNIT_SELECTION_SLACK 10.0f

CAttackGroup::CAttackGroup(AIClasses* ai, int groupID_in)
{
	this->ai=ai;
	this->groupID = groupID_in;
//	this->myEnemy = -1;
	this->pathIterator = 0;
	this->lowestAttackRange = 100000;
	this->highestAttackRange = 1;
	this->movementCounterForStuckChecking = 0;
	this->defending = false;
	this->isMoving = false;
	//L("AG: constructor sets isMoving to false.");
	this->isShooting = false;
	this->attackPosition = ZEROVECTOR;
	this->attackRadius = 1;
}

CAttackGroup::~CAttackGroup()
{
	
}

void CAttackGroup::Log() {
	int temp = 0;
	//L("AG: logging contents of group " << groupID << ":");
	for (vector<int>::iterator it = units.begin(); it != units.end(); it++) {
		temp++;
		if (ai->cb->GetUnitDef(*it)) {
			//L("" << temp << ": " << *it << " type:" << ai->cb->GetUnitDef(*it)->humanName);
		} else {
			//L("" << temp << ": " << *it << " ILLEGAL UNIT - has no unit def (wtf?)");
		}
	}
}

void CAttackGroup::AddUnit(int unitID)
{
	if (ai->cb->GetUnitDef(unitID)) {
		//add to my structure
		units.push_back(unitID);
		//set its group ID:
		ai->MyUnits[unitID]->groupID = this->groupID;
		//update the attack range properties of this group
		this->lowestAttackRange = min(this->lowestAttackRange, this->ai->ut->GetMaxRange(ai->cb->GetUnitDef(unitID)));
		this->highestAttackRange = max(this->highestAttackRange, this->ai->ut->GetMaxRange(ai->cb->GetUnitDef(unitID)));
	} else {
		bool dead_unit_added_to_group = false;
		assert(dead_unit_added_to_group);
	}
}

bool CAttackGroup::RemoveUnit(int unitID) {
	bool found = false;
	vector<int>::iterator it;
	for (it = units.begin(); it != units.end(); it++) {
		if (*it == unitID) {
			found = true;
			//L("AttackGroup: erasing unit with id:" << unitID);
			break;
		}
	}
	if (found) {
		units.erase(it);
//		//L("AttackGroup: about to attempt to reset the group ID of a removed unit");
		if( ai->cb->GetUnitDef(unitID) != NULL) {
			ai->MyUnits[unitID]->groupID = 0;
			//L("AttackGroup: groupid = 0 --> success");
//			//L("AttackGroup: by the way, this unit's stuck counter was " << ai->MyUnits[unitID]->stuckCounter);
		}
	}
	assert(found);
	//update lowestAttackRange and highestAttackRange
	this->lowestAttackRange = 10000.0f;
	this->highestAttackRange = 1.0f;
	for (int i = 0; i < (int)units.size(); i++) {
		int unitID = units[i];
		if (ai->cb->GetUnitDef(unitID) != NULL) {
			this->lowestAttackRange = min(this->lowestAttackRange, this->ai->ut->GetMaxRange(ai->cb->GetUnitDef(unitID)));
			this->highestAttackRange = max(this->highestAttackRange, this->ai->ut->GetMaxRange(ai->cb->GetUnitDef(unitID)));
		}
	}
	return found;
}

void CAttackGroup::MoveTo(float3 newPosition)
{
	//atm i dont think this will be implemented
}

int CAttackGroup::Size()
{
//LONG DEAD-UNIT TEST:
	int unitCounter = 0;
	int numUnits = units.size();
	int invalid = -2;
	for (int i = 0; i < numUnits; i++) {
		int unit = units[i];
		//assert(ai->cb->GetUnitDef(unit) != NULL);
		if (ai->cb->GetUnitDef(unit) != NULL) {
			unitCounter++;
		} else {
			invalid = unit;
		}
	}
	if (numUnits != unitCounter) {
		//L("AttackGroup:Size doublecheck error, size mismatch in group:" << groupID << " .size()="<<numUnits<<" units with unitdef:" << unitCounter << " last invalid unit:" << invalid);
	}
	return units.size();
}

int CAttackGroup::GetGroupID() {
	return groupID;
}

int CAttackGroup::GetWorstMoveType() {
	return PATHTOUSE;
}

vector<int>* CAttackGroup::GetAllUnits() {
	return &this->units;
}

//combined unit power of the group
float CAttackGroup::Power() {
	float sum = 0.00001f;
	for (vector<int>::iterator it = units.begin(); it != units.end(); it++) {
		if (ai->cb->GetUnitDef(*it) != NULL) {
			sum += ai->cb->GetUnitPower(*it);
		}
	}
	return sum;
}

int CAttackGroup::PopStuckUnit() {
	//removes a stuck unit from the group if there is one, and puts a marker on the map
	for (vector<int>::iterator it = units.begin(); it != units.end(); it++) {
		if (ai->MyUnits[*it]->stuckCounter > UNIT_STUCK_COUNTER_LIMIT) {
			int id = *it;
			//mark it:
			char text[512];
			sprintf(text, "stuck %i:%i, dropping from group:%i. isMoving=%i", id, ai->MyUnits[*it]->stuckCounter, groupID, isMoving);
			AIHCAddMapPoint amp;
			amp.label = text;
			amp.pos = ai->cb->GetUnitPos(id);
			//ai->cb->HandleCommand(&amp);


			//sprintf(c,"Made unit %s",  ai->cb->GetUnitDef(unit)->humanName.c_str());

			//L("AttackGroup debug: about to do crap that i think will crash (popstuckunit amp)");
			sprintf(text, "humanName:%s", ai->MyUnits[*it]->def()->humanName.c_str());
			amp.label = text;
			amp.pos = ai->cb->GetUnitPos(id) + float3(0, 0, 30);
			//ai->cb->HandleCommand(&amp);

			ai->MyUnits[*it]->stuckCounter = 0;
			units.erase(it);
			return id;
		}
	}
	return -1;
}

bool CAttackGroup::CloakedFix(int enemy) {
	const UnitDef * ud = ai->cheat->GetUnitDef(enemy);
//	ai->cb->GetLosMap(L
	return ud != NULL && !(ud->canCloak && ud->startCloaked && (ai->cb->GetUnitPos(enemy) == ZEROVECTOR));
//	return ud != NULL && !(ud->canCloak && ud->startCloaked && (ai->cb->GetUnitDef(enemy) == NULL));
//	return ud != NULL && !(ud->canCloak && (ai->cb->GetUnitDef(enemy) == NULL));
}
float3 CAttackGroup::GetGroupPos() {
		//whats the groups position (for distance checking when selecting targets)
	int unitCounter = 0;
	float3 groupPosition = float3(0,0,0);
	int numUnits = units.size();
	for (int i = 0; i < numUnits; i++) {
		int unit = units[i];
		if (ai->cb->GetUnitDef(unit) != NULL) {
			unitCounter++;
			groupPosition += ai->cb->GetUnitPos(unit);
		}
		else {
			//if (this->RemoveUnit(unit)) {
			//	//L("AttackGroup: internally removed unit " << unit);
            //    i--;
			//} else {
			//	//L("case of illegal unit in attackgroup. unitid:" << unit << " had no position. (groupPos calc).");
			//}
		}
	}
	if (unitCounter > 0) {
		groupPosition /= unitCounter;
		//find the unit closest to the center (since the actual center might be on a hill or something)
		float closestSoFar = FLT_MAX;
		int closestUnitID = -1;
		float temp;
		int unit;
		for (int i = 0; i < numUnits; i++) {
			unit = units[i];
			//is it closer. consider also low unit counts, then the first will be used since its < and not <=, assuming sufficient float accuracy
			if (ai->cb->GetUnitDef(unit) != NULL && (temp = groupPosition.distance2D(ai->cb->GetUnitPos(unit))) < closestSoFar-GROUP_MEDIAN_UNIT_SELECTION_SLACK) {
				closestSoFar = temp;
				closestUnitID = unit;
			}
		}
		assert(closestUnitID != -1);
		groupPosition = ai->cb->GetUnitPos(closestUnitID);
		//draw arrow pointing to the group center:
		if (!defending) {
			//ai->cb->CreateLineFigure(groupPosition + float3(0,140,0), groupPosition + float3(0,20,0),48,1,15,groupID);
			ai->cb->SetFigureColor(groupID, 65535.0f, 0.1f, FLT_MAX, 1); //hack to change def group color
		} else {
			//ai->cb->CreateLineFigure(groupPosition + float3(0,80,0), groupPosition + float3(0,20,0),96,1,15,groupID);
			//ai->cb->SetFigureColor(groupID, 1.0f, 1.0f, 1.0f, 1);
		}
	} else {
		//L("AttackGroup: empty attack group when calcing group pos!");
		return ERRORVECTOR; //<-----------------
	}
	return groupPosition;
}

//returns enemies in my attack area
list<int> CAttackGroup::GetAssignedEnemies() {
	list<int> takenEnemies;
	if (!defending) {
		int numTaken = ai->cheat->GetEnemyUnits(unitArray, attackPosition, attackRadius);
		for(int i = 0; i < numTaken; i++) {
			int takenEnemy = unitArray[i];
			takenEnemies.push_back(unitArray[i]);
		}
	}
	return takenEnemies;
}

void CAttackGroup::AssignTarget(vector<float3> path, float3 position, float radius) {
	this->attackPosition = position;
	this->attackRadius = radius;
	this->pathToTarget = path;
	this->isMoving = true;
	this->isShooting = false; //TODO: check if this is necessary
	this->pathIterator = 0;
	this->defending = false;

	//L("AG: drawing the assigned path to the assigned target:");
	ai->cb->DeleteFigureGroup(groupID+100);
	for (int i = 1; i < (int)pathToTarget.size(); i++) {
		//ai->cb->CreateLineFigure(pathToTarget[i-1] + float3(0,50,0), pathToTarget[i] + float3(0,50,0), 8, 0, 150, (groupID+100));
	}
	//L("AG: target assigning complete. path size: " << pathToTarget.size() << " targetx:" << attackPosition.x << " targety:" << attackPosition.y << " radius:" << attackRadius << " isMoving:" << isMoving << " frame:" << ai->cb->GetCurrentFrame() << " groupID:" << groupID);
}

void CAttackGroup::FindDefenseTarget(float3 groupPosition) {

//	ai->cb->SendTextMsg("AG: FindDefenseTarget - group is defending and selecting a target.", 0);

	int frameNr = ai->cb->GetCurrentFrame();
	char tx[500];
	sprintf(tx, "AG:running find-new-target, group %i, frame %i, numUnits %i", this->groupID, frameNr, this->units.size());
//		ai->cb->SendTextMsg(tx,0);
	//L(tx);
	//the attack routine
	//the "find new enemy" part
	//int enemies[MAXUNITS];
	int numOfEnemies;// = ai->cheat->GetEnemyUnits(unitArray);
//	if (defending) { //in LOS and radar only
//		numOfEnemies = ai->cb->GetEnemyUnits(unitArray);
//	} else {
	numOfEnemies = ai->cb->GetEnemyUnitsInRadarAndLos(unitArray);
//	}
	//TODO: dont re-make the enemies list every time
	if (numOfEnemies) {
		//build vector of enemies
		vector<float3> enemyPositions;
//			enemyPositions.clear();
		enemyPositions.reserve(numOfEnemies);
		//make a vector with the positions of all enemies
		for (int i = 0; i < numOfEnemies; i++) {
			if( unitArray[i] != -1) {
				const UnitDef *enemy_ud = ai->cheat->GetUnitDef(unitArray[i]);
				float3 enemyPos = ai->cheat->GetUnitPos(unitArray[i]);
				//do some filtering and then
				if (ai->cb->GetUnitDef(unitArray[i]) != NULL
						&& this->CloakedFix(unitArray[i])
						&& !enemy_ud->canfly) { // || !enemy_ud->canmove) { //all targets are eligible atm
					//TODO: remove currently cloaked units, 
					//remove units not reachable by my unit type and position (how? - the pathfinder does it itself but the radius based search thing is problematic)
					enemyPositions.push_back(enemyPos);
					////L("added enemy position to the list being sent to path finder x"<<enemyPos.x<<" y"<<enemyPos.y<<" z"<<enemyPos.z<<" ");
				}
			}
		}
		// if all units are cloaked or otherwise not eligible (but there are units)
		if (enemyPositions.size() < 1) {
			for (int i = 0; i < numOfEnemies; i++) {
				if( unitArray[i] != -1) {
					const UnitDef *enemy_ud = ai->cheat->GetUnitDef(unitArray[i]);
					float3 enemyPos = ai->cheat->GetUnitPos(unitArray[i]);
					enemyPositions.push_back(enemyPos);
				}
			}
		}

		pathToTarget.clear();
		float costToTarget = ai->pather->FindBestPath(&pathToTarget, &groupPosition, lowestAttackRange, &enemyPositions);
		//draw the path
//		for (int i = 1; i < (int)pathToTarget.size(); i++) {
//				//ai->cb->CreateLineFigure(pathToTarget[i-1] + float3(0,50,0), pathToTarget[i] + float3(0,50,0), 8, 0, frameSpread, (groupID+100));
//		}

//		//L("AttackGroup debug: findbestpath run to all enemies, resulting cost:" << costToTarget);

		//assert(costToTarget > 0);
		if (costToTarget == 0 && pathToTarget.size() <= 2) {
			//myEnemy = -1;
			isMoving = false;
			//L("AG: FindNewTarget sets isMoving to false.");
			//if this happens then isshooting will take care of it (there are enemies but cost is 0 = something is in range)
		} else {
			isMoving = true;
			this->pathIterator = 0;
		}
	} else { //endif there are enemies
		//attempt to path back to base if there are no targets
		//ai->cb->SendTextMsg("AG: group retreating to base", 0);
		//L("AG: found no defense targets, pathing back to base. group:" << groupID);
        pathToTarget.clear();
		float costToTarget = ai->pather->FindBestPathToRadius(&pathToTarget, &groupPosition, THREATRES*8, &ai->ah->GetClosestBaseSpot(groupPosition)); //TODO: GetKBaseMeans() for support of multiple islands/movetypes //TODO: this doesnt need to be to radius >_>
		if (costToTarget == 0 && pathToTarget.size() <= 2) {
			isMoving = false;
			if(ai->ah->DistanceToBase(groupPosition) > 500){
				//L("AG: group " << groupID << " couldnt path back to closest base spot!");
			}
		} else {
			isMoving = true;
			this->pathIterator = 0;
		}
	}
	if (!isShooting && !isMoving) {
		//ai->cb->SendTextMsg("AttackGroup couldnt path to my enemy or have no enemy!", 0);
		//L("AttackGroup: There are no accessible enemies and we're idling. groupid:" << this->groupID << " we are defending: " << this->defending);
	}
}

bool CAttackGroup::NeedsNewTarget() {
	bool needs = false;
//	int frameSpread = 3000;
//	int frameNr = ai->cb->GetCurrentFrame();
	if (!isShooting && !isMoving) {
		needs = true;
	}
	return defending && !isShooting && !isMoving;
}

void CAttackGroup::ClearTarget() {
	this->isMoving = false;
	this->defending = true;
	this->attackPosition = ZEROVECTOR;
	this->attackRadius = 0.0f;
	this->pathToTarget.clear();
	this->isShooting = true; //to avoid getting a new target the first frame after arrival.
}

void CAttackGroup::Update()
{
	int frameNr = ai->cb->GetCurrentFrame();

	////L("AG: Update-start. isShooting:" << isShooting << " isMoving:" << isMoving << " frame:" << frameNr << " groupID:" << groupID << " path size:" << pathToTarget.size());

	int numUnits = (int)units.size();
	if(!numUnits) {
		//L("updated an empty AttackGroup!");
		return;
	}
	int frameSpread; // variable used as a varying "constant" for determining how often a part of the update scheme is done

	float3 groupPosition = this->GetGroupPos();
	if(groupPosition == ERRORVECTOR) return;

	//debug line:
	//	//ai->cb->CreateLineFigure(groupPosition + float3(0,100,50), groupPosition + float3(5+(unitCounter*20),100,50),20,0,2,groupID+1500);

	//sets the isShooting variable. - this part of the code checks for nearby enemies and does focus fire/maneuvering
	frameSpread = 30; 
	if (/*myEnemy == -1 && */frameNr % frameSpread == (groupID*4) % frameSpread) {
//		//L("AG: isShooting start");
		this->isShooting = false;
//		int enemies[MAXUNITS];
		//get all enemies within attack range:
		float range = this->highestAttackRange + 100.0f;//(this->highestAttackRange+200.0f)*2.0f;
		int numEnemies = ai->cheat->GetEnemyUnits(unitArray, groupPosition, range);
//		//L("this is the isShooting setting procedure, numEnemies=" << numEnemies << " range checked: " << range << " and the position of the group is (" << groupPosition.x << " " << groupPosition.y << " " << groupPosition.z << ") numUnits:" << numUnits);
		//assert(numEnemies > 0);
		if(numEnemies > 0) {
			////L("this is the isShooting setting procedure, numEnemies=" << numEnemies << " range checked: " << range << " and the position of the group is (" << groupPosition.x << " " << groupPosition.y << " " << groupPosition.z << ") numUnits:" << numUnits);
//			//L("this is the isShooting setting and numEnemies was more than 0 : " << numEnemies);
			//ai->cb->SendTextMsg("attackgroup : Im not sure that this happens, ever", 0);
			//selecting one of the enemies
			int enemySelected = -1;
			float shortestDistanceFound = FLT_MAX;
			float temp;
			//bool closestSoFarIsBuilding = false; shoot units first
			//float3 enemyPos;// = ai->cheat->GetUnitPos(enemies[enemySelected]);
			int xSize = 40;
			int lineGroupID = groupID+5000;
//			int heightOffset = 10;
			for (int i = 0; i < numEnemies; i++) {
				//my range not considered in picking the closest one
				//TODO: is it air? is it cloaked? 
				if (((temp = groupPosition.distance2D(ai->cheat->GetUnitPos(unitArray[i]))) < shortestDistanceFound)
						&& ai->cheat->GetUnitDef(unitArray[i]) != NULL 
						&& CloakedFix(unitArray[i])
						&& !ai->cheat->GetUnitDef(unitArray[i])->canfly) {
					enemySelected = i;
					shortestDistanceFound = temp;
				}
//				enemyPos = ai->cheat->GetUnitPos(unitArray[i]);
//				//ai->cb->CreateLineFigure(enemyPos + float3(-xSize,heightOffset,0), enemyPos + float3(xSize,heightOffset,0), 5, 0, frameSpread/2, 6000+(groupID*6000 + i));
//				//ai->cb->CreateLineFigure(enemyPos + float3(0,heightOffset,-xSize), enemyPos + float3(0,heightOffset,xSize), 5, 0, frameSpread/2, 6000+(groupID*6000 + numEnemies + i));
			}
//			enemyPos = ai->cheat->GetUnitPos(unitArray[enemySelected]);
			//X marks the target
//			//ai->cb->CreateLineFigure(enemyPos + float3(-xSize,heightOffset,-xSize), enemyPos + float3(xSize,heightOffset,xSize), 5, 0, frameSpread, lineGroupID);
//			//ai->cb->CreateLineFigure(enemyPos + float3(-xSize,heightOffset,xSize), enemyPos + float3(xSize,heightOffset,-xSize), 5, 0, frameSpread, lineGroupID);
			//tiny line from there to groupPosition
//			//ai->cb->CreateLineFigure(enemyPos, groupPosition, 5, 0, frameSpread, lineGroupID);

			//for every unit, pathfind to enemy perifery(with personal range-10) then if distance to that last point in the path is < 10 or cost is 0, attack
			// hack: just attack it
			////L("index of the best enemy: " << enemySelected << " its distance: " << shortestDistanceFound << " its id:" << enemies[enemySelected] << " humanName:" << ai->cheat->GetUnitDef(enemies[enemySelected])->humanName);
			//TODO: can enemy id be 0
			//GIVING ATTACK ORDER:
			if (enemySelected != -1) {
				//L("AG:isShooting: YES");
				float3 enemyPos = ai->cheat->GetUnitPos(unitArray[enemySelected]);
				assert (CloakedFix(unitArray[enemySelected]));
				this->isShooting = true;
				for (int i = 0; i < numUnits; i++) {
					int unit = units[i];
					//does our unit exist and its not currently maneuvering	
					if (ai->cb->GetUnitDef(unit) != NULL && ai->MyUnits[unit]->maneuverCounter-- <= 0) {
						//add a routine finding the best(not just closest) target, but for now just fire away
						//TODO: add canattack
						ai->MyUnits[unit]->Attack(unitArray[enemySelected]); //TODO: some cases, force fire on position
						
						//assert(!ai->cheat->GetUnitDef(unitArray[enemySelected])->canfly);

						//TODO: this should be the max range of the weapon the unit has with the lowest range
						//assuming you want to rush in with the heavy stuff, that is
						assert(range >= ai->cb->GetUnitMaxRange(unit));

						//SINGLE UNIT MANEUVERING:
						//testing the possibility of retreating to max range if target is too close
						float3 myPos = ai->cb->GetUnitPos(unit);
						float myRange = this->ai->ut->GetMaxRange(ai->cb->GetUnitDef(unit));
						//is it air, or air thats landed
						if(!ai->cb->GetUnitDef(unit)->canfly || myPos.y < ai->cb->GetElevation(myPos.x, myPos.z) + 25 && (myRange - UNIT_MIN_MANEUVER_RANGE_DELTA) > myPos.distance2D(enemyPos)) {
							bool debug1 = true;
							bool debug2 = false;
//							ai->cb->SendTextMsg("range diff makes it good to maneuver && im not air", 0);
							vector<float3> tempPath;
							//two things: 1. dont need a path, just a position and 2. it should avoid other immediate friendly units and/or immediate enemy units+radius 
							//needs to be a point ON the perifery circle, not INSIDE it - fixed
							//maybe include the height parameter in the search? probably not possible
							//TODO: OBS doesnt this mean pathing might happen every second? outer limit should be more harsh than inner
							float dist = ai->pather->FindBestPathToRadius(&tempPath, &myPos, myRange, &ai->cheat->GetUnitPos(unitArray[enemySelected]));
							if (tempPath.size() > 0) {
								float3 moveHere = tempPath.back();
								dist = myPos.distance2D(moveHere);

								//TODO: penetrators are now broken. kind of.

								//is the position between the proposed destination and the enemy higher than the average of mine and his height?
								bool losHack = ((moveHere.y + enemyPos.y)/2.0f) + UNIT_MAX_MANEUVER_HEIGHT_DIFFERENCE_UP > ai->cb->GetElevation((moveHere.x+enemyPos.x)/2, (moveHere.z+enemyPos.z)/2);
								//im here assuming the pathfinder returns correct Y values
								if (dist > max((UNIT_MIN_MANEUVER_RANGE_PERCENTAGE*myRange), UNIT_MIN_MANEUVER_DISTANCE)
									&& losHack) { //(enemyPos.y - moveHere.y) < UNIT_MAX_MANEUVER_HEIGHT_DIFFERENCE_UP) {
									debug2 = true;
									//ai->cb->SendTextMsg("AG: maneuvering", 0);
									//Draw where it would move:
									//ai->cb->CreateLineFigure(myPos, moveHere, 80, 1, frameSpread, RANDINT%1000000);
									ai->MyUnits[unit]->maneuverCounter = int(ceilf(max((float)UNIT_MIN_MANEUVER_TIME/frameSpread, (dist/ai->MyUnits[unit]->def()->speed))));
									//L("AG maneuver debug, maneuverCounter set to " << ai->MyUnits[unit]->maneuverCounter << " and dist:" << dist << " speed:" << ai->MyUnits[unit]->def()->speed << " frameSpread:" << frameSpread << " dist/speed:" << (dist/ai->MyUnits[unit]->def()->speed) << " its a " << ai->MyUnits[unit]->def()->humanName);
									ai->MyUnits[unit]->Move(moveHere);
									//REMEMBER that this will suck for planes (which is why i wont add them to this group)
								}
							}
							if (debug1 && !debug2) {
								//L("AG: maneuver: pathfinder run but path not used");
							}
						} else if (!ai->cb->GetUnitDef(unit)->canfly || myPos.y < ai->cb->GetElevation(myPos.x, myPos.z) + 25){
							//L("AttackGroup: this unit is an air unit: " << ai->cb->GetUnitDef(unit)->humanName);
						}//endif our unit exists && we arent currently maneuvering
					} else {
						////L("isShooting: OUR unit is dead. or something. Shouldnt happen ever. or maneuvering.");
					}
					//if cant attack, stop with the others, assuming someone was able to attack something
				}
			}
		}
//		//L("AG: isShooting end");
	}

	if (frameNr % 30 == 0 && !defending) {
		//drawing the target region:
		//L("AG update: drawing target region (attacking)");
		int heightOffset = 50;
		vector<float3> circleHack;
		float diagonalHack = attackRadius * (2.0f/3.0f);
		circleHack.resize(8, attackPosition);
		circleHack[0] += float3(-diagonalHack,heightOffset,-diagonalHack);
		circleHack[1] += float3(0,heightOffset,-attackRadius);
		circleHack[2] += float3(diagonalHack,heightOffset,-diagonalHack);
		circleHack[3] += float3(attackRadius,heightOffset,0);
		circleHack[4] += float3(diagonalHack,heightOffset,diagonalHack);
		circleHack[5] += float3(0,heightOffset,attackRadius);
		circleHack[6] += float3(-diagonalHack,heightOffset,diagonalHack);
		circleHack[7] += float3(-attackRadius,heightOffset,0);
		for(int i = 0; i < 8; i++) {
			//ai->cb->CreateLineFigure(circleHack[i], circleHack[(i+1)%8], 5, 0, 300, GetGroupID()+6000);
		}
		//from pos to circle center
		//ai->cb->CreateLineFigure(groupPosition+float3(0,heightOffset,0), attackPosition+float3(0,heightOffset,0), 5, 1, 30, GetGroupID()+6000);
		ai->cb->SetFigureColor(GetGroupID() + 6000, 0, 0, 0, 1.0f);
		//L("AG update: done drawing stuff");
	}

	//TODO: if arrived and not isShooting, set defending? already done in ismoving-end

	if (defending && !isShooting && !isMoving && frameNr % 60 == groupID % 60) {
		FindDefenseTarget(groupPosition);
	}

	// GIVE MOVE ORDERS TO UNITS ALONG PATHTOTARGET
	frameSpread = 60;
	if (!isShooting && isMoving && frameNr % frameSpread == (groupID*5) % frameSpread) {
		//L("AG: start of move order part before loop");
		//do the moving
		const int maxStepsAhead = 8;
		int pathMaxIndex = (int)pathToTarget.size()-1;
		int step1 = min(this->pathIterator + maxStepsAhead/2, pathMaxIndex);
		int step2 = min(this->pathIterator + maxStepsAhead, pathMaxIndex);
		//L("AG: picking stuff out of pathToTarget");
		//L("AG: pathtotarget size: " << pathToTarget.size());
		float3 moveToHereFirst = this->pathToTarget[step1];
		float3 moveToHere = this->pathToTarget[step2];
		
		//drawing destination
//		//ai->cb->CreateLineFigure(groupPosition + float3(0,50,0), pathToTarget[pathMaxIndex] + float3(0,50,0),8,1,frameSpread,groupID+1200);
		
		//L("AG: move order still before loop");
		
        //if we aint there yet
		if (groupPosition.distance2D(pathToTarget[pathMaxIndex]) > GROUP_DESTINATION_SLACK) {
			for(int i = 0; i < numUnits;i++) {
				int unit = units[i];
				if (ai->cb->GetUnitDef(unit) != NULL) {
					//TODO: when they are near target, change this so they line up or something while some are here and some arent. attack checker will take over soon.
					//theres also something that should be done with the units in front that are given the same order+shiftorder and skittle around back and forth meanwhile
	//				if (groupPosition.distance2D(ai->cheat->GetUnitPos(myEnemy)) > this->highestAttackRange) {
					//if the single unit aint there yet
					if (ai->cb->GetUnitPos(unit).distance2D(pathToTarget[pathMaxIndex]) > UNIT_DESTINATION_SLACK) {
						ai->MyUnits[unit]->Move(moveToHereFirst);
//						//ai->cb->CreateLineFigure(ai->cb->GetUnitPos(unit) + float3(0,50,0), moveToHereFirst + float3(0,50,0),15,1,10,groupID+50000);
						if (moveToHere != moveToHereFirst) {
							ai->MyUnits[unit]->MoveShift(moveToHere);
//							//ai->cb->CreateLineFigure(moveToHereFirst + float3(0,50,0), moveToHere + float3(0,50,0),15,1,10,groupID+50000);
						}
						//TODO: give a spring group the order instead of each unit
						//draw thin line from unit to groupPos
						//ai->cb->CreateLineFigure(ai->cb->GetUnitPos(unit) + float3(0,50,0), groupPosition + float3(0,50,0),4,0,frameSpread,groupID+500);
					} else {
						AIHCAddMapPoint amp;
						amp.label = "case-sua";
						amp.pos = groupPosition;
//						//ai->cb->HandleCommand(&amp);
					}
				}
			}
			//if group is as close as the pathiterator-indicated target is to the end of the path, increase pathIterator
			
			//L("AG: after move order, about to adjust stuff");

/*			
			float3 endOfPathPos = pathToTarget[pathMaxIndex];
			float groupDistanceToEnemy = groupPosition.distance2D(endOfPathPos);
			float pathIteratorTargetDistanceToEnemy = pathToTarget[pathIterator].distance2D(endOfPathPos);
			if (groupDistanceToEnemy <= pathIteratorTargetDistanceToEnemy) {
				this->pathIterator = min(pathIterator + maxStepsAhead/2, pathMaxIndex);
			}
			
			if (groupDistanceToEnemy > (pathIteratorTargetDistanceToEnemy*3.0f)) {
				//L("AG: path iterator reset bug thing");
				//this->pathIterator = min(pathIterator + maxStepsAhead/2, pathMaxIndex);
				this->pathIterator = 1;
			}
*/
			pathIterator = 0;
			float3 endOfPathPos = pathToTarget[pathMaxIndex];
			float groupDistanceToEnemy = groupPosition.distance2D(endOfPathPos);
			float pathIteratorTargetDistanceToEnemy = pathToTarget[pathIterator].distance2D(endOfPathPos);
			int increment = maxStepsAhead/2;
			while (groupDistanceToEnemy <= pathIteratorTargetDistanceToEnemy && pathIterator < pathMaxIndex) {
				pathIterator = min(pathIterator + increment, pathMaxIndex);
				pathIteratorTargetDistanceToEnemy = pathToTarget[pathIterator].distance2D(endOfPathPos);
			}
			pathIterator = min(pathIterator, pathMaxIndex);
			
		} else {
			//L("AG: group thinks it has arrived at the destination:" << groupID);
//			AIHCAddMapPoint amp;
//			amp.label = "group thinks it has arrived at the destination.";
//			amp.pos = groupPosition;
//			//ai->cb->HandleCommand(&amp);
			this->ClearTarget();
		}
		//L("AG: done updating move stuff (yay)");

	}//endif move
	
	//stuck fix stuff. disabled because the spring one works now and thats not taken into consideration yet.
	frameSpread = 60;
	if (false && isMoving && !isShooting && frameNr % frameSpread == 0) {
		//L("AG: unit stuck checking start");
		//stuck unit counter update. (at a 60 frame modulo)
		//TODO: if one unit has completed the path, it wont be given any new orders, but this will still happen
		//so there might be false positives here :<
		for (vector<int>::iterator it = units.begin(); it != units.end(); it++) {
			int unit = *it;
			if (ai->cb->GetUnitDef(unit)){
				CUNIT *u = ai->MyUnits[unit];
				float distanceMovedSinceLastUpdate = ai->cb->GetUnitPos(unit).distance2D(u->earlierPosition);
				if (distanceMovedSinceLastUpdate < UNIT_STUCK_MOVE_DISTANCE) {
					u->stuckCounter++;
				} else { //it moved so it isnt stuck
					u->stuckCounter = 0;
				}
				u->earlierPosition = u->pos();
				//hack for slight maneuvering around buildings, spring fails at this sometimes
				if (u->stuckCounter == UNIT_STUCK_COUNTER_MANEUVER_LIMIT) {
					//L("AG: attempting unit unstuck manuevering for " << unit << " at pos " << u->pos().x << " " << u->pos().y << " " << u->pos().z);
					AIHCAddMapPoint amp;
					amp.label = "stuck-fix";
					amp.pos = u->pos();
					//ai->cb->HandleCommand(&amp);
					//findbestpath to perifery of circle around mypos, distance 2x resolution or somesuch
					//dont reset counter, that would make it loop manuever attempts.
					vector<float3> tempPath;
					float dist = ai->pather->FindBestPathToRadius(&tempPath, &u->pos(), UNIT_STUCK_MANEUVER_DISTANCE, &u->pos()); //500 = hack
					//float dist = ai->pather->FindBestPathToRadius(&tempPath, &ai->cb->GetUnitPos(unit), myRange, &ai->cheat->GetUnitPos(enemies[enemySelected]));
					if (tempPath.size() > 0) {
						float3 moveHere = tempPath.back();
						//ai->cb->CreateLineFigure(u->pos(), moveHere, 14, 1, frameSpread, RANDINT%1000000);
						u->Move(moveHere);
					}
				}
			}
		}//endfor unstuck
	}
}
