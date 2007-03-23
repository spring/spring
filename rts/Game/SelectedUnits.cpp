// SelectedUnits.cpp: implementation of the CSelectedUnits class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Game/Team.h"
#include "SelectedUnits.h"
#include <map>
#include <SDL_types.h>
#include <SDL_keysym.h>
#include "WaitCommandsAI.h"
#include "Rendering/GL/myGL.h"
#include "NetProtocol.h"
#include "ExternalAI/GroupHandler.h"
#include "ExternalAI/Group.h"
#include "ExternalAI/GlobalAIHandler.h"
#include "UI/CommandColors.h"
#include "UI/GuiHandler.h"
#include "UI/TooltipConsole.h"
#include "LogOutput.h"
#include "Rendering/UnitModels/3DOParser.h"
#include "SelectedUnitsAI.h"
#include "Sim/Misc/Feature.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/CommandAI/BuilderCAI.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/CommandAI/LineDrawer.h"
#include "Sim/Units/UnitTypes/TransportUnit.h"
#include "System/Platform/ConfigHandler.h"
#include "Player.h"
#include "Camera.h"
#include "Sound.h"
#include "mmgr.h"

extern Uint8 *keys;


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CSelectedUnits selectedUnits;

CSelectedUnits::CSelectedUnits()
: selectionChanged(false),
	possibleCommandsChanged(true),
	selectedGroup(-1)
{
	buildIconsFirst = !!configHandler.GetInt("BuildIconsFirst", 0);
}


CSelectedUnits::~CSelectedUnits()
{
}


void CSelectedUnits::ToggleBuildIconsFirst()
{
	buildIconsFirst = !buildIconsFirst;
	possibleCommandsChanged = true;
}


CSelectedUnits::AvailableCommandsStruct CSelectedUnits::GetAvailableCommands()
{
	possibleCommandsChanged=false;

	if(selectedGroup!=-1 && grouphandler->groups[selectedGroup]->ai){
		AvailableCommandsStruct ac;
		ac.commandPage=grouphandler->groups[selectedGroup]->lastCommandPage;
		ac.commands=grouphandler->groups[selectedGroup]->GetPossibleCommands();

		CommandDescription c;			//make sure we can clear the group even when selected
		c.id=CMD_GROUPCLEAR;
		c.action="groupclear";
		c.type=CMDTYPE_ICON;
		c.name="Clear group";
		c.tooltip="Removes the units from any group they belong to";
		c.hotkey="Shift+q";
		ac.commands.push_back(c);

		return ac;
	}

	int commandPage=1000;
	int foundGroup=-2;
	int foundGroup2=-2;
	map<int,int> count;

	for(set<CUnit*>::iterator ui=selectedUnits.begin();ui!=selectedUnits.end();++ui){
		vector<CommandDescription>* c=&(*ui)->commandAI->GetPossibleCommands();
		vector<CommandDescription>::iterator ci;
		for(ci=c->begin();ci!=c->end();++ci)
			count[ci->id]=1;
		if((*ui)->commandAI->lastSelectedCommandPage<commandPage)
			commandPage=(*ui)->commandAI->lastSelectedCommandPage;

		if(foundGroup==-2 && (*ui)->group)
			foundGroup=(*ui)->group->id;
		if(!(*ui)->group || foundGroup!=(*ui)->group->id)
			foundGroup=-1;

		if(foundGroup2==-2 && (*ui)->group)
			foundGroup2=(*ui)->group->id;
		if(foundGroup2>=0 && (*ui)->group && (*ui)->group->id!=foundGroup2)
			foundGroup2=-1;
	}

	vector<CommandDescription> groupCommands;
	if (!gs->noHelperAIs) {
		//create a new group
		if (foundGroup != -2) {
			CommandDescription c;
			c.id=CMD_AISELECT;
			c.action="aiselect";
			c.type=CMDTYPE_COMBO_BOX;
			c.name="Select AI";
			c.tooltip="Create a new group using the selected units and with the ai selected";
			c.hotkey="Ctrl+q";

			c.params.push_back("0");
			c.params.push_back("None");
			map<AIKey,string>::iterator aai;
			map<AIKey,string> suitedAis = grouphandler->GetSuitedAis(selectedUnits);
			for (aai = suitedAis.begin(); aai != suitedAis.end(); ++aai) {
				c.params.push_back((aai->second).c_str());
			}
			groupCommands.push_back(c);
		}

		// add the selected units to a previous group (that at least one unit is also selected from)
		if ((foundGroup < 0) && (foundGroup2 >= 0)) {
			CommandDescription c;
			c.id=CMD_GROUPADD;
			c.action="groupadd";
			c.type=CMDTYPE_ICON;
			c.name="Add to group";
			c.tooltip="Adds the selected to an existing group (of which one or more units is already selected)";
			c.hotkey="q";
			groupCommands.push_back(c);
		}

		// select the group to which the units belong
		if (foundGroup >= 0) {
			CommandDescription c;

			c.id=CMD_GROUPSELECT;
			c.action="groupselect";
			c.type=CMDTYPE_ICON;
			c.name="Select group";
			c.tooltip="Select the group that these units belong to";
			c.hotkey="q";
			groupCommands.push_back(c);
		}

		// remove all selected units from any groups they belong to
		if (foundGroup2 != -2) {
			CommandDescription c;

			c.id=CMD_GROUPCLEAR;
			c.action="groupclear";
			c.type=CMDTYPE_ICON;
			c.name="Clear group";
			c.tooltip="Removes the units from any group they belong to";
			c.hotkey="Shift+q";
			groupCommands.push_back(c);
		}
	} // end if (!gs->noHelperAIs)

	vector<CommandDescription> commands ;
	// load the first set  (separating build and non-build commands)
	for(set<CUnit*>::iterator ui=selectedUnits.begin();ui!=selectedUnits.end();++ui){
		vector<CommandDescription>* c=&(*ui)->commandAI->GetPossibleCommands();
		vector<CommandDescription>::iterator ci;
		for(ci=c->begin(); ci!=c->end(); ++ci){
			if (buildIconsFirst) {
				if (ci->id >= 0) { continue; }
			} else {
				if (ci->id < 0)  { continue; }
			}
			if(ci->showUnique && selectedUnits.size()>1)
				continue;
			if(count[ci->id]>0){
				commands.push_back(*ci);
				count[ci->id]=0;
			}
		}
	}

	if (!buildIconsFirst && !gs->noHelperAIs) {
		vector<CommandDescription>::iterator ci;
		for(ci=groupCommands.begin(); ci!=groupCommands.end(); ++ci){
			commands.push_back(*ci);
		}
	}

	// load the second set  (all those that have not already been included)
	for(set<CUnit*>::iterator ui=selectedUnits.begin();ui!=selectedUnits.end();++ui){
		vector<CommandDescription>* c=&(*ui)->commandAI->GetPossibleCommands();
		vector<CommandDescription>::iterator ci;
		for(ci=c->begin(); ci!=c->end(); ++ci){
			if (buildIconsFirst) {
				if (ci->id < 0)  { continue; }
			} else {
				if (ci->id >= 0) { continue; }
			}
			if(ci->showUnique && selectedUnits.size()>1)
				continue;
			if(count[ci->id]>0){
				commands.push_back(*ci);
				count[ci->id]=0;
			}
		}
	}
	if (buildIconsFirst && !gs->noHelperAIs) {
		vector<CommandDescription>::iterator ci;
		for(ci=groupCommands.begin(); ci!=groupCommands.end(); ++ci){
			commands.push_back(*ci);
		}
	}

	AvailableCommandsStruct ac;
	ac.commandPage=commandPage;
	ac.commands=commands;
	return ac;
}


void CSelectedUnits::GiveCommand(Command c, bool fromUser)
{
//	logOutput.Print("Command given %i",c.id);
	if(gu->spectating || selectedUnits.empty())
		return;

	if(fromUser){		//add some statistics
		gs->players[gu->myPlayerNum]->currentStats->numCommands++;
		if(selectedGroup!=-1){
			gs->players[gu->myPlayerNum]->currentStats->unitCommands+=grouphandler->groups[selectedGroup]->units.size();
		} else {
			gs->players[gu->myPlayerNum]->currentStats->unitCommands+=selectedUnits.size();
		}
	}

	if (c.id == CMD_GROUPCLEAR) {
		for(set<CUnit*>::iterator ui=selectedUnits.begin();ui!=selectedUnits.end();++ui){
			if((*ui)->group){
				(*ui)->SetGroup(0);
				possibleCommandsChanged=true;
			}
		}
		return;
	}
	else if (c.id == CMD_GROUPSELECT) {
		SelectGroup((*selectedUnits.begin())->group->id);
		return;
	}
	else if (c.id == CMD_GROUPADD) {
		CGroup* group=0;
		for(set<CUnit*>::iterator ui=selectedUnits.begin();ui!=selectedUnits.end();++ui){
			if((*ui)->group){
				group=(*ui)->group;
				possibleCommandsChanged=true;
				break;
			}
		}
		if(group){
			for(set<CUnit*>::iterator ui=selectedUnits.begin();ui!=selectedUnits.end();++ui){
				if(!(*ui)->group)
					(*ui)->SetGroup(group);
			}
			SelectGroup(group->id);
		}
		return;
	}
	else if (c.id == CMD_AISELECT) {
		if (gs->noHelperAIs) {
			logOutput.Print("GroupAI and LuaUI control is disabled");
			return;
		}
		if(c.params[0]!=0){
			map<AIKey,string>::iterator aai;
			int a=0;
			for(aai=grouphandler->lastSuitedAis.begin();aai!=grouphandler->lastSuitedAis.end() && a<c.params[0]-1;++aai){
				a++;
			}
			CGroup* group=grouphandler->CreateNewGroup(aai->first);

			for(set<CUnit*>::iterator ui=selectedUnits.begin();ui!=selectedUnits.end();++ui){
				(*ui)->SetGroup(group);
			}
			SelectGroup(group->id);
		}
		return;
	}
	else if (c.id == CMD_TIMEWAIT) {
		waitCommandsAI.AddTimeWait(c);
		return;
	}
	else if (c.id == CMD_DEATHWAIT) {
		if (gs->activeAllyTeams <= 2) {
			waitCommandsAI.AddDeathWait(c);
		} else {
			logOutput.Print("DeathWait can only be used when there are 2 Ally Teams");
		}
		return;
	}
	else if (c.id == CMD_SQUADWAIT) {
		waitCommandsAI.AddSquadWait(c);
		return;
	}
	else if (c.id == CMD_GATHERWAIT) {
		waitCommandsAI.AddGatherWait(c);
		return;
	}

//	FIXME:  selectedUnitsAI.GiveCommand(c);

	if ((selectedGroup != -1) && grouphandler->groups[selectedGroup]->ai) {
		grouphandler->groups[selectedGroup]->GiveCommand(c);
		return;
	}

	SendCommand(c);

	if(!selectedUnits.empty()){
		set<CUnit*>::iterator ui = selectedUnits.begin();
		if((*ui)->unitDef->sounds.ok.id)
			sound->PlayUnitReply((*ui)->unitDef->sounds.ok.id, (*ui), (*ui)->unitDef->sounds.ok.volume, true);
	}
}


void CSelectedUnits::AddUnit(CUnit* unit)
{
	// if unit is being transported by eg. Hulk or Atlas
	// then we should not be able to select it
	if (unit->transporter != NULL && !unit->transporter->unitDef->isfireplatform) {
		return;
	}
	
	if (unit->noSelect) {
		return;
	}

	selectedUnits.insert(unit);
	AddDeathDependence(unit);
	selectionChanged = true;
	possibleCommandsChanged = true;

	if (!(unit->group) || unit->group->id != selectedGroup)
		selectedGroup = -1;

	PUSH_CODE_MODE;
	ENTER_MIXED;
	unit->commandAI->selected = true;
	POP_CODE_MODE;
}


void CSelectedUnits::RemoveUnit(CUnit* unit)
{
	selectedUnits.erase(unit);
	DeleteDeathDependence(unit);
	selectionChanged=true;
	possibleCommandsChanged=true;
	selectedGroup=-1;
	PUSH_CODE_MODE;
	ENTER_MIXED;
	unit->commandAI->selected=false;
	POP_CODE_MODE;
}


void CSelectedUnits::ClearSelected()
{
	set<CUnit*>::iterator ui;
	ENTER_MIXED;
	for(ui=selectedUnits.begin();ui!=selectedUnits.end();++ui){
		(*ui)->commandAI->selected=false;
		DeleteDeathDependence(*ui);
	}
	ENTER_UNSYNCED;

	selectedUnits.clear();
	selectionChanged=true;
	possibleCommandsChanged=true;
	selectedGroup=-1;
}


void CSelectedUnits::SelectGroup(int num)
{
	ClearSelected();
	selectedGroup=num;
	CGroup* group=grouphandler->groups[num];

	set<CUnit*>::iterator ui;
	ENTER_MIXED;
	for(ui=group->units.begin();ui!=group->units.end();++ui){
		(*ui)->commandAI->selected=true;
		selectedUnits.insert(*ui);
		AddDeathDependence(*ui);
	}
	ENTER_UNSYNCED;

	possibleCommandsChanged=true;
	selectionChanged=true;
}


void CSelectedUnits::Draw()
{
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND); // for line smoothing
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glLineWidth(cmdColors.UnitBoxLineWidth());

	glColor4fv(cmdColors.unitBox);

	glBegin(GL_QUADS);
	set<CUnit*>::iterator ui;
	if(selectedGroup!=-1){
		for(ui=grouphandler->groups[selectedGroup]->units.begin();ui!=grouphandler->groups[selectedGroup]->units.end();++ui){
			if((*ui)->isIcon)
				continue;
			float3 pos((*ui)->pos+(*ui)->speed*gu->timeOffset);
			glVertexf3(pos+float3((*ui)->xsize*4,0,(*ui)->ysize*4));
			glVertexf3(pos+float3(-(*ui)->xsize*4,0,(*ui)->ysize*4));
			glVertexf3(pos+float3(-(*ui)->xsize*4,0,-(*ui)->ysize*4));
			glVertexf3(pos+float3((*ui)->xsize*4,0,-(*ui)->ysize*4));
		}
	} else {
		for(ui=selectedUnits.begin();ui!=selectedUnits.end();++ui){
			if((*ui)->isIcon)
				continue;
			float3 pos((*ui)->pos+(*ui)->speed*gu->timeOffset);
			glVertexf3(pos+float3((*ui)->xsize*4,0,(*ui)->ysize*4));
			glVertexf3(pos+float3(-(*ui)->xsize*4,0,(*ui)->ysize*4));
			glVertexf3(pos+float3(-(*ui)->xsize*4,0,-(*ui)->ysize*4));
			glVertexf3(pos+float3((*ui)->xsize*4,0,-(*ui)->ysize*4));
		}
	}
	glEnd();

	// highlight queued build sites if we are about to build something
	// (or old-style, whenever the shift key is being held down)
	if (!selectedUnits.empty() &&
	    ((cmdColors.BuildBoxesOnShift() && keys[SDLK_LSHIFT]) ||
	     ((guihandler->inCommand >= 0) &&
	      (guihandler->inCommand < guihandler->commands.size()) &&
	      (guihandler->commands[guihandler->inCommand].id < 0)))) {
		set<CBuilderCAI*>::const_iterator bi;
		for (bi = uh->builderCAIs.begin(); bi != uh->builderCAIs.end(); ++bi) {
			if ((*bi)->owner->team == gu->myTeam) {
				(*bi)->DrawQuedBuildingSquares();
			}
		}
	}

	glLineWidth(1.0f);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
}


void CSelectedUnits::DependentDied(CObject *o)
{
	selectedUnits.erase((CUnit*)o);
	selectionChanged=true;
	possibleCommandsChanged=true;
}


void CSelectedUnits::NetSelect(vector<int>& s,int player)
{
	netSelected[player] = s;
}


void CSelectedUnits::NetOrder(Command &c, int player)
{
	selectedUnitsAI.GiveCommandNet(c,player);

	if (netSelected[player].size() > 0)
		globalAI->PlayerCommandGiven(netSelected[player],c,player);
}


bool CSelectedUnits::CommandsChanged()
{
	return possibleCommandsChanged;
}


/******************************************************************************/
//
//  GetDefaultCmd() and friends
//

static bool targetIsEnemy = false;
static const CUnit* targetUnit = NULL;
static const CFeature* targetFeature = NULL;


static inline bool CanDamage(const UnitDef* ud)
{
	return ((ud->canAttack && !ud->weapons.empty()) || ud->canKamikaze);
}


static inline bool IsBetterLeader(const UnitDef* newDef, const UnitDef* oldDef)
{
	// There is a lot more that could be done here to make better
	// selections, but the users may prefer simplicity over smarts.

	if (targetUnit) {
		if (targetIsEnemy) {
			const bool newCanDamage = CanDamage(newDef);
			const bool oldCanDamage = CanDamage(oldDef);
			if ( newCanDamage && !oldCanDamage) { return true;  }
			if (!newCanDamage &&  oldCanDamage) { return false; }
			if (!CanDamage(targetUnit->unitDef)) {
				if ( newDef->canReclaim && !oldDef->canReclaim) { return true;  }
				if (!newDef->canReclaim &&  oldDef->canReclaim) { return false; }
			}
		}
		else { // targetIsAlly
			if (targetUnit->health < targetUnit->maxHealth) {
				if ( newDef->canRepair && !oldDef->canRepair) { return true;  }
				if (!newDef->canRepair &&  oldDef->canRepair) { return false; }
			}
			const bool newCanLoad = (newDef->transportCapacity > 0);
			const bool oldCanLoad = (oldDef->transportCapacity > 0);
			if ( newCanLoad && !oldCanLoad) { return true;  }
			if (!newCanLoad &&  oldCanLoad) { return false; }
			if ( newDef->canGuard && !oldDef->canGuard) { return true;  }
			if (!newDef->canGuard &&  oldDef->canGuard) { return false; }
		}
	}
	else if (targetFeature) {
		if (!targetFeature->createdFromUnit.empty()) {
			if ( newDef->canResurrect && !oldDef->canResurrect) { return true;  }
			if (!newDef->canResurrect &&  oldDef->canResurrect) { return false; }
		}
		if ( newDef->canReclaim && !oldDef->canReclaim) { return true;  }
		if (!newDef->canReclaim &&  oldDef->canReclaim) { return false; }
	}

	return (newDef->speed > oldDef->speed); // CMD_MOVE?
}


int CSelectedUnits::GetDefaultCmd(CUnit *unit, CFeature* feature)
{
	// NOTE: the unitDef->aihint value is being ignored

	if ((selectedGroup != -1) && grouphandler->groups[selectedGroup]->ai) {
		return grouphandler->groups[selectedGroup]->GetDefaultCmd(unit, feature);
	}

	// return the default if there are no units selected
	set<CUnit*>::const_iterator ui = selectedUnits.begin();
	if (ui == selectedUnits.end()) {
		return CMD_STOP;
	}

	// setup the locals for IsBetterLeader()
	targetUnit = unit;
	targetFeature = feature;
	if (targetUnit) {
		targetIsEnemy = !gs->Ally(gu->myAllyTeam, targetUnit->allyteam);
	}

	// find the best leader to pick the command
	const CUnit* leaderUnit = *ui;
	const UnitDef* leaderDef = leaderUnit->unitDef;
	for (ui++; ui != selectedUnits.end(); ++ui) {
		const CUnit* testUnit = *ui;
		const UnitDef* testDef = testUnit->unitDef;
		if (testDef != leaderDef) {
			if (IsBetterLeader(testDef, leaderDef)) {
				leaderDef = testDef;
				leaderUnit = testUnit;
			}
		}
	}

	return (leaderUnit->commandAI->GetDefaultCmd(unit, feature));
}


/******************************************************************************/

void CSelectedUnits::AiOrder(int unitid, Command &c)
{
	if(uh->units[unitid]!=0)
		uh->units[unitid]->commandAI->GiveCommand(c);
}


void CSelectedUnits::PossibleCommandChange(CUnit* sender)
{
	if(sender==0 || selectedUnits.find(sender)!=selectedUnits.end())
		possibleCommandsChanged=true;
}


void CSelectedUnits::DrawCommands()
{
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);

	lineDrawer.Configure(cmdColors.UseColorRestarts(),
	                     cmdColors.UseRestartColor(),
	                     cmdColors.restart,
	                     cmdColors.RestartAlpha());
	lineDrawer.SetupLineStipple();

	glEnable(GL_BLEND);
	glBlendFunc((GLenum)cmdColors.QueuedBlendSrc(),
	            (GLenum)cmdColors.QueuedBlendDst());

	glLineWidth(cmdColors.QueuedLineWidth());

	set<CUnit*>::iterator ui;
	if (selectedGroup != -1) {
		set<CUnit*>& groupUnits = grouphandler->groups[selectedGroup]->units;
		for(ui = groupUnits.begin(); ui != groupUnits.end(); ++ui) {
			(*ui)->commandAI->DrawCommands();
		}
	} else {
		for(ui = selectedUnits.begin(); ui != selectedUnits.end(); ++ui) {
			(*ui)->commandAI->DrawCommands();
		}
	}

	// draw the commands from AIs
	grouphandler->DrawCommands();
	waitCommandsAI.DrawCommands();

	glLineWidth(1.0f);

	glEnable(GL_DEPTH_TEST);
}


std::string CSelectedUnits::GetTooltip(void)
{
	std::string s;
	if ((selectedGroup != -1) && grouphandler->groups[selectedGroup]->ai) {
		s = "Group selected";
	} else if (!selectedUnits.empty()) {
		// show the player name instead of unit name if it has FBI tag showPlayerName
		if ((*selectedUnits.begin())->unitDef->showPlayerName) {
			s = gs->players[gs->Team((*selectedUnits.begin())->team)->leader]->playerName.c_str();
		} else {
			s = (*selectedUnits.begin())->tooltip;
		}
	}
	if (selectedUnits.empty()) {
		return s;
	}

	char tmp[500];
	int numFuel = 0;
	float maxHealth = 0.0f, curHealth = 0.0f;
	float maxFuel = 0.0f, curFuel = 0.0f;
	float exp = 0.0f, cost = 0.0f, range = 0.0f;
	float metalMake = 0.0f, metalUse = 0.0f, energyMake = 0.0f, energyUse = 0.0f;

	set<CUnit*>::iterator ui;
	for (ui = selectedUnits.begin(); ui != selectedUnits.end(); ++ui) {
		const CUnit* unit = *ui;
		maxHealth  += unit->maxHealth;
		curHealth  += unit->health;
		exp        += unit->experience;
		cost       += unit->metalCost + (unit->energyCost / 60.0f);
		range      += unit->maxRange;
		metalMake  += unit->metalMake;
		metalUse   += unit->metalUse;
		energyMake += unit->energyMake;
		energyUse  += unit->energyUse;
		maxFuel    += unit->unitDef->maxFuel;
		curFuel    += unit->currentFuel;
		if (unit->unitDef->maxFuel > 0) {
			numFuel++;
		}
	}
	if ((numFuel > 0) && (maxFuel > 0.0f)) {
		curFuel = curFuel / numFuel;
		maxFuel = maxFuel / numFuel;
	}
	const float num = selectedUnits.size();

	s += CTooltipConsole::MakeUnitStatsString(
	       curHealth, maxHealth,
	       curFuel,   maxFuel,
	       (exp / num), cost, (range / num),
	       metalMake,  metalUse,
	       energyMake, energyUse);

  if (gs->cheatEnabled && (selectedUnits.size() == 1)) {
  	CUnit* unit = *selectedUnits.begin();
    SNPRINTF(tmp, sizeof(tmp), "\xff\xc0\xc0\xff  [TechLevel %i]",
             unit->unitDef->techLevel);
    s += tmp;
	}

	return s;
}


void CSelectedUnits::SetCommandPage(int page)
{
	if(selectedGroup!=-1 && grouphandler->groups[selectedGroup]->ai){
		grouphandler->groups[selectedGroup]->lastCommandPage=page;
	}

	std::set<CUnit*>::iterator ui;
	for (ui = selectedUnits.begin(); ui != selectedUnits.end(); ++ui) {
		(*ui)->commandAI->lastSelectedCommandPage = page;
	}
}


void CSelectedUnits::SendSelection(void)
{
	// first, convert CUnit* to unit IDs.
	std::vector<short> selectedUnitIDs(selectedUnits.size());
	std::vector<short>::iterator i = selectedUnitIDs.begin();
	std::set<CUnit*>::const_iterator ui = selectedUnits.begin();
	for(; ui != selectedUnits.end(); ++i, ++ui) *i = (*ui)->id;
	net->SendSelect(gu->myPlayerNum, selectedUnitIDs);
	selectionChanged=false;
}


void CSelectedUnits::SendCommand(Command& c)
{
	if(selectionChanged){		//send new selection
		SendSelection();
	}
	net->SendCommand(gu->myPlayerNum, c.id, c.options, c.params);
}


void CSelectedUnits::SendCommandsToUnits(const vector<int>& unitIDs,
                                         const vector<Command>& commands)
{
	// NOTE: does not check for invalid unitIDs

	if (gu->spectating) {
		return; // don't waste bandwidth
	}

	int u, c;
	unsigned char buf[8192];
	const int unitIDCount  = (int)unitIDs.size();
	const int commandCount = (int)commands.size();

	if ((unitIDCount <= 0) || (commandCount <= 0)) {
		return;
	}

	int totalParams = 0;
	for (c = 0; c < commandCount; c++) {
		totalParams += commands[c].params.size();
	}

	int msgLen = 0;
	msgLen += (1 + 2 + 1); // msg type, msg size, player ID
	msgLen += 2; // unitID count
	msgLen += unitIDCount * 2;
	msgLen += 2; // command count
	msgLen += commandCount * (4 + 1 + 2); // id, options, params size
	msgLen += totalParams * 4;
	if (msgLen > sizeof(buf)) {
		logOutput.Print("Discarded oversized NETMSG_AICOMMANDS packet: %i\n",
		                msgLen);
		return; // drop the oversized packet
	}

	unsigned char* ptr = buf;

// FIXME -- hackish
#define PACK(type, value) *((type*)(ptr)) = value; ptr = ptr + sizeof(type)

	PACK(unsigned char,  NETMSG_AICOMMANDS);
	PACK(unsigned short, msgLen);
	PACK(unsigned char,  gu->myPlayerNum);

	PACK(unsigned short, unitIDCount);
	for (u = 0; u < unitIDCount; u++) {
		PACK(unsigned short, unitIDs[u]);
	}

	PACK(unsigned short, commandCount);
	for (c = 0; c < commandCount; c++) {
		const Command& cmd = commands[c];
		PACK(unsigned int,   cmd.id);
		PACK(unsigned char,  cmd.options);
		PACK(unsigned short, cmd.params.size());
		for (int p = 0; p < (int)cmd.params.size(); p++) {
			PACK(float, cmd.params[p]);
		}
	}

	net->SendData(buf, msgLen);

	return;
}
