#ifndef SELECTED_UNITS_H
#define SELECTED_UNITS_H

#include "command.h"
#include "Sim/Units/Unit.h"
#include "float3.h"
#include <map>
#include <set>

class CSelectedUnitsAI {
public:
	/* set<int> selUnits;
	
	void AddUnit(int unit); (And include update() in game.cpp to call every frame)
	void RemoveUnit(int unit);
	*/

	CSelectedUnitsAI();
	void GiveCommandNet(Command &c,int player);
	float3 centerPos, rightPos;
	int sumLength;
	float avgLength;
	float frontLength;
	float addSpace;

	void Update();

private:
	void CalculateGroupData(int player);
	void MakeFrontMove(Command* c,int player);
	void CreateUnitOrder(std::multimap<float,int>& out,int player);
	float3 MoveToPos(int unit, float3 nextCornerPos, float3 dir,unsigned char options);
	void AddUnitSetMaxSpeedCommand(CUnit* unit, unsigned char options);
	void AddGroupSetMaxSpeedCommand(CUnit* unit, unsigned char options);
	
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
