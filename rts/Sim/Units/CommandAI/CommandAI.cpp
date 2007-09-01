#include "StdAfx.h"
#include "CommandAI.h"
#include "FactoryCAI.h"
#include "LineDrawer.h"
#include "ExternalAI/GlobalAIHandler.h"
#include "ExternalAI/Group.h"
#include "Game/GameHelper.h"
#include "Game/SelectedUnits.h"
#include "Game/WaitCommandsAI.h"
#include "Game/UI/CommandColors.h"
#include "Game/UI/CursorIcons.h"
#include "Rendering/GL/myGL.h"
#include "Lua/LuaCallInHandler.h"
#include "Lua/LuaRules.h"
#include "Sim/Misc/Feature.h"
#include "Sim/Misc/FeatureHandler.h"
#include "Sim/MoveTypes/MoveType.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitTypes/Factory.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Weapons/Weapon.h"
#include "Map/Ground.h"
#include "LoadSaveInterface.h"
#include "LogOutput.h"
#include "myMath.h"
#include "mmgr.h"
#include "creg/STL_Set.h"
#include "creg/STL_Deque.h"

#define TARGET_LOST_TIMER 120	// in calls to SlowUpdate() (approx. once every second)

CR_BIND(CCommandQueue, );

CR_REG_METADATA(CCommandQueue, (
				CR_MEMBER(queue),
				CR_MEMBER(tagCounter),
				CR_ENUM_MEMBER(queueType)
				));
CR_BIND_DERIVED(CCommandAI, CObject, );

CR_REG_METADATA(CCommandAI, (
				CR_MEMBER(stockpileWeapon),

				CR_MEMBER(possibleCommands),
				CR_MEMBER(commandQue),
				CR_MEMBER(nonQueingCommands),
				CR_MEMBER(lastUserCommand),
				CR_MEMBER(selfDCountdown),
				CR_MEMBER(lastFinishCommand),

				CR_MEMBER(owner),

				CR_MEMBER(orderTarget),
				CR_MEMBER(targetDied),
				CR_MEMBER(inCommand),
				CR_MEMBER(selected),
				CR_MEMBER(repeatOrders),
				CR_MEMBER(lastSelectedCommandPage),
				CR_MEMBER(unimportantMove),
				CR_MEMBER(targetLostTimer),
				CR_RESERVED(64),
				CR_POSTLOAD(PostLoad)
				));
CCommandAI::CCommandAI()
:	lastUserCommand(-1000),
	orderTarget(0),
	targetDied(false),
	inCommand(false),
	selected(false),
	owner(owner),
	stockpileWeapon(0),
	lastSelectedCommandPage(0),
	repeatOrders(false),
	lastFinishCommand(0),
	unimportantMove(false),
	targetLostTimer(TARGET_LOST_TIMER)
{}

CCommandAI::CCommandAI(CUnit* owner)
:	lastUserCommand(-1000),
	orderTarget(0),
	targetDied(false),
	inCommand(false),
	selected(false),
	owner(owner),
	stockpileWeapon(0),
	lastSelectedCommandPage(0),
	repeatOrders(false),
	lastFinishCommand(0),
	unimportantMove(false),
	targetLostTimer(TARGET_LOST_TIMER)
{
	owner->commandAI=this;
	CommandDescription c;
	c.id=CMD_STOP;
	c.action="stop";
	c.type=CMDTYPE_ICON;
	c.name="Stop";
	c.mouseicon=c.name;
	c.hotkey="s";
	c.tooltip="Stop: Cancel the units current actions";
	possibleCommands.push_back(c);

	if(owner->unitDef->canAttack && isAttackCapable()) {
		c.id=CMD_ATTACK;
		c.action="attack";
		c.type=CMDTYPE_ICON_UNIT_OR_MAP;
		c.name="Attack";
		c.mouseicon=c.name;
		c.hotkey="a";
		c.tooltip="Attack: Attacks an unit or a position on the ground";
		possibleCommands.push_back(c);
	}

	if(owner->unitDef->canDGun){
		c.id=CMD_DGUN;
		c.action="dgun";
		c.type=CMDTYPE_ICON_UNIT_OR_MAP;
		c.name="DGun";
		c.mouseicon=c.name;
		c.hotkey="d";
		c.tooltip="DGun: Attacks using the units special weapon";
		possibleCommands.push_back(c);
	}

 	c.id=CMD_WAIT;
	c.action="wait";
 	c.type=CMDTYPE_ICON;
 	c.name="Wait";
 	c.mouseicon=c.name;
 	c.hotkey="w";
 	c.onlyKey=true;
 	c.tooltip="Wait: Tells the unit to wait until another units handles him";
 	possibleCommands.push_back(c);
//	nonQueingCommands.insert(CMD_WAIT);

	c.id = CMD_TIMEWAIT;
	c.action = "timewait";
	c.type = CMDTYPE_NUMBER;
	c.name = "TimeWait";
	c.mouseicon=c.name;
	c.hotkey = "";
	c.onlyKey = true;
	c.tooltip = "TimeWait: Wait for a period of time before continuing";
	c.params.push_back("1");  // min
	c.params.push_back("60"); // max
	possibleCommands.push_back(c);
	c.params.clear();

	// only for games with 2 ally teams  --  checked later
	c.id = CMD_DEATHWAIT;
	c.action = "deathwait";
	c.type = CMDTYPE_ICON_UNIT_OR_RECTANGLE;
	c.name = "DeathWait";
	c.mouseicon=c.name;
	c.hotkey = "";
	c.onlyKey = true;
	c.tooltip = "DeathWait: Wait until units die before continuing";
	possibleCommands.push_back(c);

	c.id = CMD_SQUADWAIT;
	c.action = "squadwait";
	c.type = CMDTYPE_NUMBER;
	c.name = "SquadWait";
	c.mouseicon=c.name;
	c.hotkey = "";
	c.onlyKey = true;
	c.tooltip = "SquadWait: Wait for a number of units to arrive before continuing";
	c.params.push_back("2");   // min
	c.params.push_back("100"); // max
	possibleCommands.push_back(c);
	c.params.clear();

	c.id = CMD_GATHERWAIT;
	c.action = "gatherwait";
	c.type = CMDTYPE_ICON;
	c.name = "GatherWait";
	c.mouseicon=c.name;
	c.hotkey = "";
	c.onlyKey = true;
	c.tooltip = "GatherWait: Wait until all units arrive before continuing";
	possibleCommands.push_back(c);

	c.id=CMD_SELFD;
	c.action="selfd";
	c.type=CMDTYPE_ICON;
	c.name="SelfD";
	c.mouseicon=c.name;
	c.hotkey="Ctrl+d";
	c.onlyKey=true;
	c.tooltip="SelfD: Tells the unit to self destruct";
	possibleCommands.push_back(c);
//	nonQueingCommands.insert(CMD_SELFD);

	c.onlyKey=false;
	c.hotkey="";

	if(!owner->unitDef->noAutoFire){
		if(!owner->unitDef->weapons.empty() || owner->unitDef->type=="Factory"/* || owner->unitDef->canKamikaze*/){
			c.id=CMD_FIRE_STATE;
			c.action="firestate";
			c.type=CMDTYPE_ICON_MODE;
			c.name="Fire state";
			c.mouseicon=c.name;
			c.params.push_back("2");
			c.params.push_back("Hold fire");
			c.params.push_back("Return fire");
			c.params.push_back("Fire at will");
			c.tooltip="Fire State: Sets under what conditions an\n unit will start to fire at enemy units\n without an explicit attack order";
			possibleCommands.push_back(c);
			nonQueingCommands.insert(CMD_FIRE_STATE);
		}
	} else{
		owner->fireState=0;
	}

	if(owner->unitDef->canmove || owner->unitDef->builder)
	{
		c.params.clear();
		c.id=CMD_MOVE_STATE;
		c.action="movestate";
		c.type=CMDTYPE_ICON_MODE;
		c.name="Move state";
		c.mouseicon=c.name;
		c.params.push_back("1");
		c.params.push_back("Hold pos");
		c.params.push_back("Maneuver");
		c.params.push_back("Roam");
		c.tooltip="Move State: Sets how far out of its way\n an unit will move to attack enemies";
		possibleCommands.push_back(c);
		owner->moveState=1;
		nonQueingCommands.insert(CMD_MOVE_STATE);
	}
	else
	{
		owner->moveState=0;
	}

	if (owner->unitDef->canRepeat) {
		c.params.clear();
		c.id=CMD_REPEAT;
		c.action="repeat";
		c.type=CMDTYPE_ICON_MODE;
		c.name="Repeat";
		c.mouseicon=c.name;
		c.params.push_back("0");
		c.params.push_back("Repeat off");
		c.params.push_back("Repeat on");
		c.tooltip="Repeat: If on the unit will continously\n push finished orders to the end of its\n order que";
		possibleCommands.push_back(c);
		nonQueingCommands.insert(CMD_REPEAT);
	}

	if(owner->unitDef->highTrajectoryType>1){
		c.params.clear();
		c.id=CMD_TRAJECTORY;
		c.action="trajectory";
		c.type=CMDTYPE_ICON_MODE;
		c.name="Trajectory";
		c.mouseicon=c.name;
		c.params.push_back("0");
		c.params.push_back("Low traj");
		c.params.push_back("High traj");
		c.tooltip="Trajectory: If set to high, weapons that\n support it will try to fire in a higher\n trajectory than usual (experimental)";
		possibleCommands.push_back(c);
		nonQueingCommands.insert(CMD_TRAJECTORY);
	}

	if(owner->unitDef->onoffable)
	{
		c.params.clear();
		c.id=CMD_ONOFF;
		c.action="onoff";
		c.type=CMDTYPE_ICON_MODE;
		c.name="Active state";
		c.mouseicon=c.name;
		c.hotkey="x";
		if(owner->unitDef->activateWhenBuilt)
			c.params.push_back("1");
		else
			c.params.push_back("0");

		c.params.push_back(" Off ");
		c.params.push_back(" On ");

		c.tooltip="Active State: Sets the active state of the unit to on or off";
		possibleCommands.push_back(c);
		nonQueingCommands.insert(CMD_ONOFF);
		c.hotkey="";
	}

	if(owner->unitDef->canCloak)
	{
		c.params.clear();
		c.id=CMD_CLOAK;
		c.action="cloak";
		c.type=CMDTYPE_ICON_MODE;
		c.name="Cloak state";
		c.mouseicon=c.name;
		c.hotkey="k";
		if(owner->unitDef->startCloaked)
			c.params.push_back("1");
		else
			c.params.push_back("0");

		c.params.push_back("UnCloaked");
		c.params.push_back("Cloaked");

		c.tooltip="Cloak State: Sets wheter the unit is cloaked or not";
		possibleCommands.push_back(c);
		nonQueingCommands.insert(CMD_CLOAK);
		c.hotkey="";
	}
}

CCommandAI::~CCommandAI()
{
	if(orderTarget){
		DeleteDeathDependence(orderTarget);
		orderTarget=0;
	}
}

void CCommandAI::PostLoad()
{
	selected=false;//HACK: selected list does not serialized
}

vector<CommandDescription>& CCommandAI::GetPossibleCommands()
{
	return possibleCommands;
}


bool CCommandAI::isAttackCapable() const
{
	const UnitDef* ud = owner->unitDef;
	return (!ud->weapons.empty() || ud->canKamikaze || (ud->type == "Factory"));
}


static inline bool isCommandInMap(const Command& c)
{
	if (c.params.size() >= 3 &&
			(c.params[0] < 0.f || c.params[2] < 0.f
			 || c.params[0] > gs->mapx*SQUARE_SIZE
			 || c.params[2] > gs->mapy*SQUARE_SIZE))
		return false;
	return true;
}

bool CCommandAI::AllowedCommand(const Command& c)
{
	// check if the command is in the map first
	switch (c.id) {
		case CMD_MOVE:
		case CMD_ATTACK:
		case CMD_RECLAIM:
		case CMD_REPAIR:
		case CMD_RESURRECT:
		case CMD_PATROL:
		case CMD_RESTORE:
		case CMD_FIGHT:
		case CMD_DGUN:
			if (!isCommandInMap(c)) return false;
			break;
		default:
			// build commands
			if (c.id < 0 && !isCommandInMap(c)) return false;
			break;
	}

	const UnitDef* ud = owner->unitDef;
	int maxHeightDiff = 200;

	switch (c.id) {
		case CMD_ATTACK:
			maxHeightDiff = 10;
		case CMD_DGUN: {
			if (!ud->canAttack || !isAttackCapable())
				return false;

			if (c.params.size() == 3) {
				// check if attack ground is really attack ground
				if (fabs(c.params[1] - ground->GetHeight2(c.params[0], c.params[2])) >
					maxHeightDiff) {
					return false;
				}
			}
			break;
		}

		case CMD_MOVE:      if (!ud->canmove)       return false; break;
		case CMD_FIGHT:     if (!ud->canFight)      return false; break;
		case CMD_GUARD:     if (!ud->canGuard)      return false; break;
		case CMD_PATROL:    if (!ud->canPatrol)     return false; break;
		case CMD_CAPTURE:   if (!ud->canCapture)    return false; break;
		case CMD_RECLAIM:   if (!ud->canReclaim)    return false; break;
		case CMD_RESTORE:   if (!ud->canRestore)    return false; break;
		case CMD_RESURRECT: if (!ud->canResurrect)  return false; break;
		case CMD_REPAIR: {
			if (!ud->canRepair && !ud->canAssist)   return false; break;
		}
	}

	if ((c.id == CMD_RECLAIM) && (c.params.size() == 1)) {
		const int unitID = (int) c.params[0];
		if (unitID < MAX_UNITS) { // not a feature
			CUnit* unit = uh->units[unitID];
			if (unit && !unit->unitDef->reclaimable)
				return false;
		} else {
			const CFeatureSet& fset = featureHandler->GetActiveFeatures();
			CFeatureSet::const_iterator f = fset.find(unitID - MAX_UNITS);
			if (f != fset.end() && !(*f)->def->reclaimable)
				return false;
		}
	}

	if ((c.id == CMD_REPAIR) && (c.params.size() == 1)) {
		CUnit* unit = uh->units[(int) c.params[0]];
		if (unit && ((unit->beingBuilt && !ud->canAssist) || (!unit->beingBuilt && !ud->canRepair)))
			return false;
	}

	if (c.id == CMD_FIRE_STATE && (c.params.empty() || ud->noAutoFire || (ud->weapons.empty() && ud->type!="Factory")))
		return false;
	if (c.id == CMD_MOVE_STATE && (c.params.empty() || (!ud->canmove && !ud->builder)))
		return false;
	if (c.id == CMD_REPEAT && (c.params.empty() || !ud->canRepeat))
		return false;
	if (c.id == CMD_TRAJECTORY && (c.params.empty() || ud->highTrajectoryType < 2))
		return false;
	if (c.id == CMD_ONOFF && (c.params.empty() || !ud->onoffable || owner->beingBuilt))
		return false;
	if (c.id == CMD_CLOAK && (c.params.empty() || !ud->canCloak))
		return false;
	if(c.id == CMD_STOCKPILE && !stockpileWeapon)
		return false;

	return true;
}


void CCommandAI::GiveCommand(const Command& c)
{
	if (luaRules && !luaRules->AllowCommand(owner, c)) {
		return;
	}
	this->GiveCommandReal(c); // send to the sub-classes
}


void CCommandAI::GiveCommandReal(const Command& c)
{
	if (!AllowedCommand(c)) {
		return;
	}
	GiveAllowedCommand(c);
}


inline void CCommandAI::SetCommandDescParam0(const Command& c)
{
	vector<CommandDescription>::iterator cdi;
	for (cdi = possibleCommands.begin(); cdi != possibleCommands.end(); ++cdi) {
		if (cdi->id == c.id) {
			char t[10];
			SNPRINTF(t, 10, "%d", (int)c.params[0]);
			cdi->params[0] = t;
			return;
		}
	}
}


bool CCommandAI::ExecuteStateCommand(const Command& c)
{
	switch (c.id) {
		case CMD_FIRE_STATE: {
			owner->fireState = (int)c.params[0];
			SetCommandDescParam0(c);
			selectedUnits.PossibleCommandChange(owner);
			return true;
		}
		case CMD_MOVE_STATE: {
			owner->moveState = (int)c.params[0];
			SetCommandDescParam0(c);
			selectedUnits.PossibleCommandChange(owner);
			return true;
		}
		case CMD_REPEAT: {
			repeatOrders = !!c.params[0];
			SetCommandDescParam0(c);
			selectedUnits.PossibleCommandChange(owner);
			return true;
		}
		case CMD_TRAJECTORY: {
			owner->useHighTrajectory = !!c.params[0];
			SetCommandDescParam0(c);
			selectedUnits.PossibleCommandChange(owner);
			return true;
		}
		case CMD_ONOFF: {
			if (c.params[0] == 1) {
				owner->Activate();
			} else if (c.params[0] == 0) {
				owner->Deactivate();
			}
			SetCommandDescParam0(c);
			selectedUnits.PossibleCommandChange(owner);
			return true;
		}
		case CMD_CLOAK: {
			if (c.params[0] == 1) {
				owner->wantCloak = true;
			} else if(c.params[0]==0) {
				owner->wantCloak = false;
				owner->curCloakTimeout = gs->frameNum + owner->cloakTimeout;
			}
			SetCommandDescParam0(c);
			selectedUnits.PossibleCommandChange(owner);
			return true;
		}
		case CMD_STOCKPILE: {
			int change = 1;
			if (c.options & RIGHT_MOUSE_KEY) { change *= -1; }
			if (c.options & SHIFT_KEY)       { change *=  5; }
			if (c.options & CONTROL_KEY)     { change *= 20; }
			stockpileWeapon->numStockpileQued += change;
			if (stockpileWeapon->numStockpileQued < 0) {
				stockpileWeapon->numStockpileQued = 0;
			}
			UpdateStockpileIcon();
			return true;
		}
	}
	return false;
}


void CCommandAI::GiveAllowedCommand(const Command& c)
{
	if (ExecuteStateCommand(c)) {
		return;
	}

	switch (c.id) {
		case CMD_SELFD: {
			if (!(c.options & SHIFT_KEY) || commandQue.empty()) {
				if (owner->selfDCountdown != 0) {
					owner->selfDCountdown = 0;
				} else {
					owner->selfDCountdown = owner->unitDef->selfDCountdown*2+1;
				}
			}
			else {
	  		if (commandQue.back().id == CMD_SELFD) {
  				commandQue.pop_back();
				} else {
	  			commandQue.push_back(c);
				}
			}
			return;
		}
	  case CMD_SET_WANTED_MAX_SPEED: {
			if (CanSetMaxSpeed() &&
			    (commandQue.empty() ||
			     (commandQue.back().id != CMD_SET_WANTED_MAX_SPEED))) {
				// bail early, do not check for overlaps or queue cancelling
				commandQue.push_back(c);
				if (commandQue.size()==1 && !owner->beingBuilt) {
					SlowUpdate();
				}
			}
			return;
		}
	  case CMD_WAIT: {
	  	GiveWaitCommand(c);
	  	return;
		}
		case CMD_INSERT: {
			ExecuteInsert(c);
			return;
		}
		case CMD_REMOVE: {
			ExecuteRemove(c);
			return;
		}
	}

	// flush the queue for immediate commands
	if(!(c.options & SHIFT_KEY)) {
		if (!commandQue.empty()) {
			if ((commandQue.front().id == CMD_DGUN)      ||
			    (commandQue.front().id == CMD_ATTACK)    ||
			    (commandQue.front().id == CMD_AREA_ATTACK)) {
				owner->AttackUnit(0,true);
			}
 			waitCommandsAI.ClearUnitQueue(owner, commandQue);
			commandQue.clear();
		}
		inCommand=false;
		if(orderTarget){
			DeleteDeathDependence(orderTarget);
			orderTarget=0;
		}
	}

	if(c.id == CMD_PATROL){
		CCommandQueue::iterator ci = commandQue.begin();
		for(; ci != commandQue.end() && ci->id!=CMD_PATROL; ci++);
		if(ci==commandQue.end()){
			if(commandQue.empty()){
				Command c2;
				c2.id = CMD_PATROL;
				c2.params.push_back(owner->pos.x);
				c2.params.push_back(owner->pos.y);
				c2.params.push_back(owner->pos.z);
				c2.options = c.options;
				commandQue.push_back(c2);
			} else {
				do {
					ci--;
					if(ci->params.size() >=3){
						Command c2;
						c2.id = CMD_PATROL;
						c2.params = ci->params;
						c2.options = c.options;
						commandQue.push_back(c2);
						ci=commandQue.begin();
					} else if(ci==commandQue.begin()){
						Command c2;
						c2.id = CMD_PATROL;
						c2.params.push_back(owner->pos.x);
						c2.params.push_back(owner->pos.y);
						c2.params.push_back(owner->pos.z);
						c2.options = c.options;
						commandQue.push_back(c2);
					}
				} while(ci!=commandQue.begin());
			}
		}
	}

	// cancel duplicated commands
	bool first;
	if (CancelCommands(c, commandQue, first) > 0) {
		if (first) {
			Command stopCommand;
			stopCommand.id = CMD_STOP;
			commandQue.push_front(stopCommand);
			SlowUpdate();
		}
		return;
	}

	// do not allow overlapping commands
	if (!GetOverlapQueued(c).empty()) {
		return;
	}

	if (c.id == CMD_ATTACK) {
		// avoid weaponless units moving to 0 distance when given attack order
		if (owner->weapons.empty() && (owner->unitDef->canKamikaze == false)) {
			Command c2;
			c2.id = CMD_STOP;
			commandQue.push_back(c2);
			return;
		}
	}

	commandQue.push_back(c);

	if (commandQue.size()==1 && !owner->beingBuilt && !owner->stunned) {
		SlowUpdate();
	}
}


void CCommandAI::GiveWaitCommand(const Command& c)
{
	if (commandQue.empty()) {
		commandQue.push_back(c);
		return;
	}
	else if (c.options & SHIFT_KEY) {
		if (commandQue.back().id == CMD_WAIT) {
			waitCommandsAI.RemoveWaitCommand(owner, commandQue.back());
			commandQue.pop_back();
		} else {
			commandQue.push_back(c);
			return;
		}
	}
	else if (commandQue.front().id == CMD_WAIT) {
		waitCommandsAI.RemoveWaitCommand(owner, commandQue.front());
		commandQue.pop_front();
	}
	else {
		// shutdown the current order
		owner->AttackUnit(0, true);
		StopMove();
		inCommand = false;
		targetDied = false;
		unimportantMove = false;
		commandQue.push_front(c);
		return;
	}

	if (commandQue.empty()) {
		if (!owner->group) {
			globalAI->UnitIdle(owner);
		}
		luaCallIns.UnitIdle(owner);
	}

	return;
}


void CCommandAI::ExecuteInsert(const Command& c)
{
	if (c.params.size() < 3) {
		return;
	}

	// make the command
	Command newCmd;
	newCmd.id                  = (int)c.params[1];
	newCmd.options   = (unsigned char)c.params[2];
	for (int p = 3; p < (int)c.params.size(); p++) {
		newCmd.params.push_back(c.params[p]);
	}

	// validate the command
	if (!AllowedCommand(newCmd)) {
		return;
	}

	CCommandQueue* queue = &commandQue;

	bool facBuildQueue = false;
	CFactoryCAI* facCAI = dynamic_cast<CFactoryCAI*>(this);
	if (facCAI) {
		if (c.options & CONTROL_KEY) {
			// check the build order
			const map<int, CFactoryCAI::BuildOption>& bOpts = facCAI->buildOptions;
			if ((newCmd.id != CMD_STOP) && (newCmd.id != CMD_WAIT) &&
			    ((newCmd.id >= 0) || (bOpts.find(newCmd.id) == bOpts.end()))) {
				return;
			}
			facBuildQueue = true;
		} else {
			// use the new commands
			queue = &facCAI->newUnitCommands;
		}
	}

	// FIXME: handle CMD_LOOPBACKATTACK, etc...

	CCommandQueue::iterator insertIt = queue->begin();

	if (c.options & ALT_KEY) {
		// treat param0 as a position
		int pos = (int)c.params[0];
		const int qsize = (int)queue->size();
		if (pos < 0) {
			pos = qsize + pos + 1; // convert the negative index
			if (pos < 0) {
				pos = 0;
			}
		}
		if (pos > qsize) {
			pos = qsize;
		}
		std::advance(insertIt, pos);
	}
	else {
		// treat param0 as a command tag
		const int tag = (int)c.params[0];
		CCommandQueue::iterator ci;
		bool found = false;
		for (ci = queue->begin(); ci != queue->end(); ++ci) {
			const Command& qc = *ci;
			if (qc.tag == tag) {
				insertIt = ci;
				found = true;
				break;
			}
		}
		if (!found) {
			return;
		}
		if ((c.options & RIGHT_MOUSE_KEY) && (insertIt != queue->end())) {
			insertIt++; // insert after the tagged command
		}
	}

	if (facBuildQueue) {
		facCAI->InsertBuildCommand(insertIt, newCmd);
		if (!owner->stunned) {
			SlowUpdate();
		}
		return;
	}

	// shutdown the current order if the insertion is at the beginning
	if (!queue->empty() && (insertIt == queue->begin())) {
		inCommand = false;
		targetDied = false;
		unimportantMove = false;
		orderTarget = NULL;
		if (owner->group) {
			owner->group->CommandFinished(owner->id, commandQue.front().id);
		}
	}

	queue->insert(insertIt, newCmd);

	if (!owner->stunned) {
		SlowUpdate();
	}

	return;
}


void CCommandAI::ExecuteRemove(const Command& c)
{
	// disable repeating during the removals
	const bool prevRepeat = repeatOrders;
	repeatOrders = false;

	CCommandQueue* queue = &commandQue;

	bool facBuildQueue = false;
	CFactoryCAI* facCAI = dynamic_cast<CFactoryCAI*>(this);

	if (facCAI) {
		if (c.options & CONTROL_KEY) {
			// check the build order
			facBuildQueue = true;
		} else {
			// use the new commands
			queue = &facCAI->newUnitCommands;
		}
	}

	if ((c.params.size() <= 0) || (queue->size() <= 0)) {
		return;
	}

	// erase commands by a list of command types
	bool active = false;
	for (int p = 0; p < (int)c.params.size(); p++) {
		const int removeValue = (int)c.params[p]; // tag or id

		if (facBuildQueue && (removeValue == 0) && !(c.options & ALT_KEY)) {
			continue; // don't remove tag=0 commands from build queues, they
			          // are used the same way that CMD_STOP is, to void orders
		}
		CCommandQueue::iterator ci;
		do {
			for (ci = queue->begin(); ci != queue->end(); ++ci) {
				const Command& qc = *ci;
				if (c.options & ALT_KEY) {
					if (qc.id != removeValue)  { continue; }
				} else {
					if (qc.tag != removeValue) { continue; }
				}

				if (qc.id == CMD_WAIT) {
					waitCommandsAI.RemoveWaitCommand(owner, qc);
				}

				if (facBuildQueue) {
					facCAI->RemoveBuildCommand(ci);
					ci = queue->begin();
					break; // the removal may have corrupted the iterator
				}

				if (!facCAI && (ci == queue->begin())) {
					if (!active) {
						active = true;
						FinishCommand();
						ci = queue->begin();
						break;
					}
					active = true;
				}
				queue->erase(ci);
				ci = queue->begin();
				break; // the removal may have corrupted the iterator
			}
		}
		while (ci != queue->end());
	}

	repeatOrders = prevRepeat;
	return;
}


/**
* @brief Determins if c will cancel a queued command
* @return true if c will cancel a queued command
**/
bool CCommandAI::WillCancelQueued(Command &c)
{
	return (this->GetCancelQueued(c, commandQue) != this->commandQue.end());
}


/**
* @brief Finds the queued command that would be canceled by the Command c
* @return An iterator located at the command, or commandQue.end() if no such queued command exsists
**/
CCommandQueue::iterator CCommandAI::GetCancelQueued(const Command &c,
                                                          CCommandQueue& q)
{
	CCommandQueue::iterator ci = q.end();

	while (ci != q.begin()) {

		ci--; //iterate from the end and dont check the current order
		const Command& t = *ci;

		if (((c.id == t.id) || ((c.id < 0) && (t.id < 0))
				|| (t.id == CMD_FIGHT && c.id == CMD_ATTACK && t.params.size() == 1))
				&& (t.params.size() == c.params.size())) {
			if (c.params.size() == 1) {
			  // assume the param is a unit of feature id
				if ((t.params[0] == c.params[0]) &&
				    (t.id != CMD_SET_WANTED_MAX_SPEED)) {
					return ci;
				}
			}
			else if (c.params.size() >= 3) {
				if (c.id < 0) {
					BuildInfo bc(c);
					BuildInfo bt(t);
					if (bc.def && bt.def
					    && fabs(bc.pos.x - bt.pos.x) * 2 <= max(bc.GetXSize(), bt.GetXSize()) * SQUARE_SIZE
					    && fabs(bc.pos.z - bt.pos.z) * 2 <= max(bc.GetYSize(), bt.GetYSize()) * SQUARE_SIZE) {
						return ci;
					}
				} else {
					// assume this means that the first 3 makes a position
					float3 cp(c.params[0], c.params[1], c.params[2]);
					float3 tp(t.params[0], t.params[1], t.params[2]);
					if ((cp - tp).SqLength2D() < (17.0f * 17.0f)) {
						return ci;
					}
				}
			}
		}
	}
	return q.end();
}


int CCommandAI::CancelCommands(const Command &c, CCommandQueue& q,
                               bool& first)
{
	first = false;
	int cancelCount = 0;

	while (true) {
		CCommandQueue::iterator ci = GetCancelQueued(c, q);
		if (ci == q.end()) {
			return cancelCount;
		}
		first = first || (ci == q.begin());
		cancelCount++;

		CCommandQueue::iterator firstErase = ci;
		CCommandQueue::iterator lastErase = ci;

		ci++;
		if ((ci != q.end()) && (ci->id == CMD_SET_WANTED_MAX_SPEED)) {
			lastErase = ci;
			cancelCount++;
			ci++;
		}

		if ((ci != q.end()) && (ci->id == CMD_WAIT)) {
  		waitCommandsAI.RemoveWaitCommand(owner, *ci);
			lastErase = ci;
			cancelCount++;
			ci++;
		}

		lastErase++; // STL: erase the range [first, last)
		q.erase(firstErase, lastErase);

		if (c.id >= 0) {
			return cancelCount; // only delete one non-build order
		}
	}

	return cancelCount;
}


/**
* @brief Returns commands that overlap c, but will not be canceled by c
* @return a vector containing commands that overlap c
*/
std::vector<Command> CCommandAI::GetOverlapQueued(const Command &c)
{
	return GetOverlapQueued(c, commandQue);
}


std::vector<Command> CCommandAI::GetOverlapQueued(const Command &c,
                                                  CCommandQueue& q)
{
	CCommandQueue::iterator ci = q.end();
	std::vector<Command> v;
	BuildInfo cbi(c);

	if (ci != q.begin()){
		do {
			ci--; //iterate from the end and dont check the current order
			const Command& t = *ci;

			if (((t.id == c.id) || ((c.id < 0) && (t.id < 0))) &&
			    (t.params.size() == c.params.size())){
				if (c.params.size()==1) {
					// assume the param is a unit or feature id
					if (t.params[0] == c.params[0]) {
						v.push_back(t);
					}
				}
				else if (c.params.size() >= 3) {
					// assume this means that the first 3 makes a position
					BuildInfo tbi;
					if (tbi.Parse(t)) {
						const float dist2X = 2.0f * fabs(cbi.pos.x - tbi.pos.x);
						const float dist2Z = 2.0f * fabs(cbi.pos.z - tbi.pos.z);
						const float addSizeX = SQUARE_SIZE * (cbi.GetXSize() + tbi.GetXSize());
						const float addSizeZ = SQUARE_SIZE * (cbi.GetYSize() + tbi.GetYSize());
						const float maxSizeX = SQUARE_SIZE * max(cbi.GetXSize(), tbi.GetXSize());
						const float maxSizeZ = SQUARE_SIZE * max(cbi.GetYSize(), tbi.GetYSize());
						if (cbi.def && tbi.def &&
						    ((dist2X > maxSizeX) || (dist2Z > maxSizeZ)) &&
						    ((dist2X < addSizeX) && (dist2Z < addSizeZ))) {
							v.push_back(t);
						}
					} else {
						if ((cbi.pos - tbi.pos).SqLength2D() < (17.0f * 17.0f)) {
							v.push_back(t);
						}
					}
				}
			}
		} while (ci != q.begin());
	}
	return v;
}


int CCommandAI::UpdateTargetLostTimer(int unitid)
{
	if (targetLostTimer)
		--targetLostTimer;

	if (uh->units[unitid] && (uh->units[unitid]->losStatus[owner->allyteam] & LOS_INRADAR))
		targetLostTimer = TARGET_LOST_TIMER;

	return targetLostTimer;
}

/**
* @brief Causes this CommandAI to execute the attack order c
*/
void CCommandAI::ExecuteAttack(Command &c)
{
	assert(owner->unitDef->canAttack);
	if(inCommand){
		if(targetDied || (c.params.size()==1 && UpdateTargetLostTimer(int(c.params[0])) == 0)){
			FinishCommand();
			return;
		}
		if ((c.params.size() == 3) && (owner->commandShotCount > 0) && (commandQue.size() > 1)) {
			FinishCommand();
			return;
		}
		if (!(c.options & ALT_KEY) && SkipParalyzeTarget(orderTarget)) {
			FinishCommand();
			return;
		}
	}
	else {
		owner->commandShotCount = -1;
		if(c.params.size()==1){
			if(uh->units[int(c.params[0])]!=0 && uh->units[int(c.params[0])]!=owner){
				owner->AttackUnit(uh->units[int(c.params[0])], c.id==CMD_DGUN);
				if(orderTarget)
					DeleteDeathDependence(orderTarget);
				orderTarget=uh->units[int(c.params[0])];
				AddDeathDependence(orderTarget);
				inCommand=true;
			} else {
				FinishCommand();
				return;
			}
		} else {
			float3 pos(c.params[0],c.params[1],c.params[2]);
			owner->AttackGround(pos, c.id==CMD_DGUN);
			inCommand=true;
		}
	}
}

/**
* @brief executes the stop command c
*/
void CCommandAI::ExecuteStop(Command &c)
{
	owner->AttackUnit(0,true);
	std::vector<CWeapon*>::iterator wi;
	for(wi=owner->weapons.begin();wi!=owner->weapons.end();++wi){
		(*wi)->HoldFire();
	}
	FinishCommand();
}

/**
* @brief executes the DGun command c
*/
void CCommandAI::ExecuteDGun(Command &c)
{
	ExecuteAttack(c);
	return;
}

void CCommandAI::SlowUpdate()
{
	if(commandQue.empty()){
		return;
	}

	Command& c = commandQue.front();

	switch(c.id){
		case CMD_WAIT:{
			return;
    }
  	case CMD_SELFD:{
			if (owner->selfDCountdown != 0) {
				owner->selfDCountdown = 0;
			} else {
				owner->selfDCountdown = (owner->unitDef->selfDCountdown * 2) + 1;
			}
			FinishCommand();
			return;
		}
		case CMD_STOP:{
			ExecuteStop(c);
			return;
    }
		case CMD_ATTACK:{
			ExecuteAttack(c);
			return;
    }
		case CMD_DGUN:{
			ExecuteDGun(c);
			return;
    }
	}
	if (ExecuteStateCommand(c)) {
		FinishCommand();
		return;
	}
	if (luaRules && !luaRules->CommandFallback(owner, c)) {
		return; // luaRules wants the command to stay at the front
	}
	FinishCommand();
}


int CCommandAI::GetDefaultCmd(CUnit* pointed, CFeature* feature)
{
	if (pointed) {
		if (!gs->Ally(gu->myAllyTeam, pointed->allyteam)) {
			if (owner->unitDef->canAttack && isAttackCapable()) {
				return CMD_ATTACK;
			}
		}
	}
	return CMD_STOP;
}

void CCommandAI::DependentDied(CObject *o)
{
	if(o==orderTarget){
		targetDied=true;
		orderTarget=0;
	}
}


bool CCommandAI::isTrackable(const CUnit* unit) const
{
	return ((unit->losStatus[owner->allyteam] & (LOS_INLOS | LOS_INRADAR)) != 0) ||
	       (unit->unitDef->speed <= 0.0f);
}


void CCommandAI::DrawWaitIcon(const Command& cmd) const
{
	waitCommandsAI.AddIcon(cmd, lineDrawer.GetLastPos());
}


void CCommandAI::DrawCommands(void)
{
	lineDrawer.StartPath(owner->midPos, cmdColors.start);

	if (owner->selfDCountdown != 0) {
		lineDrawer.DrawIconAtLastPos(CMD_SELFD);
	}

	CCommandQueue::iterator ci;
	for(ci=commandQue.begin();ci!=commandQue.end();++ci){
		switch(ci->id){
			case CMD_ATTACK:
			case CMD_DGUN:{
				if(ci->params.size()==1){
					const CUnit* unit = uh->units[int(ci->params[0])];
					if((unit != NULL) && isTrackable(unit)) {
						const float3 endPos =
							helper->GetUnitErrorPos(unit, owner->allyteam);
						lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.attack);
					}
				} else {
					const float3 endPos(ci->params[0],ci->params[1]+3,ci->params[2]);
					lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.attack);
				}
				break;
			}
			case CMD_WAIT:{
				DrawWaitIcon(*ci);
				break;
			}
			case CMD_SELFD:{
				lineDrawer.DrawIconAtLastPos(ci->id);
				break;
			}
			default:{
				break;
			}
		}
	}

	lineDrawer.FinishPath();
}

void CCommandAI::FinishCommand(void)
{
	const int type = commandQue.front().id;
	const bool dontrepeat = commandQue.front().options & DONT_REPEAT
			|| commandQue.front().options & INTERNAL_ORDER;
	if (repeatOrders
		&& !dontrepeat
	    && (type != CMD_STOP)
	    && (type != CMD_PATROL)
	    && (type != CMD_SET_WANTED_MAX_SPEED)){
		commandQue.push_back(commandQue.front());
	}
	commandQue.pop_front();
	inCommand=false;
	targetDied=false;
	unimportantMove=false;
	orderTarget=0;
	if (owner->group) {
		owner->group->CommandFinished(owner->id,type);
	}

	if (commandQue.empty()) {
		if (!owner->group) {
			globalAI->UnitIdle(owner);
		}
		luaCallIns.UnitIdle(owner);
	}

	if (lastFinishCommand != gs->frameNum) {	//avoid infinite loops
		lastFinishCommand = gs->frameNum;
		if (!owner->stunned) {
			SlowUpdate();
		}
	}
}

void CCommandAI::AddStockpileWeapon(CWeapon* weapon)
{
	stockpileWeapon=weapon;
	CommandDescription c;
	c.id=CMD_STOCKPILE;
	c.action="stockpile";
	c.hotkey="";
	c.type=CMDTYPE_ICON;
	c.name="0/0";
	c.tooltip="Stockpile: Queue up ammunition for later use";
	c.iconname="bitmaps/armsilo1.bmp";
	possibleCommands.push_back(c);
}

void CCommandAI::StockpileChanged(CWeapon* weapon)
{
	UpdateStockpileIcon();
}

void CCommandAI::UpdateStockpileIcon(void)
{
	vector<CommandDescription>::iterator pci;
	for(pci=possibleCommands.begin();pci!=possibleCommands.end();++pci){
		if(pci->id==CMD_STOCKPILE){
			char name[50];
			sprintf(name, "%i/%i",
			        stockpileWeapon->numStockpiled,
			        stockpileWeapon->numStockpiled + stockpileWeapon->numStockpileQued);
			pci->name = name;
			selectedUnits.PossibleCommandChange(owner);
		}
	}
}

void CCommandAI::WeaponFired(CWeapon* weapon)
{
	if(weapon->weaponDef->manualfire && !weapon->weaponDef->dropped && !commandQue.empty()
		&& (commandQue.front().id==CMD_ATTACK || commandQue.front().id==CMD_DGUN) && inCommand)
	{
		owner->AttackUnit(0,true);
		globalAI->WeaponFired(owner,weapon->weaponDef);
		FinishCommand();
	}
}

void CCommandAI::BuggerOff(float3 pos, float radius)
{
}

void CCommandAI::LoadSave(CLoadSaveInterface* file, bool loading)
{
	int queSize=commandQue.size();
	file->lsInt(queSize);
	for(int a=0;a<queSize;++a){
		if(loading){
			commandQue.push_back(Command());
		}
		Command& c=commandQue[a];
		file->lsInt(c.id);
		file->lsUChar(c.options);
		file->lsInt(c.timeOut);

		int paramSize=c.params.size();
		file->lsInt(paramSize);
		for(int b=0;b<paramSize;++b){
			if(loading){
				c.params.push_back(0);
			}
			file->lsFloat(c.params[b]);
		}
	}
}


/**
 * @brief gets the command that keeps the unit close to the path
 * @return a Fight Command with 6 arguments, the first three being where to return to (the current position of the
 *	unit), and the second being the location of the origional command.
 * @param c the command to return to
 **/
void CCommandAI::PushOrUpdateReturnFight(const float3& cmdPos1, const float3& cmdPos2)
{
	float3 pos = ClosestPointOnLine(cmdPos1, cmdPos2, owner->pos);
	Command& c(commandQue.front());
	assert(c.id == CMD_FIGHT && c.params.size() >= 3);
	if (c.params.size() >= 6) {
		c.params[0] = pos.x;
		c.params[1] = pos.y;
		c.params[2] = pos.z;
	} else {
		Command c2;
		c2.id = CMD_FIGHT;
		c2.params.push_back(pos.x);
		c2.params.push_back(pos.y);
		c2.params.push_back(pos.z);
		c2.params.push_back(c.params[0]);
		c2.params.push_back(c.params[1]);
		c2.params.push_back(c.params[2]);
		c2.options = c.options|INTERNAL_ORDER;
		commandQue.push_front(c2);
	}
}

bool CCommandAI::HasMoreMoveCommands()
{
	if (!commandQue.empty()) {
		for(CCommandQueue::iterator i = (commandQue.begin()++); i != commandQue.end(); i++) {
			int id = i->id;
			if(id == CMD_FIGHT || id == CMD_AREA_ATTACK || id == CMD_ATTACK || id == CMD_CAPTURE
				|| id == CMD_DGUN || id == CMD_GUARD || id == CMD_LOAD_UNITS || id == CMD_MOVE
				|| id == CMD_PATROL || id == CMD_RECLAIM || id == CMD_REPAIR || id == CMD_RESTORE
				|| id == CMD_RESURRECT || id == CMD_UNLOAD_UNIT || id == CMD_UNLOAD_UNITS)
			{
				return true;
			} else if(id < 0 || id == CMD_DEATHWAIT || id == CMD_GATHERWAIT || id == CMD_SELFD
				|| id == CMD_SQUADWAIT || id == CMD_STOP || id == CMD_TIMEWAIT || id == CMD_WAIT)
			{
				return false;
			}
		}
	}
	return false;
}


bool CCommandAI::SkipParalyzeTarget(const CUnit* target)
{
	// check to see if we are about to paralyze a unit that is already paralyzed
	if ((target == NULL) || (owner->weapons.size() <= 0)) {
		return false;
	}
	const CWeapon* w = owner->weapons.front();
	if (!w->weaponDef->paralyzer) {
		return false;
	}
	if ((orderTarget->losStatus[owner->allyteam] & LOS_INLOS) && // visible
			(orderTarget->paralyzeDamage > orderTarget->health)) {  // stunned
		if ((commandQue.size() > 2) ||
				((commandQue.size() == 2) &&
				 (commandQue.back().id != CMD_SET_WANTED_MAX_SPEED))) {
			return true;
		}
	}
	return false;
}

