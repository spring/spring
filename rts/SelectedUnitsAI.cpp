////////////////////////////////////////
//         CSelectedUnitsAI           //
// Group-AI. Handling commands given  //
// to the selected group of units.    //
// - Controlling formations.          //
////////////////////////////////////////

#include "StdAfx.h"
#include "SelectedUnitsAI.h"
#include "SelectedUnits.h"
#include "InfoConsole.h"
#include "Net.h"
#include "GlobalStuff.h"
#include "MoveType.h"
#include "UnitHandler.h"
#include "CommandAI.h"
#include "UnitDef.h"
//#include "GroundMoveType.h"
//#include "mmgr.h"

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
void CSelectedUnitsAI::GiveCommandNet(Command &c,int player) {
	int nbrOfSelectedUnits = selectedUnits.netSelected[player].size();

	if(nbrOfSelectedUnits < 1)			//No units to command.
		return;

	if(nbrOfSelectedUnits == 1) {	//Only one single unit selected.
		CUnit* unit=uh->units[*selectedUnits.netSelected[player].begin()];
		if(unit)
			unit->commandAI->GiveCommand(c);
		return;
	}


	if(c.id==CMD_MOVE && c.params.size()==6){
		MakeFrontMove(&c,player);

	} else if(c.id == CMD_MOVE && (c.options & ALT_KEY)) {
		CalculateGroupData(player);
		
		float3 pos(c.params[0],c.params[1],c.params[2]);
		float3 frontdir=pos-centerCoor;										//use the vector from the middle of group to new pos as forward dir
		frontdir.y=0;
		frontdir.Normalize();

		float3 sideDir=frontdir.cross(UpVector);

		float length=100+sqrtf(nbrOfSelectedUnits)*32;		//calculate so that the units form in an aproximate square

		c.params.push_back(pos.x+sideDir.x*length);				//push back some extra params so it confer with a front move
		c.params.push_back(pos.y+sideDir.y*length);
		c.params.push_back(pos.z+sideDir.z*length);

		MakeFrontMove(&c,player);
		
	} else if((c.id == CMD_MOVE || c.id == CMD_PATROL)&& (c.options & CONTROL_KEY)) {
		CalculateGroupData(player);
		for(vector<int>::iterator ui = selectedUnits.netSelected[player].begin(); ui != selectedUnits.netSelected[player].end(); ++ui) {
			CUnit* unit=uh->units[*ui];
			if(unit){
				//Sets the wanted speed of this unit (and the group).
				Command uc;
				uc.id = CMD_SET_WANTED_MAX_SPEED;
				uc.options = c.options;
				if(c.options & ALT_KEY)		//ALT overrides group-speed-setting.
					uc.params.push_back(unit->moveType->maxSpeed);
				else
					uc.params.push_back(minMaxSpeed);
				unit->commandAI->GiveCommand(uc);
				//Modifying the destination relative to the center of the group.
				uc = c;
				uc.params[CMDPARAM_MOVE_X] += unit->midPos.x - centerCoor.x;
				uc.params[CMDPARAM_MOVE_Y] += unit->midPos.y - centerCoor.y;
				uc.params[CMDPARAM_MOVE_Z] += unit->midPos.z - centerCoor.z;
				unit->commandAI->GiveCommand(uc);
			}
		}
	} else {
		for(vector<int>::iterator ui = selectedUnits.netSelected[player].begin(); ui != selectedUnits.netSelected[player].end(); ++ui) {
			CUnit* unit=uh->units[*ui];
			if(unit){
				unit->commandAI->GiveCommand(c);
			}
		}
	}
}


/*
Cacluate the outer limits and the center of the group coordinates.
*/
void CSelectedUnitsAI::CalculateGroupData(int player) {
	//Finding the highest, lowest and weighted central positional coordinates among the selected units.
	float3 sumCoor = minCoor = maxCoor = float3(0, 0, 0);
	float3 mobileSumCoor = sumCoor;
	int mobileUnits = 0;
	minMaxSpeed = 1e9;
	for(vector<int>::iterator ui = selectedUnits.netSelected[player].begin(); ui != selectedUnits.netSelected[player].end(); ++ui) {
		CUnit* unit=uh->units[*ui];
		if(unit){
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
			if(unit->mobility && !unit->mobility->canFly) {
				mobileUnits++;
				mobileSumCoor += unitPos;
				if(unit->mobility->maxSpeed < minMaxSpeed) {
					minMaxSpeed = unit->mobility->maxSpeed;
				}
			}
		}
	}
	//Weighted center
	if(mobileUnits > 0)
		centerCoor = mobileSumCoor / mobileUnits;
	else
		centerCoor = sumCoor / selectedUnits.netSelected[player].size();
}

void CSelectedUnitsAI::MakeFrontMove(Command* c,int player)
{
	float3 centerPos(c->params[0],c->params[1],c->params[2]);
	float3 rightPos(c->params[3],c->params[4],c->params[5]);

	if(centerPos.distance(rightPos)<selectedUnits.netSelected[player].size()){	//treat this as a standard move if the front isnt long enough 
		for(vector<int>::iterator ui = selectedUnits.netSelected[player].begin(); ui != selectedUnits.netSelected[player].end(); ++ui) {
			CUnit* unit=uh->units[*ui];
			if(unit){
				unit->commandAI->GiveCommand(*c);
			}
		}		
		return;
	}

	float frontLength=centerPos.distance(rightPos)*2;
	sideDir=centerPos-rightPos;
	sideDir.y=0;
	sideDir.Normalize();
	frontDir=sideDir.cross(float3(0,1,0));

	numColumns=(int)(frontLength/columnDist);

	int positionsUsed=0;

	std::multimap<float,int> orderedUnits;
	CreateUnitOrder(orderedUnits,player);

	for(multimap<float,int>::iterator oi=orderedUnits.begin();oi!=orderedUnits.end();++oi){
		MoveToPos(oi->second,centerPos,positionsUsed++,c->options);		
	}
}

void CSelectedUnitsAI::MoveToPos(int unit, float3& basePos, int posNum,unsigned char options)
{
	int lineNum=posNum/numColumns;
	int colNum=posNum-lineNum*numColumns;
	float side=(0.25f+colNum*0.5f)*columnDist*(colNum&1 ? -1:1);

	float3 pos=basePos-frontDir*(float)lineNum*lineDist+sideDir*side;

	Command c;
	c.id=CMD_MOVE;
	c.params.push_back(pos.x);
	c.params.push_back(pos.y);
	c.params.push_back(pos.z);
	c.options=0;

	uh->units[unit]->commandAI->GiveCommand(c);
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
