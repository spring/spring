#include "StdAfx.h"
#include "mmgr.h"

#include "FactoryCAI.h"
#include "ExternalAI/EngineOutHandler.h"
#include "LineDrawer.h"
#include "Sim/Units/Groups/Group.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Game/GameHelper.h"
#include "Game/SelectedUnits.h"
#include "Game/WaitCommandsAI.h"
#include "Game/UI/CommandColors.h"
#include "Game/UI/CursorIcons.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/glExtra.h"
#include "Rendering/UnitModels/UnitDrawer.h"
#include "Lua/LuaRules.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitLoader.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitTypes/Factory.h"
#include "LogOutput.h"
#include "creg/STL_Map.h"
#include "GlobalUnsynced.h"
#include "Util.h"
#include "Exceptions.h"

CR_BIND_DERIVED(CFactoryCAI ,CCommandAI , );

CR_REG_METADATA(CFactoryCAI , (
				CR_MEMBER(newUnitCommands),
				CR_MEMBER(buildOptions),
				CR_MEMBER(building),
				CR_MEMBER(lastRestrictedWarning),
				CR_RESERVED(16),
				CR_POSTLOAD(PostLoad)
				));

CR_BIND(CFactoryCAI::BuildOption, );

CR_REG_METADATA_SUB(CFactoryCAI,BuildOption , (
				CR_MEMBER(name),
				CR_MEMBER(fullName),
				CR_MEMBER(numQued)
				));


CFactoryCAI::CFactoryCAI()
: CCommandAI(),
	building(false),
	lastRestrictedWarning(0)
{}


CFactoryCAI::CFactoryCAI(CUnit* owner)
: CCommandAI(owner),
	building(false),
	lastRestrictedWarning(0)
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

	map<int,string>::const_iterator bi;
	for (bi = fac->unitDef->buildOptions.begin(); bi != fac->unitDef->buildOptions.end(); ++bi) {
		const string name = bi->second;

		const UnitDef* ud = unitDefHandler->GetUnitByName(name);
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


CFactoryCAI::~CFactoryCAI()
{
}


void CFactoryCAI::PostLoad()
{
}


void CFactoryCAI::GiveCommandReal(const Command& c, bool fromSynced)
{
	// move is always allowed for factories (passed to units it produces)
	if ((c.id == CMD_SET_WANTED_MAX_SPEED) ||
	    ((c.id != CMD_MOVE) && !AllowedCommand(c, fromSynced))) {
		return;
	}

	map<int, BuildOption>::iterator boi = buildOptions.find(c.id);

	// not a build order so queue it to built units
	if (boi == buildOptions.end()) {
		if ((nonQueingCommands.find(c.id) != nonQueingCommands.end()) ||
		    (c.id == CMD_INSERT) || (c.id == CMD_REMOVE) ||
		    (!(c.options & SHIFT_KEY) && ((c.id == CMD_WAIT) || (c.id == CMD_SELFD)))) {
			CCommandAI::GiveAllowedCommand(c);
			return;
		}

		if (!(c.options & SHIFT_KEY)) {
 			waitCommandsAI.ClearUnitQueue(owner, newUnitCommands);
			newUnitCommands.clear();
		}

		if (c.id != CMD_STOP) {
			if ((c.id == CMD_WAIT) || (c.id == CMD_SELFD)) {
				if (!newUnitCommands.empty() && (newUnitCommands.back().id == c.id)) {
					if (c.id == CMD_WAIT) {
						waitCommandsAI.RemoveWaitCommand(owner, c);
					}
					newUnitCommands.pop_back();
				} else {
					newUnitCommands.push_back(c);
				}
			}
			else {
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
			const int id = newUnitCommands.front().id;
			if ((id == CMD_WAIT) || (id == CMD_SELFD)) {
				if (c.id == CMD_WAIT) {
					waitCommandsAI.RemoveWaitCommand(owner, c);
				}
				newUnitCommands.pop_front();
			} else {
				break;
			}
		}

		return;
	}

	BuildOption &bo = boi->second;

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
				if (commandQue[cmdNum].id == c.id) {
					commandQue[cmdNum].id = CMD_STOP;
					numToErase--;
				}
			}
		}
		else {
			for (int cmdNum = commandQue.size() - 1; cmdNum != -1 && numToErase; --cmdNum) {
				if (commandQue[cmdNum].id == c.id) {
					commandQue[cmdNum].id = CMD_STOP;
					numToErase--;
				}
			}
		}
		UpdateIconName(c.id,bo);
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
				building=false;
				CFactory* fac = (CFactory*)owner;
				fac->StopBuild();
			}
		} else {
			for (int a = 0; a < numItems; ++a) {
				commandQue.push_back(c);
			}
		}
		bo.numQued += numItems;
		UpdateIconName(c.id, bo);

		SlowUpdate();
	}
}


void CFactoryCAI::InsertBuildCommand(CCommandQueue::iterator& it,
                                     const Command& newCmd)
{
	map<int, BuildOption>::iterator boi = buildOptions.find(newCmd.id);
	if (boi != buildOptions.end()) {
		boi->second.numQued++;
		UpdateIconName(newCmd.id, boi->second);
	}
	if (!commandQue.empty() && (it == commandQue.begin())) {
		// ExecuteStop(), without the pop_front()
		CFactory* fac = (CFactory*)owner;
		building = false;
		fac->StopBuild();
	}
	commandQue.insert(it, newCmd);
}


void CFactoryCAI::RemoveBuildCommand(CCommandQueue::iterator& it)
{
	Command& cmd = *it;
	map<int, BuildOption>::iterator boi = buildOptions.find(cmd.id);
	if (boi != buildOptions.end()) {
		boi->second.numQued--;
		UpdateIconName(cmd.id, boi->second);
	}
	if (!commandQue.empty() && (it == commandQue.begin())) {
		ExecuteStop(cmd);
		return;
	}
	if (cmd.id < 0) {
		cmd.id = CMD_STOP;
		cmd.tag = 0;
	}
}


void CFactoryCAI::CancelRestrictedUnit(const Command& c, BuildOption& buildOption)
{
	if(!repeatOrders || c.options & DONT_REPEAT) {
		buildOption.numQued--;
		if (owner->team == gu->myTeam) {
			if(lastRestrictedWarning+100<gs->frameNum) {
				logOutput.Print("%s: Build failed, unit type limit reached",owner->unitDef->humanName.c_str());
				logOutput.SetLastMsgPos(owner->pos);
				lastRestrictedWarning = gs->frameNum;
			}
		}
	}
	UpdateIconName(c.id, buildOption);
	FinishCommand();
}


void CFactoryCAI::SlowUpdate()
{
	if (commandQue.empty() || owner->beingBuilt) {
		return;
	}

	CFactory* fac=(CFactory*)owner;

	unsigned int oldSize;
	do {
		Command& c = commandQue.front();
		oldSize = commandQue.size();
		map<int,BuildOption>::iterator boi;
		if ((boi=buildOptions.find(c.id))!=buildOptions.end()) {
			const UnitDef *def = unitDefHandler->GetUnitByName(boi->second.name);
			if (building) {
				if (!fac->curBuild && !fac->quedBuild) {
					building=false;
					if (!repeatOrders || c.options & DONT_REPEAT) {
						boi->second.numQued--;
					}
					UpdateIconName(c.id,boi->second);
					FinishCommand();
				}
				// This can only be true if two factories started building
				// the restricted unit in the same simulation frame
				else if(uh->unitsByDefs[owner->team][def->id].size() > def->maxThisUnit){ //unit restricted?
					CFactory* fac=(CFactory*)owner;
					building = false;
					fac->StopBuild();
					CancelRestrictedUnit(c, boi->second);
				}
			} else {
				const UnitDef *def = unitDefHandler->GetUnitByName(boi->second.name);
				if(luaRules && !luaRules->AllowUnitCreation(def, owner, NULL)) {
					if(!repeatOrders || c.options & DONT_REPEAT){
						boi->second.numQued--;
					}
					UpdateIconName(c.id,boi->second);
					FinishCommand();
				}
				else if(uh->unitsByDefs[owner->team][def->id].size() >= def->maxThisUnit){ //unit restricted?
					CancelRestrictedUnit(c, boi->second);
				}
				else if(uh->MaxUnitsPerTeam() > teamHandler->Team(owner->team)->units.size()){  //max unitlimit reached?
					fac->StartBuild(def);
					building=true;
				}
			}
		}
		else {
			switch(c.id){
				case CMD_STOP: {
					ExecuteStop(c);
					break;
				}
				default: {
					CCommandAI::SlowUpdate();
					return;
				}
			}
		}
	} while ((oldSize != commandQue.size()) && !commandQue.empty());

	return;
}


void CFactoryCAI::ExecuteStop(Command &c)
{
	CFactory* fac=(CFactory*)owner;
	building=false;
	fac->StopBuild();
	commandQue.pop_front();
	return;
}


int CFactoryCAI::GetDefaultCmd(CUnit* pointed, CFeature* feature)
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


void CFactoryCAI::UpdateIconName(int id,BuildOption& bo)
{
	vector<CommandDescription>::iterator pci;
	for(pci=possibleCommands.begin();pci!=possibleCommands.end();++pci){
		if(pci->id==id){
			char t[32];
			SNPRINTF(t,10,"%d",bo.numQued);
			pci->name=bo.name;
			pci->params.clear();
			if(bo.numQued)
				pci->params.push_back(t);
			break;
		}
	}
	selectedUnits.PossibleCommandChange(owner);
}


void CFactoryCAI::DrawCommands(void)
{
	lineDrawer.StartPath(owner->drawMidPos, cmdColors.start);

	if (owner->selfDCountdown != 0) {
		lineDrawer.DrawIconAtLastPos(CMD_SELFD);
	}

	if (!commandQue.empty() && (commandQue.front().id == CMD_WAIT)) {
		DrawWaitIcon(commandQue.front());
	}

	CCommandQueue::iterator ci;
	for(ci=newUnitCommands.begin();ci!=newUnitCommands.end();++ci){
		switch(ci->id){
			case CMD_MOVE:{
				const float3 endPos(ci->params[0],ci->params[1]+3,ci->params[2]);
				lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.move);
				break;
			}
			case CMD_FIGHT:{
				const float3 endPos(ci->params[0],ci->params[1]+3,ci->params[2]);
				lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.fight);
				break;
			}
			case CMD_PATROL:{
				const float3 endPos(ci->params[0],ci->params[1]+3,ci->params[2]);
				lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.patrol);
				break;
			}
			case CMD_ATTACK:{
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
			case CMD_GUARD:{
				const CUnit* unit = uh->units[int(ci->params[0])];
				if ((unit != NULL) && isTrackable(unit)) {
					const float3 endPos =
						helper->GetUnitErrorPos(unit, owner->allyteam);
					lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.guard);
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
			default:
				DrawDefaultCommand(*ci);
				break;
		}

		if ((ci->id < 0) && (ci->params.size() >= 3)) {
			BuildInfo bi;
			bi.def = unitDefHandler->GetUnitByID(-(ci->id));
			if (ci->params.size() == 4) {
				bi.buildFacing = int(ci->params[3]);
			}
			bi.pos = float3(ci->params[0], ci->params[1], ci->params[2]);
			bi.pos = helper->Pos2BuildPos(bi);

			cursorIcons.AddBuildIcon(ci->id, bi.pos, owner->team, bi.buildFacing);
			lineDrawer.DrawLine(bi.pos, cmdColors.build);

			// draw metal extraction range
			if (bi.def->extractRange > 0) {
				lineDrawer.Break(bi.pos, cmdColors.build);
				glColor4fv(cmdColors.rangeExtract);

				if (bi.def->extractSquare) {
					glSurfaceSquare(bi.pos, bi.def->extractRange, bi.def->extractRange);
				} else {
					glSurfaceCircle(bi.pos, bi.def->extractRange, 40);
				}

				lineDrawer.Restart();
			}
		}
	}
	lineDrawer.FinishPath();
}
