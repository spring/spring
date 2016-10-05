/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SelectedUnitsHandler.h"
#include "SelectedUnitsAI.h"
#include "Camera.h"
#include "GlobalUnsynced.h"
#include "WaitCommandsAI.h"
#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"
#include "UI/CommandColors.h"
#include "UI/GuiHandler.h"
#include "UI/TooltipConsole.h"
#include "ExternalAI/EngineOutHandler.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include "Rendering/CommandDrawer.h"
#include "Rendering/LineDrawer.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/Features/Feature.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/CommandAI/BuilderCAI.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Game/UI/Groups/GroupHandler.h"
#include "Game/UI/Groups/Group.h"
#include "System/Config/ConfigHandler.h"
#include "System/Color.h"
#include "System/EventHandler.h"
#include "System/Log/ILog.h"
#include "System/Util.h"
#include "Net/Protocol/NetProtocol.h"
#include "System/Net/PackPacket.h"
#include "System/FileSystem/SimpleParser.h"
#include "System/Input/KeyInput.h"
#include "System/Sound/ISound.h"
#include "System/Sound/ISoundChannels.h"

#include <SDL_mouse.h>
#include <SDL_keycode.h>
#include <map>


CONFIG(bool, BuildIconsFirst).defaultValue(false);
CONFIG(bool, AutoAddBuiltUnitsToFactoryGroup).defaultValue(false).description("Controls whether or not units built by factories will inherit that factory's unit group.");
CONFIG(bool, AutoAddBuiltUnitsToSelectedGroup).defaultValue(false);

CSelectedUnitsHandler selectedUnitsHandler;


CSelectedUnitsHandler::CSelectedUnitsHandler()
	: selectionChanged(false)
	, possibleCommandsChanged(true)
	, selectedGroup(-1)
	, soundMultiselID(0)
	, autoAddBuiltUnitsToFactoryGroup(false)
	, autoAddBuiltUnitsToSelectedGroup(false)
	, buildIconsFirst(false)
{
}



void CSelectedUnitsHandler::Init(unsigned numPlayers)
{
	soundMultiselID = sound->GetSoundId("MultiSelect");
	buildIconsFirst = configHandler->GetBool("BuildIconsFirst");
	autoAddBuiltUnitsToFactoryGroup = configHandler->GetBool("AutoAddBuiltUnitsToFactoryGroup");
	autoAddBuiltUnitsToSelectedGroup = configHandler->GetBool("AutoAddBuiltUnitsToSelectedGroup");
	netSelected.resize(numPlayers);
}


bool CSelectedUnitsHandler::IsUnitSelected(const CUnit* unit) const
{
	return (selectedUnits.find(const_cast<CUnit*>(unit)) != selectedUnits.end());
}

bool CSelectedUnitsHandler::IsUnitSelected(const int unitID) const
{
	const CUnit* u = unitHandler->GetUnit(unitID);
	return (u != NULL && IsUnitSelected(u));
}


void CSelectedUnitsHandler::ToggleBuildIconsFirst()
{
	buildIconsFirst = !buildIconsFirst;
	possibleCommandsChanged = true;
}


CSelectedUnitsHandler::AvailableCommandsStruct CSelectedUnitsHandler::GetAvailableCommands()
{
	possibleCommandsChanged = false;

	int commandPage = 1000;
	int foundGroup = -2;
	int foundGroup2 = -2;
	std::map<int, int> states;

	for (const CUnit* u: selectedUnits) {
		for (const SCommandDescription* cmdDesc: u->commandAI->GetPossibleCommands()) {
			states[cmdDesc->id] = cmdDesc->disabled ? 2 : 1;
		}
		if (u->commandAI->lastSelectedCommandPage < commandPage) {
			commandPage = u->commandAI->lastSelectedCommandPage;
		}

		if (foundGroup == -2 && u->group) {
			foundGroup = u->group->id;
		}
		if (!u->group || foundGroup != u->group->id) {
			foundGroup = -1;
		}

		if (foundGroup2 == -2 && u->group) {
			foundGroup2 = u->group->id;
		}
		if (foundGroup2 >= 0 && u->group && u->group->id != foundGroup2) {
			foundGroup2 = -1;
		}
	}

	std::vector<SCommandDescription> commands;
	// load the first set (separating build and non-build commands)
	for (const CUnit* u: selectedUnits) {
		for (const SCommandDescription* cmdDesc: u->commandAI->GetPossibleCommands()) {
			if (buildIconsFirst) {
				if (cmdDesc->id >= 0) { continue; }
			} else {
				if (cmdDesc->id < 0)  { continue; }
			}
			if (cmdDesc->showUnique && selectedUnits.size() > 1) {
				continue;
			}
			if (states[cmdDesc->id] > 0) {
				commands.push_back(*cmdDesc);
				states[cmdDesc->id] = 0;
			}
		}
	}

	// load the second set (all those that have not already been included)
	for (const CUnit* u: selectedUnits) {
		for (const SCommandDescription* cmdDesc: u->commandAI->GetPossibleCommands()) {
			if (buildIconsFirst) {
				if (cmdDesc->id < 0)  { continue; }
			} else {
				if (cmdDesc->id >= 0) { continue; }
			}
			if (cmdDesc->showUnique && selectedUnits.size() > 1) {
				continue;
			}
			if (states[cmdDesc->id] > 0) {
				commands.push_back(*cmdDesc);
				states[cmdDesc->id] = 0;
			}
		}
	}

	AvailableCommandsStruct ac;
	ac.commandPage = commandPage;
	ac.commands = commands;
	return ac;
}


void CSelectedUnitsHandler::GiveCommand(Command c, bool fromUser)
{
	if (gu->spectating && !gs->godMode)
		return;
	if (selectedUnits.empty())
		return;

	const int cmd_id = c.GetID();

	if (fromUser) { // add some statistics
		playerHandler->Player(gu->myPlayerNum)->currentStats.numCommands++;
		if (selectedGroup != -1) {
			playerHandler->Player(gu->myPlayerNum)->currentStats.unitCommands += grouphandlers[gu->myTeam]->groups[selectedGroup]->units.size();
		} else {
			playerHandler->Player(gu->myPlayerNum)->currentStats.unitCommands += selectedUnits.size();
		}
	}

	if (cmd_id == CMD_GROUPCLEAR) {
		for (CUnitSet::iterator ui = selectedUnits.begin(); ui != selectedUnits.end(); ++ui) {
			if ((*ui)->group) {
				(*ui)->SetGroup(0);
				possibleCommandsChanged = true;
			}
		}
		return;
	}
	else if (cmd_id == CMD_GROUPSELECT) {
		SelectGroup((*selectedUnits.begin())->group->id);
		return;
	}
	else if (cmd_id == CMD_GROUPADD) {
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
	else if (cmd_id == CMD_TIMEWAIT) {
		waitCommandsAI.AddTimeWait(c);
		return;
	}
	else if (cmd_id == CMD_DEATHWAIT) {
		waitCommandsAI.AddDeathWait(c);
		return;
	}
	else if (cmd_id == CMD_SQUADWAIT) {
		waitCommandsAI.AddSquadWait(c);
		return;
	}
	else if (cmd_id == CMD_GATHERWAIT) {
		waitCommandsAI.AddGatherWait(c);
		return;
	}

	SendCommand(c);

	if (!selectedUnits.empty()) {
		CUnitSet::const_iterator ui = selectedUnits.begin();
		Channels::UnitReply->PlayRandomSample((*ui)->unitDef->sounds.ok, *ui);
	}
}


void CSelectedUnitsHandler::HandleUnitBoxSelection(const float4& planeRight, const float4& planeLeft, const float4& planeTop, const float4& planeBottom)
{
	CUnit* unit = NULL;
	int addedunits = 0;
	int team, lastTeam;

	if (gu->spectatingFullSelect || gs->godMode) {
		// any team's units can be *selected*
		// (whether they can be given orders
		// depends on our ability to play god)
		team = 0;
		lastTeam = teamHandler->ActiveTeams() - 1;
	} else {
		team = gu->myTeam;
		lastTeam = gu->myTeam;
	}
	for (; team <= lastTeam; team++) {
		for (CUnit* u: teamHandler->Team(team)->units) {
			const float4 vec(u->midPos, 1.0f);

			if (vec.dot4(planeRight) < 0.0f && vec.dot4(planeLeft) < 0.0f && vec.dot4(planeTop) < 0.0f && vec.dot4(planeBottom) < 0.0f) {
				if (KeyInput::GetKeyModState(KMOD_CTRL) && (selectedUnits.find(u) != selectedUnits.end())) {
					RemoveUnit(u);
				} else {
					AddUnit(u);
					unit = u;
					addedunits++;
				}
			}
		}
	}

	if (addedunits >= 2) {
		Channels::UserInterface->PlaySample(soundMultiselID);
	}
	else if (addedunits == 1) {
		Channels::UnitReply->PlayRandomSample(unit->unitDef->sounds.select, unit);
	}
}


void CSelectedUnitsHandler::HandleSingleUnitClickSelection(CUnit* unit, bool doInViewTest, bool selectType)
{
	//FIXME make modular?

	if (unit == NULL)
		return;
	if (unit->team != gu->myTeam && !gu->spectatingFullSelect && !gs->godMode)
		return;

	if (!selectType) {
		if (KeyInput::GetKeyModState(KMOD_CTRL) && (selectedUnits.find(unit) != selectedUnits.end())) {
			RemoveUnit(unit);
		} else {
			AddUnit(unit);
		}
	} else {
		//double click, select all units of same type (on screen, unless CTRL is pressed)
		int team, lastTeam;

		if (gu->spectatingFullSelect || gs->godMode) {
			team = 0;
			lastTeam = teamHandler->ActiveTeams() - 1;
		} else {
			team = gu->myTeam;
			lastTeam = gu->myTeam;
		}
		for (; team <= lastTeam; team++) {
			for (CUnit* u: teamHandler->Team(team)->units) {
				if (u->unitDef->id == unit->unitDef->id) {
					if (!doInViewTest || KeyInput::GetKeyModState(KMOD_CTRL) || camera->InView((u)->midPos)) {
						AddUnit(u);
					}
				}
			}
		}
	}

	Channels::UnitReply->PlayRandomSample(unit->unitDef->sounds.select, unit);
}



void CSelectedUnitsHandler::AddUnit(CUnit* unit)
{
	// if unit is being transported by eg. Hulk or Atlas
	// then we should not be able to select it
	const CUnit* trans = unit->GetTransporter();
	if (trans != NULL && trans->unitDef->IsTransportUnit() && !trans->unitDef->isFirePlatform) {
		return;
	}

	if (unit->noSelect) {
		return;
	}

	if (selectedUnits.insert(unit).second)
		AddDeathDependence(unit, DEPENDENCE_SELECTED);
	selectionChanged = true;
	possibleCommandsChanged = true;

	if (!(unit->group) || unit->group->id != selectedGroup) {
		selectedGroup = -1;
	}

	unit->isSelected = true;
}


void CSelectedUnitsHandler::RemoveUnit(CUnit* unit)
{
	if (selectedUnits.erase(unit))
		DeleteDeathDependence(unit, DEPENDENCE_SELECTED);
	selectionChanged = true;
	possibleCommandsChanged = true;
	selectedGroup = -1;
	unit->isSelected = false;
}


void CSelectedUnitsHandler::ClearSelected()
{
	for (CUnit* u: selectedUnits) {
		u->isSelected = false;
		DeleteDeathDependence(u, DEPENDENCE_SELECTED);
	}

	selectedUnits.clear();
	selectionChanged = true;
	possibleCommandsChanged = true;
	selectedGroup = -1;
}


void CSelectedUnitsHandler::SelectGroup(int num)
{
	ClearSelected();
	selectedGroup=num;
	CGroup* group=grouphandlers[gu->myTeam]->groups[num];

	for (CUnit* u: group->units) {
		if (!u->noSelect) {
			u->isSelected = true;
			selectedUnits.insert(u);
			AddDeathDependence(u, DEPENDENCE_SELECTED);
		}
	}

	selectionChanged = true;
	possibleCommandsChanged = true;
}


void CSelectedUnitsHandler::SelectUnits(const std::string& line)
{
	const std::vector<string>& args = CSimpleParser::Tokenize(line, 0);
	for (int i = 0; i < (int)args.size(); i++) {
		const std::string& arg = args[i];
		if (arg == "clear") {
			selectedUnitsHandler.ClearSelected();
		} else if ((arg[0] == '+') || (arg[0] == '-')) {
			char* endPtr;
			const char* startPtr = arg.c_str() + 1;
			const int unitIndex = strtol(startPtr, &endPtr, 10);
			if (endPtr == startPtr)
				continue; // bad number

			if ((unitIndex < 0) || (static_cast<unsigned int>(unitIndex) >= unitHandler->MaxUnits()))
				continue; // bad index

			CUnit* unit = unitHandler->units[unitIndex];
			if (unit == nullptr)
				continue;

			if (!gu->spectatingFullSelect && (unit->team != gu->myTeam))
				continue; // not mine to select

			// perform the selection
			if (arg[0] == '+') {
				AddUnit(unit);
			} else {
				RemoveUnit(unit);
			}
		}
	}
}


void CSelectedUnitsHandler::SelectCycle(const std::string& command)
{
	static std::set<int> unitIDs;
	static int lastID = -1;

	if (command == "restore") {
		ClearSelected();
		for (const int& unitID: unitIDs) {
			CUnit* unit = unitHandler->units[unitID];
			if (unit != NULL) {
				AddUnit(unit);
			}
		}
		return;
	}

	if (selectedUnits.size() >= 2) {
		// assign the cycle units
		unitIDs.clear();
		for (const CUnit* u: selectedUnits) {
			unitIDs.insert(u->id);
		}
		ClearSelected();
		lastID = *unitIDs.begin();
		AddUnit(unitHandler->units[lastID]);
		return;
	}

	// clean the list
	std::set<int> tmpSet;
	for (const int& unitID: unitIDs) {
		if (unitHandler->units[unitID] != NULL) {
			tmpSet.insert(unitID);
		}
	}
	unitIDs = tmpSet;
	if ((lastID >= 0) && (unitHandler->units[lastID] == NULL)) {
		lastID = -1;
	}

	// selectedUnits size is 0 or 1
	ClearSelected();
	if (!unitIDs.empty()) {
		std::set<int>::const_iterator fit = unitIDs.find(lastID);
		if (fit == unitIDs.end()) {
			lastID = *unitIDs.begin();
		} else {
			++fit;
			if (fit != unitIDs.end()) {
				lastID = *fit;
			} else {
				lastID = *unitIDs.begin();
			}
		}
		AddUnit(unitHandler->units[lastID]);
	}
}


void CSelectedUnitsHandler::Draw()
{
	SCOPED_GMARKER("CSelectedUnitsHandler::Draw");

	glDisable(GL_TEXTURE_2D);
	glDepthMask(false);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND); // for line smoothing
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glLineWidth(cmdColors.UnitBoxLineWidth());

	SColor color1(cmdColors.unitBox);
	SColor color2(cmdColors.unitBox);
	color2.r = 255 - color2.r;
	color2.g = 255 - color2.g;
	color2.b = 255 - color2.b;

	if (color1.a > 0) {
		const CUnitSet* unitSet;
		if (selectedGroup != -1) {
			// note: units in this set are not necessarily all selected themselves, eg.
			// if autoAddBuiltUnitsToSelectedGroup is true, so we check IsUnitSelected
			// for each
			unitSet = &grouphandlers[gu->myTeam]->groups[selectedGroup]->units;
		} else {
			unitSet = &selectedUnits;
		}

		CVertexArray* va = GetVertexArray();
		va->Initialize();
		va->EnlargeArrays(unitSet->size() * 8, 0, VA_SIZE_C);

		for (CUnitSet::const_iterator ui = unitSet->begin(); ui != unitSet->end(); ++ui) {
			const CUnit* unit = *ui;
			const MoveDef* moveDef = unit->moveDef;

			if (unit->isIcon) continue;
			if (!IsUnitSelected(unit)) continue;

			const int
				uhxsize = (unit->xsize * SQUARE_SIZE) >> 1,
				uhzsize = (unit->zsize * SQUARE_SIZE) >> 1,
				mhxsize = (moveDef == NULL)? uhxsize: ((moveDef->xsize * SQUARE_SIZE) >> 1),
				mhzsize = (moveDef == NULL)? uhzsize: ((moveDef->zsize * SQUARE_SIZE) >> 1);
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

			va->AddVertexQC(verts[0], color1);
			va->AddVertexQC(verts[1], color1);
			va->AddVertexQC(verts[2], color1);
			va->AddVertexQC(verts[3], color1);

			if (globalRendering->drawdebug && (mhxsize != uhxsize || mhzsize != uhzsize)) {
				va->AddVertexQC(verts[4], color2);
				va->AddVertexQC(verts[5], color2);
				va->AddVertexQC(verts[6], color2);
				va->AddVertexQC(verts[7], color2);
			}
		}

		va->DrawArrayC(GL_QUADS);
	}

	// highlight queued build sites if we are about to build something
	// (or old-style, whenever the shift key is being held down)
	if (cmdColors.buildBox[3] > 0.0f) {
		if (!selectedUnits.empty() &&
				((cmdColors.BuildBoxesOnShift() && KeyInput::GetKeyModState(KMOD_SHIFT)) ||
				 ((guihandler->inCommand >= 0) &&
					(guihandler->inCommand < int(guihandler->commands.size())) &&
					(guihandler->commands[guihandler->inCommand].id < 0)))) {

			bool myColor = true;
			glColor4fv(cmdColors.buildBox);

			for (const auto bi: unitHandler->GetBuilderCAIs()) {
				const CBuilderCAI* builderCAI = bi.second;
				const CUnit* builder = builderCAI->owner;

				if (builder->team == gu->myTeam) {
					if (!myColor) {
						glColor4fv(cmdColors.buildBox);
						myColor = true;
					}
					commandDrawer->DrawQuedBuildingSquares(builderCAI);
				}
				else if (teamHandler->AlliedTeams(builder->team, gu->myTeam)) {
					if (myColor) {
						glColor4fv(cmdColors.allyBuildBox);
						myColor = false;
					}
					commandDrawer->DrawQuedBuildingSquares(builderCAI);
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


void CSelectedUnitsHandler::DependentDied(CObject *o)
{
	selectedUnits.erase(static_cast<CUnit*>(o));
	selectionChanged = true;
	possibleCommandsChanged = true;
}


void CSelectedUnitsHandler::NetSelect(std::vector<int>& s, int playerId)
{
	assert(unsigned(playerId) < netSelected.size());
	netSelected[playerId] = s;
}


void CSelectedUnitsHandler::NetOrder(Command& c, int playerId)
{
	assert(unsigned(playerId) < netSelected.size());
	selectedUnitsAI.GiveCommandNet(c, playerId);

	if (netSelected[playerId].size() > 0) {
		eoh->PlayerCommandGiven(netSelected[playerId], c, playerId);
	}
}

void CSelectedUnitsHandler::ClearNetSelect(int playerId)
{
	netSelected[playerId].clear();
}

void CSelectedUnitsHandler::AiOrder(int unitid, const Command &c, int playerId)
{
	CUnit* unit = unitHandler->units[unitid];
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

		//LOG_L(L_WARNING, "Invalid order from player %i for (unit %i %s, team %i)",
		//		playerId, unitid, unit->unitDefName.c_str(), unit->team);
		return;
	}

	unit->commandAI->GiveCommand(c, false);
}


bool CSelectedUnitsHandler::CommandsChanged()
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
		if (targetFeature->udef != NULL) {
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
int CSelectedUnitsHandler::GetDefaultCmd(const CUnit* unit, const CFeature* feature)
{
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

void CSelectedUnitsHandler::PossibleCommandChange(CUnit* sender)
{
	if (sender == NULL || selectedUnits.find(sender) != selectedUnits.end())
		possibleCommandsChanged = true;
}

// CALLINFO:
// CGame::Draw --> DrawCommands
// CMiniMap::DrawForReal --> DrawCommands
void CSelectedUnitsHandler::DrawCommands()
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

	if (selectedGroup != -1) {
		CUnitSet& groupUnits = grouphandlers[gu->myTeam]->groups[selectedGroup]->units;
		for(CUnit* u: groupUnits) {
			commandDrawer->Draw(u->commandAI);
		}
	} else {
		for(CUnit* u: selectedUnits) {
			commandDrawer->Draw(u->commandAI);
		}
	}

	// draw the commands from AIs
	waitCommandsAI.DrawCommands();

	glLineWidth(1.0f);

	glEnable(GL_DEPTH_TEST);
}


// CALLINFO:
// CTooltipConsole::Draw --> CMouseHandler::GetCurrentTooltip
// LuaUnsyncedRead::GetCurrentTooltip --> CMouseHandler::GetCurrentTooltip
// CMouseHandler::GetCurrentTooltip --> CMiniMap::GetToolTip --> GetTooltip
// CMouseHandler::GetCurrentTooltip --> GetTooltip
std::string CSelectedUnitsHandler::GetTooltip()
{
	std::string s = "";
	{
		if (!selectedUnits.empty()) {
			const CUnit* unit = (*selectedUnits.begin());
			const CTeam* team = NULL;

			// show the player name instead of unit name if it has FBI tag showPlayerName
			if (unit->unitDef->showPlayerName) {
				team = teamHandler->Team(unit->team);
				s = team->GetControllerName();
			} else {
				s = unit->tooltip;
			}
		}

		if (selectedUnits.empty()) {
			return s;
		}
	}

	const string custom = eventHandler.WorldTooltip(NULL, NULL, NULL);
	if (!custom.empty()) {
		return custom;
	}

	{
		#define NO_TEAM -32
		#define MULTI_TEAM -64
		int ctrlTeam = NO_TEAM;

		SUnitStats stats;

		for (const CUnit* unit: selectedUnits) {
			stats.AddUnit(unit, false);

			if (ctrlTeam == NO_TEAM) {
				ctrlTeam = unit->team;
			} else if (ctrlTeam != unit->team) {
				ctrlTeam = MULTI_TEAM;
			}
		}

		s += CTooltipConsole::MakeUnitStatsString(stats);

		const float num = selectedUnits.size();

		if (gs->cheatEnabled && (num == 1)) {
			const CUnit* unit = *selectedUnits.begin();
			char tmp[500];
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
}


void CSelectedUnitsHandler::SetCommandPage(int page)
{
	for (const CUnit* u: selectedUnits) {
		u->commandAI->lastSelectedCommandPage = page;
	}
}



void CSelectedUnitsHandler::SendCommand(const Command& c)
{
	if (selectionChanged) {
		// send new selection

		// first, convert CUnit* to unit IDs.
		std::vector<short> selectedUnitIDs(selectedUnits.size());
		std::vector<short>::iterator i = selectedUnitIDs.begin();
		CUnitSet::const_iterator ui = selectedUnits.begin();
		for(; ui != selectedUnits.end(); ++i, ++ui) {
			*i = (*ui)->id;
		}
		clientNet->Send(CBaseNetProtocol::Get().SendSelect(gu->myPlayerNum, selectedUnitIDs));
		selectionChanged = false;
	}

	clientNet->Send(CBaseNetProtocol::Get().SendCommand(gu->myPlayerNum, c.GetID(), c.options, c.params));
}


void CSelectedUnitsHandler::SendCommandsToUnits(const std::vector<int>& unitIDs, const std::vector<Command>& commands, bool pairwise)
{
	if (gu->spectating && !gs->godMode) {
		// do not waste bandwidth (units can be selected
		// by any spectator, but not given orders without
		// god-mode)
		// note: clients verify this every NETMSG_SELECT
		return;
	}

	const unsigned unitIDCount  = unitIDs.size();
	const unsigned commandCount = commands.size();

	if ((unitIDCount == 0) || (commandCount == 0)) {
		return;
	}

	unsigned totalParams = 0;
	int sameCmdID = commands[0].GetID();
	unsigned char sameCmdOpt = commands[0].options;
	int sameCmdParamSize = commands[0].params.size();
	for (unsigned c = 0; c < commandCount; c++) {
		totalParams += commands[c].params.size();
		if (sameCmdID != 0 && sameCmdID != commands[c].GetID())
			sameCmdID = 0;
		if (sameCmdOpt != 0xFF && sameCmdOpt != commands[c].options)
			sameCmdOpt = 0xFF;
		if (sameCmdParamSize != 0xFFFF && sameCmdParamSize != commands[c].params.size())
			sameCmdParamSize = 0xFFFF;
	}

	unsigned msgLen = 0;
	msgLen += (1 + 2 + 1 + 1 + 1 + 4 + 1 + 2); // msg type, msg size, player ID, AI ID, pairwise, sameCmdID, sameCmdOpt, sameCmdParamSize
	msgLen += 2; // unitID count
	msgLen += unitIDCount * 2;
	msgLen += 2; // command count
	int psize = ((sameCmdID == 0) ? 4 : 0) + ((sameCmdOpt == 0xFF) ? 1 : 0) + ((sameCmdParamSize == 0xFFFF) ? 2 : 0);
	msgLen += commandCount * psize; // id, options, params size
	msgLen += totalParams * 4;
	if (msgLen > 8192) {
		LOG_L(L_WARNING, "Discarded oversized NETMSG_AICOMMANDS packet: %i",
				msgLen);
		return; // drop the oversized packet
	}
	netcode::PackPacket* packet = new netcode::PackPacket(msgLen);
	*packet << static_cast<unsigned char>(NETMSG_AICOMMANDS)
	        << static_cast<unsigned short>(msgLen)
	        << static_cast<unsigned char>(gu->myPlayerNum)
	        << skirmishAIHandler.GetCurrentAIID()
	        << static_cast<unsigned char>(pairwise)
	        << static_cast<unsigned int>(sameCmdID)
	        << static_cast<unsigned char>(sameCmdOpt)
	        << static_cast<unsigned short>(sameCmdParamSize);

	// NOTE: does not check for invalid unitIDs
	*packet << static_cast<unsigned short>(unitIDCount);
	for (std::vector<int>::const_iterator it = unitIDs.begin(); it != unitIDs.end(); ++it) {
		*packet << static_cast<short>(*it);
	}

	*packet << static_cast<unsigned short>(commandCount);

	for (unsigned i = 0; i < commandCount; ++i) {
		const Command& cmd = commands[i];
		if (sameCmdID == 0)
			*packet << static_cast<unsigned int>(cmd.GetID());
		if (sameCmdOpt == 0xFF)
			*packet << cmd.options;
		if (sameCmdParamSize == 0xFFFF)
			*packet << static_cast<unsigned short>(cmd.params.size());
		*packet << cmd.params;
	}

	clientNet->Send(boost::shared_ptr<netcode::RawPacket>(packet));
	return;
}
