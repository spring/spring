#ifndef SELECTED_UNITS_H
#define SELECTED_UNITS_H

#include "command.h"
#include "Sim/Units/Unit.h"
#include "float3.h"
#include <map>

class CSelectedUnitsAI {
public:
	CSelectedUnitsAI();
	void GiveCommandNet(Command &c,int player);

private:
	void CalculateGroupData(int player);

	void MakeFrontMove(Command* c,int player);
	void CreateUnitOrder(std::multimap<float,int>& out,int player);
	void MoveToPos(int unit, float3& basePos, int posNum,unsigned char options);

	float3 minCoor, maxCoor, centerCoor;
	float minMaxSpeed;

	float3 frontDir;
	float3 sideDir;
	float columnDist;
	float lineDist;
	int numColumns;
};

extern CSelectedUnitsAI selectedUnitsAI;

#endif
