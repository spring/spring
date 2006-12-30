#include "StdAfx.h"
#include "FactoryCAI.h"
#include "LineDrawer.h"
#include "ExternalAI/Group.h"
#include "Game/GameHelper.h"
#include "Game/SelectedUnits.h"
#include "Game/Team.h"
#include "Game/WaitCommandsAI.h"
#include "Game/UI/CommandColors.h"
#include "Game/UI/CursorIcons.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/glExtra.h"
#include "Rendering/UnitModels/UnitDrawer.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitLoader.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitTypes/Factory.h"
#include "mmgr.h"
#include "LogOutput.h"

CFactoryCAI::CFactoryCAI(CUnit* owner)
: CCommandAI(owner),
	building(false),
	lastRestrictedWarning(0)
{
	CommandDescription c;

	// can't check for canmove here because it should be possible to assign rallypoint
	// with a non-moving factory.
	c.id=CMD_MOVE;
	c.action="move";
	c.type=CMDTYPE_ICON_MAP;
	c.name="Move";
	c.hotkey="m";
	c.tooltip="Move: Order ready built units to move to a position";
	possibleCommands.push_back(c);

	if (owner->unitDef->canPatrol) {
		c.id=CMD_PATROL;
		c.action="patrol";
		c.type=CMDTYPE_ICON_MAP;
		c.name="Patrol";
		c.hotkey="p";
		c.tooltip="Patrol: Order ready built units to patrol to one or more waypoints";
		possibleCommands.push_back(c);
	}

	if (owner->unitDef->canFight) {
		c.id = CMD_FIGHT;
		c.action="fight";
		c.type = CMDTYPE_ICON_MAP;
		c.name = "Fight";
		c.hotkey = "f";
		c.tooltip = "Fight: Order ready built units to take action while moving to a position";
		possibleCommands.push_back(c);
	}

	if (owner->unitDef->canGuard) {
		c.id=CMD_GUARD;
		c.action="guard";
		c.type=CMDTYPE_ICON_UNIT;
		c.name="Guard";
		c.hotkey="g";
		c.tooltip="Guard: Order ready built units to guard another unit and attack units attacking it";
		possibleCommands.push_back(c);
	}

	CFactory* fac=(CFactory*)owner;

	map<int,string>::iterator bi;
	for(bi=fac->unitDef->buildOptions.begin();bi!=fac->unitDef->buildOptions.end();++bi){
		string name=bi->second;
		UnitDef* ud= unitDefHandler->GetUnitByName(name);
		CommandDescription c;
		c.id=-ud->id; //build options are always negative
		c.action="buildunit_" + StringToLower(ud->name);
		c.type=CMDTYPE_ICON;
		c.name=name;

		char tmp[500];
		sprintf(tmp,"\nHealth %.0f\nMetal cost %.0f\nEnergy cost %.0f Build time %.0f",ud->health,ud->metalCost,ud->energyCost,ud->buildTime);
		c.tooltip=string("Build: ")+ud->humanName + " " + ud->tooltip+tmp;

		possibleCommands.push_back(c);
		BuildOption bo;
		bo.name=name;
		bo.fullName=name;
		bo.numQued=0;
		buildOptions[c.id]=bo;
	}
}


CFactoryCAI::~CFactoryCAI()
{
}


void CFactoryCAI::GiveCommand(const Command& c)
{
	// move is always allowed for factories (passed to units it produces)
	if ((c.id == CMD_SET_WANTED_MAX_SPEED) ||
	    ((c.id != CMD_MOVE) && !AllowedCommand(c))) {
		return;
	}

	map<int, BuildOption>::iterator boi = buildOptions.find(c.id);

	// not a build order so queue it to built units
	if (boi == buildOptions.end()) {
		if ((nonQueingCommands.find(c.id) != nonQueingCommands.end()) ||
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

	BuildOption &bo=boi->second;
	
	int numItems=1;
	if(c.options& SHIFT_KEY)
		numItems*=5;
	if(c.options & CONTROL_KEY)
		numItems*=20;

	if(c.options & RIGHT_MOUSE_KEY){
		bo.numQued-=numItems;
		if(bo.numQued<0)
			bo.numQued=0;

		int numToErase=numItems;
		if(c.options & ALT_KEY){
			for(unsigned int cmdNum=0;cmdNum<commandQue.size() && numToErase;++cmdNum){
				if(commandQue[cmdNum].id==c.id){
					commandQue[cmdNum].id=CMD_STOP;
					numToErase--;
				}
			}
		} else {
			for(int cmdNum=commandQue.size()-1;cmdNum!=-1 && numToErase;--cmdNum){
				if(commandQue[cmdNum].id==c.id){
					commandQue[cmdNum].id=CMD_STOP;
					numToErase--;
				}
			}
		}
		UpdateIconName(c.id,bo);
		SlowUpdate();

	} else {
		if(c.options & ALT_KEY){
			for(int a=0;a<numItems;++a){
				commandQue.push_front(c);
			}
			building=false;
			CFactory* fac=(CFactory*)owner;
			fac->StopBuild();
		} else {
			for(int a=0;a<numItems;++a){
				commandQue.push_back(c);
			}
		}
		bo.numQued+=numItems;
		UpdateIconName(c.id,bo);

		SlowUpdate();
	}
}

void CFactoryCAI::SlowUpdate()
{
	if (commandQue.empty() || owner->beingBuilt) {
		return;
	}

	CFactory* fac=(CFactory*)owner;

	unsigned int oldSize;
	do {
		Command& c=commandQue.front();
		oldSize=commandQue.size();
		map<int,BuildOption>::iterator boi;
		if((boi=buildOptions.find(c.id))!=buildOptions.end()){
			if(building){
				if(!fac->curBuild && !fac->quedBuild){
					building=false;
					if(owner->group)
						owner->group->CommandFinished(owner->id,commandQue.front().id);
					if(!repeatOrders)
						boi->second.numQued--;
					UpdateIconName(c.id,boi->second);
					FinishCommand();
				}
			} else {
				const UnitDef *def = unitDefHandler->GetUnitByName(boi->second.name);
				if(uh->unitsType[owner->team][def->id]>=def->maxThisUnit){ //unit restricted?
					if(!repeatOrders){
						boi->second.numQued--;
						ENTER_MIXED;
						if (owner->team == gu->myTeam) {
							if(lastRestrictedWarning+100<gs->frameNum){
								logOutput.Print("%s: Build failed, unit type limit reached",owner->unitDef->humanName.c_str());
								logOutput.SetLastMsgPos(owner->pos);
								lastRestrictedWarning = gs->frameNum;
							}
						}
						ENTER_SYNCED;
					}
					UpdateIconName(c.id,boi->second);
					FinishCommand();
				}
				else if(uh->maxUnits>gs->Team(owner->team)->units.size()){  //max unitlimit reached?
					fac->StartBuild(boi->second.fullName);
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
		if (gs->Ally(gu->myAllyTeam, pointed->allyteam)) {
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
	lineDrawer.StartPath(owner->midPos, cmdColors.start);

	if (owner->selfDCountdown != 0) {
		lineDrawer.DrawIconAtLastPos(CMD_SELFD);
	}

	if (!commandQue.empty() && (commandQue.front().id == CMD_WAIT)) {
		DrawWaitIcon(commandQue.front());
	}

	deque<Command>::iterator ci;
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
				if((unit != NULL) && isTrackable(unit)) {
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
				glSurfaceCircle(bi.pos, bi.def->extractRange, 40);
				lineDrawer.Restart();
			}				
		}
	}
	lineDrawer.FinishPath();
}
