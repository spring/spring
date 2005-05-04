#ifndef SELECTED_UNITS_H

#define SELECTED_UNITS_H



#include "command.h"

#include "Unit.h"

#include "float3.h"



class CSelectedUnitsAI {

public:

	void GiveCommandNet(Command &c,int player);



private:

	void CalculateGroupData(int player);



	float3 minCoor, maxCoor, centerCoor;

	float minMaxSpeed;

};



extern CSelectedUnitsAI selectedUnitsAI;



#endif
