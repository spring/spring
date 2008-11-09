// WaitCommandsAI.cpp: implementation of the CWaitCommands class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "mmgr.h"

#include <SDL_timer.h>
#include "WaitCommandsAI.h"
#include "SelectedUnits.h"
#include "Sim/Misc/Team.h"
#include "GameHelper.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/CommandAI/CommandQueue.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/CommandAI/FactoryCAI.h"
#include "Sim/Units/CommandAI/LineDrawer.h"
#include "Sim/Units/UnitTypes/Factory.h"
#include "Sim/Misc/GlobalSynced.h"
#include "GlobalUnsynced.h"
#include "Object.h"
#include "UI/CommandColors.h"
#include "UI/CursorIcons.h"
#include "creg/STL_Map.h"
#include "creg/STL_List.h"
#include "Util.h"
#include <assert.h>

CWaitCommandsAI waitCommandsAI;


static const int maxNetDelay = 30;  // in seconds

static const int updatePeriod = 3;  // 100 ms


CR_BIND(CWaitCommandsAI, );

CR_REG_METADATA(CWaitCommandsAI, (
				CR_MEMBER(waitMap),
				CR_MEMBER(unackedMap),
				CR_RESERVED(16)
				));

CR_BIND_DERIVED_INTERFACE(CWaitCommandsAI::Wait, CObject);

CR_REG_METADATA_SUB(CWaitCommandsAI,Wait, (
					CR_MEMBER(code),
					CR_MEMBER(key),
					CR_MEMBER(valid),
					CR_RESERVED(16),
					CR_POSTLOAD(PostLoad)
					));

CR_BIND_DERIVED(CWaitCommandsAI::TimeWait, CWaitCommandsAI::Wait, (1,0));

CR_REG_METADATA_SUB(CWaitCommandsAI,TimeWait , (
					CR_MEMBER(unit),
					CR_MEMBER(enabled),
					CR_MEMBER(duration),
					CR_MEMBER(endFrame),
					CR_MEMBER(factory),
					CR_RESERVED(16)
					));

CR_BIND_DERIVED(CWaitCommandsAI::DeathWait, CWaitCommandsAI::Wait, (Command()));

CR_REG_METADATA_SUB(CWaitCommandsAI,DeathWait , (
					CR_MEMBER(waitUnits),
					CR_MEMBER(deathUnits),
					CR_MEMBER(unitPos),
					CR_RESERVED(16)
					));

CR_BIND_DERIVED(CWaitCommandsAI::SquadWait, CWaitCommandsAI::Wait, (Command()));

CR_REG_METADATA_SUB(CWaitCommandsAI,SquadWait , (
					CR_MEMBER(squadCount),
					CR_MEMBER(buildUnits),
					CR_MEMBER(waitUnits),
					CR_MEMBER(stateText),
					CR_RESERVED(16)
					));

CR_BIND_DERIVED(CWaitCommandsAI::GatherWait, CWaitCommandsAI::Wait, (Command()));

CR_REG_METADATA_SUB(CWaitCommandsAI,GatherWait , (
					CR_MEMBER(waitUnits),
					CR_RESERVED(8)
					));

/******************************************************************************/
/******************************************************************************/

CWaitCommandsAI::CWaitCommandsAI()
{
	assert(sizeof(float) == sizeof(KeyType));
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
		tmp++;
		it->second->Update();
		it = tmp;
	}

	// delete old unacknowledged waits
	const time_t nowTime = time(NULL);
	it = unackedMap.begin();
	while (it != unackedMap.end()) {
		WaitMap::iterator tmp = it;
		tmp++;
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
	GML_RECMUTEX_LOCK(sel); // AddTimeWait

	// save the current selection
	const CUnitSet tmpSet = selectedUnits.selectedUnits;
	CUnitSet::const_iterator it;
	for (it = tmpSet.begin(); it != tmpSet.end(); ++it) {
		InsertWaitObject(TimeWait::New(cmd, *it));
	}
	// restore the selection
	selectedUnits.ClearSelected();
	for (it = tmpSet.begin(); it != tmpSet.end(); ++it) {
		selectedUnits.AddUnit(*it);
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
	if ((cmd.id != CMD_WAIT) || (cmd.params.size() != 2)) {
		return;
	}
	const KeyType key = Wait::GetKeyFromFloat(cmd.params[1]);
	WaitMap::iterator it = unackedMap.find(key);
	if (it != unackedMap.end()) {
		Wait* wait = it->second;
		if (wait->GetCode() != cmd.params[0]) {
			return; // code mismatch
		}
		// move into the acknowledged pool
		unackedMap.erase(key);
		waitMap[key] = wait;
	}
}


void CWaitCommandsAI::AddLocalUnit(CUnit* unit, const CUnit* builder)
{
	// NOTE: the wait keys will link the right units to
	//       the correct player (for multi-player teams)
	if ((unit->team != gu->myTeam) || waitMap.empty()) {
		return;
	}

	const CCommandQueue& dq = unit->commandAI->commandQue;
	CCommandQueue::const_iterator qit;
	for (qit = dq.begin(); qit != dq.end(); ++qit) {
		const Command& cmd = *qit;
		if ((cmd.id != CMD_WAIT) || (cmd.params.size() != 2)) {
			continue;
		}

		const KeyType key = Wait::GetKeyFromFloat(cmd.params[1]);
		WaitMap::iterator wit = waitMap.find(key);
		if (wit == waitMap.end()) {
			continue;
		}

		Wait* wait = wit->second;
		if (cmd.params[0] != wait->GetCode()) {
			continue;
		}

		const float code = cmd.params[0];
		if (code != CMD_WAITCODE_TIMEWAIT) {
			wait->AddUnit(unit);
		}
		else {
			// add a unit-specific TimeWait
			// (straight into the waitMap, no net ack required)
			const int duration = ((TimeWait*)wait)->GetDuration();
			TimeWait* tw = TimeWait::New(duration, unit);
			if (tw != NULL) {
				if (waitMap.find(tw->GetKey()) != waitMap.end()) {
					delete tw;
				} else {
					waitMap[tw->GetKey()] = tw;
					PUSH_CODE_MODE;
					ENTER_MIXED;
					// should not affect the sync state
					const_cast<Command&>(cmd).params[1] =
						Wait::GetFloatFromKey(tw->GetKey());
					POP_CODE_MODE;
				}
			}
		}
	}
}


void CWaitCommandsAI::RemoveWaitCommand(CUnit* unit, const Command& cmd)
{
	if ((cmd.params.size() != 2) ||
	    (unit->team != gu->myTeam)) {
		return;
	}
	const KeyType key = Wait::GetKeyFromFloat(cmd.params[1]);
	WaitMap::iterator it = waitMap.find(key);
	if (it != waitMap.end()) {
		it->second->RemoveUnit(unit);
	}
}


void CWaitCommandsAI::ClearUnitQueue(CUnit* unit, const CCommandQueue& queue)
{
	if ((unit->team != gu->myTeam) || waitMap.empty()) {
		return;
	}
	CCommandQueue::const_iterator qit;
	for (qit = queue.begin(); qit != queue.end(); ++qit) {
		const Command& cmd = *qit;
		if ((cmd.id == CMD_WAIT) && (cmd.params.size() == 2)) {
			const KeyType key = Wait::GetKeyFromFloat(cmd.params[1]);
			WaitMap::iterator wit = waitMap.find(key);
			if (wit != waitMap.end()) {
				wit->second->RemoveUnit(unit);
			}
		}
	}
}


bool CWaitCommandsAI::InsertWaitObject(Wait* wait)
{
	if (wait == NULL) {
		return false;
	}
	if (unackedMap.find(wait->GetKey()) != unackedMap.end()) {
		return false;
	}
	unackedMap[wait->GetKey()] = wait;
	return true;
}


void CWaitCommandsAI::RemoveWaitObject(Wait* wait)
{
	WaitMap::iterator it;

	it = waitMap.find(wait->GetKey());
	if (it != waitMap.end()) {
		 waitMap.erase(it);
		 return;
	}

	it = unackedMap.find(wait->GetKey());
	if (it != unackedMap.end()) {
		 unackedMap.erase(it);
		 return;
	}
}


void CWaitCommandsAI::AddIcon(const Command& cmd, const float3& pos) const
{
	if (cmd.params.size() != 2) {
		lineDrawer.DrawIconAtLastPos(CMD_WAIT);
		return;
	}

	const float code = cmd.params[0];
	const KeyType key = Wait::GetKeyFromFloat(cmd.params[1]);
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

const string CWaitCommandsAI::Wait::noText = "";


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
	deadTime = time(NULL) + maxNetDelay;
}

// static
float CWaitCommandsAI::Wait::GetFloatFromKey(KeyType k)
{
	return *((float*)&k);
}


CWaitCommandsAI::Wait::Wait(float _code)
: code(_code)
{
	key = 0;
	valid = false;
	deadTime = time(NULL) + maxNetDelay;
}


CWaitCommandsAI::Wait::~Wait()
{
	waitCommandsAI.RemoveWaitObject(this);
}


CWaitCommandsAI::Wait::WaitState
	CWaitCommandsAI::Wait::GetWaitState(const CUnit* unit) const
{
	const CCommandQueue& dq = unit->commandAI->commandQue;
	if (dq.empty()) {
		return Missing;
	}
	const Command& cmd = dq.front();
	if ((cmd.id == CMD_WAIT) && (cmd.params.size() == 2) &&
			(cmd.params[0] == code) &&
			(GetKeyFromFloat(cmd.params[1]) == key)) {
		return Active;
	}

	CCommandQueue::const_iterator it = dq.begin();
	it++;
	for ( ; it != dq.end(); ++it) {
		const Command& qcmd = *it;
		if ((qcmd.id == CMD_WAIT) && (qcmd.params.size() == 2) &&
				(qcmd.params[0] == code) &&
				(GetKeyFromFloat(qcmd.params[1]) == key)) {
			return Queued;
		}
	}
	return Missing;
}


bool CWaitCommandsAI::Wait::IsWaitingOn(const CUnit* unit) const
{
	const CCommandQueue& dq = unit->commandAI->commandQue;
	if (dq.empty()) {
		return false;
	}
	const Command& cmd = dq.front();
	if ((cmd.id == CMD_WAIT) && (cmd.params.size() == 2) &&
			(cmd.params[0] == code) &&
			(GetKeyFromFloat(cmd.params[1]) == key)) {
		return true;
	}
	return false;
}


void CWaitCommandsAI::Wait::SendCommand(const Command& cmd,
																				const CUnitSet& unitSet)
{
	GML_RECMUTEX_LOCK(sel); // SendCommand

	if (unitSet.empty()) {
		return;
	}

	const CUnitSet& selUnits = selectedUnits.selectedUnits;
	if (unitSet == selUnits) {
		selectedUnits.GiveCommand(cmd, false);
		return;
	}

	CUnitSet tmpSet = selUnits;
	CUnitSet::const_iterator it;

	selectedUnits.ClearSelected();
	for (it = unitSet.begin(); it != unitSet.end(); ++it) {
		selectedUnits.AddUnit(*it);
	}

	selectedUnits.GiveCommand(cmd, false);

	selectedUnits.ClearSelected();
	for (it = tmpSet.begin(); it != tmpSet.end(); ++it) {
		selectedUnits.AddUnit(*it);
	}
}


void CWaitCommandsAI::Wait::SendWaitCommand(const CUnitSet& unitSet)
{
	Command waitCmd;
	waitCmd.id = CMD_WAIT;
	SendCommand(waitCmd, unitSet);
}


CUnitSet::iterator CWaitCommandsAI::Wait::RemoveUnitFromSet(CUnitSet::iterator it, CUnitSet& unitSet)
{
	CUnitSet::iterator tmp = it;
	++tmp;
	unitSet.erase(it);
	return tmp;
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
		return NULL;
	}
	return tw;
}


CWaitCommandsAI::TimeWait*
	CWaitCommandsAI::TimeWait::New(int duration, CUnit* unit)
{
	TimeWait* tw = new TimeWait(duration, unit);
	if (!tw->valid) {
		delete tw;
		return NULL;
	}
	return tw;
}


CWaitCommandsAI::TimeWait::TimeWait(const Command& cmd, CUnit* _unit)
: Wait(CMD_WAITCODE_TIMEWAIT)
{
	if (cmd.params.size() != 1) {
		return;
	}

	valid = true;
	key = GetNewKey();

	unit = _unit;
	enabled = false;
	endFrame = 0;
	duration = GAME_SPEED * (int)cmd.params[0];
	factory = (dynamic_cast<CFactory*>(unit) != NULL);

	Command waitCmd;
	waitCmd.id = CMD_WAIT;
	waitCmd.options = cmd.options;
	waitCmd.params.push_back(code);
	waitCmd.params.push_back(GetFloatFromKey(key));

	selectedUnits.ClearSelected();
	selectedUnits.AddUnit(unit);
	selectedUnits.GiveCommand(waitCmd);

	AddDeathDependence((CObject*)unit);

	return;
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

	AddDeathDependence((CObject*)unit);
}


CWaitCommandsAI::TimeWait::~TimeWait()
{
	// do nothing
}


void CWaitCommandsAI::TimeWait::DependentDied(CObject* object)
{
	unit = NULL;
}


void CWaitCommandsAI::TimeWait::AddUnit(CUnit* unit)
{
	// do nothing
}


void CWaitCommandsAI::TimeWait::RemoveUnit(CUnit* _unit)
{
	if (_unit == unit) {
		delete this;
		return;
	}
}


void CWaitCommandsAI::TimeWait::Update()
{
	if (unit == NULL) {
		delete this;
		return;
	}

	WaitState state = GetWaitState(unit);

	if (state == Active) {
		if (!enabled) {
			enabled = true;
			endFrame = (gs->frameNum + duration);
		}
		else {
			if (endFrame <= gs->frameNum) {
				CUnitSet smallSet;
				smallSet.insert(unit);
				SendWaitCommand(smallSet);
				if (!factory) {
					delete this;
					return;
				} else {
					enabled = false;
				}
			}
		}
	}
	else if (state == Queued) {
		return;
	}
	else if (state == Missing) {
		if (!factory) { // FIXME
			delete this;
			return;
		}
	}
}


const string& CWaitCommandsAI::TimeWait::GetStateText() const
{
	static char buf[32];
	if (enabled) {
		const int remaining =
			1 + (std::max(0, (endFrame - gs->frameNum - 1)) / GAME_SPEED);
		SNPRINTF(buf, sizeof(buf), "%i", remaining);
	} else {
		SNPRINTF(buf, sizeof(buf), "%i", duration / GAME_SPEED);
	}
	static string text;
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
		return NULL;
	}
	return dw;
}


CWaitCommandsAI::DeathWait::DeathWait(const Command& cmd)
: Wait(CMD_WAITCODE_DEATHWAIT)
{
	GML_RECMUTEX_LOCK(sel); // DeathWait

	const CUnitSet& selUnits = selectedUnits.selectedUnits;

	if (cmd.params.size() == 1) {
		const int unitID = (int)cmd.params[0];
		if ((unitID < 0) || (unitID >= MAX_UNITS)) {
			return;
		}
		CUnit* unit = uh->units[unitID];
		if (unit == NULL) {
			return;
		}
		if (selUnits.find(unit) != selUnits.end()) {
			return;
		}
		deathUnits.insert(unit);
	}
	else if (cmd.params.size() == 6) {
		const float3 pos0(cmd.params[0], cmd.params[1], cmd.params[2]);
		const float3 pos1(cmd.params[3], cmd.params[4], cmd.params[5]);
		CUnitSet tmpSet;
		SelectAreaUnits(pos0, pos1, tmpSet, false);
		CUnitSet::iterator it;
		for (it = tmpSet.begin(); it != tmpSet.end(); ++it) {
			if (selUnits.find(*it) == selUnits.end()) {
				deathUnits.insert(*it);
			}
		}
		if (deathUnits.empty()) {
			return;
		}
	}
	else {
		return; // unknown param config
	}

	valid = true;
	key = GetNewKey();

	waitUnits = selUnits;

	Command waitCmd;
	waitCmd.id = CMD_WAIT;
	waitCmd.options = cmd.options;
	waitCmd.params.push_back(code);
	waitCmd.params.push_back(GetFloatFromKey(key));
	selectedUnits.GiveCommand(waitCmd);

	CUnitSet::iterator it;
	for (it = waitUnits.begin(); it != waitUnits.end(); ++it) {
		AddDeathDependence((CObject*)(*it));
	}
	for (it = deathUnits.begin(); it != deathUnits.end(); ++it) {
		AddDeathDependence((CObject*)(*it));
	}

	return;
}


CWaitCommandsAI::DeathWait::~DeathWait()
{
	// do nothing
}


void CWaitCommandsAI::DeathWait::DependentDied(CObject* object)
{
	CUnit* unit = (CUnit*)object;

	CUnitSet::iterator wit = waitUnits.find(unit);
	if (wit != waitUnits.end()) {
		waitUnits.erase(wit);
	}
	if (waitUnits.empty()) {
		return;
	}

	CUnitSet::iterator dit = deathUnits.find(unit);
	if (dit != deathUnits.end()) {
		deathUnits.erase(dit);
	}
}


void CWaitCommandsAI::DeathWait::AddUnit(CUnit* unit)
{
	waitUnits.insert(unit);
	AddDeathDependence((CObject*)unit);
}


void CWaitCommandsAI::DeathWait::RemoveUnit(CUnit* unit)
{
	CUnitSet::iterator it = waitUnits.find(unit);
	if (it != waitUnits.end()) {
		DeleteDeathDependence(*it);
		waitUnits.erase(it);
	}
}


void CWaitCommandsAI::DeathWait::Update()
{
	if (waitUnits.empty()) {
		delete this;
		return;
	}

	if (!deathUnits.empty()) {
		return; // more must die
	}

	CUnitSet unblockSet;
	CUnitSet::iterator it = waitUnits.begin();
	while (it != waitUnits.end()) {
		WaitState state = GetWaitState(*it);
		if (state == Active) {
			unblockSet.insert(*it);
			DeleteDeathDependence(*it);
			it = RemoveUnitFromSet(it, waitUnits);
			continue;
		}
		else if (state == Queued) {
			// do nothing
		}
		else if (state == Missing) {
			DeleteDeathDependence(*it);
			it = RemoveUnitFromSet(it, waitUnits);
			continue;
		}
		++it;
	}
	SendWaitCommand(unblockSet);
	if (waitUnits.empty()) {
		delete this;
		return;
	}
}


void CWaitCommandsAI::DeathWait::Draw() const
{
	CUnitSet drawSet;
	CUnitSet::const_iterator it;

	if (unitPos.empty()) {
		return;
	}

	float3 midPos(0.0f, 0.0f, 0.0f);
	for (int i = 0; i < (int)unitPos.size(); i++) {
		midPos += unitPos[i];
	}
	midPos /= (float)unitPos.size();

	cursorIcons.AddIcon(CMD_DEATHWAIT, midPos);

	for (int i = 0; i < (int)unitPos.size(); i++) {
		lineDrawer.StartPath(unitPos[i], cmdColors.start);
		lineDrawer.DrawLine(midPos, cmdColors.deathWait);
		lineDrawer.FinishPath();
	}

	for (it = deathUnits.begin(); it != deathUnits.end(); ++it) {
		const CUnit* unit = *it;
		if (unit->losStatus[gu->myAllyTeam] & (LOS_INLOS | LOS_INRADAR)) {
			cursorIcons.AddIcon(CMD_SELFD, (*it)->midPos);
			lineDrawer.StartPath(midPos, cmdColors.start);
			lineDrawer.DrawLine(unit->midPos, cmdColors.deathWait);
			lineDrawer.FinishPath();
		}
	}

	unitPos.clear();
}


void CWaitCommandsAI::DeathWait::AddUnitPosition(const float3& pos) const
{
	unitPos.push_back(pos);
	return;
}


void CWaitCommandsAI::DeathWait::SelectAreaUnits(
	const float3& pos0, const float3& pos1, CUnitSet& units, bool enemies)
{
	units.clear();

	const float3 mins(std::min(pos0.x, pos1.x), 0.0f, std::min(pos0.z, pos1.z));
	const float3 maxs(std::max(pos0.x, pos1.x), 0.0f, std::max(pos0.z, pos1.z));

	std::vector<CUnit*> tmpUnits = qf->GetUnitsExact(mins, maxs);

	const int count = (int)tmpUnits.size();
	for (int i = 0; i < count; i++) {
		CUnit* unit = tmpUnits[i];
		if (enemies && gs->Ally(unit->allyteam, gu->myAllyTeam)) {
			continue;
		}
		if (!(unit->losStatus[gu->myAllyTeam] & (LOS_INLOS | LOS_INRADAR))) {
			continue;
		}
		units.insert(unit);
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
		return NULL;
	}
	return sw;
}


CWaitCommandsAI::SquadWait::SquadWait(const Command& cmd)
: Wait(CMD_WAITCODE_SQUADWAIT)
{
	GML_RECMUTEX_LOCK(sel); // SquadWait

	if (cmd.params.size() != 1) {
		return;
	}

	squadCount = (int)cmd.params[0];
	if (squadCount < 2) {
		return;
	}

	const CUnitSet& selUnits = selectedUnits.selectedUnits;
	CUnitSet::const_iterator it;
	for (it = selUnits.begin(); it != selUnits.end(); ++it) {
		CUnit* unit = *it;
		if (dynamic_cast<CFactory*>(unit)) {
			buildUnits.insert(unit);
		} else {
			waitUnits.insert(unit);
		}
	}
	if (buildUnits.empty() && ((int)waitUnits.size() < squadCount)) {
		return;
	}

	valid = true;
	key = GetNewKey();

	Command waitCmd;
	waitCmd.id = CMD_WAIT;
	waitCmd.options = cmd.options;
	waitCmd.params.push_back(code);
	waitCmd.params.push_back(GetFloatFromKey(key));

	SendCommand(waitCmd, buildUnits);
	SendCommand(waitCmd, waitUnits);

	for (it = buildUnits.begin(); it != buildUnits.end(); ++it) {
		AddDeathDependence((CObject*)(*it));
	}
	for (it = waitUnits.begin(); it != waitUnits.end(); ++it) {
		AddDeathDependence((CObject*)(*it));
	}

	UpdateText();

	return;
}


CWaitCommandsAI::SquadWait::~SquadWait()
{
	// do nothing
}


void CWaitCommandsAI::SquadWait::DependentDied(CObject* object)
{
	CUnit* unit = (CUnit*)object;

	CUnitSet::iterator bit = buildUnits.find(unit);
	if (bit != buildUnits.end()) {
		buildUnits.erase(bit);
	}
	CUnitSet::iterator wit = waitUnits.find(unit);
	if (wit != waitUnits.end()) {
		waitUnits.erase(wit);
	}
}


void CWaitCommandsAI::SquadWait::AddUnit(CUnit* unit)
{
	waitUnits.insert(unit);
	AddDeathDependence((CObject*)unit);
}


void CWaitCommandsAI::SquadWait::RemoveUnit(CUnit* unit)
{
	CUnitSet::iterator bit = buildUnits.find(unit);
	if (bit != buildUnits.end()) {
		DeleteDeathDependence(*bit);
		buildUnits.erase(bit);
	}
	CUnitSet::iterator wit = waitUnits.find(unit);
	if (wit != waitUnits.end()) {
		DeleteDeathDependence(*wit);
		waitUnits.erase(wit);
	}
}


void CWaitCommandsAI::SquadWait::Update()
{
	if (buildUnits.empty() && ((int)waitUnits.size() < squadCount)) {
		// FIXME -- unblock remaining waitUnits ?
		delete this;
		return;
	}

	if ((int)waitUnits.size() >= squadCount) {
		CUnitSet unblockSet;
		CUnitSet::iterator it = waitUnits.begin();
		while (it != waitUnits.end()) {
			WaitState state = GetWaitState(*it);
			if (state == Active) {
				unblockSet.insert(*it);
				if ((int)unblockSet.size() >= squadCount) {
					break; // we've got our squad
				}
			}
			else if (state == Queued) {
				// do nothing
			}
			else if (state == Missing) {
				DeleteDeathDependence(*it);
				it = RemoveUnitFromSet(it, waitUnits);
				continue;
			}
			++it;
		}

		if ((int)unblockSet.size() >= squadCount) {
			// FIXME -- rebuild the order queue so
			//          that formations are created?
			SendWaitCommand(unblockSet);
			for (it = unblockSet.begin(); it != unblockSet.end(); ++it) {
				DeleteDeathDependence(*it);
				waitUnits.erase(it);
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
		return NULL;
	}
	return gw;
}


CWaitCommandsAI::GatherWait::GatherWait(const Command& cmd)
: Wait(CMD_WAITCODE_GATHERWAIT)
{
	GML_RECMUTEX_LOCK(sel); // GatherWait

	if (cmd.params.size() != 0) {
		return;
	}

	// only add valid units
	const CUnitSet& selUnits = selectedUnits.selectedUnits;
	CUnitSet::const_iterator sit;
	for (sit = selUnits.begin(); sit != selUnits.end(); ++sit) {
		CUnit* unit = *sit;
		const UnitDef* ud = unit->unitDef;
		if (ud->canmove && (dynamic_cast<CFactory*>(unit) == NULL)) {
			waitUnits.insert(unit);
		}
	}

	if (waitUnits.size() < 2) {
		return; // one man does not a gathering make
	}

	valid = true;
	key = GetNewKey();

	Command waitCmd;
	waitCmd.id = CMD_WAIT;
	waitCmd.options = SHIFT_KEY;
	waitCmd.params.push_back(code);
	waitCmd.params.push_back(GetFloatFromKey(key));
	selectedUnits.GiveCommand(waitCmd, true);

	CUnitSet::iterator wit;
	for (wit = waitUnits.begin(); wit != waitUnits.end(); ++wit) {
		AddDeathDependence((CObject*)(*wit));
	}

	return;
}


CWaitCommandsAI::GatherWait::~GatherWait()
{
	// do nothing
}


void CWaitCommandsAI::GatherWait::DependentDied(CObject* object)
{
	CUnit* unit = (CUnit*)object;

	CUnitSet::iterator it = waitUnits.find(unit);
	if (it != waitUnits.end()) {
		waitUnits.erase(it);
	}
}


void CWaitCommandsAI::GatherWait::AddUnit(CUnit* unit)
{
	// do nothing
}


void CWaitCommandsAI::GatherWait::RemoveUnit(CUnit* unit)
{
	CUnitSet::iterator it = waitUnits.find(unit);
	if (it != waitUnits.end()) {
		DeleteDeathDependence(*it);
		waitUnits.erase(it);
	}
}


void CWaitCommandsAI::GatherWait::Update()
{
	if (waitUnits.size() < 2) {
		delete this;
		return;
	}

	CUnitSet::iterator it = waitUnits.begin();
	while (it != waitUnits.end()) {
		WaitState state = GetWaitState(*it);
		if (state == Active) {
			// do nothing
		}
		else if(state == Queued) {
			return;
		}
		else if (state == Missing) {
			DeleteDeathDependence(*it);
			it = RemoveUnitFromSet(it, waitUnits);
			if (waitUnits.empty()) {
				delete this;
				return;
			}
			continue;
		}
		++it;
	}

	// all units are actively waiting on this command, unblock them and die
	SendWaitCommand(waitUnits);
	delete this;
}


/******************************************************************************/
