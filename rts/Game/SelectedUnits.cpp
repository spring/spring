/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include <map>
#include <SDL_keysym.h>

#include "mmgr.h"

#include "SelectedUnits.h"
#include "SelectedUnitsAI.h"
#include "Camera.h"
#include "WaitCommandsAI.h"
#include "PlayerHandler.h"
#include "UI/CommandColors.h"
#include "UI/GuiHandler.h"
#include "UI/TooltipConsole.h"
#include "ExternalAI/EngineOutHandler.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Features/Feature.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/CommandAI/BuilderCAI.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/CommandAI/LineDrawer.h"
#include "Sim/Units/Groups/GroupHandler.h"
#include "Sim/Units/Groups/Group.h"
#include "Sim/Units/UnitTypes/TransportUnit.h"
#include "System/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/LogOutput.h"
#include "System/Util.h"
#include "System/NetProtocol.h"
#include "System/Net/PackPacket.h"
#include "System/Input/KeyInput.h"
#include "System/Sound/IEffectChannel.h"

#define PLAY_SOUNDS 1

CSelectedUnits selectedUnits;


CSelectedUnits::CSelectedUnits()
	: selectionChanged(false)
	, possibleCommandsChanged(true)
	, buildIconsFirst(false)
	, selectedGroup(-1)
{
}


CSelectedUnits::~CSelectedUnits()
{
}


void CSelectedUnits::Init(unsigned numPlayers)
{
	buildIconsFirst = !!configHandler->Get("BuildIconsFirst", 0);
	netSelected.resize(numPlayers);
}


void CSelectedUnits::ToggleBuildIconsFirst()
{
	buildIconsFirst = !buildIconsFirst;
	possibleCommandsChanged = true;
}


CSelectedUnits::AvailableCommandsStruct CSelectedUnits::GetAvailableCommands()
{
	GML_RECMUTEX_LOCK(grpsel); // GetAvailableCommands

	possibleCommandsChanged = false;

	int commandPage = 1000;
	int foundGroup = -2;
	int foundGroup2 = -2;
	map<int, int> states;

	for (CUnitSet::const_iterator ui = selectedUnits.begin(); ui != selectedUnits.end(); ++ui) {
		const std::vector<CommandDescription>* c = &((*ui)->commandAI->GetPossibleCommands());
		std::vector<CommandDescription>::const_iterator ci;
		for (ci = c->begin(); ci != c->end(); ++ci) {
			states[ci->id] = ci->disabled ? 2 : 1;
		}
		if ((*ui)->commandAI->lastSelectedCommandPage < commandPage) {
			commandPage = (*ui)->commandAI->lastSelectedCommandPage;
		}

		if (foundGroup == -2 && (*ui)->group) {
			foundGroup = (*ui)->group->id;
		}
		if (!(*ui)->group || foundGroup!=(*ui)->group->id) {
			foundGroup = -1;
		}

		if (foundGroup2 == -2 && (*ui)->group) {
			foundGroup2 = (*ui)->group->id;
		}
		if (foundGroup2 >= 0 && (*ui)->group && (*ui)->group->id != foundGroup2) {
			foundGroup2 = -1;
		}
	}

	std::vector<CommandDescription> groupCommands;

	std::vector<CommandDescription> commands ;
	// load the first set (separating build and non-build commands)
	for (CUnitSet::const_iterator ui = selectedUnits.begin(); ui != selectedUnits.end(); ++ui) {
		const std::vector<CommandDescription>* c = &((*ui)->commandAI->GetPossibleCommands());
		std::vector<CommandDescription>::const_iterator ci;
		for (ci = c->begin(); ci != c->end(); ++ci) {
			if (buildIconsFirst) {
				if (ci->id >= 0) { continue; }
			} else {
				if (ci->id < 0)  { continue; }
			}
			if (ci->showUnique && selectedUnits.size() > 1) {
				continue;
			}
			if (states[ci->id] > 0) {
				commands.push_back(*ci);
				states[ci->id] = 0;
			}
		}
	}

	if (!buildIconsFirst && !gs->noHelperAIs) {
		std::vector<CommandDescription>::const_iterator ci;
		for(ci = groupCommands.begin(); ci != groupCommands.end(); ++ci) {
			commands.push_back(*ci);
		}
	}

	// load the second set (all those that have not already been included)
	for (CUnitSet::const_iterator ui = selectedUnits.begin(); ui != selectedUnits.end(); ++ui) {
		std::vector<CommandDescription>* c = &(*ui)->commandAI->GetPossibleCommands();
		std::vector<CommandDescription>::const_iterator ci;
		for (ci = c->begin(); ci != c->end(); ++ci) {
			if (buildIconsFirst) {
				if (ci->id < 0)  { continue; }
			} else {
				if (ci->id >= 0) { continue; }
			}
			if (ci->showUnique && selectedUnits.size() > 1) {
				continue;
			}
			if (states[ci->id] > 0) {
				commands.push_back(*ci);
				states[ci->id] = 0;
			}
		}
	}
	if (buildIconsFirst && !gs->noHelperAIs) {
		std::vector<CommandDescription>::const_iterator ci;
		for (ci = groupCommands.begin(); ci != groupCommands.end(); ++ci) {
			commands.push_back(*ci);
		}
	}

	AvailableCommandsStruct ac;
	ac.commandPage = commandPage;
	ac.commands = commands;
	return ac;
}


void CSelectedUnits::GiveCommand(Command c, bool fromUser)
{
	GML_RECMUTEX_LOCK(grpsel); // GiveCommand

//	logOutput.Print("Command given %i",c.id);
	if ((gu->spectating && !gs->godMode) || selectedUnits.empty()) {
		return;
	}

	if (fromUser) { // add some statistics
		playerHandler->Player(gu->myPlayerNum)->currentStats.numCommands++;
		if (selectedGroup != -1) {
			playerHandler->Player(gu->myPlayerNum)->currentStats.unitCommands += grouphandlers[gu->myTeam]->groups[selectedGroup]->units.size();
		} else {
			playerHandler->Player(gu->myPlayerNum)->currentStats.unitCommands += selectedUnits.size();
		}
	}

	if (c.id == CMD_GROUPCLEAR) {
		for (CUnitSet::iterator ui = selectedUnits.begin(); ui != selectedUnits.end(); ++ui) {
			if ((*ui)->group) {
				(*ui)->SetGroup(0);
				possibleCommandsChanged = true;
			}
		}
		return;
	}
	else if (c.id == CMD_GROUPSELECT) {
		SelectGroup((*selectedUnits.begin())->group->id);
		return;
	}
	else if (c.id == CMD_GROUPADD) {
		CGroup* group = NULL;
		for (CUnitSet::iterator ui = selectedUnits.begin(); ui != selectedUnits.end(); ++ui) {
			if ((*ui)->group) {
				group = (*ui)->group;
				possibleCommandsChanged = true;
				break;
			}
		}
		if (group) {
			for (CUnitSet::iterator ui = selectedUnits.begin(); ui != selectedUnits.end(); ++ui) {
				if (!(*ui)->group) {
					(*ui)->SetGroup(group);
				}
			}
			SelectGroup(group->id);
		}
		return;
	}
	else if (c.id == CMD_AISELECT) {
		if (gs->noHelperAIs) {
			logOutput.Print("LuaUI control is disabled");
			return;
		}
		if (c.params[0] != 0) {
			CGroup* group = grouphandlers[gu->myTeam]->CreateNewGroup();

			for (CUnitSet::iterator ui = selectedUnits.begin(); ui != selectedUnits.end(); ++ui) {
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
		waitCommandsAI.AddDeathWait(c);
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

	SendCommand(c);

	#ifdef PLAY_SOUNDS
	if (!selectedUnits.empty()) {
		CUnitSet::const_iterator ui = selectedUnits.begin();

		const int soundIdx = (*ui)->unitDef->sounds.ok.getRandomIdx();
		if (soundIdx >= 0) {
			Channels::UnitReply.PlaySample(
				(*ui)->unitDef->sounds.ok.getID(soundIdx), (*ui),
				(*ui)->unitDef->sounds.ok.getVolume(soundIdx));
		}
	}
	#endif
}


void CSelectedUnits::AddUnit(CUnit* unit)
{
	// if unit is being transported by eg. Hulk or Atlas
	// then we should not be able to select it
	const CTransportUnit* trans = unit->GetTransporter();
	if (trans != NULL && !trans->unitDef->isFirePlatform) {
		return;
	}

	if (unit->noSelect) {
		return;
	}

	GML_RECMUTEX_LOCK(sel); // AddUnit

	selectedUnits.insert(unit);
	AddDeathDependence(unit);
	selectionChanged = true;
	possibleCommandsChanged = true;

	if (!(unit->group) || unit->group->id != selectedGroup) {
		selectedGroup = -1;
	}

	unit->commandAI->selected = true;
}


void CSelectedUnits::RemoveUnit(CUnit* unit)
{
	GML_RECMUTEX_LOCK(sel); // RemoveUnit

	selectedUnits.erase(unit);
	DeleteDeathDependence(unit);
	selectionChanged = true;
	possibleCommandsChanged = true;
	selectedGroup = -1;
	unit->commandAI->selected = false;
}


void CSelectedUnits::ClearSelected()
{
	GML_RECMUTEX_LOCK(sel); // ClearSelected

	CUnitSet::iterator ui;
	for (ui = selectedUnits.begin(); ui != selectedUnits.end(); ++ui) {
		(*ui)->commandAI->selected = false;
		DeleteDeathDependence(*ui);
	}

	selectedUnits.clear();
	selectionChanged = true;
	possibleCommandsChanged = true;
	selectedGroup = -1;
}


void CSelectedUnits::SelectGroup(int num)
{
	GML_RECMUTEX_LOCK(grpsel); // SelectGroup - not needed? only reading group

	ClearSelected();
	selectedGroup=num;
	CGroup* group=grouphandlers[gu->myTeam]->groups[num];

	CUnitSet::iterator ui;
	for (ui = group->units.begin(); ui != group->units.end(); ++ui) {
		if (!(*ui)->noSelect) {
			(*ui)->commandAI->selected = true;
			selectedUnits.insert(*ui);
			AddDeathDependence(*ui);
		}
	}

	selectionChanged = true;
	possibleCommandsChanged = true;
}


void CSelectedUnits::Draw()
{
	glDisable(GL_TEXTURE_2D);
	glDepthMask(false);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND); // for line smoothing
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glLineWidth(cmdColors.UnitBoxLineWidth());

	GML_RECMUTEX_LOCK(grpsel); // Draw

	if (cmdColors.unitBox[3] > 0.05f) {
		const CUnitSet* unitSet;
		if (selectedGroup != -1) {
			unitSet = &grouphandlers[gu->myTeam]->groups[selectedGroup]->units;
		} else {
			unitSet = &selectedUnits;
		}

		CVertexArray* va = GetVertexArray();
		va->Initialize();
		va->EnlargeArrays(unitSet->size() * 8, 0, VA_SIZE_C);

		for (CUnitSet::const_iterator ui = unitSet->begin(); ui != unitSet->end(); ++ui) {
			const CUnit* unit = *ui;
			if (unit->isIcon) {
				continue;
			}

			const int
				uhxsize = (unit->xsize * SQUARE_SIZE) >> 1,
				uhzsize = (unit->zsize * SQUARE_SIZE) >> 1,
				mhxsize = (unit->mobility == NULL)? uhxsize: ((unit->mobility->xsize * SQUARE_SIZE) >> 1),
				mhzsize = (unit->mobility == NULL)? uhzsize: ((unit->mobility->zsize * SQUARE_SIZE) >> 1);
			const float3 verts[8] = {
				// UnitDef footprint corners
				float3(unit->drawPos.x + uhxsize, unit->drawPos.y, unit->drawPos.z + uhzsize),
				float3(unit->drawPos.x - uhxsize, unit->drawPos.y, unit->drawPos.z + uhzsize),
				float3(unit->drawPos.x - uhxsize, unit->drawPos.y, unit->drawPos.z - uhzsize),
				float3(unit->drawPos.x + uhxsize, unit->drawPos.y, unit->drawPos.z - uhzsize),
				// MoveDef footprint corners
				float3(unit->drawPos.x + mhxsize, unit->drawPos.y, unit->drawPos.z + mhzsize),
				float3(unit->drawPos.x - mhxsize, unit->drawPos.y, unit->drawPos.z + mhzsize),
				float3(unit->drawPos.x - mhxsize, unit->drawPos.y, unit->drawPos.z - mhzsize),
				float3(unit->drawPos.x + mhxsize, unit->drawPos.y, unit->drawPos.z - mhzsize),
			};

			const unsigned char colors[2][4] = {
				{(0.0f + cmdColors.unitBox[0]) * 255, (0.0f + cmdColors.unitBox[1]) * 255, (0.0f + cmdColors.unitBox[2] * 255), cmdColors.unitBox[3] * 255},
				{(1.0f - cmdColors.unitBox[0]) * 255, (1.0f - cmdColors.unitBox[1]) * 255, (1.0f - cmdColors.unitBox[2] * 255), cmdColors.unitBox[3] * 255},
			};

			va->AddVertexQC(verts[0], colors[0]);
			va->AddVertexQC(verts[1], colors[0]);
			va->AddVertexQC(verts[2], colors[0]);
			va->AddVertexQC(verts[3], colors[0]);

			if (globalRendering->drawdebug && (mhxsize != uhxsize || mhzsize != uhzsize)) {
				va->AddVertexQC(verts[4], colors[1]);
				va->AddVertexQC(verts[5], colors[1]);
				va->AddVertexQC(verts[6], colors[1]);
				va->AddVertexQC(verts[7], colors[1]);
			}
		}

		va->DrawArrayC(GL_QUADS);
	}

	// highlight queued build sites if we are about to build something
	// (or old-style, whenever the shift key is being held down)
	if (cmdColors.buildBox[3] > 0.0f) {
		if (!selectedUnits.empty() &&
				((cmdColors.BuildBoxesOnShift() && keyInput->IsKeyPressed(SDLK_LSHIFT)) ||
				 ((guihandler->inCommand >= 0) &&
					(guihandler->inCommand < int(guihandler->commands.size())) &&
					(guihandler->commands[guihandler->inCommand].id < 0)))) {

			GML_STDMUTEX_LOCK(cai); // Draw

			bool myColor = true;
			glColor4fv(cmdColors.buildBox);
			std::list<CBuilderCAI*>::const_iterator bi;
			for (bi = uh->builderCAIs.begin(); bi != uh->builderCAIs.end(); ++bi) {
				CBuilderCAI* builder = *bi;
				if (builder->owner->team == gu->myTeam) {
					if (!myColor) {
						glColor4fv(cmdColors.buildBox);
						myColor = true;
					}
					builder->DrawQuedBuildingSquares();
				}
				else if (teamHandler->AlliedTeams(builder->owner->team, gu->myTeam)) {
					if (myColor) {
						glColor4fv(cmdColors.allyBuildBox);
						myColor = false;
					}
					builder->DrawQuedBuildingSquares();
				}
			}
		}
	}

	glLineWidth(1.0f);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(true);
	glEnable(GL_TEXTURE_2D);
}


void CSelectedUnits::DependentDied(CObject *o)
{
	GML_RECMUTEX_LOCK(sel); // DependentDied - maybe superfluous, too late anyway

	selectedUnits.erase((CUnit*)o);
	selectionChanged = true;
	possibleCommandsChanged = true;
}


void CSelectedUnits::NetSelect(std::vector<int>& s, int playerId)
{
	assert(unsigned(playerId) < netSelected.size());
	netSelected[playerId] = s;
}


void CSelectedUnits::NetOrder(Command& c, int playerId)
{
	assert(unsigned(playerId) < netSelected.size());
	selectedUnitsAI.GiveCommandNet(c, playerId);

	if (netSelected[playerId].size() > 0) {
		eoh->PlayerCommandGiven(netSelected[playerId], c, playerId);
	}
}

void CSelectedUnits::ClearNetSelect(int playerId)
{
	netSelected[playerId].clear();
}

void CSelectedUnits::AiOrder(int unitid, const Command &c, int playerId)
{
	CUnit* unit = uh->units[unitid];
	if (unit == NULL) {
		return;
	}

	const CPlayer* player = playerHandler->Player(playerId);
	if (player == NULL) {
		return;
	}
	if (!player->CanControlTeam(unit->team)) {
		// Outputting a warning will result in false bug reports due to lag
		// between time of giving valid orders on units which then change team
		// due to e.g. LuaRules.

		//logOutput.Print("Invalid order from player %i for (unit %i %s, team %i)",
		//                playerId, unitid, unit->unitDefName.c_str(), unit->team);
		return;
	}

	unit->commandAI->GiveCommand(c, false);
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


// CALLINFO:
// DrawMapStuff --> CGuiHandler::GetDefaultCommand --> GetDefaultCmd
// CMouseHandler::DrawCursor --> DrawCentroidCursor --> CGuiHandler::GetDefaultCommand --> GetDefaultCmd
// LuaUnsyncedRead::GetDefaultCommand --> CGuiHandler::GetDefaultCommand --> GetDefaultCmd
int CSelectedUnits::GetDefaultCmd(const CUnit* unit, const CFeature* feature)
{
	GML_RECMUTEX_LOCK(sel); // GetDefaultCmd

	int luaCmd;
	if (eventHandler.DefaultCommand(unit, feature, luaCmd)) {
		return luaCmd;
	}

	// return the default if there are no units selected
	CUnitSet::const_iterator ui = selectedUnits.begin();
	if (ui == selectedUnits.end()) {
		return CMD_STOP;
	}

	// setup the locals for IsBetterLeader()
	targetUnit = unit;
	targetFeature = feature;
	if (targetUnit) {
		targetIsEnemy = !teamHandler->Ally(gu->myAllyTeam, targetUnit->allyteam);
	}

	// find the best leader to pick the command
	const CUnit* leaderUnit = *ui;
	const UnitDef* leaderDef = leaderUnit->unitDef;
	for (++ui; ui != selectedUnits.end(); ++ui) {
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

void CSelectedUnits::PossibleCommandChange(CUnit* sender)
{
	GML_RECMUTEX_LOCK(sel); // PossibleCommandChange

	if (sender == NULL || selectedUnits.find(sender) != selectedUnits.end())
		possibleCommandsChanged = true;
}

// CALLINFO:
// CGame::Draw --> DrawCommands
// CMiniMap::DrawForReal --> DrawCommands
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

	GML_RECMUTEX_LOCK(unit); // DrawCommands
	GML_RECMUTEX_LOCK(grpsel); // DrawCommands
	GML_STDMUTEX_LOCK(cai); // DrawCommands

	CUnitSet::iterator ui;
	if (selectedGroup != -1) {
		CUnitSet& groupUnits = grouphandlers[gu->myTeam]->groups[selectedGroup]->units;
		for(ui = groupUnits.begin(); ui != groupUnits.end(); ++ui) {
			(*ui)->commandAI->DrawCommands();
		}
	} else {
		for(ui = selectedUnits.begin(); ui != selectedUnits.end(); ++ui) {
			(*ui)->commandAI->DrawCommands();
		}
	}

	// draw the commands from AIs
	grouphandlers[gu->myTeam]->DrawCommands();
	waitCommandsAI.DrawCommands();

	glLineWidth(1.0f);

	glEnable(GL_DEPTH_TEST);
}


// CALLINFO:
// CTooltipConsole::Draw --> CMouseHandler::GetCurrentTooltip
// LuaUnsyncedRead::GetCurrentTooltip --> CMouseHandler::GetCurrentTooltip
// CMouseHandler::GetCurrentTooltip --> CMiniMap::GetToolTip --> GetTooltip
// CMouseHandler::GetCurrentTooltip --> GetTooltip
std::string CSelectedUnits::GetTooltip()
{
	GML_RECMUTEX_LOCK(sel); // GetTooltip - called from TooltipConsole::Draw --> MouseHandler::GetCurrentTooltip --> GetTooltip

	std::string s = "";
	if (!selectedUnits.empty()) {
		const CUnit* unit = (*selectedUnits.begin());
		const CTeam* team = NULL;

		// show the player name instead of unit name if it has FBI tag showPlayerName
		if (unit->unitDef->showPlayerName) {
			team = teamHandler->Team(unit->team);
			s = team->GetControllerName();
		} else {
			s = (*selectedUnits.begin())->tooltip;
		}

	}

	if (selectedUnits.empty()) {
		return s;
	}

	const string custom = eventHandler.WorldTooltip(NULL, NULL, NULL);
	if (!custom.empty()) {
		return custom;
	}

	char tmp[500];
	int numFuel = 0;
	float maxHealth = 0.0f, curHealth = 0.0f;
	float maxFuel = 0.0f, curFuel = 0.0f;
	float exp = 0.0f, cost = 0.0f, range = 0.0f;
	float metalMake = 0.0f, metalUse = 0.0f, energyMake = 0.0f, energyUse = 0.0f;

#define NO_TEAM -32
#define MULTI_TEAM -64
	int ctrlTeam = NO_TEAM;

	CUnitSet::const_iterator ui;
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
		if (ctrlTeam == NO_TEAM) {
			ctrlTeam = unit->team;
		} else if (ctrlTeam != unit->team) {
			ctrlTeam = MULTI_TEAM;
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

	if (gs->cheatEnabled && (num == 1)) {
		const CUnit* unit = *selectedUnits.begin();
		SNPRINTF(tmp, sizeof(tmp), "\xff\xc0\xc0\xff  [TechLevel %i]",
				unit->unitDef->techLevel);
		s += tmp;
	}

	std::string ctrlName = "";
	if (ctrlTeam == MULTI_TEAM) {
		ctrlName = "(Multiple teams)";
	} else if (ctrlTeam != NO_TEAM) {
		ctrlName = teamHandler->Team(ctrlTeam)->GetControllerName();
	}
	s += "\n\xff\xff\xff\xff" + ctrlName;

	return s;
}


void CSelectedUnits::SetCommandPage(int page)
{
	GML_RECMUTEX_LOCK(sel); // SetCommandPage - called from CGame::Draw --> RunLayoutCommand --> LayoutIcons --> RevertToCmdDesc

	CUnitSet::iterator ui;
	for (ui = selectedUnits.begin(); ui != selectedUnits.end(); ++ui) {
		(*ui)->commandAI->lastSelectedCommandPage = page;
	}
}


void CSelectedUnits::SendSelection()
{
	GML_RECMUTEX_LOCK(sel); // SendSelection

	// first, convert CUnit* to unit IDs.
	std::vector<short> selectedUnitIDs(selectedUnits.size());
	std::vector<short>::iterator i = selectedUnitIDs.begin();
	CUnitSet::const_iterator ui = selectedUnits.begin();
	for(; ui != selectedUnits.end(); ++i, ++ui) {
		*i = (*ui)->id;
	}
	net->Send(CBaseNetProtocol::Get().SendSelect(gu->myPlayerNum, selectedUnitIDs));
	selectionChanged = false;
}


void CSelectedUnits::SendCommand(const Command& c)
{
	if (selectionChanged) {
		// send new selection
		SendSelection();
	}
	net->Send(CBaseNetProtocol::Get().SendCommand(gu->myPlayerNum, c.id, c.options, c.params));
}


void CSelectedUnits::SendCommandsToUnits(const std::vector<int>& unitIDs, const std::vector<Command>& commands)
{
	// NOTE: does not check for invalid unitIDs

	if (gu->spectating && !gs->godMode) {
		return; // do not waste bandwidth
	}

	const unsigned unitIDCount  = unitIDs.size();
	const unsigned commandCount = commands.size();

	if ((unitIDCount == 0) || (commandCount == 0)) {
		return;
	}

	unsigned totalParams = 0;
	for (unsigned c = 0; c < commandCount; c++) {
		totalParams += commands[c].params.size();
	}

	unsigned msgLen = 0;
	msgLen += (1 + 2 + 1); // msg type, msg size, player ID
	msgLen += 2; // unitID count
	msgLen += unitIDCount * 2;
	msgLen += 2; // command count
	msgLen += commandCount * (4 + 1 + 2); // id, options, params size
	msgLen += totalParams * 4;
	if (msgLen > 8192) {
		logOutput.Print("Discarded oversized NETMSG_AICOMMANDS packet: %i\n",
		                msgLen);
		return; // drop the oversized packet
	}
	netcode::PackPacket* packet = new netcode::PackPacket(msgLen);
	*packet << static_cast<unsigned char>(NETMSG_AICOMMANDS)
	        << static_cast<unsigned short>(msgLen)
	        << static_cast<unsigned char>(gu->myPlayerNum);

	*packet << static_cast<unsigned short>(unitIDCount);
	for (std::vector<int>::const_iterator it = unitIDs.begin(); it != unitIDs.end(); ++it)
	{
		*packet << static_cast<short>(*it);
	}

	*packet << static_cast<unsigned short>(commandCount);
	for (unsigned i = 0; i < commandCount; ++i) {
		const Command& cmd = commands[i];
		*packet << static_cast<unsigned int>(cmd.id)
		        << cmd.options
		        << static_cast<unsigned short>(cmd.params.size()) << cmd.params;
	}

	net->Send(boost::shared_ptr<netcode::RawPacket>(packet));
	return;
}
