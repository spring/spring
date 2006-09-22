////////////////////////////////////////
//         CSelectedUnitsAI           //
// Group-AI. Handling commands given  //
// to the selected group of units.    //
// - Controlling formations.          //
////////////////////////////////////////

#include "StdAfx.h"
#include "SelectedUnitsAI.h"
#include "SelectedUnits.h"
#include "LogOutput.h"
#include "Net.h"
#include "GlobalStuff.h"
#include "Sim/MoveTypes/MoveType.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/UnitDef.h"
//#include "GroundMoveType.h"
#include "mmgr.h"

const int CMDPARAM_MOVE_X = 0;
const int CMDPARAM_MOVE_Y = 1;
const int CMDPARAM_MOVE_Z = 2;


//Global object
CSelectedUnitsAI selectedUnitsAI;

CSelectedUnitsAI::CSelectedUnitsAI()
{
	columnDist=64;
	lineDist=64;
}

/*
Group-AI.
Processing commands given to selected group.
*/


/*
void CSelectedUnitsAI::AddUnit(int unit)
{
	myUnits.insert(unit);
	unitsChanged=true;
	return true;				
}


void CSelectedUnitsAI::RemoveUnit(int unit)
{
	myUnits.erase(unit);
	unitsChanged=true;
}
*/


inline void CSelectedUnitsAI::AddUnitSetMaxSpeedCommand(CUnit* unit,
                                                        unsigned char options)
{
	// sets the wanted speed of this unit to its max speed
	CCommandAI* cai = unit->commandAI;
	if (cai->CanSetMaxSpeed()) {
		UnitDef* ud = unit->unitDef;
		Command c;
		c.id = CMD_SET_WANTED_MAX_SPEED;
		c.options = options;
		c.params.push_back(ud->speed / 30.0f);
		cai->GiveCommand(c);
	}
}


inline void CSelectedUnitsAI::AddGroupSetMaxSpeedCommand(CUnit* unit,
                                                         unsigned char options)
{
	// sets the wanted speed of this unit to the group minimum
	CCommandAI* cai = unit->commandAI;
	if (cai->CanSetMaxSpeed()) {
		Command c;
		c.id = CMD_SET_WANTED_MAX_SPEED;
		c.options = options;
		c.params.push_back(minMaxSpeed / 30.0f);
		cai->GiveCommand(c);
	}
}


void CSelectedUnitsAI::GiveCommandNet(Command &c,int player)
{
	const vector<int>& netSelected = selectedUnits.netSelected[player];
	vector<int>::const_iterator ui;
	
	int nbrOfSelectedUnits = netSelected.size();

	if (nbrOfSelectedUnits < 1) {
		// no units to command
		return;
	}

	if(nbrOfSelectedUnits == 1) {
		// a single unit selected
		CUnit* unit = uh->units[*netSelected.begin()];
		if(unit) {
			unit->commandAI->GiveCommand(c);
			AddUnitSetMaxSpeedCommand(unit, c.options);
		}
		return;
	}
	
	// User Move Front Command:
	//
	//   CTRL:      Group Front/Speed  command
	//
	// User Move Command:
	//
	//   ALT:       Group Front        command
	//   ALT+CTRL:  Group Front/Speed  command
	//   CTRL:      Group Locked/Speed command  (maintain relative positions)
	//
	// User Patrol and Fight Commands:
	//
	//   CTRL:      Group Locked/Speed command  (maintain relative positions)
	//   ALT+CTRL:  Group Locked       command  (maintain relative positions)
	//

	if ((c.id == CMD_MOVE) && (c.params.size() == 6)) {
		CalculateGroupData(player);

		MakeFrontMove(&c, player);

		const bool groupSpeed = (c.options & CONTROL_KEY);
		for(ui = netSelected.begin(); ui != netSelected.end(); ++ui) {
			CUnit* unit = uh->units[*ui];
			if(unit){
				if (groupSpeed) {
					AddGroupSetMaxSpeedCommand(unit, c.options);
				} else {
					AddUnitSetMaxSpeedCommand(unit, c.options);
				}
			}
		}
	}
	else if ((c.id == CMD_MOVE) && (c.options & ALT_KEY)) {
		CalculateGroupData(player);
		
		// use the vector from the middle of group to new pos as forward dir
		const float3 pos(c.params[0], c.params[1], c.params[2]);
		float3 frontdir = pos - centerCoor; 
		frontdir.y = 0.0f;
		frontdir.Normalize();
		const float3 sideDir = frontdir.cross(UpVector);
		
		// calculate so that the units form in an aproximate square
		float length = 100.0f + (sqrt((float)nbrOfSelectedUnits) * 32.0f);

		// push back some extra params so it confer with a front move
		c.params.push_back(pos.x + (sideDir.x * length));
		c.params.push_back(pos.y + (sideDir.y * length));
		c.params.push_back(pos.z + (sideDir.z * length));

		MakeFrontMove(&c, player);

		const bool groupSpeed = (c.options & CONTROL_KEY);
		for(ui = netSelected.begin(); ui != netSelected.end(); ++ui) {
			CUnit* unit = uh->units[*ui];
			if(unit){
				if (groupSpeed) {
					AddGroupSetMaxSpeedCommand(unit, c.options);
				} else {
					AddUnitSetMaxSpeedCommand(unit, c.options);
				}
			}
		}
	}
	else if ((c.id == CMD_MOVE || c.id == CMD_PATROL || c.id == CMD_FIGHT) && (c.options & CONTROL_KEY)) {
		CalculateGroupData(player);

		const bool groupSpeed = !(c.options & ALT_KEY);
		for (ui = netSelected.begin(); ui != netSelected.end(); ++ui) {
			CUnit* unit = uh->units[*ui];
			if (unit) {
				// Modifying the destination relative to the center of the group
				Command uc = c;
				uc.params[CMDPARAM_MOVE_X] += unit->midPos.x - centerCoor.x;
				uc.params[CMDPARAM_MOVE_Y] += unit->midPos.y - centerCoor.y;
				uc.params[CMDPARAM_MOVE_Z] += unit->midPos.z - centerCoor.z;
				unit->commandAI->GiveCommand(uc);
				if (groupSpeed) {
					AddGroupSetMaxSpeedCommand(unit, c.options);
				} else {
					AddUnitSetMaxSpeedCommand(unit, c.options);
				}
			}
		}
	}
	else {
		for (ui = netSelected.begin(); ui != netSelected.end(); ++ui) {
			CUnit* unit = uh->units[*ui];
			if (unit) {
				// appending a CMD_SET_WANTED_MAX_SPEED command to
				// every command is a little bit wasteful, n'est pas?
				unit->commandAI->GiveCommand(c);
				AddUnitSetMaxSpeedCommand(unit, c.options);
			}
		}
	}
}


//
// Calculate the outer limits and the center of the group coordinates.
//
void CSelectedUnitsAI::CalculateGroupData(int player) {
	//Finding the highest, lowest and weighted central positional coordinates among the selected units.
	float3 sumCoor = minCoor = maxCoor = float3(0, 0, 0);
	float3 mobileSumCoor = sumCoor;
	sumLength = 0; ///
	int mobileUnits = 0;
	minMaxSpeed = 1e9f;
	for(vector<int>::iterator ui = selectedUnits.netSelected[player].begin(); ui != selectedUnits.netSelected[player].end(); ++ui) {
		CUnit* unit=uh->units[*ui];
		if(unit){
			UnitDef* ud=unit->unitDef;
			sumLength += (int)((ud->xsize + ud->ysize)/2);

			float3 unitPos = unit->midPos;
			if(unitPos.x < minCoor.x)
				minCoor.x = unitPos.x;
			else if(unitPos.x > maxCoor.x)
				maxCoor.x = unitPos.x;
			if(unitPos.y < minCoor.y)
				minCoor.y = unitPos.y;
			else if(unitPos.y > maxCoor.y)
				maxCoor.y = unitPos.y;
			if(unitPos.z < minCoor.z)
				minCoor.z = unitPos.z;
			else if(unitPos.z > maxCoor.z)
				maxCoor.z = unitPos.z;
			sumCoor += unitPos;
			if(unit->commandAI->CanSetMaxSpeed()) {
				mobileUnits++;
				mobileSumCoor += unitPos;
				const float maxSpeed = unit->unitDef->speed;
				if(maxSpeed < minMaxSpeed) {
					minMaxSpeed = maxSpeed;
				}
			}
		}
	}
	avgLength = sumLength/selectedUnits.netSelected[player].size();
	//Weighted center
	if(mobileUnits > 0)
		centerCoor = mobileSumCoor / mobileUnits;
	else
		centerCoor = sumCoor / selectedUnits.netSelected[player].size();
}


void CSelectedUnitsAI::MakeFrontMove(Command* c,int player)
{
	centerPos.x=c->params[0];
	centerPos.y=c->params[1];
	centerPos.z=c->params[2];
	rightPos.x=c->params[3];
	rightPos.y=c->params[4];
	rightPos.z=c->params[5];

	float3 nextPos(0.0f, 0.0f, 0.0f);//it's in "front" coordinates (rotated to real, moved by rightPos)

	if(centerPos.distance(rightPos)<selectedUnits.netSelected[player].size()+33){	//Strange line! treat this as a standard move if the front isnt long enough 
		for(vector<int>::iterator ui = selectedUnits.netSelected[player].begin(); ui != selectedUnits.netSelected[player].end(); ++ui) {
			CUnit* unit=uh->units[*ui];
			if(unit){
				unit->commandAI->GiveCommand(*c);
			}
		}		
		return;
	}

	frontLength=centerPos.distance(rightPos)*2;
	addSpace = 0;
	if(frontLength > sumLength*2*8) addSpace = (frontLength - sumLength*2*8)/selectedUnits.netSelected[player].size();
	sideDir=centerPos-rightPos;
	sideDir.y=0;
	float3 sd = sideDir;
	sd.y=frontLength/2;
	sideDir.Normalize();
	frontDir=sideDir.cross(float3(0,1,0));

	numColumns=(int)(frontLength/columnDist);
	if(numColumns==0)
		numColumns=1;

	int positionsUsed=0;

	std::multimap<float,int> orderedUnits;
	CreateUnitOrder(orderedUnits,player);
	
	for(multimap<float,int>::iterator oi=orderedUnits.begin();oi!=orderedUnits.end();++oi){	

		
		nextPos = MoveToPos(oi->second,nextPos,sd,c->options);	

	}
}


float3 CSelectedUnitsAI::MoveToPos(int unit, float3 nextCornerPos, float3 dir, unsigned char options)
{
	//int lineNum=posNum/numColumns;
	//int colNum=posNum-lineNum*numColumns;
	//float side=(0.25f+colNum*0.5f)*columnDist*(colNum&1 ? -1:1);

	if(nextCornerPos.x-addSpace>frontLength) {
		nextCornerPos.x=0; nextCornerPos.z-=avgLength*2*8;
	}
	int unitSize=16;
	CUnit* u=uh->units[unit];
		if(u){
			UnitDef* ud=u->unitDef;
			unitSize = (int)((ud->xsize + ud->ysize)/2);
		}
	float3 retPos(nextCornerPos.x+unitSize*8*2+addSpace, 0, nextCornerPos.z);
	float3 movePos(nextCornerPos.x+unitSize*8+addSpace, 0, nextCornerPos.z); //posit in coordinates of "front"
	if(nextCornerPos.x==0) { movePos.x=unitSize*8; retPos.x -= addSpace ;}
	float3 pos(rightPos.x + movePos.x*(dir.x/dir.y) - movePos.z*(dir.z/dir.y), 0, rightPos.z + movePos.x*(dir.z/dir.y) + movePos.z*(dir.x/dir.y));//posit in real coord

	Command c;
	c.id=CMD_MOVE;
	c.params.push_back(pos.x);
	c.params.push_back(pos.y);
	c.params.push_back(pos.z);
	c.options=options;

	uh->units[unit]->commandAI->GiveCommand(c);
	return retPos;
}


void CSelectedUnitsAI::CreateUnitOrder(std::multimap<float,int>& out,int player)
{
	for(vector<int>::iterator ui=selectedUnits.netSelected[player].begin();ui!=selectedUnits.netSelected[player].end();++ui){
		CUnit* unit=uh->units[*ui];
		if(unit){
			UnitDef* ud=unit->unitDef;
			float range=unit->maxRange;
			if(range<1)
				range=2000;		//give weaponless units a long range to make them go to the back
			float value=(ud->metalCost*60+ud->energyCost)/unit->maxHealth*range;
			out.insert(pair<float,int>(value,*ui));
		}
	}
}


void CSelectedUnitsAI::Update()
{
}
