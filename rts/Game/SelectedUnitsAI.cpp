/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SelectedUnitsAI.h"

#include "SelectedUnitsHandler.h"
#include "GlobalUnsynced.h"
#include "WaitCommandsAI.h"
#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"
#include "Map/Ground.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/MoveTypes/MoveType.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Net/Protocol/NetProtocol.h"

const int CMDPARAM_MOVE_X = 0;
const int CMDPARAM_MOVE_Y = 1;
const int CMDPARAM_MOVE_Z = 2;


// Global object
CSelectedUnitsHandlerAI selectedUnitsAI;

CSelectedUnitsHandlerAI::CSelectedUnitsHandlerAI()
	: centerPos(ZeroVector)
	, rightPos(ZeroVector)
	, sumLength(0)
	, avgLength(0.0f)
	, frontLength(0.0f)
	, addSpace(0.0f)
	, minCoor(ZeroVector)
	, maxCoor(ZeroVector)
	, centerCoor(ZeroVector)
	, minMaxSpeed(0.0f)

	, frontDir(ZeroVector)
	, sideDir(ZeroVector)
	, columnDist(64.0f)
	, numColumns(0)
{
}


inline void CSelectedUnitsHandlerAI::AddUnitSetMaxSpeedCommand(CUnit* unit,
                                                        unsigned char options)
{
	// this sets the WANTED maximum speed of <unit>
	// (via the CommandAI --> MoveType chain) to be
	// equal to its current ACTUAL maximum (not the
	// UnitDef maximum, which can be overridden by
	// scripts)
	CCommandAI* cai = unit->commandAI;

	if (cai->CanSetMaxSpeed()) {
		Command c(CMD_SET_WANTED_MAX_SPEED, options, unit->moveType->GetMaxSpeed());

		cai->GiveCommand(c, false);
	}
}

inline void CSelectedUnitsHandlerAI::AddGroupSetMaxSpeedCommand(CUnit* unit,
                                                         unsigned char options)
{
	// sets the wanted speed of this unit to that of
	// the group's current-slowest member (minMaxSpeed
	// is derived from GetMaxSpeed, not GetMaxSpeedDef)
	CCommandAI* cai = unit->commandAI;

	if (cai->CanSetMaxSpeed()) {
		Command c(CMD_SET_WANTED_MAX_SPEED, options, minMaxSpeed);

		cai->GiveCommand(c, false);
	}
}


static inline bool MayRequireSetMaxSpeedCommand(const Command &c)
{
	switch (c.GetID()) {
		// this is not a complete list
		case CMD_STOP:
		case CMD_WAIT:
		case CMD_SELFD:
		case CMD_FIRE_STATE:
		case CMD_MOVE_STATE:
		case CMD_ONOFF:
		case CMD_REPEAT: {
			return false;
		}
	}
	return true;
}

void CSelectedUnitsHandlerAI::GiveCommandNet(Command &c, int player)
{
	const std::vector<int>& netSelected = selectedUnitsHandler.netSelected[player];
	std::vector<int>::const_iterator ui;

	const int nbrOfSelectedUnits = netSelected.size();
	const int cmd_id = c.GetID();

	if (nbrOfSelectedUnits < 1) {
		// no units to command
	}
	else if ((cmd_id == CMD_ATTACK) && (
			(c.GetParamsCount() == 6) ||
			((c.GetParamsCount() == 4) && (c.GetParam(3) > 0.001f))
		))
	{
		SelectAttack(c, player);
	}
	else if (nbrOfSelectedUnits == 1) {
		// a single unit selected
		CUnit* unit = unitHandler->units[*netSelected.begin()];
		if (unit) {
			unit->commandAI->GiveCommand(c, true);
			if (MayRequireSetMaxSpeedCommand(c)) {
				AddUnitSetMaxSpeedCommand(unit, c.options);
			}
			if (cmd_id == CMD_WAIT) {
				if (player == gu->myPlayerNum) {
					waitCommandsAI.AcknowledgeCommand(c);
				}
			}
		}
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

	else if (((cmd_id == CMD_MOVE) || (cmd_id == CMD_FIGHT)) && (c.GetParamsCount() == 6)) {
		CalculateGroupData(player, !!(c.options & SHIFT_KEY));

		MakeFrontMove(&c, player);

		const bool groupSpeed = !!(c.options & CONTROL_KEY);
		for(ui = netSelected.begin(); ui != netSelected.end(); ++ui) {
			CUnit* unit = unitHandler->units[*ui];
			if (unit) {
				if (groupSpeed) {
					AddGroupSetMaxSpeedCommand(unit, c.options);
				} else {
					AddUnitSetMaxSpeedCommand(unit, c.options);
				}
			}
		}
	}
	else if ((cmd_id == CMD_MOVE) && (c.options & ALT_KEY)) {
		CalculateGroupData(player, !!(c.options & SHIFT_KEY));

		// use the vector from the middle of group to new pos as forward dir
		const float3 pos(c.GetParam(0), c.GetParam(1), c.GetParam(2));
		float3 frontdir = pos - centerCoor;
		frontdir.y = 0.0f;
		frontdir.ANormalize();
		const float3 sideDir = frontdir.cross(UpVector);

		// calculate so that the units form in an aproximate square
		float length = 100.0f + (math::sqrt((float)nbrOfSelectedUnits) * 32.0f);

		// push back some extra params so it confer with a front move
		c.PushPos(pos + (sideDir * length));

		MakeFrontMove(&c, player);

		const bool groupSpeed = !!(c.options & CONTROL_KEY);
		for(ui = netSelected.begin(); ui != netSelected.end(); ++ui) {
			CUnit* unit = unitHandler->units[*ui];
			if (unit) {
				if (groupSpeed) {
					AddGroupSetMaxSpeedCommand(unit, c.options);
				} else {
					AddUnitSetMaxSpeedCommand(unit, c.options);
				}
			}
		}
	}
	else if ((c.options & CONTROL_KEY) &&
	         ((cmd_id == CMD_MOVE) || (cmd_id == CMD_PATROL) || (cmd_id == CMD_FIGHT))) {
		CalculateGroupData(player, !!(c.options & SHIFT_KEY));

		const bool groupSpeed = !(c.options & ALT_KEY);

		for (ui = netSelected.begin(); ui != netSelected.end(); ++ui) {
			CUnit* unit = unitHandler->units[*ui];
			if (unit) {
				// Modifying the destination relative to the center of the group
				Command uc = c;
				float3 midPos;
				if (c.options & SHIFT_KEY) {
					midPos = LastQueuePosition(unit);
				} else {
					midPos = unit->midPos;
				}
				uc.params[CMDPARAM_MOVE_X] += midPos.x - centerCoor.x;
				uc.params[CMDPARAM_MOVE_Y] += midPos.y - centerCoor.y;
				uc.params[CMDPARAM_MOVE_Z] += midPos.z - centerCoor.z;
				unit->commandAI->GiveCommand(uc, false);

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
			CUnit* unit = unitHandler->units[*ui];
			if (unit) {
				// appending a CMD_SET_WANTED_MAX_SPEED command to
				// every command is a little bit wasteful, n'est pas?
				unit->commandAI->GiveCommand(c, false);
				if (MayRequireSetMaxSpeedCommand(c)) {
					AddUnitSetMaxSpeedCommand(unit, c.options);
				}
			}
		}
		if (cmd_id == CMD_WAIT) {
			if (player == gu->myPlayerNum) {
				waitCommandsAI.AcknowledgeCommand(c);
			}
		}
	}
}


//
// Calculate the outer limits and the center of the group coordinates.
//
void CSelectedUnitsHandlerAI::CalculateGroupData(int player, bool queueing) {
	//Finding the highest, lowest and weighted central positional coordinates among the selected units.
	float3 sumCoor, minCoor, maxCoor;
	float3 mobileSumCoor = sumCoor;
	sumLength = 0; ///
	int mobileUnits = 0;
	minMaxSpeed = 1e9f;
	for(std::vector<int>::iterator ui = selectedUnitsHandler.netSelected[player].begin(); ui != selectedUnitsHandler.netSelected[player].end(); ++ui) {
		CUnit* unit=unitHandler->units[*ui];
		if(unit){
			const UnitDef* ud = unit->unitDef;
			sumLength += (int)((ud->xsize + ud->zsize)/2);

			float3 unitPos;
			if (queueing) {
				unitPos = LastQueuePosition(unit);
			} else {
				unitPos = unit->midPos;
			}
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

			if (unit->commandAI->CanSetMaxSpeed()) {
				mobileUnits++;
				mobileSumCoor += unitPos;

				const float maxSpeed = unit->moveType->GetMaxSpeed();

				if (maxSpeed < minMaxSpeed) {
					minMaxSpeed = maxSpeed;
				}
			}
		}
	}
	avgLength = sumLength/selectedUnitsHandler.netSelected[player].size();
	//Weighted center
	if(mobileUnits > 0)
		centerCoor = mobileSumCoor / mobileUnits;
	else
		centerCoor = sumCoor / selectedUnitsHandler.netSelected[player].size();
}


void CSelectedUnitsHandlerAI::MakeFrontMove(Command* c,int player)
{
	centerPos.x=c->params[0];
	centerPos.y=c->params[1];
	centerPos.z=c->params[2];
	rightPos.x=c->params[3];
	rightPos.y=c->params[4];
	rightPos.z=c->params[5];

	// in "front" coordinates (rotated to real, moved by rightPos)
	float3 nextPos;

	if(centerPos.distance(rightPos)<selectedUnitsHandler.netSelected[player].size()+33){	//Strange line! treat this as a standard move if the front isnt long enough
		for(std::vector<int>::iterator ui = selectedUnitsHandler.netSelected[player].begin(); ui != selectedUnitsHandler.netSelected[player].end(); ++ui) {
			CUnit* unit=unitHandler->units[*ui];
			if(unit){
				unit->commandAI->GiveCommand(*c, false);
			}
		}
		return;
	}

	frontLength=centerPos.distance(rightPos)*2;
	addSpace = 0;
	if (frontLength > sumLength*2*8) {
		addSpace = (frontLength - sumLength*2*8)/(selectedUnitsHandler.netSelected[player].size() - 1);
	}
	sideDir=centerPos-rightPos;
	sideDir.y=0;
	float3 sd = sideDir;
	sd.y=frontLength/2;
	sideDir.ANormalize();
	frontDir=sideDir.cross(UpVector);

	numColumns=(int)(frontLength/columnDist);
	if(numColumns==0)
		numColumns=1;

	std::multimap<float,int> orderedUnits;
	CreateUnitOrder(orderedUnits,player);

	std::multimap<float, std::vector<int> > sortUnits;
	std::vector<std::pair<int,Command> > frontcmds;
	for (std::multimap<float,int>::iterator oi = orderedUnits.begin(); oi != orderedUnits.end();){
		bool newline;
		nextPos = MoveToPos(oi->second, nextPos, sd, c, &frontcmds, &newline);
		// mix units in each row to avoid weak flanks consisting solely of e.g. artillery units
		std::multimap<float, std::vector<int> >::iterator si = sortUnits.find(oi->first);
		if(si == sortUnits.end())
			si = sortUnits.insert(std::pair<float, std::vector<int> >(oi->first, std::vector<int>()));
		si->second.push_back(oi->second);
		++oi;

		if(oi != orderedUnits.end())
			MoveToPos(oi->second, nextPos, sd, c, NULL, &newline);

		if(oi == orderedUnits.end() || newline) {
			std::vector<std::vector<int>*> sortUnitsVector;
			for (std::multimap<float, std::vector<int> >::iterator ui = sortUnits.begin(); ui!=sortUnits.end(); ++ui) {
				sortUnitsVector.push_back(&(*ui).second);
			}

			std::vector<int> randunits;
			int sz = sortUnitsVector.size();
			std::vector<int> sortPos(sz, 0);
			for (std::vector<std::pair<int,Command> >::iterator fi = frontcmds.begin(); fi != frontcmds.end(); ++fi) {
				int bestpos = 0;
				float bestval = 1.0f;
				for (int i = 0; i < sz; ++i) {
					const int n = sortUnitsVector[i]->size();
					const int k = sortPos[i];
					if (k < n) {
						const float val = (0.5f + k) / (float)n;
						if (val < bestval) {
							bestval = val;
							bestpos = i;
						}
					}
				}
				int pos = sortPos[bestpos];
				randunits.push_back((*sortUnitsVector[bestpos])[pos]);
				sortPos[bestpos] = pos + 1;
			}
			sortUnits.clear();

			int i = 0;
			for (std::vector<std::pair<int,Command> >::iterator fi = frontcmds.begin(); fi != frontcmds.end(); ++fi) {
				unitHandler->units[randunits[i++]]->commandAI->GiveCommand(fi->second, false);
			}
			frontcmds.clear();
		}
	}
}


void CSelectedUnitsHandlerAI::CreateUnitOrder(std::multimap<float,int>& out,int player)
{
	const std::vector<int>& netUnits = selectedUnitsHandler.netSelected[player];

	for (auto ui = netUnits.cbegin(); ui != netUnits.cend(); ++ui) {
		const CUnit* unit = unitHandler->units[*ui];

		if (unit) {
			const UnitDef* ud = unit->unitDef;
			float range = unit->maxRange;
			if (range < 1) {
				// give weaponless units a long range to make them go to the back
				range = 2000;
			}
			const float value = ((ud->metal * 60) + ud->energy) / unit->unitDef->health * range;
			out.insert(std::pair<float, int>(value, *ui));
		}
	}
}


float3 CSelectedUnitsHandlerAI::MoveToPos(int unit, float3 nextCornerPos, float3 dir, Command* command, std::vector<std::pair<int,Command> >* frontcmds, bool* newline)
{
	//int lineNum=posNum/numColumns;
	//int colNum=posNum-lineNum*numColumns;
	//float side=(0.25f+colNum*0.5f)*columnDist*(colNum&1 ? -1:1);
	*newline = false;
	if ((nextCornerPos.x - addSpace) > frontLength) {
		nextCornerPos.x = 0; nextCornerPos.z -= avgLength * 2 * 8;
		*newline = true;
	}
	if (!frontcmds) {
		return nextCornerPos;
	}
	int unitSize = 16;
	const CUnit* u = unitHandler->units[unit];
	if (u) {
		const UnitDef* ud = u->unitDef;
		unitSize = (int)((ud->xsize + ud->zsize) / 2);
	}
	float3 retPos( nextCornerPos.x + unitSize * 8 * 2 + addSpace, 0, nextCornerPos.z);
	float3 movePos(nextCornerPos.x + unitSize * 8     + addSpace, 0, nextCornerPos.z); // posit in coordinates of "front"
	if (nextCornerPos.x == 0) {
		movePos.x = unitSize * 8;
		retPos.x -= addSpace;
	}
	float3 pos;
	pos.x = rightPos.x + (movePos.x * (dir.x / dir.y)) - (movePos.z * (dir.z/dir.y));
	pos.z = rightPos.z + (movePos.x * (dir.z / dir.y)) + (movePos.z * (dir.x/dir.y));
	pos.y = CGround::GetHeightAboveWater(pos.x, pos.z);

	Command c(command->GetID(), command->options, pos);

	frontcmds->push_back(std::pair<int, Command>(unit, c));

	return retPos;
}


struct DistInfo {
	bool operator<(const DistInfo& di) const { return dist < di.dist; }
	float dist;
	int unitID;
};


void CSelectedUnitsHandlerAI::SelectAttack(const Command& cmd, int player)
{
	std::vector<int> targets;

	if (cmd.params.size() == 4) {
		SelectCircleUnits(cmd.GetPos(0), cmd.params[3], player, targets);
	} else {
		SelectRectangleUnits(cmd.GetPos(0), cmd.GetPos(3), player, targets);
	}

	if (targets.empty())
		return;

	const bool queueing = !!(cmd.options & SHIFT_KEY);
	const std::vector<int>& selected = selectedUnitsHandler.netSelected[player];

	const unsigned int targetsCount = targets.size();
	const unsigned int selectedCount = selected.size();

	if (selectedCount == 0)
		return;

	Command attackCmd(CMD_ATTACK, cmd.options, 0.0f);

	// delete the attack commands and bail for CONTROL_KEY
	if (cmd.options & CONTROL_KEY) {
		attackCmd.options |= SHIFT_KEY;

		for (unsigned int s = 0; s < selectedCount; s++) {
			const CUnit* unit = unitHandler->units[ selected[s] ];

			if (unit == NULL)
				continue;

			CCommandAI* commandAI = unitHandler->units[selected[s]]->commandAI;

			for (unsigned int t = 0; t < targetsCount; t++) {
				attackCmd.params[0] = targets[t];

				if (commandAI->WillCancelQueued(attackCmd)) {
					commandAI->GiveCommand(attackCmd, false);
				}
			}
		}

		return;
	}

	// get the group center
	float3 midPos;
	unsigned int realCount = 0;
	for (unsigned int s = 0; s < selectedCount; s++) {
		CUnit* unit = unitHandler->units[selected[s]];

		if (unit == NULL)
			continue;

		if (queueing) {
			midPos += LastQueuePosition(unit);
		} else {
			midPos += unit->midPos;
		}

		realCount++;
	}

	if (realCount == 0)
		return;

	midPos /= realCount;

	// sort the targets
	std::vector<DistInfo> distVec;

	for (unsigned int t = 0; t < targetsCount; t++) {
		const CUnit* unit = unitHandler->units[ targets[t] ];
		const float3 unitPos = float3(unit->midPos);

		DistInfo di;
		di.unitID = targets[t];
		di.dist = (unitPos - midPos).SqLength2D();
		distVec.push_back(di);
	}
	stable_sort(distVec.begin(), distVec.end());

	// give the commands
	for (unsigned int s = 0; s < selectedCount; s++) {
		if (!queueing) {
			// clear it for the first command
			attackCmd.options &= ~SHIFT_KEY;
		}

		CUnit* unit = unitHandler->units[selected[s]];

		if (unit == NULL)
			continue;

		CCommandAI* commandAI = unit->commandAI;

		for (unsigned t = 0; t < targetsCount; t++) {
			attackCmd.params[0] = distVec[t].unitID;

			if (!queueing || !commandAI->WillCancelQueued(attackCmd)) {
				commandAI->GiveCommand(attackCmd, false);

				AddUnitSetMaxSpeedCommand(unit, attackCmd.options);
				// following commands are always queued
				attackCmd.options |= SHIFT_KEY;
			}
		}
	}
}


void CSelectedUnitsHandlerAI::SelectCircleUnits(
	const float3& pos,
	float radius,
	int player,
	std::vector<int>& units
) {
	units.clear();

	if (!playerHandler->IsValidPlayer(player))
		return;

	const CPlayer* p = playerHandler->Player(player);

	if (p == NULL)
		return;

	const std::vector<CUnit*>& tmpUnits = quadField->GetUnitsExact(pos, radius, false);

	const float radiusSqr = radius * radius;
	const unsigned int count = tmpUnits.size();
	const int allyTeam = teamHandler->AllyTeam(p->team);

	units.reserve(count);

	for (unsigned int i = 0; i < count; i++) {
		CUnit* unit = tmpUnits[i];

		if (unit == NULL)
			continue;
		if (unit->allyteam == allyTeam)
			continue;
		if (!(unit->losStatus[allyTeam] & (LOS_INLOS | LOS_INRADAR)))
			continue;

		const float dx = (pos.x - unit->midPos.x);
		const float dz = (pos.z - unit->midPos.z);

		if (((dx * dx) + (dz * dz)) > radiusSqr)
			continue;

		units.push_back(unit->id);
	}
}


void CSelectedUnitsHandlerAI::SelectRectangleUnits(
	const float3& pos0,
	const float3& pos1,
	int player,
	std::vector<int>& units
) {
	units.clear();

	if (!playerHandler->IsValidPlayer(player))
		return;

	const CPlayer* p = playerHandler->Player(player);

	if (p == NULL)
		return;

	const float3 mins(std::min(pos0.x, pos1.x), 0.0f, std::min(pos0.z, pos1.z));
	const float3 maxs(std::max(pos0.x, pos1.x), 0.0f, std::max(pos0.z, pos1.z));

	const std::vector<CUnit*>& tmpUnits = quadField->GetUnitsExact(mins, maxs);

	const unsigned int count = tmpUnits.size();
	const int allyTeam = teamHandler->AllyTeam(p->team);

	units.reserve(count);

	for (unsigned int i = 0; i < count; i++) {
		const CUnit* unit = tmpUnits[i];

		if (unit == NULL)
			continue;
		if (unit->allyteam == allyTeam)
			continue;
		if (!(unit->losStatus[allyTeam] & (LOS_INLOS | LOS_INRADAR)))
			continue;

		units.push_back(unit->id);
	}
}


float3 CSelectedUnitsHandlerAI::LastQueuePosition(const CUnit* unit)
{
	const CCommandQueue& queue = unit->commandAI->commandQue;
	CCommandQueue::const_reverse_iterator it;
	for (it = queue.rbegin(); it != queue.rend(); ++it) {
		const Command& cmd = *it;
		if (cmd.params.size() >= 3) {
			return cmd.GetPos(0);
		}
	}
	return unit->midPos;
}
