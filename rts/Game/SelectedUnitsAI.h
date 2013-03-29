/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SELECTED_UNITS_AI_H
#define SELECTED_UNITS_AI_H

#include "Sim/Units/CommandAI/Command.h"
#include "System/float3.h"
#include <map>
#include <set>

class CUnit;


/// Handling commands given to the currently  selected group of units.
class CSelectedUnitsHandlerAI {
public:
	/* set<int> selUnits;

	void AddUnit(int unit); (And include update() in game.cpp to call every frame)
	void RemoveUnit(int unit);
	*/

	CSelectedUnitsHandlerAI();
	void GiveCommandNet(Command& c, int player);

	float3 centerPos;
	float3 rightPos;
	int sumLength;
	float avgLength;
	float frontLength;
	float addSpace;

private:
	void CalculateGroupData(int player, bool queueing);
	void MakeFrontMove(Command* c, int player);
	void CreateUnitOrder(std::multimap<float, int>& out, int player);
	float3 MoveToPos(int unit, float3 nextCornerPos, float3 dir, Command* command, std::vector<std::pair<int, Command> >* frontcmds, bool* newline);
	void AddUnitSetMaxSpeedCommand(CUnit* unit, unsigned char options);
	void AddGroupSetMaxSpeedCommand(CUnit* unit, unsigned char options);
	void SelectAttack(const Command& cmd, int player);
	void SelectCircleUnits(const float3& pos, float radius, std::vector<int>& units, int player);
	void SelectRectangleUnits(const float3& pos0, const float3& pos1, std::vector<int>& units, int player);
	float3 LastQueuePosition(const CUnit* unit);

	float3 minCoor, maxCoor, centerCoor;
	float minMaxSpeed;

	float3 frontDir;
	float3 sideDir;
	float columnDist;
	float lineDist;
	int numColumns;
};

extern CSelectedUnitsHandlerAI selectedUnitsAI;

#endif // SELECTED_UNITS_AI_H
