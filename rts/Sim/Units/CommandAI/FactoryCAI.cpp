/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/mmgr.h"

#include "FactoryCAI.h"
#include "ExternalAI/EngineOutHandler.h"
#include "Sim/Units/Groups/Group.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Game/GameHelper.h"
#include "Game/GlobalUnsynced.h"
#include "Game/SelectedUnits.h"
#include "Game/WaitCommandsAI.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/BuildInfo.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitLoader.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitTypes/Factory.h"
#include "System/Log/ILog.h"
#include "System/creg/STL_Map.h"
#include "System/Util.h"
#include "System/EventHandler.h"
#include "System/Exceptions.h"

CR_BIND_DERIVED(CFactoryCAI ,CCommandAI , );

CR_REG_METADATA(CFactoryCAI , (
	CR_MEMBER(newUnitCommands),
	CR_MEMBER(buildOptions),
	CR_RESERVED(16),
	CR_POSTLOAD(PostLoad)
));

CR_BIND(CFactoryCAI::BuildOption, );

CR_REG_METADATA_SUB(CFactoryCAI,BuildOption , (
	CR_MEMBER(name),
	CR_MEMBER(fullName),
	CR_MEMBER(numQued)
));


CFactoryCAI::CFactoryCAI(): CCommandAI()
{}


CFactoryCAI::CFactoryCAI(CUnit* owner): CCommandAI(owner)
{
	commandQue.SetQueueType(CCommandQueue::BuildQueueType);
	newUnitCommands.SetQueueType(CCommandQueue::NewUnitQueueType);

	CommandDescription c;

	// can't check for canmove here because it should be possible to assign rallypoint
	// with a non-moving factory.
	c.id=CMD_MOVE;
	c.action="move";
	c.type=CMDTYPE_ICON_MAP;
	c.name="Move";
	c.mouseicon=c.name;
	c.tooltip="Move: Order ready built units to move to a position";
	possibleCommands.push_back(c);

	if (owner->unitDef->canPatrol) {
		c.id=CMD_PATROL;
		c.action="patrol";
		c.type=CMDTYPE_ICON_MAP;
		c.name="Patrol";
		c.mouseicon=c.name;
		c.tooltip="Patrol: Order ready built units to patrol to one or more waypoints";
		possibleCommands.push_back(c);
	}

	if (owner->unitDef->canFight) {
		c.id = CMD_FIGHT;
		c.action="fight";
		c.type = CMDTYPE_ICON_MAP;
		c.name = "Fight";
		c.mouseicon=c.name;
		c.tooltip = "Fight: Order ready built units to take action while moving to a position";
		possibleCommands.push_back(c);
	}

	if (owner->unitDef->canGuard) {
		c.id=CMD_GUARD;
		c.action="guard";
		c.type=CMDTYPE_ICON_UNIT;
		c.name="Guard";
		c.mouseicon=c.name;
		c.tooltip="Guard: Order ready built units to guard another unit and attack units attacking it";
		possibleCommands.push_back(c);
	}

	CFactory* fac=(CFactory*)owner;

	map<int, string>::const_iterator bi;
	for (bi = fac->unitDef->buildOptions.begin(); bi != fac->unitDef->buildOptions.end(); ++bi) {
		const string name = bi->second;

		const UnitDef* ud = unitDefHandler->GetUnitDefByName(name);
		if (ud == NULL) {
		  string errmsg = "MOD ERROR: loading ";
		  errmsg += name.c_str();
		  errmsg += " for ";
		  errmsg += owner->unitDef->name;
			throw content_error(errmsg);
		}

		CommandDescription c;
		c.id = -ud->id; //build options are always negative
		c.action = "buildunit_" + StringToLower(ud->name);
		c.type = CMDTYPE_ICON;
		c.name = name;
		c.mouseicon = c.name;
		c.disabled = (ud->maxThisUnit <= 0);

		char tmp[1024];
		sprintf(tmp, "\nHealth %.0f\nMetal cost %.0f\nEnergy cost %.0f Build time %.0f",
		        ud->health, ud->metalCost, ud->energyCost, ud->buildTime);
		if (c.disabled) {
			c.tooltip = "\xff\xff\x22\x22" "DISABLED: " "\xff\xff\xff\xff";
		} else {
			c.tooltip = "Build: ";
		}
		c.tooltip += ud->humanName + " - " + ud->tooltip + tmp;

		possibleCommands.push_back(c);
		BuildOption bo;
		bo.name = name;
		bo.fullName = name;
		bo.numQued = 0;
		buildOptions[c.id] = bo;
	}
}



void CFactoryCAI::GiveCommandReal(const Command& c, bool fromSynced)
{
	const int& cmd_id = c.GetID();
	
	// move is always allowed for factories (passed to units it produces)
	if ((cmd_id == CMD_SET_WANTED_MAX_SPEED) ||
	    ((cmd_id != CMD_MOVE) && !AllowedCommand(c, fromSynced))) {
		return;
	}

	map<int, BuildOption>::iterator boi = buildOptions.find(cmd_id);

	// not a build order so queue it to built units
	if (boi == buildOptions.end()) {
		if ((nonQueingCommands.find(cmd_id) != nonQueingCommands.end()) ||
		    (cmd_id == CMD_INSERT) || (cmd_id == CMD_REMOVE) ||
		    (!(c.options & SHIFT_KEY) && ((cmd_id == CMD_WAIT) || (cmd_id == CMD_SELFD)))) {
			CCommandAI::GiveAllowedCommand(c);
			return;
		}

		if (!(c.options & SHIFT_KEY)) {
 			waitCommandsAI.ClearUnitQueue(owner, newUnitCommands);
			CCommandAI::ClearCommandDependencies();
			newUnitCommands.clear();
		}

		CCommandAI::AddCommandDependency(c);

		if (cmd_id != CMD_STOP) {
			if ((cmd_id == CMD_WAIT) || (cmd_id == CMD_SELFD)) {
				if (!newUnitCommands.empty() && (newUnitCommands.back().GetID() == cmd_id)) {
					if (cmd_id == CMD_WAIT) {
						waitCommandsAI.RemoveWaitCommand(owner, c);
					}
					newUnitCommands.pop_back();
				} else {
					newUnitCommands.push_back(c);
				}
			} else {
				bool dummy;
				if (CancelCommands(c, newUnitCommands, dummy) > 0) {
					return;
				} else {
					if (GetOverlapQueued(c, newUnitCommands).empty()) {
						newUnitCommands.push_back(c);
					} else {
						return;
					}
				}
			}
		}

		// the first new-unit build order can not be WAIT or SELFD
		while (!newUnitCommands.empty()) {
			const int& id = newUnitCommands.front().GetID();
			if ((id == CMD_WAIT) || (id == CMD_SELFD)) {
				if (cmd_id == CMD_WAIT) {
					waitCommandsAI.RemoveWaitCommand(owner, c);
				}
				newUnitCommands.pop_front();
			} else {
				break;
			}
		}

		return;
	}

	BuildOption& bo = boi->second;

	int numItems = 1;
	if (c.options & SHIFT_KEY)   { numItems *= 5; }
	if (c.options & CONTROL_KEY) { numItems *= 20; }

	if (c.options & RIGHT_MOUSE_KEY) {
		bo.numQued -= numItems;
		if (bo.numQued < 0) {
			bo.numQued = 0;
		}

		int numToErase = numItems;
		if (c.options & ALT_KEY) {
			for (unsigned int cmdNum = 0; cmdNum < commandQue.size() && numToErase; ++cmdNum) {
				if (commandQue[cmdNum].GetID() == cmd_id) {
					commandQue[cmdNum] = Command(CMD_STOP);
					numToErase--;
				}
			}
		} else {
			for (int cmdNum = commandQue.size() - 1; cmdNum != -1 && numToErase; --cmdNum) {
				if (commandQue[cmdNum].GetID() == cmd_id) {
					commandQue[cmdNum] = Command(CMD_STOP);
					numToErase--;
				}
			}
		}
		UpdateIconName(cmd_id, bo);
		SlowUpdate();
	}
	else {
		if (c.options & ALT_KEY) {
			for (int a = 0; a < numItems; ++a) {
				if (repeatOrders) {
					Command nc(c);
					nc.options |= DONT_REPEAT;
					if (commandQue.empty()) {
						commandQue.push_front(nc);
					} else {
						commandQue.insert(commandQue.begin()+1, nc);
					}
				} else {
					commandQue.push_front(c);
				}
			}
			if (!repeatOrders) {
				CFactory* fac = (CFactory*)owner;
				fac->StopBuild();
			}
		} else {
			for (int a = 0; a < numItems; ++a) {
				commandQue.push_back(c);
			}
		}
		bo.numQued += numItems;
		UpdateIconName(cmd_id, bo);

		SlowUpdate();
	}
}


void CFactoryCAI::InsertBuildCommand(CCommandQueue::iterator& it,
                                     const Command& newCmd)
{
	map<int, BuildOption>::iterator boi = buildOptions.find(newCmd.GetID());
	if (boi != buildOptions.end()) {
		boi->second.numQued++;
		UpdateIconName(newCmd.GetID(), boi->second);
	}
	if (!commandQue.empty() && (it == commandQue.begin())) {
		// ExecuteStop(), without the pop_front()
		CFactory* fac = (CFactory*)owner;
		fac->StopBuild();
	}
	commandQue.insert(it, newCmd);
}


bool CFactoryCAI::RemoveBuildCommand(CCommandQueue::iterator& it)
{
	Command& cmd = *it;
	map<int, BuildOption>::iterator boi = buildOptions.find(cmd.GetID());
	if (boi != buildOptions.end()) {
		boi->second.numQued--;
		UpdateIconName(cmd.GetID(), boi->second);
	}
	if (!commandQue.empty() && (it == commandQue.begin())) {
		ExecuteStop(cmd);
		return true;
	}

	if (cmd.GetID() < 0) {
		// build command, convert into a stop command
		cmd = Command(CMD_STOP);
	}

	return false;
}


void CFactoryCAI::DecreaseQueueCount(const Command& buildCommand, BuildOption& buildOption)
{
	const Command frontCommand = commandQue.front();

	if (!repeatOrders || (buildCommand.options & DONT_REPEAT))
		buildOption.numQued--;

	UpdateIconName(buildCommand.GetID(), buildOption);

	// if true, factory was set to wait and its buildee
	// could only have been finished by assisting units
	// --> make sure not to cancel the wait-order
	if (frontCommand.GetID() == CMD_WAIT)
		commandQue.pop_front();

	FinishCommand();

	if (frontCommand.GetID() == CMD_WAIT)
		commandQue.push_front(frontCommand);
}



// owner->curBuild can remain NULL for several frames after calling
// QueueBuild(); hence we need a callback or listen for an event to
// detect when the build-process actually finished
//
// NOTE: only called if Factory::QueueBuild returned FACTORY_NEXT_BUILD_ORDER
void FactoryFinishBuildCallBack(CFactory* factory, const Command& command) {
	CFactoryCAI* cai = dynamic_cast<CFactoryCAI*>(factory->commandAI);
	CFactoryCAI::BuildOption& bo = cai->buildOptions[command.GetID()];

	cai->DecreaseQueueCount(command, bo);
}

void CFactoryCAI::SlowUpdate()
{
	// Commands issued may invoke SlowUpdate when paused
	if (gs->paused)
		return;
	if (commandQue.empty() || owner->beingBuilt)
		return;

	CFactory* fac = (CFactory*) owner;

	while (!commandQue.empty()) {
		Command& c = commandQue.front();

		const size_t oldQueueSize = commandQue.size();
		const std::map<int, BuildOption>::iterator buildOptIt = buildOptions.find(c.GetID());

		if (buildOptIt != buildOptions.end()) {
			// build-order
			switch (fac->QueueBuild(unitDefHandler->GetUnitDefByID(-c.GetID()), c, &FactoryFinishBuildCallBack)) {
				case CFactory::FACTORY_SKIP_BUILD_ORDER: {
					// order rejected and we want to skip it permanently
					DecreaseQueueCount(c, buildOptions[c.GetID()]);
				} break;
			}
		} else {
			// regular order (move/wait/etc)
			switch (c.GetID()) {
				case CMD_STOP: {
					ExecuteStop(c);
				} break;
				default: {
					CCommandAI::SlowUpdate();
					break;
				}
			}
		}

		// exit if no command was consumed
		if (oldQueueSize == commandQue.size())
			break;
	}
}


void CFactoryCAI::ExecuteStop(Command& c)
{
	CFactory* fac = (CFactory*) owner;
	fac->StopBuild();

	commandQue.pop_front();
}


int CFactoryCAI::GetDefaultCmd(const CUnit* pointed, const CFeature* feature)
{
	if (pointed) {
		if (teamHandler->Ally(gu->myAllyTeam, pointed->allyteam)) {
			if (owner->unitDef->canGuard) {
				return CMD_GUARD;
			}
		}
	}
	return CMD_MOVE;
}


void CFactoryCAI::UpdateIconName(int cmdID, const BuildOption& bo)
{
	vector<CommandDescription>::iterator pci;
	for (pci = possibleCommands.begin(); pci != possibleCommands.end(); ++pci) {
		if (pci->id != cmdID)
			continue;

		char t[32];
		SNPRINTF(t, 10, "%d", bo.numQued);

		pci->name = bo.name;
		pci->params.clear();

		if (bo.numQued)
			pci->params.push_back(t);

		break;
	}

	selectedUnits.PossibleCommandChange(owner);
}
