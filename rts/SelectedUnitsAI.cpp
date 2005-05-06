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

//#include "GroundMoveType.h"

//#include "mmgr.h"



const int CMDPARAM_MOVE_X = 0;

const int CMDPARAM_MOVE_Y = 1;

const int CMDPARAM_MOVE_Z = 2;





//Global object

CSelectedUnitsAI selectedUnitsAI;





/*

Group-AI.

Processing commands given to selected group.

*/

void CSelectedUnitsAI::GiveCommandNet(Command &c,int player) {

	int nbrOfSelectedUnits = selectedUnits.netSelected[player].size();



	if(nbrOfSelectedUnits < 1)			//No units to command.

		return;



	else if(nbrOfSelectedUnits == 1) {	//Only one single unit selected.

		CUnit* unit=uh->units[*selectedUnits.netSelected[player].begin()];

		if(unit)

			unit->commandAI->GiveCommand(c);

	}



	else {								//Several units selected.

		if((c.id == CMD_MOVE || c.id == CMD_PATROL)&& (c.options & CONTROL_KEY)) {

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