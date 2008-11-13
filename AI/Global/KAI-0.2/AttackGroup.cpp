#include "AttackGroup.h"
#include "GlobalAI.h"
#include "creg/STL_List.h"

#undef assert
#define assert(a) if (!(a)) throw "Assertion " #a "faild";

//#define UNIT_STUCK_MOVE_DISTANCE 2.0f
//not moving for 5*60 frames = stuck :
#define UNIT_STUCK_COUNTER_MANEUVER_LIMIT 60
//gives it (STUCK_COUNTER_LIMIT - STUCK_COUNTER_MANEUVER_LIMIT)*60 seconds
// to move longer than UNIT_STUCK_MOVE_DISTANCE (7 sec)
#define UNIT_STUCK_COUNTER_LIMIT 120
//stuck maneuver distance, how far it moves when trying to get unstuck. has to be higher than minimum pather resolution
#define UNIT_STUCK_MANEUVER_DISTANCE 250
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
#define MINIMUM_GROUP_RETREAT_DISTANCE 400
#define BASE_REFERENCE_UPDATE_FRAMES 1800
#define FLEE_MAX_THREAT_DIFFERENCE 1.15 //1.5
#define FLEE_MIN_PATH_RECALC_SECONDS 3
#define MIN_DEFENSE_RADIUS 100
#define DAMAGED_UNIT_FACTOR 0.50f

CR_BIND(CAttackGroup ,(NULL,0))

CR_REG_METADATA(CAttackGroup,(
				CR_MEMBER(ai),
				CR_MEMBER(units),
				CR_MEMBER(lowestAttackRange),
				CR_MEMBER(highestAttackRange),
				CR_MEMBER(lowestUnitSpeed),
				CR_MEMBER(highestUnitSpeed),
				CR_MEMBER(groupPhysicalSize),
				CR_MEMBER(highestTurnRadius),

				CR_MEMBER(worstMoveType),

				CR_MEMBER(attackPosition),
				CR_MEMBER(attackRadius),

				CR_MEMBER(pathToTarget),
				CR_MEMBER(pathIterator),

				CR_MEMBER(unitArray),

				CR_MEMBER(groupID),

				CR_MEMBER(isMoving),
				CR_MEMBER(isFleeing),
				CR_MEMBER(isDefending),
				CR_MEMBER(isShooting),

				CR_MEMBER(groupDPS),

				CR_MEMBER(groupPos),
				CR_MEMBER(lastGroupPosUpdateFrame),

				CR_MEMBER(baseReference),
				CR_MEMBER(lastBaseReferenceUpdate),

				CR_MEMBER(assignedEnemies),
				CR_RESERVED(256)
				));

float3 CAttackGroup::CheckCoordinates(float3 pos)
{
	if (pos.x<0.01) pos.x=0.01;
	if (pos.z<0.01) pos.z=0.01;
	if (pos.x>ai->cb->GetMapWidth()*8-0.01) pos.x=ai->cb->GetMapWidth()*8-0.01;
	if (pos.z>ai->cb->GetMapHeight()*8-0.01) pos.z=ai->cb->GetMapHeight()*8-0.01;
	return pos;
}

CAttackGroup::CAttackGroup()
{
	this->ai=0;
	this->groupID = -1;
	this->pathIterator = 0;
	this->lowestAttackRange = 100000;
	this->highestAttackRange = 1;
//	this->movementCounterForStuckChecking = 0;
	this->isDefending = true;
	this->isMoving = false;
	this->isShooting = false;
	this->attackPosition = float3(0,0,0);
	this->attackRadius = 1;
	this->isFleeing = false;
	this->groupPos = ZEROVECTOR;
	this->lastGroupPosUpdateFrame = -1;
	this->baseReference = ERRORVECTOR;
	this->lastBaseReferenceUpdate = -BASE_REFERENCE_UPDATE_FRAMES-1;
	this->worstMoveType = 0xFFFFFFFF;
	this->groupPhysicalSize = 1.0f;
}

CAttackGroup::CAttackGroup(AIClasses* ai, int groupID_in)
{
	this->ai=ai;
	this->groupID = groupID_in;
	this->pathIterator = 0;
	this->lowestAttackRange = 100000;
	this->highestAttackRange = 1;
//	this->movementCounterForStuckChecking = 0;
	this->isDefending = true;
	this->isMoving = false;
	this->isShooting = false;
	this->attackPosition = float3(0,0,0);
	this->attackRadius = 1;
	this->isFleeing = false;
	this->groupPos = ZEROVECTOR;
	this->lastGroupPosUpdateFrame = -1;
	this->baseReference = ERRORVECTOR;
	this->lastBaseReferenceUpdate = -BASE_REFERENCE_UPDATE_FRAMES-1;
	this->worstMoveType = 0xFFFFFFFF;
	this->groupPhysicalSize = 1.0f;
}

CAttackGroup::~CAttackGroup()
{
	
}

void CAttackGroup::Log() {
	int temp = 0;
	if (ai->ah->debug) L("AG: logging contents of group " << groupID << ":");
	for (vector<int>::iterator it = units.begin(); it != units.end(); it++) {
		temp++;
		if (ai->ah->debug) {
			if (ai->cb->GetUnitDef(*it)) {
				L("" << temp << ": " << *it << " type:" << ai->cb->GetUnitDef(*it)->humanName);
			} else {
				L("" << temp << ": " << *it << " ILLEGAL UNIT - has no unit def");
			}
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
		//this->lowestAttackRange = min(this->lowestAttackRange, this->ai->ut->GetMaxRange(ai->cb->GetUnitDef(unitID)));
		//this->highestAttackRange = max(this->highestAttackRange, this->ai->ut->GetMaxRange(ai->cb->GetUnitDef(unitID)));
	} else {
		bool dead_unit_added_to_group = false;
		assert(dead_unit_added_to_group);
	}
	this->RecalcGroupProperties();
}

bool CAttackGroup::RemoveUnit(int unitID) {
	bool found = false;
	vector<int>::iterator it;
	for (it = units.begin(); it != units.end(); it++) {
		if (*it == unitID) {
			found = true;
//			L("AttackGroup: erasing unit with id:" << unitID);
			break;
		}
	}
	if (found) {
		units.erase(it);
		if( ai->cb->GetUnitDef(unitID) != NULL) {
			ai->MyUnits[unitID]->groupID = 0;
//			L("AttackGroup: groupid = 0 --> success");
		}
	}
	assert(found);
	this->RecalcGroupProperties();
	return found;
}

int CAttackGroup::Size()
{
	return units.size();
}

int CAttackGroup::GetGroupID() {
	return groupID;
}

unsigned CAttackGroup::GetWorstMoveType() {
	return worstMoveType;
}

vector<int>* CAttackGroup::GetAllUnits() {
	return &this->units;
}

bool CAttackGroup::Defending() {
	return isDefending;
}

void CAttackGroup::RecalcGroupProperties() {
	this->worstMoveType = 0xFFFFFFFF;
	this->lowestAttackRange = 10000.0f;
	this->highestAttackRange = 1.0f;
	this->lowestUnitSpeed = 10000.0f;
	this->highestUnitSpeed = 1.0f;
	this->groupPhysicalSize = 1.0f;
	this->groupDPS = 0.00001f;
	this->highestTurnRadius = 0.00001f;
	if (units.size() == 0) return;
	const UnitDef* ud;
	for (vector<int>::iterator it = units.begin(); it != units.end(); it++) {
		int unitID = *it;
		ud = ai->cb->GetUnitDef(unitID);
		if(ud != NULL) {
			worstMoveType &= ai->MyUnits[unitID]->GetMoveType();
			this->lowestAttackRange = min(this->lowestAttackRange, this->ai->ut->GetMinRange(ud));
			this->highestAttackRange = max(this->highestAttackRange, this->ai->ut->GetMaxRange(ud));
			this->lowestUnitSpeed = min(this->lowestUnitSpeed, ud->speed);
			this->highestUnitSpeed = max(this->highestUnitSpeed, ud->speed);
			this->groupPhysicalSize += (ud->xsize+ud->zsize) * 0.3f;
			this->groupDPS += ai->MyUnits[unitID]->GetAverageDPS();
			if(ud->canfly)this->highestTurnRadius = max(this->highestTurnRadius, ud->turnRadius);
		}
	}
	if (ai->ah->debug) L("CAttackGroup::RecalcGroupProperties() worstMoveType:"<<worstMoveType<<" lowestAttackRange:"<<lowestAttackRange<<" highestAttackRange:"<<highestAttackRange<<" lowestUnitSpeed:"<<lowestUnitSpeed<<" highestUnitSpeed:"<<highestUnitSpeed<<" groupPhysicalSize:"<<groupPhysicalSize<<" groupDPS:"<<groupDPS<<" highestTurnRadius:"<<highestTurnRadius);
}


//combined ut-dps of the group
float CAttackGroup::DPS() {
	return this->groupDPS;
}

float CAttackGroup::GetLowestUnitSpeed() {
	return this->lowestUnitSpeed;
}

float CAttackGroup::GetHighestUnitSpeed() {
	return this->highestUnitSpeed;
}

int CAttackGroup::PopStuckUnit() {
	//removes a stuck unit from the group if there is one, and puts a marker on the map
	for (vector<int>::iterator it = units.begin(); it != units.end(); it++) {
		if (ai->MyUnits[*it]->stuckCounter > UNIT_STUCK_COUNTER_LIMIT && ai->MyUnits[*it]->attemptedUnstuck) {
			int id = *it;
			//mark it:
			if (ai->ah->debugDraw) {
				char text[512];
				sprintf(text, "stuck %i:%i, dropping from group:%i. isMoving=%i", id, ai->MyUnits[*it]->stuckCounter, groupID, isMoving);
				AIHCAddMapPoint amp;
				amp.label = text;
				amp.pos = ai->cb->GetUnitPos(id);
				ai->cb->HandleCommand(AIHCAddMapPointId, &amp);

				sprintf(text, "humanName:%s", ai->MyUnits[*it]->def()->humanName.c_str());
				amp.label = text;
				amp.pos = ai->cb->GetUnitPos(id) + float3(0, 0, 30);
				ai->cb->HandleCommand(AIHCAddMapPointId, &amp);
			}
			ai->MyUnits[id]->stuckCounter = 0;
			//units.erase(it);
			this->RemoveUnit(id);
			return id;
		}
	}
	return -1;
}

int CAttackGroup::PopDamagedUnit() {
	for (vector<int>::iterator it = units.begin(); it != units.end(); it++) {
		if (ai->MyUnits[*it]->Health() < ai->MyUnits[*it]->MaxHealth()*DAMAGED_UNIT_FACTOR) {
			this->RemoveUnit(*it);
            return *it;
		}
	}
	return -1;
}

bool CAttackGroup::CloakedFix(int enemy) {
	const UnitDef * ud = ai->cheat->GetUnitDef(enemy);
//	return ud != NULL && !(ud->canCloak && ud->startCloaked && (ai->cb->GetUnitPos(enemy) == ZEROVECTOR));
	return ud != NULL && !(ud->canCloak && ai->cb->GetUnitPos(enemy) == ZEROVECTOR);
}

void CAttackGroup::RecalcGroupPos() {
	//whats the groups position (for distance checking when selecting targets)
	int unitCounter = 0;
	float3 newGroupPosition = float3(0,0,0);
	int numUnits = units.size();
	for (int i = 0; i < numUnits; i++) {
		int unit = units[i];
		if (ai->cb->GetUnitDef(unit) != NULL) {
			unitCounter++;
			newGroupPosition += ai->cb->GetUnitPos(unit);
		}
	}
	if (unitCounter > 0) {
		newGroupPosition /= unitCounter;
		//find the unit closest to the center (since the actual center might be on a hill or something)
		float closestSoFar = FLT_MAX;
		int closestUnitID = -1;
		float temp;
		int unit;
		for (int i = 0; i < numUnits; i++) {
			unit = units[i];
			//is it closer. consider also low unit counts, then the first will be used since its < and not <=, assuming sufficient float accuracy
			if (ai->cb->GetUnitDef(unit) != NULL && (temp = newGroupPosition.distance2D(ai->cb->GetUnitPos(unit))) < closestSoFar-GROUP_MEDIAN_UNIT_SELECTION_SLACK) {
				closestSoFar = temp;
				closestUnitID = unit;
			}
		}
		assert(closestUnitID != -1);
		newGroupPosition = ai->cb->GetUnitPos(closestUnitID);
	} else {
		if (ai->ah->debug) L("AttackGroup: empty attack group when calcing group pos!");
		newGroupPosition = ERRORVECTOR; //<-----------------

	}
	newGroupPosition = CheckCoordinates(newGroupPosition);
	this->groupPos = newGroupPosition;
	this->lastGroupPosUpdateFrame = ai->cb->GetCurrentFrame();
}

float3 CAttackGroup::GetGroupPos() {
	if (this->lastGroupPosUpdateFrame < ai->cb->GetCurrentFrame()) {
		this->RecalcGroupPos();
	}
	return this->groupPos;
}

float CAttackGroup::GetLowestAttackRange() {
	return this->lowestAttackRange;
}
float CAttackGroup::GetHighestAttackRange() {
	return this->highestAttackRange;
}

void CAttackGroup::DrawGroupPosition() { //debug-filtered at call
	//draw arrow pointing to the group center:
	int figureID = groupID + 1700;
	float3 groupPosition = GetGroupPos();
	if (!isDefending) { //att
		ai->cb->SetFigureColor(figureID, 65535.0f, 0.0f, 0.0f, 0.2f);
		ai->cb->CreateLineFigure(groupPosition + float3(0,140,0), groupPosition + float3(0,20,0),48,1,300,figureID);
		ai->cb->SetFigureColor(figureID, 65535.0f, 0.0f, 0.0f, 0.2f);
	} else if(!isFleeing) { //def but not flee
		ai->cb->SetFigureColor(figureID, 0.0f, 65535.0f, 0.0f, 0.2f);
		ai->cb->CreateLineFigure(groupPosition + float3(0,80,0), groupPosition + float3(0,20,0),96,1,300,figureID);
		ai->cb->SetFigureColor(figureID, 0.0f, 65535.0f, 0.0f, 0.2f);
	} else {//flee
		ai->cb->SetFigureColor(figureID, 65535.0f, 65535.0f, 0.0f, 0.2f);
		ai->cb->CreateLineFigure(groupPosition + float3(0,60,0), groupPosition + float3(0,20,0),220,0,300,figureID);
		ai->cb->SetFigureColor(figureID, 65535.0f, 65535.0f, 0.0f, 0.2f);
	}
}

//returns enemies in my attack area
list<int>* CAttackGroup::GetAssignedEnemies() {
	bool why_would_you_ask_if_a_defending_group_has_assigned_enemies = !isDefending;
	assert(why_would_you_ask_if_a_defending_group_has_assigned_enemies);
	return &assignedEnemies;
}

//TODO change it so it traces enemies.. or something. something.
void CAttackGroup::RecalcAssignedEnemies() {
	assignedEnemies.clear();
	if (!isDefending) {
		int numTaken = ai->cheat->GetEnemyUnits(unitArray, attackPosition, attackRadius);
		for(int i = 0; i < numTaken; i++) {
			assignedEnemies.push_back(unitArray[i]);
		}
	}
}

void CAttackGroup::AssignTarget(vector<float3> in_path, float3 in_position, float in_radius) {
	this->attackPosition = in_position;
	this->attackRadius = in_radius;
	this->pathToTarget = in_path;
	this->isMoving = true;
	this->isShooting = false;
	this->pathIterator = 0;
	this->isDefending = false;
	this->isFleeing = false;
	if (ai->ah->debugDraw) {
		int figureID = (groupID+4200);
		ai->cb->SetFigureColor(figureID, 65535.0f, 0.0f, 0.0f, 0.2f);
		for (int i = 1; i < (int)pathToTarget.size(); i++) {
			ai->cb->CreateLineFigure(pathToTarget[i-1] + float3(0,50,0), pathToTarget[i] + float3(0,50,0), 8, 0, 300, figureID);
		}
		ai->cb->SetFigureColor(figureID, 65535.0f, 0.0f, 0.0f, 0.2f);
	}
//	L("AG: target assigning complete. path size: " << pathToTarget.size() << " targetx:" << attackPosition.x << " targety:" << attackPosition.y << " radius:" << attackRadius << " isMoving:" << isMoving << " frame:" << ai->cb->GetCurrentFrame() << " groupID:" << groupID);
	RecalcAssignedEnemies();
}

void CAttackGroup::ClearTarget() {
	this->isMoving = false;
	this->isFleeing = false;
	this->isDefending = true;
	this->attackPosition = ZEROVECTOR;
	this->attackRadius = 0.0f;
	this->pathToTarget.clear();
	this->isShooting = false;
}

void CAttackGroup::Defend() {
ai->math->StartTimer(ai->ah->ah_timer_Defend);
	this->ClearTarget();
	if (!this->FindDefenseTarget(GetGroupPos())) {
		if (ai->ah->debug) L("AG: Defend() Couldnt find defense target !!");
		this->PathToBase(GetGroupPos());
	}
ai->math->StopTimer(ai->ah->ah_timer_Defend);
}

void CAttackGroup::Flee() {
	//ignore if we are close to base
	this->ClearTarget();
	this->isFleeing = true;
	this->PathToBase(this->GetGroupPos());
}

bool CAttackGroup::NeedsNewTarget() {
ai->math->StartTimer(ai->ah->ah_timer_NeedsNewTarget);
	bool needs = false;
	//if we arent doing anything
	if (!isShooting) {
		if (!isMoving) {
ai->math->StopTimer(ai->ah->ah_timer_NeedsNewTarget);
			return true;
		}
		//if we're supposed to have a target but theres no enemies in the target area
		if (!isFleeing && isMoving && ai->cheat->GetEnemyUnits(this->unitArray, pathToTarget.back(), max(highestAttackRange, max(this->attackRadius, float(MIN_DEFENSE_RADIUS)))) == 0){
ai->math->StopTimer(ai->ah->ah_timer_NeedsNewTarget);
			return true;
		}
		if (isFleeing && groupDPS < ai->tm->ThreatAtThisPoint(groupPos) + ai->tm->GetUnmodifiedAverageThreat()) {
ai->math->StopTimer(ai->ah->ah_timer_NeedsNewTarget);
			return true;
		}
		//if we're not fighting but theres nowwhere to go, OR the path has too high threat somewhere
		if (!ConfirmPathToTargetIsOK()) {// all states?
ai->math->StopTimer(ai->ah->ah_timer_NeedsNewTarget);
			return true;
		}
	}
ai->math->StopTimer(ai->ah->ah_timer_NeedsNewTarget);
	return needs;
}

bool CAttackGroup::ConfirmPathToTargetIsOK()
{
	float threatLimit = (groupDPS * FLEE_MAX_THREAT_DIFFERENCE) + ai->tm->GetUnmodifiedAverageThreat();
	int stopPoint = (int)pathToTarget.size();//it should only path to lowest range anyway - (lowestAttackRange/THREATRES);
	int startPoint = this->pathIterator;
	if (isFleeing) {
		startPoint += int(ceilf((lowestUnitSpeed*FLEE_MIN_PATH_RECALC_SECONDS)/THREATRES));
//		L(" CAttackGroup::ConfirmPathToTargetIsOK startpoint: " << startPoint);
	}
	for (int i = startPoint; i < stopPoint; i++) {
		if (ai->tm->ThreatAtThisPoint(CheckCoordinates(pathToTarget[i])) > threatLimit) {
			return false;
		}
	}
	return true;
}

void CAttackGroup::PathToBase(float3 groupPosition) {
	groupPosition = groupPosition;

//	L("AG: pathing back to base. group:" << groupID);
	vector<float3> basePositions;
	for (list<Factory>::iterator i = ai->uh->Factories.begin();i!=ai->uh->Factories.end();i++) {
		basePositions.push_back(ai->cb->GetUnitPos(i->id));
	}
    pathToTarget.clear();
ai->math->StopTimer(ai->ah->ah_timer_totalTimeMinusPather);
	ai->pather->micropather->SetMapData(&ai->pather->canMoveIntMaskArray.front(),&ai->tm->ThreatArray.front(),ai->tm->ThreatMapWidth,ai->tm->ThreatMapHeight, this->GetWorstMoveType());
	ai -> pather -> PathToSet(&pathToTarget,GetGroupPos(),&basePositions,DPS()*4);
ai->math->StartTimer(ai->ah->ah_timer_totalTimeMinusPather);
	if (pathToTarget.size() <= 2) {
		//isMoving = false;
		this->ClearTarget();
	} else {
		isMoving = true;
		isShooting = false;
		this->pathIterator = 0;
		if (ai->ah->debugDraw) {
			int figureID = (groupID+600);
			ai->cb->SetFigureColor(figureID, 65535.0f, 65535.0f, 0.0f, 0.2f);
			for (int i = 1; i < (int)pathToTarget.size(); i++) {
				ai->cb->CreateLineFigure(pathToTarget[i-1] + float3(0,50,0), pathToTarget[i] + float3(0,50,0), 8, 0, 300, figureID);
			}
			ai->cb->SetFigureColor(figureID, 65535.0f, 65535.0f, 0.0f, 0.2f);
		}
	}
}

bool CAttackGroup::FindDefenseTarget(float3 groupPosition)
{
	//ai->cb->SendTextMsg("AG: FindDefenseTarget - group is defending and selecting a target.", 0);
	int frameNr = ai->cb->GetCurrentFrame();
	//the "find new enemy" part
	int numOfEnemies;
	numOfEnemies = ai->sh->GetNumberOfEnemies();
	const int* enemyArray = ai->sh->GetEnemiesList();
	if (numOfEnemies > 0) {
		//build vector of enemies
		static vector<float3> enemyPositions;
		enemyPositions.reserve(numOfEnemies);
		enemyPositions.clear();
		//make a vector with the positions of all enemies
		for (int i = 0; i < numOfEnemies; i++) {
			if (enemyArray[i] != -1) {
				const UnitDef *enemy_ud = ai->cheat->GetUnitDef(enemyArray[i]);
				float3 enemyPos = ai->cheat->GetUnitPos(enemyArray[i]);
				//do some filtering and then
				if (ai->cb->GetUnitDef(enemyArray[i]) != NULL
						&& this->CloakedFix(enemyArray[i])
						&& !enemy_ud->canfly
						&& !enemy_ud->weapons.size() == 0) {
					//pathfinder removes units not reachable by my unit type/position
					enemyPositions.push_back(enemyPos);
				}
			}
		}
		// if all units are cloaked or otherwise not eligible (but there are units) then target them anyway
		if (enemyPositions.size() < 1) {
			for (int i = 0; i < numOfEnemies; i++) {
				if (enemyArray[i] != -1) {
					float3 enemyPos = ai->cheat->GetUnitPos(enemyArray[i]);
					enemyPositions.push_back(enemyPos);
				}
			}
		}

		//now it becomes sexy :p
		static vector<float3> tempPathToTarget;
		tempPathToTarget.clear();
		//verify the validity of the current base reference point
		if (this->lastBaseReferenceUpdate < (frameNr - BASE_REFERENCE_UPDATE_FRAMES)) {
			//find closest reachable base spot
ai->math->StopTimer(ai->ah->ah_timer_totalTimeMinusPather);
			ai->pather->micropather->SetMapData(&ai->pather->canMoveIntMaskArray.front(),&ai->tm->ThreatArray.front(),ai->tm->ThreatMapWidth,ai->tm->ThreatMapHeight, this->GetWorstMoveType());
			float cost = ai->pather->PathToPrioritySet(&tempPathToTarget, groupPosition, ai->ah->GetKMeansBase());
ai->math->StartTimer(ai->ah->ah_timer_totalTimeMinusPather);
			if (cost != 0) { //tempPathToTarget.size() >= 2) {
	//			L("AG: FindDefTarget: base spot to search from found");
				//searchStartPosition = tempPathToTarget.back();
				baseReference = tempPathToTarget.back();
			} else { //no base spot found :o
				//searchStartPosition = groupPosition;
				baseReference = ERRORVECTOR;
				if (ai->ah->debug) L("AG: no base reference found for group " << groupID);
			}
			lastBaseReferenceUpdate = frameNr;
		}
		
		float3 searchStartPosition;
		if (baseReference != ERRORVECTOR) searchStartPosition = baseReference;
		else searchStartPosition = groupPosition;
		
		//search for enemy from closest reachable base spot
		tempPathToTarget.clear();
ai->math->StopTimer(ai->ah->ah_timer_totalTimeMinusPather);
		ai->pather->micropather->SetMapData(&ai->pather->canMoveIntMaskArray.front(),&ai->tm->EmptyThreatArray.front(),ai->tm->ThreatMapWidth,ai->tm->ThreatMapHeight, this->GetWorstMoveType());
		float costToTarget = ai->pather->PathToSet(&tempPathToTarget, searchStartPosition, &enemyPositions);
ai->math->StartTimer(ai->ah->ah_timer_totalTimeMinusPather);
		if (tempPathToTarget.size() <= 2) { //if theres no enemies from either your pos or closest base spot pos
			isMoving = false;
//			L("AG: FindNewTarget sets isMoving to false.");
		} else {
//			L("AG: FindDefTarget: we selected the enemy which is closest to the base");
			float3 selectedEnemyPosition = tempPathToTarget.back();
/*
			AIHCAddMapLine aml;
			aml.posfrom = groupPosition;
			aml.posto = selectedEnemyPosition;
			ai->cb->HandleCommand(AIHCAddMapLineId, &aml);
*/			
			//lets make the actual path
			
			float costToEnemy;
			//only calc new path if it will actually be different (if searchStartPosition is different from in previous search
			if (searchStartPosition == groupPosition) {
                costToEnemy = costToTarget;
			} else {
ai->math->StopTimer(ai->ah->ah_timer_totalTimeMinusPather);
				ai->pather->micropather->SetMapData(&ai->pather->canMoveIntMaskArray.front(),&ai->tm->EmptyThreatArray.front(),ai->tm->ThreatMapWidth,ai->tm->ThreatMapHeight, this->GetWorstMoveType());
				costToEnemy = ai->pather->PathToPos(&tempPathToTarget, groupPosition, selectedEnemyPosition);
ai->math->StartTimer(ai->ah->ah_timer_totalTimeMinusPather);
			}
			//if tempPathToTarget is a valid attack path
			if (costToEnemy > 0 && tempPathToTarget.size() > 2) { //we have found cheapest way to the enemy which is closest to base (or us if theres no reachable base)
				this->pathToTarget = tempPathToTarget;
				isMoving = true;
				isFleeing = false;
				this->pathIterator = 0;
				//draw the path
				if (ai->ah->debugDraw) {
					int figureID = (groupID+2700);
					ai->cb->SetFigureColor(figureID, 0.0f, 65535.0f, 0.0f, 0.2f);
					for (int i = 1; i < (int)pathToTarget.size(); i++) {
						ai->cb->CreateLineFigure(pathToTarget[i-1] + float3(0,50,0), pathToTarget[i] + float3(0,50,0), 8, 0, 300, figureID);
					}
					ai->cb->SetFigureColor(figureID, 0.0f, 65535.0f, 0.0f, 0.2f);
				}
				return true;
			}
		}
	} else { //endif there are enemies
		//attempt to path back to base if there are no targets
		//L("AG: found no defense targets, pathing back to base. group:" << groupID);
		//this->PathToBase(groupPosition);
	}
	if (!isShooting && !isMoving) {
		if (ai->ah->debug) L("AttackGroup: There are no accessible enemies and we're idling. groupid:" << this->groupID << " we are defending: " << this->isDefending);
	}
	return false;
}

void CAttackGroup::MicroUpdate(float3 groupPosition) {
ai->math->StartTimer(ai->ah->ah_timer_MicroUpdate);
	int numUnits = (int)units.size();
	assert (numUnits > 0);
	if (numUnits == 0) return;
	int frameNr = ai->cb->GetCurrentFrame();
	int frameSpread = 15; 
	if (!isFleeing && frameNr % frameSpread == (groupID*4) % frameSpread) {
//		L("AG: isShooting start");
		this->isShooting = false;
		//get all enemies within attack range:
		float searchRange = this->highestAttackRange + highestTurnRadius + groupPhysicalSize;
		int numEnemies = ai->cheat->GetEnemyUnits(unitArray, groupPosition, searchRange);
//		L("AG shoot debug 1 numEnemies:" << numEnemies << " range:" << range);
		if (numEnemies > 0) {
			//selecting one of the enemies
			int enemySelected = -1;
			{ //local variable space for selecting the enemy
				float distanceToTempTarget = FLT_MAX;
				float temp;
				//bool closestSoFarIsBuilding = false; shoot units first
				CUNIT* exampleUnit = ai->MyUnits[units.front()];
				bool foundUnitThatCanAttackMe = false;
				bool tempItCanAttackMe = false;
				const UnitDef* enemyDef;
				int currentEnemyID;
				for (int i = 0; i < numEnemies; i++) {
					//pick our general target preference
					currentEnemyID = unitArray[i];
					enemyDef = ai->cheat->GetUnitDef(currentEnemyID);
					if (	enemyDef != NULL 
							&& CloakedFix(unitArray[i])
							&& (((temp = groupPosition.distance2D(ai->cheat->GetUnitPos(currentEnemyID))) < distanceToTempTarget && foundUnitThatCanAttackMe) //when were are only considering threats AND this is better
								|| (temp < distanceToTempTarget && !foundUnitThatCanAttackMe) //the regular version with no threats
								|| (!foundUnitThatCanAttackMe && (tempItCanAttackMe = exampleUnit->CanAttackMe(currentEnemyID))))//its the first that can attack me
							&& exampleUnit->CanAttack(currentEnemyID)) {
						enemySelected = i;
						distanceToTempTarget = temp;
						if (!foundUnitThatCanAttackMe && tempItCanAttackMe) foundUnitThatCanAttackMe = true;
					}
				}
			}
/*
mass TODO:
check dps of selected enemy
check your dps vs selected enemy and check if its a high percentage of our highest average dps and thus if its worth considering
store these things in the target selection process
*/
			// theres some kind of target in range
			if (enemySelected != -1) {
				//X marks the target
				float3 enemyPos = ai->cheat->GetUnitPos(unitArray[enemySelected]);
				int xSize = 40;
				int lineGroupID = groupID+5000;
				int heightOffset = 20;
				if (ai->ah->debugDraw) ai->cb->CreateLineFigure(enemyPos + float3(-xSize,heightOffset,-xSize), enemyPos + float3(xSize,heightOffset,xSize), 5, 0, frameSpread, lineGroupID);
				if (ai->ah->debugDraw) ai->cb->CreateLineFigure(enemyPos + float3(-xSize,heightOffset,xSize), enemyPos + float3(xSize,heightOffset,-xSize), 5, 0, frameSpread, lineGroupID);
//				float3 enemyPos = ai->cheat->GetUnitPos(unitArray[enemySelected]); //TODO: add half the height of the unit, or something of that effect. or our unit weapon height.
				assert (CloakedFix(unitArray[enemySelected]));
				//TODO: add some check verifying that the target can actually be reached by a given unit.
				float myDPS = this->DPS();
				//float threatAtEnemy = ai->tm->ThreatAtThisPoint(enemyPos) - ai->tm->GetUnmodifiedAverageThreat();
				//const float FLEE_MAX_THREAT_DIFFERENCE = 1.30f;
				//TODO, This is at fault, wrong position is being checked
				//me-----------range------>enemy
				//           check here   not here
				float3 vectorUsToEnemy = enemyPos - groupPos;
				//TODO: maybe averagerange? hmm
				float distanceToEnemy = groupPos.distance2D(enemyPos);
				float distanceCheckPoint = max (distanceToEnemy - this->lowestAttackRange, 0.0f); //shouldnt be behind our current pos
				float factor = distanceCheckPoint/distanceToEnemy;
				float3 pointAtRange = groupPos + (vectorUsToEnemy*factor);
				pointAtRange = CheckCoordinates(pointAtRange);
				float threatAtRange = ai->tm->ThreatAtThisPoint(pointAtRange) - ai->tm->GetUnmodifiedAverageThreat();
				
				if (ai->ah->debugDraw) {
					//Debug: draw the threat-checked position
					xSize = 10;
					lineGroupID = groupID+15000;
					heightOffset = 20;
					ai->cb->CreateLineFigure(pointAtRange + float3(-xSize,heightOffset,-xSize), pointAtRange + float3(xSize,heightOffset,xSize), 5, 0, frameSpread, lineGroupID);
					ai->cb->CreateLineFigure(pointAtRange + float3(-xSize,heightOffset,xSize), pointAtRange + float3(xSize,heightOffset,-xSize), 5, 0, frameSpread, lineGroupID);
				}
				//GIVING ATTACK ORDER to each unit:
				if (myDPS * FLEE_MAX_THREAT_DIFFERENCE > threatAtRange) { //attack
					this->isShooting = true;
					int myUnit;
					const UnitDef* myUnitDef;
					float3 myPos;
					int enemyID = unitArray[enemySelected];
					const UnitDef* enemyUnitDef = ai->cheat->GetUnitDef(enemyID);
					CUNIT* myUnitReference;
					float myMaxRange;
					float myMinRange;
					for (int i = 0; i < numUnits; i++) {
						myUnit = units[i];
						myUnitDef = ai->cb->GetUnitDef(myUnit);
						//does our unit exist and its not currently maneuvering	
						if (myUnitDef != NULL && (myUnitReference = ai->MyUnits[myUnit])->maneuverCounter-- <= 0) {
							//add a routine finding the best(not just closest) target, but for now just fire away
							//TODO: add canattack
							//position focus fire for buildings
//							if(enemyUnitDef->speed > 0) {
								myUnitReference->Attack(enemyID); //
//							} else {
//								myUnitReference->Attack(enemyPos, 0.001f);
//							}
							//TODO: this should be the max range of the weapon the unit has with the lowest range
							//assuming you want to rush in with the heavy stuff, that is
							//SINGLE UNIT MANEUVERING:

							//TODO SOMEHOW PICK CLOSEST UNIT IN THE ARRAY THAT CAN FIRE AT ME or something
							//since targetting selection changed this might have to change too

							//testing the possibility of retreating to max range if target is too close
							myPos = ai->cb->GetUnitPos(myUnit);
							myMaxRange = this->ai->ut->GetMaxRange(myUnitDef);
							myMinRange = this->ai->ut->GetMinRange(myUnitDef);
							//is it air, //or air thats landed 
							if(!myUnitDef->canfly //if we are not a plane
									// AND difference makes it a good idea
									&& (myMinRange - UNIT_MIN_MANEUVER_RANGE_DELTA) > myPos.distance2D(enemyPos)
									//AND it can attack me
									&& ai->MyUnits[myUnit]->CanAttackMe(unitArray[enemySelected])) {
								bool debug1 = true;
								bool debug2 = false;
								static vector<float3> tempPath;
								tempPath.clear();
								//two things: 1. dont need a path, just a position and 2. it should avoid other immediate friendly units and/or immediate enemy units+radius 
								//maybe include the height parameter in the search? probably not possible
								//TODO: OBS doesnt this mean pathing might happen every second? outer limit should be more harsh than inner
								float3 moveHere;
ai->math->StopTimer(ai->ah->ah_timer_totalTimeMinusPather);
								//OBS: the move type of the single unit. maybe the group center unit will maneuver to positions that the group itself cant path from ?
								ai->pather->micropather->SetMapData(&ai->pather->canMoveIntMaskArray.front(),&ai->tm->ThreatArray.front(),ai->tm->ThreatMapWidth,ai->tm->ThreatMapHeight, myUnitReference->GetMoveType());
								float cost = ai->pather->ManeuverToPosRadius(&moveHere, myPos, enemyPos, myMinRange);
ai->math->StartTimer(ai->ah->ah_timer_totalTimeMinusPather);
								if (cost > 0) { //TODO: is this the right check at this point ?
									float dist = myPos.distance2D(moveHere);
									//is the position between the proposed destination and the enemy higher than the average of mine and his height?
									bool losHack = ((moveHere.y + enemyPos.y)/2.0f) + UNIT_MAX_MANEUVER_HEIGHT_DIFFERENCE_UP > ai->cb->GetElevation((moveHere.x+enemyPos.x)/2, (moveHere.z+enemyPos.z)/2);
									//im here assuming the pathfinder returns correct Y values
									//get threat info:
									float currentThreat = ai->tm->ThreatAtThisPoint(CheckCoordinates(myPos));
									float newThreat = ai->tm->ThreatAtThisPoint(CheckCoordinates(moveHere));
	//								if (newThreat <= currentThreat && dist > max((UNIT_MIN_MANEUVER_RANGE_PERCENTAGE*myRange), float(UNIT_MIN_MANEUVER_DISTANCE))
									if (newThreat < currentThreat && dist > max((UNIT_MIN_MANEUVER_RANGE_PERCENTAGE*myMinRange), float(UNIT_MIN_MANEUVER_DISTANCE))
										&& losHack) { //(enemyPos.y - moveHere.y) < UNIT_MAX_MANEUVER_HEIGHT_DIFFERENCE_UP) {
										debug2 = true;
										//Draw where it would move:
										if (ai->ah->debugDraw) ai->cb->CreateLineFigure(myPos, moveHere, 80, 1, frameSpread, RANDINT%1000000);
										myUnitReference->maneuverCounter = int(ceilf(max((float)UNIT_MIN_MANEUVER_TIME/frameSpread, (dist/myUnitDef->speed))));
	//									L("AG maneuver debug, maneuverCounter set to " << ai->MyUnits[unit]->maneuverCounter << " and cost:" << cost << " speed:" << ai->MyUnits[unit]->def()->speed << " frameSpread:" << frameSpread << " dist/speed:" << (dist/ai->MyUnits[unit]->def()->speed) << " its a " << ai->MyUnits[unit]->def()->humanName);
										myUnitReference->Move(moveHere);
									}
								}
								if (debug1 && !debug2) {
									//L("AG: maneuver: pathfinder run but path not used");
									//TODO: hack, and re-enable L()
									myUnitReference->maneuverCounter = 2;
								}
							}//endif we are not a plane && we want to manuver right now
						}// we exist and arent already involved in maneuvering
					}
				//} else { //there are enemies but my dps is lower than at in range of enemy (we're screwed)
					//Flee();
				}
			}
		}
//		L("AG: isShooting end");
	}
ai->math->StopTimer(ai->ah->ah_timer_MicroUpdate);
}

/*
TODO: make it support planes by:
1. letting speed determine how far ahead the maxStepsAhead is DONE
2. using priority-searches to enemy structures for fast GROUPS
will support bomb runs and also zipper runs (if group formation permits)
TODO2: make enemies rush to their real target and not stop to mess around with whatever they find (needs integration with micro)
*/
void CAttackGroup::MoveOrderUpdate(float3 groupPosition) {
ai->math->StartTimer(ai->ah->ah_timer_MoveOrderUpdate);
	int numUnits = this->units.size();
	int frameNr = ai->cb->GetCurrentFrame();
	int frameSpread = 60;
	if (!isShooting && isMoving && frameNr % frameSpread == 0 && pathToTarget.size() > 2) {
		//do the moving
		float distPerStep = pathToTarget[0].distance2D(pathToTarget[1]);
		const int maxStepsAhead = int(max(1.0f, ceilf(frameSpread/30 * this->highestUnitSpeed/distPerStep)));
		int pathMaxIndex = (int)pathToTarget.size()-1;
		int step1 = min(this->pathIterator + maxStepsAhead, pathMaxIndex);
		int step2 = min(this->pathIterator + maxStepsAhead*2, pathMaxIndex);
		//TODO: maybe add step 3 ? :P
		float3 moveToHereFirst = this->pathToTarget[step1];
		float3 moveToHere = this->pathToTarget[step2];
		//drawing destination
        //if we aint there yet
		if (groupPosition.distance2D(pathToTarget[pathMaxIndex]) > GROUP_DESTINATION_SLACK + this->groupPhysicalSize + this->highestTurnRadius) {
			//increase pathIterator
			pathIterator = 0;
			float3 tempAhead;
			tempAhead = pathToTarget[min(pathIterator + maxStepsAhead, pathMaxIndex)];
			float3 tempBehind;
			if (pathIterator - maxStepsAhead < 0) {
				tempBehind = pathToTarget[0];
			} else {
				tempBehind = pathToTarget[pathIterator - maxStepsAhead];
			}
			while(pathIterator < pathMaxIndex 
					&& groupPosition.distance2D(tempAhead) - this->groupPhysicalSize - this->highestTurnRadius < groupPosition.distance2D(tempBehind)) {
				pathIterator = min(pathIterator + maxStepsAhead, pathMaxIndex);
				tempAhead = pathToTarget[min(pathIterator + maxStepsAhead, pathMaxIndex)];
				if (pathIterator - maxStepsAhead < 0) {
					tempBehind = pathToTarget[0];
				} else {
					tempBehind = pathToTarget[pathIterator - maxStepsAhead];
				}
			}
			//check the threat at group pos and the threat at the next pos on the path, if its high, flee
			float threatCheck = max(ai->tm->ThreatAtThisPoint(CheckCoordinates(pathToTarget[pathIterator])), ai->tm->ThreatAtThisPoint(groupPos)) - ai->tm->GetUnmodifiedAverageThreat();
			if (!isFleeing && threatCheck > this->groupDPS*FLEE_MAX_THREAT_DIFFERENCE) {
				ai->math->StopTimer(ai->ah->ah_timer_MoveOrderUpdate);
				this->Flee();
				if (ai->ah->debug) L("CAttackGroup::MoveOrderUpdate caused group:"<<groupID<<" to flee");
				return;
			}
			//move the units
			for(int i = 0; i < numUnits; i++) {
				int unit = units[i];
				if (ai->cb->GetUnitDef(unit) != NULL) {
					//TODO: when they are near target, change this so they line up or something while some are here and some arent. attack checker will take over soon.
					//if the single unit aint there yet
					if (ai->cb->GetUnitPos(unit).distance2D(pathToTarget[pathMaxIndex]) > UNIT_DESTINATION_SLACK) {
//						ai->cb->CreateLineFigure(ai->cb->GetUnitPos(unit) + float3(0,50,0), moveToHereFirst + float3(0,50,0),15,1,10,groupID+50000);
						if (moveToHere != moveToHereFirst) {
							ai->MyUnits[unit]->MoveTwice(&moveToHereFirst, &moveToHere);
//							ai->cb->CreateLineFigure(moveToHereFirst + float3(0,50,0), moveToHere + float3(0,50,0),15,1,10,groupID+50000);
						} else {
							ai->MyUnits[unit]->Move(moveToHereFirst);
						}
						//draw thin line from unit to groupPos
						if (ai->ah->debugDraw) ai->cb->CreateLineFigure(ai->cb->GetUnitPos(unit) + float3(0,50,0), groupPosition + float3(0,50,0),4,0,frameSpread,groupID+500);
					}
				}
			}
		} else {
			if (ai->ah->debug) L("AG: group thinks it has arrived at the destination:" << groupID);
			this->ClearTarget();
		}
	}//endif move
ai->math->StopTimer(ai->ah->ah_timer_MoveOrderUpdate);
}

void CAttackGroup::StuckUnitFix() {
	int frameNr = ai->cb->GetCurrentFrame();
	int frameSpread = 300;
	if (isMoving && !isShooting && frameNr % frameSpread == 0) {
//		L("AG: unit stuck checking start");
		//stuck unit update. (at a 60 frame modulo)
		for (vector<int>::iterator it = units.begin(); it != units.end(); it++) {
			int unit = *it;
			if (ai->cb->GetUnitDef(unit) != NULL){
				CUNIT *u = ai->MyUnits[unit];
				//hack for slight maneuvering around buildings, spring fails at this sometimes
				if (u->stuckCounter >= UNIT_STUCK_COUNTER_MANEUVER_LIMIT && !u->attemptedUnstuck) {
					if (ai->ah->debug) L("AG: attempting unit unstuck manuevering for " << unit << " at pos " << u->pos().x << " " << u->pos().y << " " << u->pos().z);
					if (ai->ah->debugDraw) {
						AIHCAddMapPoint amp;
						amp.label = "stuck-fix";
						amp.pos = u->pos();
						ai->cb->HandleCommand(AIHCAddMapPointId, &amp);
					}
					//findbestpath to perifery of circle around mypos, distance 2x resolution or somesuch
					vector<float3> tempPath;
					float3 destination;
ai->math->StopTimer(ai->ah->ah_timer_totalTimeMinusPather);
					ai->pather->micropather->SetMapData(&ai->pather->canMoveIntMaskArray.front(),&ai->tm->ThreatArray.front(),ai->tm->ThreatMapWidth,ai->tm->ThreatMapHeight, this->GetWorstMoveType());
					float dist = ai->pather->ManeuverToPosRadius(&destination, u->pos(), u->pos(), UNIT_STUCK_MANEUVER_DISTANCE);
ai->math->StartTimer(ai->ah->ah_timer_totalTimeMinusPather);
					if (dist > 0) {
						if (ai->ah->debugDraw) ai->cb->CreateLineFigure(u->pos(), destination, 14, 1, frameSpread, RANDINT%1000000);
						u->Move(destination);
					}
					u->stuckCounter = 0;
					u->attemptedUnstuck = true;
				}
			}
		}//endfor unstuck
	}
}

void CAttackGroup::Update()
{
	int frameNr = ai->cb->GetCurrentFrame();
	int numUnits = (int)units.size();
	if(!numUnits) {
		if (ai->ah->debug) L("updated an empty AttackGroup!");
		return;
	}

//	int frameSpread; // variable used as a varying "constant" for determining how often a part of the update scheme is done
	float3 groupPosition = this->GetGroupPos();
	if(groupPosition == ERRORVECTOR) return;

	if (ai->ah->debugDraw) DrawGroupPosition();

	//sets the isShooting variable. - this part of the code checks for nearby enemies and does focus fire/maneuvering
	this->MicroUpdate(groupPosition);

	if (ai->ah->debugDraw && frameNr % 30 == 0 && !isDefending) {
		//drawing the target region:
//		L("AG update: drawing target region (attacking)");
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
			ai->cb->CreateLineFigure(circleHack[i], circleHack[(i+1)%8], 5, 0, 300, GetGroupID()+6000);
		}
		//from pos to circle center
		ai->cb->CreateLineFigure(groupPosition+float3(0,heightOffset,0), attackPosition+float3(0,heightOffset,0), 5, 1, 30, GetGroupID()+6000);
		ai->cb->SetFigureColor(GetGroupID() + 6000, 0, 0, 0, 1.0f);
//		L("AG update: done drawing stuff");
	}

	//if we're defending and we dont have a target, every 2 secs look for a target
//	if (defending && !isShooting && !isMoving && frameNr % 60 == groupID % 60) {
//		FindDefenseTarget(groupPosition);
//	}
	// GIVE MOVE ORDERS TO UNITS ALONG PATHTOTARGET
	this->MoveOrderUpdate(groupPosition);

	//stuck fix stuff.
	StuckUnitFix();
}
