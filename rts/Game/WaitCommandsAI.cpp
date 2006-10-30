// WaitCommandsAI.cpp: implementation of the CWaitCommands class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "WaitCommandsAI.h"
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


const int maxNetDelay = 60;  // in frames  (2 seconds)


/******************************************************************************/
/******************************************************************************/

CWaitCommandsAI::CWaitCommandsAI()
{
}


CWaitCommandsAI::~CWaitCommandsAI()
{
  WaitSet::iterator it;
  for (it = waitSet.begin(); it != waitSet.end(); ++it) {
    delete *it;
  }
}


void CWaitCommandsAI::Update()
{
  if ((gs->frameNum % 30) == 0) printf("Waits: %i\n", waitSet.size());
  WaitSet::iterator it = waitSet.begin();
  while (it != waitSet.end()) {
    // grab an incremented iterator in case the active iterator deletes itself
    WaitSet::iterator tmp = it;
    tmp++;
    (*it)->Update();
    it = tmp;
  }
}


void CWaitCommandsAI::DrawCommands()
{
  WaitSet::iterator it;
  for (it = waitSet.begin(); it != waitSet.end(); ++it) {
    (*it)->Draw();
  }
}


void CWaitCommandsAI::AddTimeWait(const Command& cmd)
{
  const UnitSet tmpSet = selectedUnits.selectedUnits;
  UnitSet::const_iterator it;
  for (it = tmpSet.begin(); it != tmpSet.end(); ++it) {
    TimeWait* tw = TimeWait::New(cmd, *it);
    if (tw != NULL)  {
      waitSet.insert(tw);
    }
  }
  selectedUnits.ClearSelected();
  for (it = tmpSet.begin(); it != tmpSet.end(); ++it) {
    selectedUnits.AddUnit(*it);
  }
}


void CWaitCommandsAI::AddSquadWait(const Command& cmd)
{
  SquadWait* sw = SquadWait::New(cmd);
  if (sw != NULL)  {
    waitSet.insert(sw);
  }
}


void CWaitCommandsAI::AddDeathWatch(const Command& cmd)
{
  DeathWatch* tw = DeathWatch::New(cmd);
  if (tw != NULL)  {
    waitSet.insert(tw);
  }
}


void CWaitCommandsAI::AddRallyPoint(const Command& cmd)
{
  RallyPoint* tw = RallyPoint::New(cmd);
  if (tw != NULL)  {
    waitSet.insert(tw);
  }
}


void CWaitCommandsAI::NewUnit(CUnit* unit, const CUnit* builder)
{
  const deque<Command>& dq = unit->commandAI->commandQue;
  deque<Command>::const_iterator qit;
  for (qit = dq.begin(); qit != dq.end(); ++qit) {
    const Command& cmd = *qit;
    if ((cmd.id == CMD_WAIT) && (cmd.params.size() == 2)) {
      WaitSet::iterator wit;
      for (wit = waitSet.begin(); wit != waitSet.end(); ++wit) {
        Wait* wait = *wit;
        if ((cmd.params[0] == wait->GetCode()) &&
            (cmd.params[1] == wait->GetKey())) {
          const float code = cmd.params[0];
          if (code == CMD_WAITCODE_TIMEWAIT) {
            const int duration = ((TimeWait*)wait)->GetDuration();
            TimeWait* tw = TimeWait::New(duration, unit);
            if (tw != NULL)  {
              waitSet.insert(tw);
              // FIXME -- const_cast<Command&>(cmd).params[1] = tw->GetKey();
            }
          }
          else if ((code == CMD_WAITCODE_SQUADWAIT) ||
                   (code == CMD_WAITCODE_DEATHWATCH)) {
            wait->AddUnit(unit);
          }
          // CMD_WAITCODE_RALLYPOINT can not accept new units
        }
      }
    }
  }
}


void CWaitCommandsAI::DeleteWaitObject(Wait* wait)
{
  WaitSet::iterator it = waitSet.find(wait);
  if (it != waitSet.end()) {
    waitSet.erase(it);
  }
}


/******************************************************************************/
//
//  Wait Base Class
//

CWaitCommandsAI::Wait::Wait(float _code)
: code(_code)
{
  key = 0.0f; // invalid key
  deadFrame = (gs->frameNum + maxNetDelay);
}


CWaitCommandsAI::Wait::~Wait()
{
  waitCommandsAI.DeleteWaitObject(this);
}


CWaitCommandsAI::Wait::WaitState
  CWaitCommandsAI::Wait::GetWaitState(const CUnit* unit) const
{
  const deque<Command>& dq = unit->commandAI->commandQue;
  if (dq.empty()) {
    return (deadFrame < gs->frameNum) ? Missing : Queued;
  }
  const Command& cmd = dq.front();
  if ((cmd.id == CMD_WAIT) && (cmd.params.size() == 2) &&
      (cmd.params[0] == code) && (cmd.params[1] == key)) {
    return Active;
  }

  deque<Command>::const_iterator it = dq.begin();
  it++;
  for ( ; it != dq.end(); ++it) {
    const Command& qcmd = *it;
    if ((qcmd.id == CMD_WAIT) && (qcmd.params.size() == 2) &&
        (qcmd.params[0] == code) && (qcmd.params[1] == key)) {
      return Queued;
    }
  }
  return (deadFrame < gs->frameNum) ? Missing : Queued;
}


bool CWaitCommandsAI::Wait::IsWaitingOn(const CUnit* unit) const
{
  const deque<Command>& dq = unit->commandAI->commandQue;
  if (dq.empty()) {
    return false;
  }
  const Command& cmd = dq.front();
  if ((cmd.id == CMD_WAIT) && (cmd.params.size() == 2) &&
      (cmd.params[0] == code) && (cmd.params[1] == key)) {
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

float CWaitCommandsAI::TimeWait::keySource = 1.0f;


CWaitCommandsAI::TimeWait*
  CWaitCommandsAI::TimeWait::New(const Command& cmd, CUnit* unit)
{
  TimeWait* tw = new TimeWait(cmd, unit);
  if (tw->key == 0.0f) {
    delete tw;
    return NULL;
  }
  return tw;
}


CWaitCommandsAI::TimeWait*
  CWaitCommandsAI::TimeWait::New(int duration, CUnit* unit)
{
  TimeWait* tw = new TimeWait(duration, unit);
  if (tw->key == 0.0f) {
    delete tw;
    return NULL;
  }
  return tw;
}


CWaitCommandsAI::TimeWait::TimeWait(const Command& cmd, CUnit* _unit)
: Wait(CMD_WAITCODE_TIMEWAIT)
{
  key = 0.0f; // the invalid key
  
  if (cmd.params.size() != 1) {
    return;
  }

  key = keySource;
  keySource += 1.0f;

  unit = _unit;
  enabled = false;
  endFrame = 0;
  duration = 30 * (int)cmd.params[0];

  Command waitCmd;
  waitCmd.id = CMD_WAIT;
  waitCmd.options = cmd.options;
  waitCmd.params.push_back(code);
  waitCmd.params.push_back(key);

  selectedUnits.ClearSelected();
  selectedUnits.AddUnit(unit);
  selectedUnits.GiveCommand(waitCmd);
  
  AddDeathDependence((CObject*)unit);

  return;
}


CWaitCommandsAI::TimeWait::TimeWait(int _duration, CUnit* _unit)
: Wait(CMD_WAITCODE_TIMEWAIT)
{
  key = keySource;
  keySource += 1.0f;
  
  unit = _unit;
  enabled = false;
  endFrame = 0;
  duration = _duration;
  
  AddDeathDependence((CObject*)unit);
}


CWaitCommandsAI::TimeWait::~TimeWait()
{
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
    } else {
      if (endFrame <= gs->frameNum) {
        UnitSet smallSet;
        smallSet.insert(unit);
        SendWaitCommand(smallSet);
        delete this;
        return;
      }
    }
  }
  else if (state == Queued) {
    return;
  }
  else if (state == Missing) {
    delete this;
    return;
  }
}


void CWaitCommandsAI::TimeWait::Draw() const
{
  if (unit == NULL) {
    delete this;
    return;
  }
}


/******************************************************************************/
//
//  SquadWait
//

float CWaitCommandsAI::SquadWait::keySource = 1.0f;


CWaitCommandsAI::SquadWait*
  CWaitCommandsAI::SquadWait::New(const Command& cmd)
{
  SquadWait* sw = new SquadWait(cmd);
  if (sw->key == 0.0f) {
    delete sw;
    return NULL;
  }
  return sw;
}


CWaitCommandsAI::SquadWait::SquadWait(const Command& cmd)
: Wait(CMD_WAITCODE_SQUADWAIT)
{
  key = 0.0f; // the invalid key
  
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
  
  key = keySource;
  keySource += 1.0f;

  Command waitCmd;
  waitCmd.id = CMD_WAIT;
  waitCmd.options = cmd.options;
  waitCmd.params.push_back(code);
  waitCmd.params.push_back(key);
  
  SendCommand(waitCmd, buildUnits);

  for (it = buildUnits.begin(); it != buildUnits.end(); ++it) {
    AddDeathDependence((CObject*)(*it));
  }

  return;
}


CWaitCommandsAI::SquadWait::~SquadWait()
{
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
  // FIXME -- clean builders
}


void CWaitCommandsAI::SquadWait::Draw() const
{
  if (buildUnits.empty()) {
    delete this;
    return;
  }
}


/******************************************************************************/
//
//  DeathWatch
//

float CWaitCommandsAI::DeathWatch::keySource = 1.0f;


CWaitCommandsAI::DeathWatch*
  CWaitCommandsAI::DeathWatch::New(const Command& cmd)
{
  DeathWatch* dw = new DeathWatch(cmd);
  if (dw->key == 0.0f) {
    delete dw;
    return NULL;
  }
  return dw;
}


CWaitCommandsAI::DeathWatch::DeathWatch(const Command& cmd)
: Wait(CMD_WAITCODE_DEATHWATCH)
{
  key = 0.0f; // the invalid key
  
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
    
  waitUnits = selUnits;
  
  key = keySource;
  keySource += 1.0f;

  Command waitCmd;
  waitCmd.id = CMD_WAIT;
  waitCmd.options = cmd.options;
  waitCmd.params.push_back(code);
  waitCmd.params.push_back(key);
  selectedUnits.GiveCommand(waitCmd); // FIXME -- remove selUnits first?
  
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
  
  if (deathUnits.empty()) {
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
      cursorIcons->AddIcon(CMD_DEATHWATCH, (*it)->midPos);
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

float CWaitCommandsAI::RallyPoint::keySource = 1.0f;


CWaitCommandsAI::RallyPoint*
  CWaitCommandsAI::RallyPoint::New(const Command& cmd)
{
  RallyPoint* rp = new RallyPoint(cmd);
  if (rp->key == 0.0f) {
    delete rp;
    return NULL;
  }
  return rp;
}


CWaitCommandsAI::RallyPoint::RallyPoint(const Command& cmd)
: Wait(CMD_WAITCODE_RALLYPOINT)
{
  key = 0.0f; // the invalid key
  
  if ((cmd.params.size() != 3) && (cmd.params.size() != 6)) {
    return;
  }
  
  Command moveCmd = cmd;
  moveCmd.id = CMD_MOVE;
  selectedUnits.GiveCommand(moveCmd);
  
  if (selectedUnits.selectedUnits.size() < 2) {
    return; // one man does not a rally make
  }

  key = keySource;
  keySource += 1.0f;

  Command waitCmd;
  waitCmd.id = CMD_WAIT;
  waitCmd.options = SHIFT_KEY;
  waitCmd.params.push_back(code);
  waitCmd.params.push_back(key);
  selectedUnits.GiveCommand(waitCmd, false);
  
  waitUnits = selectedUnits.selectedUnits;

  UnitSet::iterator it;
  for (it = waitUnits.begin(); it != waitUnits.end(); ++it) {
    AddDeathDependence((CObject*)(*it));
  }

  return;
}


CWaitCommandsAI::RallyPoint::~RallyPoint()
{
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
    if (state == Queued) {
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
  // all units are actively waiting on this command, unblock them
  SendWaitCommand(waitUnits);
  delete this;
  return;
}


void CWaitCommandsAI::RallyPoint::Draw() const
{
}


/******************************************************************************/
