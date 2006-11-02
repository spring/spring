// WaitCommandsAI.cpp: implementation of the CWaitCommands class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "WaitCommandsAI.h"
#include <SDL_timer.h>
#include "SelectedUnits.h"
#include "Team.h"
#include "Game/GameHelper.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/CommandAI/LineDrawer.h"
#include "Sim/Units/UnitTypes/Factory.h"
#include "System/GlobalStuff.h"
#include "System/Object.h"
#include "UI/CommandColors.h"
#include "UI/CursorIcons.h"


CWaitCommandsAI waitCommandsAI;


static const int maxNetDelay = 30;  // in seconds

static const int updatePeriod = 3;  // 100 ms 


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
	// save the current selection
	const UnitSet tmpSet = selectedUnits.selectedUnits;
	UnitSet::const_iterator it;
	for (it = tmpSet.begin(); it != tmpSet.end(); ++it) {
		InsertWaitObject(TimeWait::New(cmd, *it));
	}
	// restore the selection
	selectedUnits.ClearSelected();
	for (it = tmpSet.begin(); it != tmpSet.end(); ++it) {
		selectedUnits.AddUnit(*it);
	}
}


void CWaitCommandsAI::AddSquadWait(const Command& cmd)
{
	InsertWaitObject(SquadWait::New(cmd));
}


void CWaitCommandsAI::AddDeathWatch(const Command& cmd)
{
	InsertWaitObject(DeathWatch::New(cmd));
}


void CWaitCommandsAI::AddRallyPoint(const Command& cmd)
{
	InsertWaitObject(RallyPoint::New(cmd));
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


void CWaitCommandsAI::NewUnit(CUnit* unit, const CUnit* builder)
{
	// NOTE: the wait keys will link the right units to
	//       the correct player (for multi-player teams)
	if (unit->team != gu->myTeam) {
		return;
	}
	const deque<Command>& dq = unit->commandAI->commandQue;
	deque<Command>::const_iterator qit;
	for (qit = dq.begin(); qit != dq.end(); ++qit) {
		const Command& cmd = *qit;
		if ((cmd.id == CMD_WAIT) && (cmd.params.size() == 2)) {
			const KeyType key = Wait::GetKeyFromFloat(cmd.params[1]);
			WaitMap::iterator wit = waitMap.find(key);
			if (wit != waitMap.end()) {
				Wait* wait = wit->second;
				if (cmd.params[0] == wait->GetCode()) {
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
	else if (code == CMD_WAITCODE_DEATHWATCH) {
		lineDrawer.DrawIconAtLastPos(CMD_DEATHWATCH);
	}
	else if (code == CMD_WAITCODE_RALLYPOINT) {
		lineDrawer.DrawIconAtLastPos(CMD_RALLYPOINT);
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
	const deque<Command>& dq = unit->commandAI->commandQue;
	if (dq.empty()) {
		return Missing;
	}
	const Command& cmd = dq.front();
	if ((cmd.id == CMD_WAIT) && (cmd.params.size() == 2) &&
			(cmd.params[0] == code) &&
			(GetKeyFromFloat(cmd.params[1]) == key)) {
		return Active;
	}

	deque<Command>::const_iterator it = dq.begin();
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
	const deque<Command>& dq = unit->commandAI->commandQue;
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
																				const UnitSet& unitSet)
{
	if (unitSet.empty()) {
		return;
	}
	
	const UnitSet& selUnits = selectedUnits.selectedUnits;
	if (unitSet == selUnits) {
		selectedUnits.GiveCommand(cmd, false);
		return;
	}
		
	UnitSet tmpSet = selUnits;
	UnitSet::const_iterator it;

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


void CWaitCommandsAI::Wait::SendWaitCommand(const UnitSet& unitSet)
{
	Command waitCmd;
	waitCmd.id = CMD_WAIT;
	SendCommand(waitCmd, unitSet);
}


CWaitCommandsAI::UnitSet::iterator
	CWaitCommandsAI::Wait::RemoveUnit(UnitSet::iterator it, UnitSet& unitSet)
{
	UnitSet::iterator tmp = it;
	tmp++;
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
				UnitSet smallSet;
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
			1 + (max(0, (endFrame - gs->frameNum - 1)) / GAME_SPEED);
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
	if (cmd.params.size() != 1) {
		return;
	}

	squadCount = (int)cmd.params[0];
	if (squadCount < 2) {
		return;
	}

	const UnitSet& selUnits = selectedUnits.selectedUnits;
	UnitSet::const_iterator it;
	for (it = selUnits.begin(); it != selUnits.end(); ++it) {
		CUnit* unit = *it;
		if (dynamic_cast<CFactory*>(unit)) {
			buildUnits.insert(unit);
		}
	}
	if (buildUnits.empty()) {
		return;
	}
	
	valid = true;
	key = GetNewKey();
	
	UpdateText();
	
	Command waitCmd;
	waitCmd.id = CMD_WAIT;
	waitCmd.options = cmd.options;
	waitCmd.params.push_back(code);
	waitCmd.params.push_back(GetFloatFromKey(key));
	
	SendCommand(waitCmd, buildUnits);

	for (it = buildUnits.begin(); it != buildUnits.end(); ++it) {
		AddDeathDependence((CObject*)(*it));
	}

	return;
}


CWaitCommandsAI::SquadWait::~SquadWait()
{
	// do nothing
}


void CWaitCommandsAI::SquadWait::DependentDied(CObject* object)
{
	CUnit* unit = (CUnit*)object;

	UnitSet::iterator bit = buildUnits.find(unit);
	if (bit != buildUnits.end()) {
		buildUnits.erase(bit);
	}
	if (buildUnits.empty()) {
		return;
	}

	UnitSet::iterator wit = waitUnits.find(unit);
	if (wit != waitUnits.end()) {
		waitUnits.erase(wit);
	}
}


void CWaitCommandsAI::SquadWait::AddUnit(CUnit* unit)
{
	waitUnits.insert(unit);
	AddDeathDependence((CObject*)unit);
}


void CWaitCommandsAI::SquadWait::Update()
{
	if (buildUnits.empty()) {
		delete this;
		return;
	}

	if ((int)waitUnits.size() >= squadCount) {
		UnitSet unblockSet;
		UnitSet::iterator it = waitUnits.begin();
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
				it = RemoveUnit(it, waitUnits);
				continue;
			}
			it++;
		}

		if ((int)unblockSet.size() >= squadCount) {
			// FIXME -- rebuild the order queue so
			//          that formations are created?
			SendWaitCommand(unblockSet);
			for (it = unblockSet.begin(); it != unblockSet.end(); ++it) {
				waitUnits.erase(*it);
				DeleteDeathDependence(*it);
			}
		}
	}

	UpdateText();
	// FIXME -- clean builders
}


void CWaitCommandsAI::SquadWait::UpdateText()
{
	// FIXME -- waitUnits.size() is not the right value
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
//  DeathWatch
//

CWaitCommandsAI::DeathWatch*
	CWaitCommandsAI::DeathWatch::New(const Command& cmd)
{
	DeathWatch* dw = new DeathWatch(cmd);
	if (!dw->valid) {
		delete dw;
		return NULL;
	}
	return dw;
}


CWaitCommandsAI::DeathWatch::DeathWatch(const Command& cmd)
: Wait(CMD_WAITCODE_DEATHWATCH)
{
	const UnitSet& selUnits = selectedUnits.selectedUnits;
	
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
		UnitSet tmpSet;
		SelectAreaUnits(pos0, pos1, tmpSet, false);
		UnitSet::iterator it;
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
	
	UnitSet::iterator it;
	for (it = waitUnits.begin(); it != waitUnits.end(); ++it) {
		AddDeathDependence((CObject*)(*it));
	}
	for (it = deathUnits.begin(); it != deathUnits.end(); ++it) {
		AddDeathDependence((CObject*)(*it));
	}
	
	return;
}


CWaitCommandsAI::DeathWatch::~DeathWatch()
{
	// do nothing
}


void CWaitCommandsAI::DeathWatch::DependentDied(CObject* object)
{
	CUnit* unit = (CUnit*)object;

	UnitSet::iterator wit = waitUnits.find(unit);
	if (wit != waitUnits.end()) {
		waitUnits.erase(wit);
	}
	if (waitUnits.empty()) {
		return;
	}

	UnitSet::iterator dit = deathUnits.find(unit);
	if (dit != deathUnits.end()) {
		deathUnits.erase(dit);
	}
}


void CWaitCommandsAI::DeathWatch::AddUnit(CUnit* unit)
{
	waitUnits.insert(unit);
	AddDeathDependence((CObject*)unit);
}


void CWaitCommandsAI::DeathWatch::Update()
{
	if (waitUnits.empty()) {
		delete this;
		return;
	}
	
	if (!deathUnits.empty()) {
		return; // more must die
	}

	UnitSet unblockSet;
	UnitSet::iterator it = waitUnits.begin();
	while (it != waitUnits.end()) {
		WaitState state = GetWaitState(*it);
		if (state == Active) {
			unblockSet.insert(*it);
			DeleteDeathDependence(*it);
			it = RemoveUnit(it, waitUnits);
			continue;
		}
		else if (state == Queued) {
			// do nothing
		}		
		else if (state == Missing) {
			DeleteDeathDependence(*it);
			it = RemoveUnit(it, waitUnits);
			continue;
		}
		it++;
	}
	SendWaitCommand(unblockSet);
	if (waitUnits.empty()) {
		delete this;
		return;
	}
}


void CWaitCommandsAI::DeathWatch::Draw() const
{
	const UnitSet& selUnits = selectedUnits.selectedUnits;
	UnitSet drawSet;
	UnitSet::const_iterator it;
	for (it = waitUnits.begin(); it != waitUnits.end(); ++it) {
		CUnit* unit = *it;
		if (IsWaitingOn(unit) && (selUnits.find(unit) != selUnits.end())) {
			drawSet.insert(*it);
		}
	}
	if (drawSet.empty()) {
		return;
	}

	for (it = deathUnits.begin(); it != deathUnits.end(); ++it) {
		const CUnit* unit = *it;
		if (unit->losStatus[gu->myAllyTeam] & (LOS_INLOS | LOS_INRADAR)) {
			cursorIcons.AddIcon(CMD_DEATHWATCH, (*it)->midPos);
		}
	}
	
	if (drawSet.size() == 1) {
		const float3& midPos = (*drawSet.begin())->midPos;
		for (it = deathUnits.begin(); it != deathUnits.end(); ++it) {
			const CUnit* unit = *it;
			if (unit->losStatus[gu->myAllyTeam] & (LOS_INLOS | LOS_INRADAR)) {
				lineDrawer.StartPath(midPos, cmdColors.start);
				lineDrawer.DrawLine(unit->midPos, cmdColors.deathWatch);
				lineDrawer.FinishPath();
			}
		}
	}
}


void CWaitCommandsAI::DeathWatch::SelectAreaUnits(
	const float3& pos0, const float3& pos1, UnitSet& units, bool enemies)
{
	units.clear();
	
	const float3 mins(min(pos0.x, pos1.x), 0.0f, min(pos0.z, pos1.z));
	const float3 maxs(max(pos0.x, pos1.x), 0.0f, max(pos0.z, pos1.z));
	
	vector<CUnit*> tmpUnits = qf->GetUnitsExact(mins, maxs);
	
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
//  RallyPoint
//

CWaitCommandsAI::RallyPoint*
	CWaitCommandsAI::RallyPoint::New(const Command& cmd)
{
	RallyPoint* rp = new RallyPoint(cmd);
	if (!rp->valid) {
		delete rp;
		return NULL;
	}
	return rp;
}


CWaitCommandsAI::RallyPoint::RallyPoint(const Command& cmd)
: Wait(CMD_WAITCODE_RALLYPOINT)
{
	if ((cmd.params.size() != 3) && (cmd.params.size() != 6)) {
		return;
	}

	// always send a move
	Command moveCmd = cmd;
	moveCmd.id = CMD_MOVE;
	selectedUnits.GiveCommand(moveCmd);

	// only add valid units
	const UnitSet& selUnits = selectedUnits.selectedUnits;
	UnitSet::const_iterator sit;
	for (sit = selUnits.begin(); sit != selUnits.end(); ++sit) {
		CUnit* unit = *sit;
		const UnitDef* ud = unit->unitDef;
		if (ud->canmove && (dynamic_cast<CFactory*>(unit) == NULL)) {
			waitUnits.insert(unit);
		}
	}
	
	if (waitUnits.size() < 2) {
		return; // one man does not a rally make
	}

	valid = true;
	key = GetNewKey();

	Command waitCmd;
	waitCmd.id = CMD_WAIT;
	waitCmd.options = SHIFT_KEY;
	waitCmd.params.push_back(code);
	waitCmd.params.push_back(GetFloatFromKey(key));
	selectedUnits.GiveCommand(waitCmd, false);
	
	UnitSet::iterator wit;
	for (wit = waitUnits.begin(); wit != waitUnits.end(); ++wit) {
		AddDeathDependence((CObject*)(*wit));
	}

	return;
}


CWaitCommandsAI::RallyPoint::~RallyPoint()
{
	// do nothing
}


void CWaitCommandsAI::RallyPoint::DependentDied(CObject* object)
{
	CUnit* unit = (CUnit*)object;

	UnitSet::iterator it = waitUnits.find(unit);
	if (it != waitUnits.end()) {
		waitUnits.erase(it);
	}
}


void CWaitCommandsAI::RallyPoint::AddUnit(CUnit* unit)
{
	// do nothing
}


void CWaitCommandsAI::RallyPoint::Update()
{
	if (waitUnits.empty()) {
		delete this;
		return;
	}
	
	UnitSet::iterator it = waitUnits.begin();
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
			it = RemoveUnit(it, waitUnits);
			if (waitUnits.empty()) {
				delete this;
				return;
			}
			continue;
		}
		it++;
	}

	// all units are actively waiting on this command, unblock them and die
	SendWaitCommand(waitUnits);
	delete this;
}


/******************************************************************************/
