/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "WaitCommandsAI.h"
#include "SelectedUnitsHandler.h"
#include "GameHelper.h"
#include "GlobalUnsynced.h"
#include "UI/CommandColors.h"
#include "Rendering/LineDrawer.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/CommandAI/CommandQueue.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/CommandAI/FactoryCAI.h"
#include "Sim/Units/UnitTypes/Factory.h"
#include "System/Object.h"
#include "System/StringUtil.h"
#include "System/creg/STL_Map.h"
#include "System/creg/STL_Set.h"

#include <cassert>

CWaitCommandsAI waitCommandsAI;


static const int maxNetDelay = 30;  // in seconds

static const int updatePeriod = 3;  // in GAME_SPEED, 100 ms


CR_BIND(CWaitCommandsAI, )
CR_REG_METADATA(CWaitCommandsAI, (
	CR_MEMBER(waitMap),
	CR_MEMBER(unackedMap)
))

CR_BIND_DERIVED_INTERFACE(CWaitCommandsAI::Wait, CObject)
CR_REG_METADATA_SUB(CWaitCommandsAI,Wait, (
	CR_MEMBER(code),
	CR_MEMBER(key),
	CR_MEMBER(valid),
	CR_MEMBER(deadTime),
	CR_POSTLOAD(PostLoad)
))

CR_BIND_DERIVED(CWaitCommandsAI::TimeWait, CWaitCommandsAI::Wait, (1,0))
CR_REG_METADATA_SUB(CWaitCommandsAI,TimeWait , (
	CR_MEMBER(unit),
	CR_MEMBER(enabled),
	CR_MEMBER(duration),
	CR_MEMBER(endFrame),
	CR_MEMBER(factory)
))

CR_BIND_DERIVED(CWaitCommandsAI::DeathWait, CWaitCommandsAI::Wait, (Command()))
CR_REG_METADATA_SUB(CWaitCommandsAI,DeathWait , (
	CR_MEMBER(waitUnits),
	CR_MEMBER(deathUnits),
	CR_MEMBER(unitPos)
))

CR_BIND_DERIVED(CWaitCommandsAI::SquadWait, CWaitCommandsAI::Wait, (Command()))
CR_REG_METADATA_SUB(CWaitCommandsAI,SquadWait , (
	CR_MEMBER(squadCount),
	CR_MEMBER(buildUnits),
	CR_MEMBER(waitUnits),
	CR_MEMBER(stateText)
))

CR_BIND_DERIVED(CWaitCommandsAI::GatherWait, CWaitCommandsAI::Wait, (Command()))
CR_REG_METADATA_SUB(CWaitCommandsAI,GatherWait , (
	CR_MEMBER(waitUnits)
))

/******************************************************************************/
/******************************************************************************/

CWaitCommandsAI::CWaitCommandsAI()
{
	static_assert(sizeof(float) == sizeof(KeyType), "");
}


CWaitCommandsAI::~CWaitCommandsAI()
{
	WaitMap::iterator it;
	for (it = waitMap.begin(); it != waitMap.end(); ++it) {
		delete it->second;
	}
	for (it = unackedMap.begin(); it != unackedMap.end(); ++it) {
		delete it->second;
	}
}


void CWaitCommandsAI::Update()
{
//	if ((gs->frameNum % GAME_SPEED) == 0) printf("Waits: %i\n", waitMap.size()); // FIXME

	// limit the updates
	if ((gs->frameNum % updatePeriod) != 0) {
		return;
	}

	// update the waits
	WaitMap::iterator it;
	it = waitMap.begin();
	while (it != waitMap.end()) {
		// grab an incremented iterator in case the active iterator deletes itself
		WaitMap::iterator tmp = it;
		++tmp;
		it->second->Update();
		it = tmp;
	}

	// delete old unacknowledged waits
	const spring_time nowTime = spring_gettime();
	it = unackedMap.begin();
	while (it != unackedMap.end()) {
		WaitMap::iterator tmp = it;
		++tmp;
		Wait* wait = it->second;
		if (wait->GetDeadTime() < nowTime) {
			delete wait;
			unackedMap.erase(it);
		}
		it = tmp;
	}
}


void CWaitCommandsAI::DrawCommands() const
{
	WaitMap::const_iterator it;
	for (it = waitMap.begin(); it != waitMap.end(); ++it) {
		it->second->Draw();
	}
}


void CWaitCommandsAI::AddTimeWait(const Command& cmd)
{
	// save the current selection
	const auto tmpSet = selectedUnitsHandler.selectedUnits;

	for (const int unitID: tmpSet) {
		InsertWaitObject(TimeWait::New(cmd, unitHandler.GetUnit(unitID)));
	}

	// restore the selection
	selectedUnitsHandler.ClearSelected();

	for (const int unitID: tmpSet) {
		selectedUnitsHandler.AddUnit(unitHandler.GetUnit(unitID));
	}
}


void CWaitCommandsAI::AddDeathWait(const Command& cmd)
{
	InsertWaitObject(DeathWait::New(cmd));
}


void CWaitCommandsAI::AddSquadWait(const Command& cmd)
{
	InsertWaitObject(SquadWait::New(cmd));
}


void CWaitCommandsAI::AddGatherWait(const Command& cmd)
{
	InsertWaitObject(GatherWait::New(cmd));
}


void CWaitCommandsAI::AcknowledgeCommand(const Command& cmd)
{
	if ((cmd.GetID() != CMD_WAIT) || (cmd.GetNumParams() != 2))
		return;

	const KeyType key = Wait::GetKeyFromFloat(cmd.GetParam(1));
	WaitMap::iterator it = unackedMap.find(key);
	if (it != unackedMap.end()) {
		Wait* wait = it->second;

		if (wait->GetCode() != cmd.GetParam(0))
			return; // code mismatch

		// move into the acknowledged pool
		unackedMap.erase(key);
		waitMap[key] = wait;
	}
}


void CWaitCommandsAI::AddLocalUnit(CUnit* unit, const CUnit* builder)
{
	// NOTE: the wait keys will link the right units to
	//       the correct player (for multi-player teams)
	if ((unit->team != gu->myTeam) || waitMap.empty())
		return;

	const CCommandQueue& dq = unit->commandAI->commandQue;

	for (const Command& cmd: dq) {
		if ((cmd.GetID() != CMD_WAIT) || (cmd.GetNumParams() != 2))
			continue;

		const KeyType key = Wait::GetKeyFromFloat(cmd.GetParam(1));
		WaitMap::iterator wit = waitMap.find(key);
		if (wit == waitMap.end())
			continue;

		Wait* wait = wit->second;
		if (cmd.GetParam(0) != wait->GetCode())
			continue;

		const float code = cmd.GetParam(0);

		if (code != CMD_WAITCODE_TIMEWAIT) {
			wait->AddUnit(unit);
		} else {
			// add a unit-specific TimeWait
			// (straight into the waitMap, no net ack required)
			const int duration = static_cast<TimeWait*>(wait)->GetDuration();
			TimeWait* tw = TimeWait::New(duration, unit);

			if (tw != nullptr) {
				if (waitMap.find(tw->GetKey()) != waitMap.end()) {
					delete tw;
				} else {
					waitMap[tw->GetKey()] = tw;
					// should not affect the sync state
					const_cast<Command&>(cmd).SetParam(1, Wait::GetFloatFromKey(tw->GetKey()));
				}
			}
		}
	}
}


void CWaitCommandsAI::RemoveWaitCommand(CUnit* unit, const Command& cmd)
{
	if ((cmd.GetNumParams() != 2) ||
	    (unit->team != gu->myTeam)) {
		return;
	}

	const KeyType key = Wait::GetKeyFromFloat(cmd.GetParam(1));
	WaitMap::iterator it = waitMap.find(key);

	if (it != waitMap.end()) {
		it->second->RemoveUnit(unit);
	}
}


void CWaitCommandsAI::ClearUnitQueue(CUnit* unit, const CCommandQueue& queue)
{
	if ((unit->team != gu->myTeam) || waitMap.empty())
		return;

	for (const Command& cmd: queue) {
		if ((cmd.GetID() == CMD_WAIT) && (cmd.GetNumParams() == 2)) {
			const KeyType key = Wait::GetKeyFromFloat(cmd.GetParam(1));
			WaitMap::iterator wit = waitMap.find(key);

			if (wit != waitMap.end()) {
				wit->second->RemoveUnit(unit);
			}
		}
	}
}


bool CWaitCommandsAI::InsertWaitObject(Wait* wait)
{
	if (wait == nullptr)
		return false;

	if (unackedMap.find(wait->GetKey()) != unackedMap.end())
		return false;

	unackedMap[wait->GetKey()] = wait;
	return true;
}


void CWaitCommandsAI::RemoveWaitObject(Wait* wait)
{
	if (waitMap.erase(wait->GetKey()))
		 return;
	if (unackedMap.erase(wait->GetKey()))
		 return;
}


void CWaitCommandsAI::AddIcon(const Command& cmd, const float3& pos) const
{
	if (cmd.GetNumParams() != 2) {
		lineDrawer.DrawIconAtLastPos(CMD_WAIT);
		return;
	}

	const float code = cmd.GetParam(0);
	const KeyType key = Wait::GetKeyFromFloat(cmd.GetParam(1));
	WaitMap::const_iterator it = waitMap.find(key);

	if (it == waitMap.end()) {
		lineDrawer.DrawIconAtLastPos(CMD_WAIT);
	}
	else if (code == CMD_WAITCODE_TIMEWAIT) {
		lineDrawer.DrawIconAtLastPos(CMD_TIMEWAIT);
		cursorIcons.AddIconText(it->second->GetStateText(), pos);
	}
	else if (code == CMD_WAITCODE_SQUADWAIT) {
		lineDrawer.DrawIconAtLastPos(CMD_SQUADWAIT);
		cursorIcons.AddIconText(it->second->GetStateText(), pos);
	}
	else if (code == CMD_WAITCODE_DEATHWAIT) {
		lineDrawer.DrawIconAtLastPos(CMD_DEATHWAIT);
		it->second->AddUnitPosition(pos);
	}
	else if (code == CMD_WAITCODE_GATHERWAIT) {
		lineDrawer.DrawIconAtLastPos(CMD_GATHERWAIT);
	}
	else {
		lineDrawer.DrawIconAtLastPos(CMD_WAIT);
	}
}


/******************************************************************************/
//
//  Wait Base Class
//

CWaitCommandsAI::KeyType CWaitCommandsAI::Wait::keySource = 0;

const std::string CWaitCommandsAI::Wait::noText;


// static
CWaitCommandsAI::KeyType CWaitCommandsAI::Wait::GetNewKey()
{
	keySource = (0xffffff00 & keySource) |
	            (0x000000ff & gu->myPlayerNum);
	keySource += 0x00000100;
	return keySource;
}


// static
CWaitCommandsAI::KeyType CWaitCommandsAI::Wait::GetKeyFromFloat(float f)
{
	return *((KeyType*)&f);
}


void CWaitCommandsAI::Wait::PostLoad()
{
	deadTime = spring_gettime() + spring_secs(maxNetDelay);
}

// static
float CWaitCommandsAI::Wait::GetFloatFromKey(KeyType k)
{
	return *((float*)&k);
}


CWaitCommandsAI::Wait::Wait(float _code)
	: code(_code),
	key(0),
	valid(false),
	deadTime(spring_gettime() + spring_secs(maxNetDelay))
{
}


CWaitCommandsAI::Wait::~Wait()
{
	waitCommandsAI.RemoveWaitObject(this);
}


CWaitCommandsAI::Wait::WaitState
	CWaitCommandsAI::Wait::GetWaitState(const CUnit* unit) const
{
	const CCommandQueue& dq = unit->commandAI->commandQue;

	if (dq.empty())
		return Missing;

	const Command& cmd = dq.front();
	if ((cmd.GetID() == CMD_WAIT) && (cmd.GetNumParams() == 2) &&
			(cmd.GetParam(0) == code) &&
			(GetKeyFromFloat(cmd.GetParam(1)) == key)) {
		return Active;
	}

	CCommandQueue::const_iterator it = dq.begin();
	++it;
	for ( ; it != dq.end(); ++it) {
		const Command& qcmd = *it;
		if ((qcmd.GetID() == CMD_WAIT) && (qcmd.GetNumParams() == 2) &&
				(qcmd.GetParam(0) == code) &&
				(GetKeyFromFloat(qcmd.GetParam(1)) == key)) {
			return Queued;
		}
	}
	return Missing;
}


bool CWaitCommandsAI::Wait::IsWaitingOn(const CUnit* unit) const
{
	const CCommandQueue& dq = unit->commandAI->commandQue;
	if (dq.empty())
		return false;

	const Command& cmd = dq.front();

	return ((cmd.GetID() == CMD_WAIT) && (cmd.GetNumParams() == 2) && (cmd.GetParam(0) == code) && (GetKeyFromFloat(cmd.GetParam(1)) == key));
}


void CWaitCommandsAI::Wait::SendCommand(const Command& cmd, const CUnitSet& unitSet)
{
	if (unitSet.empty())
		return;

	const auto& selUnits = selectedUnitsHandler.selectedUnits;

	if (unitSet == selUnits) {
		selectedUnitsHandler.GiveCommand(cmd, false);
		return;
	}

	// make a temporary copy
	auto tmpSet = selUnits;

	// create new selection for this command
	selectedUnitsHandler.ClearSelected();
	for (const int unitID: unitSet) {
		selectedUnitsHandler.AddUnit(unitHandler.GetUnit(unitID));
	}

	selectedUnitsHandler.GiveCommand(cmd, false);
	selectedUnitsHandler.ClearSelected();

	// restore previous selection
	for (const int unitID: tmpSet) {
		selectedUnitsHandler.AddUnit(unitHandler.GetUnit(unitID));
	}
}


void CWaitCommandsAI::Wait::SendWaitCommand(const CUnitSet& unitSet)
{
	Command waitCmd(CMD_WAIT);
	SendCommand(waitCmd, unitSet);
}



/******************************************************************************/
//
//  TimeWait
//

CWaitCommandsAI::TimeWait*
	CWaitCommandsAI::TimeWait::New(const Command& cmd, CUnit* unit)
{
	TimeWait* tw = new TimeWait(cmd, unit);
	if (!tw->valid) {
		delete tw;
		return nullptr;
	}
	return tw;
}


CWaitCommandsAI::TimeWait*
	CWaitCommandsAI::TimeWait::New(int duration, CUnit* unit)
{
	TimeWait* tw = new TimeWait(duration, unit);
	if (!tw->valid) {
		delete tw;
		return nullptr;
	}
	return tw;
}


CWaitCommandsAI::TimeWait::TimeWait(const Command& cmd, CUnit* _unit)
: Wait(CMD_WAITCODE_TIMEWAIT)
{
	if (cmd.GetNumParams() != 1)
		return;

	valid = true;
	key = GetNewKey();

	unit = _unit;
	enabled = false;
	endFrame = 0;
	duration = GAME_SPEED * (int)cmd.GetParam(0);
	factory = (dynamic_cast<CFactory*>(unit) != nullptr);

	Command waitCmd(CMD_WAIT, cmd.GetOpts(), code);
	waitCmd.PushParam(GetFloatFromKey(key));

	selectedUnitsHandler.ClearSelected();
	selectedUnitsHandler.AddUnit(unit);
	selectedUnitsHandler.GiveCommand(waitCmd);

	AddDeathDependence(unit, DEPENDENCE_WAITCMD);
}


CWaitCommandsAI::TimeWait::TimeWait(int _duration, CUnit* _unit)
: Wait(CMD_WAITCODE_TIMEWAIT)
{
	valid = true;
	key = GetNewKey();

	unit = _unit;
	enabled = false;
	endFrame = 0;
	duration = _duration;
	factory = false;

	AddDeathDependence(unit, DEPENDENCE_WAITCMD);
}


CWaitCommandsAI::TimeWait::~TimeWait() = default; // do nothing


void CWaitCommandsAI::TimeWait::DependentDied(CObject* object)
{
	unit = nullptr;
}


void CWaitCommandsAI::TimeWait::AddUnit(CUnit* unit)
{
	// do nothing
}


void CWaitCommandsAI::TimeWait::RemoveUnit(CUnit* _unit)
{
	if (_unit != unit)
		return;

	delete this;
}


void CWaitCommandsAI::TimeWait::Update()
{
	if (unit == nullptr) {
		delete this;
		return;
	}

	WaitState state = GetWaitState(unit);

	if (state == Active) {
		if (!enabled) {
			enabled = true;
			endFrame = (gs->frameNum + duration);
			return;
		}

		if (endFrame <= gs->frameNum) {
			SendWaitCommand(CUnitSet{unit->id});

			if (!factory) {
				delete this;
				return;
			}

			enabled = false;
		}

		return;
	}
	if (state == Queued)
		return;

	if (state == Missing) {
		if (!factory) { // FIXME
			delete this;
			return;
		}
	}
}


const std::string& CWaitCommandsAI::TimeWait::GetStateText() const
{
	char buf[32];
	if (enabled) {
		SNPRINTF(buf, sizeof(buf), "%i", 1 + (std::max(0, (endFrame - gs->frameNum - 1)) / GAME_SPEED));
	} else {
		SNPRINTF(buf, sizeof(buf), "%i", duration / GAME_SPEED);
	}
	static std::string text;
	text = buf;
	return text;
}


void CWaitCommandsAI::TimeWait::Draw() const
{
	// do nothing
}


/******************************************************************************/
//
//  DeathWait
//

CWaitCommandsAI::DeathWait*
	CWaitCommandsAI::DeathWait::New(const Command& cmd)
{
	DeathWait* dw = new DeathWait(cmd);
	if (!dw->valid) {
		delete dw;
		return nullptr;
	}
	return dw;
}


CWaitCommandsAI::DeathWait::DeathWait(const Command& cmd)
: Wait(CMD_WAITCODE_DEATHWAIT)
{
	const auto& selUnits = selectedUnitsHandler.selectedUnits;

	if (cmd.GetNumParams() == 1) {
		const int unitID = (int)cmd.GetParam(0);

		CUnit* unit = unitHandler.GetUnit(unitID);

		if (unit == nullptr)
			return;

		if (selUnits.find(unitID) != selUnits.end())
			return;

		deathUnits.insert(unitID);
	}
	else if (cmd.GetNumParams() == 6) {
		const float3& pos0 = cmd.GetPos(0);
		const float3& pos1 = cmd.GetPos(3);

		CUnitSet tmpSet;
		SelectAreaUnits(pos0, pos1, tmpSet, false);

		for (const int unitID: tmpSet) {
			if (selUnits.find(unitID) == selUnits.end()) {
				deathUnits.insert(unitID);
			}
		}

		if (deathUnits.empty())
			return;
	} else {
		return; // unknown param config
	}

	valid = true;
	key = GetNewKey();

	waitUnits = selUnits;

	Command waitCmd(CMD_WAIT, cmd.GetOpts(), code);
	waitCmd.PushParam(GetFloatFromKey(key));
	selectedUnitsHandler.GiveCommand(waitCmd);

	for (const int unitID: waitUnits) {
		AddDeathDependence((CObject*) unitHandler.GetUnit(unitID), DEPENDENCE_WAITCMD);
	}
	for (const int unitID: deathUnits) {
		AddDeathDependence((CObject*) unitHandler.GetUnit(unitID), DEPENDENCE_WAITCMD);
	}
}


CWaitCommandsAI::DeathWait::~DeathWait() = default; // do nothing


void CWaitCommandsAI::DeathWait::DependentDied(CObject* object)
{
	waitUnits.erase(static_cast<CUnit*>(object)->id);

	if (waitUnits.empty())
		return;

	deathUnits.erase(static_cast<CUnit*>(object)->id);
}


void CWaitCommandsAI::DeathWait::AddUnit(CUnit* unit)
{
	if (waitUnits.insert(unit->id).second)
		AddDeathDependence(unit, DEPENDENCE_WAITCMD);
}


void CWaitCommandsAI::DeathWait::RemoveUnit(CUnit* unit)
{
	if (waitUnits.erase(unit->id))
		DeleteDeathDependence(unit, DEPENDENCE_WAITCMD);
}


void CWaitCommandsAI::DeathWait::Update()
{
	if (waitUnits.empty()) {
		delete this;
		return;
	}

	unitPos.clear();

	if (!deathUnits.empty())
		return; // more must die

	spring::unordered_set<int> unblockSet;
	std::vector<int> voidWaitUnitIDs;

	for (const int unitID: waitUnits) {
		const WaitState state = GetWaitState(unitHandler.GetUnit(unitID));

		if (state == Active) {
			unblockSet.insert(unitID);
			DeleteDeathDependence(unitHandler.GetUnit(unitID), DEPENDENCE_WAITCMD);
			voidWaitUnitIDs.push_back(unitID);
		}
		else if (state == Queued) {} // do nothing
		else if (state == Missing) {
			DeleteDeathDependence(unitHandler.GetUnit(unitID), DEPENDENCE_WAITCMD);
			voidWaitUnitIDs.push_back(unitID);
		}
	}

	for (const int unitID: voidWaitUnitIDs) {
		waitUnits.erase(unitID);
	}

	SendWaitCommand(unblockSet);

	if (waitUnits.empty()) {
		delete this;
		return;
	}
}


void CWaitCommandsAI::DeathWait::Draw() const
{
	if (unitPos.empty())
		return;

	float3 midPos;
	for (const auto& pos: unitPos) {
		midPos += pos;
	}
	midPos /= (float)unitPos.size();

	cursorIcons.AddIcon(CMD_DEATHWAIT, midPos);

	for (const auto& pos: unitPos) {
		lineDrawer.StartPath(pos, cmdColors.start);
		lineDrawer.DrawLine(midPos, cmdColors.deathWait);
	}

	for (const int unitID: deathUnits) {
		const CUnit* unit = unitHandler.GetUnit(unitID);

		if (unit->losStatus[gu->myAllyTeam] & (LOS_INLOS | LOS_INRADAR)) {
			cursorIcons.AddIcon(CMD_SELFD, unit->midPos);
			lineDrawer.StartPath(midPos, cmdColors.start);
			lineDrawer.DrawLine(unit->midPos, cmdColors.deathWait);
		}
	}
}


void CWaitCommandsAI::DeathWait::AddUnitPosition(const float3& pos)
{
	unitPos.push_back(pos);
}


void CWaitCommandsAI::DeathWait::SelectAreaUnits(
	const float3& pos0, const float3& pos1, CUnitSet& units, bool enemies)
{
	units.clear();

	const float3 mins(std::min(pos0.x, pos1.x), 0.0f, std::min(pos0.z, pos1.z));
	const float3 maxs(std::max(pos0.x, pos1.x), 0.0f, std::max(pos0.z, pos1.z));

	QuadFieldQuery qfQuery;
	quadField.GetUnitsExact(qfQuery, mins, maxs);

	for (const CUnit* unit: *qfQuery.units) {
		if (enemies && teamHandler.Ally(unit->allyteam, gu->myAllyTeam))
			continue;

		if (!(unit->losStatus[gu->myAllyTeam] & (LOS_INLOS | LOS_INRADAR)))
			continue;

		units.insert(unit->id);
	}
}


/******************************************************************************/
//
//  SquadWait
//

CWaitCommandsAI::SquadWait*
	CWaitCommandsAI::SquadWait::New(const Command& cmd)
{
	SquadWait* sw = new SquadWait(cmd);
	if (!sw->valid) {
		delete sw;
		return nullptr;
	}
	return sw;
}


CWaitCommandsAI::SquadWait::SquadWait(const Command& cmd)
: Wait(CMD_WAITCODE_SQUADWAIT)
{
	if (cmd.GetNumParams() != 1)
		return;

	squadCount = (int)cmd.GetParam(0);
	if (squadCount < 2)
		return;

	const auto& selUnits = selectedUnitsHandler.selectedUnits;

	for (const int unitID: selUnits) {
		const CUnit* unit = unitHandler.GetUnit(unitID);

		if (dynamic_cast<const CFactory*>(unit) != nullptr) {
			buildUnits.insert(unitID);
		} else {
			waitUnits.insert(unitID);
		}
	}

	if (buildUnits.empty() && ((int)waitUnits.size() < squadCount))
		return;

	valid = true;
	key = GetNewKey();

	Command waitCmd(CMD_WAIT, cmd.GetOpts(), code);
	waitCmd.PushParam(GetFloatFromKey(key));

	SendCommand(waitCmd, buildUnits);
	SendCommand(waitCmd, waitUnits);

	for (const int unitID: buildUnits) {
		AddDeathDependence((CObject*) unitHandler.GetUnit(unitID), DEPENDENCE_WAITCMD);
	}
	for (const int unitID: waitUnits) {
		AddDeathDependence((CObject*) unitHandler.GetUnit(unitID), DEPENDENCE_WAITCMD);
	}

	UpdateText();
}


CWaitCommandsAI::SquadWait::~SquadWait() = default; // do nothing


void CWaitCommandsAI::SquadWait::DependentDied(CObject* object)
{
	buildUnits.erase(static_cast<CUnit*>(object)->id);
	waitUnits.erase(static_cast<CUnit*>(object)->id);
}


void CWaitCommandsAI::SquadWait::AddUnit(CUnit* unit)
{
	if (waitUnits.insert(unit->id).second)
		AddDeathDependence(unit, DEPENDENCE_WAITCMD);
}


void CWaitCommandsAI::SquadWait::RemoveUnit(CUnit* unit)
{
	if (buildUnits.erase(unit->id))
		DeleteDeathDependence(unit, DEPENDENCE_WAITCMD);
	if (waitUnits.erase(unit->id))
		DeleteDeathDependence(unit, DEPENDENCE_WAITCMD);
}


void CWaitCommandsAI::SquadWait::Update()
{
	if (buildUnits.empty() && ((int)waitUnits.size() < squadCount)) {
		// FIXME -- unblock remaining waitUnits ?
		delete this;
		return;
	}

	if ((int)waitUnits.size() >= squadCount) {
		spring::unordered_set<int> unblockSet;
		std::vector<int> voidWaitUnitIDs;

		for (const int unitID: waitUnits) {
			const WaitState state = GetWaitState(unitHandler.GetUnit(unitID));

			if (state == Active) {
				unblockSet.insert(unitID);

				if ((int)unblockSet.size() >= squadCount)
					break; // we've got our squad
			}
			else if (state == Queued) {} // do nothing
			else if (state == Missing) {
				DeleteDeathDependence(unitHandler.GetUnit(unitID), DEPENDENCE_WAITCMD);
				voidWaitUnitIDs.push_back(unitID);
			}
		}

		for (const int unitID: voidWaitUnitIDs) {
			waitUnits.erase(unitID);
		}

		if ((int)unblockSet.size() >= squadCount) {
			// FIXME -- rebuild the order queue so
			//          that formations are created?
			SendWaitCommand(unblockSet);

			for (const int unitID: unblockSet) {
				if (waitUnits.erase(unitID))
					DeleteDeathDependence(unitHandler.GetUnit(unitID), DEPENDENCE_WAITCMD);
			}
		}
	}

	UpdateText();
	// FIXME -- clean builders
}


void CWaitCommandsAI::SquadWait::UpdateText()
{
	static char buf[64];
	SNPRINTF(buf, sizeof(buf), "%i/%i", (int)waitUnits.size(), squadCount);
	stateText = buf;
}


void CWaitCommandsAI::SquadWait::Draw() const
{
	// do nothing
}


/******************************************************************************/
//
//  GatherWait
//

CWaitCommandsAI::GatherWait*
	CWaitCommandsAI::GatherWait::New(const Command& cmd)
{
	GatherWait* gw = new GatherWait(cmd);
	if (!gw->valid) {
		delete gw;
		return nullptr;
	}
	return gw;
}


CWaitCommandsAI::GatherWait::GatherWait(const Command& cmd)
: Wait(CMD_WAITCODE_GATHERWAIT)
{
	if (cmd.GetNumParams() != 0)
		return;

	// only add valid units
	const auto& selUnits = selectedUnitsHandler.selectedUnits;

	for (const int unitID: selUnits) {
		const CUnit* unit = unitHandler.GetUnit(unitID);
		const UnitDef* ud = unit->unitDef;

		if (ud->canmove && (dynamic_cast<const CFactory*>(unit) == nullptr)) {
			waitUnits.insert(unitID);
		}
	}

	if (waitUnits.size() < 2)
		return; // one man does not a gathering make

	valid = true;
	key = GetNewKey();

	Command waitCmd(CMD_WAIT, SHIFT_KEY, code);
	waitCmd.PushParam(GetFloatFromKey(key));
	selectedUnitsHandler.GiveCommand(waitCmd, true);

	for (const int unitID: waitUnits) {
		AddDeathDependence((CObject*) unitHandler.GetUnit(unitID), DEPENDENCE_WAITCMD);
	}
}


CWaitCommandsAI::GatherWait::~GatherWait() = default; // do nothing

void CWaitCommandsAI::GatherWait::DependentDied(CObject* object)
{
	waitUnits.erase(static_cast<CUnit*>(object)->id);
}


void CWaitCommandsAI::GatherWait::AddUnit(CUnit* unit)
{
	// do nothing
}


void CWaitCommandsAI::GatherWait::RemoveUnit(CUnit* unit)
{
	if (waitUnits.erase(unit->id))
		DeleteDeathDependence(unit, DEPENDENCE_WAITCMD);
}


void CWaitCommandsAI::GatherWait::Update()
{
	if (waitUnits.size() < 2) {
		delete this;
		return;
	}

	std::vector<int> voidWaitUnitIDs;

	for (const int unitID: waitUnits) {
		const WaitState state = GetWaitState(unitHandler.GetUnit(unitID));

		if (state == Active) {} // do nothing
		else if (state == Queued) {
			// erase any ID's we might have encountered with state=Missing
			for (const int unitID: voidWaitUnitIDs) {
				waitUnits.erase(unitID);
			}
			return;
		}
		else if (state == Missing) {
			DeleteDeathDependence(unitHandler.GetUnit(unitID), DEPENDENCE_WAITCMD);
			voidWaitUnitIDs.push_back(unitID);
		}
	}

	for (const int unitID: voidWaitUnitIDs) {
		waitUnits.erase(unitID);
	}

	// all units are actively waiting on this command, unblock them and die
	if (!waitUnits.empty())
		SendWaitCommand(waitUnits);

	delete this;
}


/******************************************************************************/
