/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SelectedUnits.h"

#include "SelectedUnitsAI.h"
#include "Camera.h"
#include "GlobalUnsynced.h"
#include "WaitCommandsAI.h"
#include "Player.h"
#include "PlayerHandler.h"
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
#include "Sim/Units/Groups/GroupHandler.h"
#include "Sim/Units/Groups/Group.h"
#include "Sim/Units/UnitTypes/TransportUnit.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/Log/ILog.h"
#include "System/Util.h"
#include "System/NetProtocol.h"
#include "System/Net/PackPacket.h"
#include "System/Input/KeyInput.h"
#include "System/Sound/ISound.h"
#include "System/Sound/SoundChannels.h"

#include <SDL_mouse.h>
#include <SDL_keysym.h>
#include <map>


#define PLAY_SOUNDS 1

CONFIG(bool, BuildIconsFirst).defaultValue(false);
CONFIG(bool, AutoAddBuiltUnitsToFactoryGroup).defaultValue(false);
CONFIG(bool, AutoAddBuiltUnitsToSelectedGroup).defaultValue(false);

CSelectedUnits selectedUnits;


CSelectedUnits::CSelectedUnits()
	: selectionChanged(false)
	, possibleCommandsChanged(true)
	, selectedGroup(-1)
	, soundMultiselID(0)
	, autoAddBuiltUnitsToFactoryGroup(false)
	, autoAddBuiltUnitsToSelectedGroup(false)
	, buildIconsFirst(false)
{
}



void CSelectedUnits::Init(unsigned numPlayers)
{
	soundMultiselID = sound->GetSoundId("MultiSelect");
	buildIconsFirst = configHandler->GetBool("BuildIconsFirst");
	autoAddBuiltUnitsToFactoryGroup = configHandler->GetBool("AutoAddBuiltUnitsToFactoryGroup");
	autoAddBuiltUnitsToSelectedGroup = configHandler->GetBool("AutoAddBuiltUnitsToSelectedGroup");
	netSelected.resize(numPlayers);
}


bool CSelectedUnits::IsUnitSelected(const CUnit* unit) const
{
	return (selectedUnits.find(const_cast<CUnit*>(unit)) != selectedUnits.end());
}

bool CSelectedUnits::IsUnitSelected(const int unitID) const
{
	const CUnit* u = unitHandler->GetUnit(unitID);
	return (u != NULL && IsUnitSelected(u));
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

	#if (PLAY_SOUNDS == 1)
	if (!selectedUnits.empty()) {
		CUnitSet::const_iterator ui = selectedUnits.begin();
		Channels::UnitReply.PlayRandomSample((*ui)->unitDef->sounds.ok, *ui);
	}
	#endif
}


void CSelectedUnits::HandleUnitBoxSelection(const float4& planeRight, const float4& planeLeft, const float4& planeTop, const float4& planeBottom)
{
	GML_RECMUTEX_LOCK(sel); // SelectUnits

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
		CUnitSet& teamUnits = teamHandler->Team(team)->units;
		for (CUnitSet::iterator ui = teamUnits.begin(); ui != teamUnits.end(); ++ui) {
			const float4 vec((*ui)->midPos, 1.0f);

			if (vec.dot4(planeRight) < 0.0f && vec.dot4(planeLeft) < 0.0f && vec.dot4(planeTop) < 0.0f && vec.dot4(planeBottom) < 0.0f) {
				if (keyInput->IsKeyPressed(SDLK_LCTRL) && (selectedUnits.find(*ui) != selectedUnits.end())) {
					RemoveUnit(*ui);
				} else {
					AddUnit(*ui);
					unit = *ui;
					addedunits++;
				}
			}
		}
	}

	#if (PLAY_SOUNDS == 1)
	if (addedunits >= 2) {
		Channels::UserInterface.PlaySample(soundMultiselID);
	}
	else if (addedunits == 1) {
		Channels::UnitReply.PlayRandomSample(unit->unitDef->sounds.select, unit);
	}
	#endif
}


void CSelectedUnits::HandleSingleUnitClickSelection(CUnit* unit, bool doInViewTest)
{
	GML_RECMUTEX_LOCK(sel); // SelectUnits

	//FIXME make modular?
	const CMouseHandler::ButtonPressEvt& bp = mouse->buttons[SDL_BUTTON_LEFT];

	if (unit == NULL)
		return;
	if (unit->team != gu->myTeam && !gu->spectatingFullSelect && !gs->godMode)
		return;

	if (bp.lastRelease < (gu->gameTime - mouse->doubleClickTime)) {
		if (keyInput->IsKeyPressed(SDLK_LCTRL) && (selectedUnits.find(unit) != selectedUnits.end())) {
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
			CUnitSet::iterator ui;
			CUnitSet& teamUnits = teamHandler->Team(team)->units;
			for (ui = teamUnits.begin(); ui != teamUnits.end(); ++ui) {
				if ((*ui)->unitDef->id == unit->unitDef->id) {
					if (!doInViewTest || keyInput->IsKeyPressed(SDLK_LCTRL) || camera->InView((*ui)->midPos)) {
						AddUnit(*ui);
					}
				}
			}
		}
	}

	#if (PLAY_SOUNDS == 1)
	Channels::UnitReply.PlayRandomSample(unit->unitDef->sounds.select, unit);
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

	if (selectedUnits.insert(unit).second)
		AddDeathDependence(unit, DEPENDENCE_SELECTED);
	selectionChanged = true;
	possibleCommandsChanged = true;

	if (!(unit->group) || unit->group->id != selectedGroup) {
		selectedGroup = -1;
	}

	unit->isSelected = true;
}


void CSelectedUnits::RemoveUnit(CUnit* unit)
{
	GML_RECMUTEX_LOCK(sel); // RemoveUnit

	if (selectedUnits.erase(unit))
		DeleteDeathDependence(unit, DEPENDENCE_SELECTED);
	selectionChanged = true;
	possibleCommandsChanged = true;
	selectedGroup = -1;
	unit->isSelected = false;
}


void CSelectedUnits::ClearSelected()
{
	GML_RECMUTEX_LOCK(sel); // ClearSelected

	CUnitSet::iterator ui;
	for (ui = selectedUnits.begin(); ui != selectedUnits.end(); ++ui) {
		(*ui)->isSelected = false;
		DeleteDeathDependence(*ui, DEPENDENCE_SELECTED);
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
			(*ui)->isSelected = true;
			selectedUnits.insert(*ui);
			AddDeathDependence(*ui, DEPENDENCE_SELECTED);
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

			const unsigned char color1[4] = {
				(unsigned char)( cmdColors.unitBox[0] * 255 ),
				(unsigned char)( cmdColors.unitBox[1] * 255 ),
				(unsigned char)( cmdColors.unitBox[2] * 255 ),
				(unsigned char)( cmdColors.unitBox[3] * 255 )
			};

			va->AddVertexQC(verts[0], color1);
			va->AddVertexQC(verts[1], color1);
			va->AddVertexQC(verts[2], color1);
			va->AddVertexQC(verts[3], color1);

			if (globalRendering->drawdebug && (mhxsize != uhxsize || mhzsize != uhzsize)) {
				const unsigned char color2[4] = {
					(unsigned char)( (1.0f - cmdColors.unitBox[0]) * 255 ),
					(unsigned char)( (1.0f - cmdColors.unitBox[1]) * 255 ),
					(unsigned char)( (1.0f - cmdColors.unitBox[2]) * 255 ),
					(unsigned char)( cmdColors.unitBox[3] * 255 )
				};

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
				((cmdColors.BuildBoxesOnShift() && keyInput->IsKeyPressed(SDLK_LSHIFT)) ||
				 ((guihandler->inCommand >= 0) &&
					(guihandler->inCommand < int(guihandler->commands.size())) &&
					(guihandler->commands[guihandler->inCommand].id < 0)))) {

			GML_STDMUTEX_LOCK(cai); // Draw

			bool myColor = true;
			glColor4fv(cmdColors.buildBox);

			const std::map<unsigned int, CBuilderCAI*>& builderCAIs = unitHandler->builderCAIs;
			      std::map<unsigned int, CBuilderCAI*>::const_iterator bi;

			for (bi = builderCAIs.begin(); bi != builderCAIs.end(); ++bi) {
				const CBuilderCAI* builderCAI = bi->second;
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


void CSelectedUnits::DependentDied(CObject *o)
{
	GML_RECMUTEX_LOCK(sel); // DependentDied - maybe superfluous, too late anyway

	selectedUnits.erase(static_cast<CUnit*>(o));
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
int CSelectedUnits::GetDefaultCmd(const CUnit* unit, const CFeature* feature)
{
	int luaCmd;
	if (eventHandler.DefaultCommand(unit, feature, luaCmd)) {
		return luaCmd;
	}

	GML_RECMUTEX_LOCK(sel); // GetDefaultCmd

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
	GML_RECMUTEX_LOCK(feat); // DrawCommands
	GML_RECMUTEX_LOCK(grpsel); // DrawCommands
	GML_STDMUTEX_LOCK(cai); // DrawCommands

	CUnitSet::iterator ui;
	if (selectedGroup != -1) {
		CUnitSet& groupUnits = grouphandlers[gu->myTeam]->groups[selectedGroup]->units;
		for(ui = groupUnits.begin(); ui != groupUnits.end(); ++ui) {
			commandDrawer->Draw((*ui)->commandAI);
		}
	} else {
		for(ui = selectedUnits.begin(); ui != selectedUnits.end(); ++ui) {
			commandDrawer->Draw((*ui)->commandAI);
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
std::string CSelectedUnits::GetTooltip()
{
	std::string s = "";
	{
		GML_RECMUTEX_LOCK(sel); // GetTooltip - called from TooltipConsole::Draw --> MouseHandler::GetCurrentTooltip --> GetTooltip

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
		GML_RECMUTEX_LOCK(sel); // GetTooltip

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


void CSelectedUnits::SetCommandPage(int page)
{
	GML_RECMUTEX_LOCK(sel); // SetCommandPage - called from CGame::Draw --> RunLayoutCommand --> LayoutIcons --> RevertToCmdDesc

	CUnitSet::iterator ui;
	for (ui = selectedUnits.begin(); ui != selectedUnits.end(); ++ui) {
		(*ui)->commandAI->lastSelectedCommandPage = page;
	}
}



void CSelectedUnits::SendCommand(const Command& c)
{
	if (selectionChanged) {
		// send new selection
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

	net->Send(CBaseNetProtocol::Get().SendCommand(gu->myPlayerNum, c.GetID(), c.options, c.params));
}


void CSelectedUnits::SendCommandsToUnits(const std::vector<int>& unitIDs, const std::vector<Command>& commands, bool pairwise)
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

	net->Send(boost::shared_ptr<netcode::RawPacket>(packet));
	return;
}
