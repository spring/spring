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
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "Rendering/GL/WideLineAdapter.hpp"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/Features/Feature.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitToolTipMap.hpp"
#include "Sim/Units/CommandAI/BuilderCAI.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Game/UI/Groups/GroupHandler.h"
#include "Game/UI/Groups/Group.h"
#include "System/Config/ConfigHandler.h"
#include "System/Color.h"
#include "System/EventHandler.h"
#include "System/Log/ILog.h"
#include "System/StringUtil.h"
#include "Net/Protocol/NetProtocol.h"
#include "System/Net/PackPacket.h"
#include "System/FileSystem/SimpleParser.h"
#include "System/Input/KeyInput.h"
#include "System/Sound/ISound.h"
#include "System/Sound/ISoundChannels.h"

#include <SDL_mouse.h>
#include <SDL_keycode.h>



CONFIG(bool, BuildIconsFirst).defaultValue(false);
CONFIG(bool, AutoAddBuiltUnitsToFactoryGroup).defaultValue(false).description("Controls whether or not units built by factories will inherit that factory's unit group.");
CONFIG(bool, AutoAddBuiltUnitsToSelectedGroup).defaultValue(false);

CSelectedUnitsHandler selectedUnitsHandler;



void CSelectedUnitsHandler::Init(unsigned numPlayers)
{
	soundMultiselID = sound->GetDefSoundId("MultiSelect");
	buildIconsFirst = configHandler->GetBool("BuildIconsFirst");
	autoAddBuiltUnitsToFactoryGroup = configHandler->GetBool("AutoAddBuiltUnitsToFactoryGroup");
	autoAddBuiltUnitsToSelectedGroup = configHandler->GetBool("AutoAddBuiltUnitsToSelectedGroup");

	netSelected.resize(numPlayers);
}


bool CSelectedUnitsHandler::IsUnitSelected(const CUnit* unit) const
{
	return (unit != nullptr && selectedUnits.find(unit->id) != selectedUnits.end());
}

bool CSelectedUnitsHandler::IsUnitSelected(const int unitID) const
{
	return (IsUnitSelected(unitHandler.GetUnit(unitID)));
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

	spring::unordered_map<int, int> states;
	std::vector<SCommandDescription> commands;

	for (const int unitID: selectedUnits) {
		const CUnit* u = unitHandler.GetUnit(unitID);
		const CCommandAI* cai = u->commandAI;
		const CGroup* group = u->GetGroup();

		for (const SCommandDescription* cmdDesc: cai->GetPossibleCommands()) {
			states[cmdDesc->id] = cmdDesc->disabled ? 2 : 1;
		}

		if (cai->lastSelectedCommandPage < commandPage)
			commandPage = cai->lastSelectedCommandPage;

		if (foundGroup == -2 && group != nullptr)
			foundGroup = group->id;

		if (group == nullptr || foundGroup != group->id)
			foundGroup = -1;

		if (foundGroup2 == -2 && group != nullptr)
			foundGroup2 = group->id;

		if (foundGroup2 >= 0 && group != nullptr && group->id != foundGroup2)
			foundGroup2 = -1;
	}

	// load the first set (separating build and non-build commands)
	for (const int unitID: selectedUnits) {
		const CUnit* u = unitHandler.GetUnit(unitID);
		const CCommandAI* cai = u->commandAI;

		for (const SCommandDescription* cmdDesc: cai->GetPossibleCommands()) {
			if (buildIconsFirst) {
				if (cmdDesc->id >= 0)
					continue;
			} else {
				if (cmdDesc->id <  0)
					continue;
			}

			if (cmdDesc->showUnique && selectedUnits.size() > 1)
				continue;

			if (states[cmdDesc->id] > 0) {
				commands.push_back(*cmdDesc);
				states[cmdDesc->id] = 0;
			}
		}
	}

	// load the second set (all those that have not already been included)
	for (const int unitID: selectedUnits) {
		const CUnit* u = unitHandler.GetUnit(unitID);
		const CCommandAI* cai = u->commandAI;

		for (const SCommandDescription* cmdDesc: cai->GetPossibleCommands()) {
			if (buildIconsFirst) {
				if (cmdDesc->id < 0)
					continue;
			} else {
				if (cmdDesc->id >= 0)
					continue;
			}

			if (cmdDesc->showUnique && selectedUnits.size() > 1)
				continue;

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


void CSelectedUnitsHandler::GiveCommand(const Command& c, bool fromUser)
{
	if (gu->spectating && gs->godMode == 0)
		return;
	if (selectedUnits.empty())
		return;

	const int cmdID = c.GetID();

	if (fromUser) {
		// add some statistics
		CPlayer* myPlayer = playerHandler.Player(gu->myPlayerNum);
		PlayerStatistics* myPlayerStats = &myPlayer->currentStats;

		myPlayerStats->numCommands++;

		if (selectedGroup != -1) {
			myPlayerStats->unitCommands += uiGroupHandlers[gu->myTeam].GetGroupSize(selectedGroup);
		} else {
			myPlayerStats->unitCommands += selectedUnits.size();
		}
	}

	if (cmdID == CMD_GROUPCLEAR) {
		for (const int unitID: selectedUnits) {
			CUnit* u = unitHandler.GetUnit(unitID);

			if (u->GetGroup() != nullptr) {
				u->SetGroup(nullptr);
				possibleCommandsChanged = true;
			}
		}
		return;
	}

	if (cmdID == CMD_GROUPSELECT) {
		const CUnit* u = unitHandler.GetUnit(*selectedUnits.begin());
		const CGroup* g = u->GetGroup();

		SelectGroup(g->id);
		return;
	}

	if (cmdID == CMD_GROUPADD) {
		const CGroup* group = nullptr;

		for (const int unitID: selectedUnits) {
			const CUnit* u = unitHandler.GetUnit(unitID);
			const CGroup* g = u->GetGroup();

			if (g != nullptr) {
				group = g;
				possibleCommandsChanged = true;
				break;
			}
		}
		if (group != nullptr) {
			for (const int unitID: selectedUnits) {
				CUnit* u = unitHandler.GetUnit(unitID);
				CGroup* g = nullptr;

				if (u == nullptr) {
					assert(false);
					continue;
				}

				if ((g = u->GetGroup()) != nullptr)
					continue;

				// change group, but do not call SUH::AddUnit while iterating
				// (the unit's id is already present in selectedUnits anyway)
				u->SetGroup(const_cast<CGroup*>(group), false, false);
			}

			SelectGroup(group->id);
		}

		return;
	}

	if (cmdID == CMD_TIMEWAIT) {
		waitCommandsAI.AddTimeWait(c);
		return;
	}

	if (cmdID == CMD_DEATHWAIT) {
		waitCommandsAI.AddDeathWait(c);
		return;
	}

	if (cmdID == CMD_SQUADWAIT) {
		waitCommandsAI.AddSquadWait(c);
		return;
	}

	if (cmdID == CMD_GATHERWAIT) {
		waitCommandsAI.AddGatherWait(c);
		return;
	}

	SendCommand(c);

	if (!selectedUnits.empty()) {
		const CUnit* u = unitHandler.GetUnit(*selectedUnits.begin());
		const UnitDef* ud = u->unitDef;
		Channels::UnitReply->PlayRandomSample(ud->sounds.ok, u);
	}
}


void CSelectedUnitsHandler::HandleUnitBoxSelection(const float4& planeRight, const float4& planeLeft, const float4& planeTop, const float4& planeBottom)
{
	CUnit* unit = nullptr;
	const CPlayer* myPlayer = gu->GetMyPlayer();

	int numUnits = 0;
	int minTeam = gu->myTeam;
	int maxTeam = gu->myTeam;

	// any team's units can be *selected*; whether they can
	// be given orders depends on our ability to play god
	if (gu->spectatingFullSelect || gs->godMode != 0) {
		minTeam = 0;
		maxTeam = teamHandler.ActiveTeams() - 1;
	}

	for (int team = minTeam; team <= maxTeam; team++) {
		if (!gu->spectatingFullSelect && !myPlayer->CanControlTeam(team))
			continue;

		for (CUnit* u: unitHandler.GetUnitsByTeam(team)) {
			const float4 vec(u->midPos, 1.0f);

			if (vec.dot4(planeRight) >= 0.0f)
				continue;
			if (vec.dot4(planeLeft) >= 0.0f)
				continue;
			if (vec.dot4(planeTop) >= 0.0f)
				continue;
			if (vec.dot4(planeBottom) >= 0.0f)
				continue;

			if (KeyInput::GetKeyModState(KMOD_CTRL) && (selectedUnits.find(u->id) != selectedUnits.end())) {
				RemoveUnit(u);
				continue;
			}

			AddUnit(unit = u);
			numUnits++;
		}
	}

	switch (numUnits) {
		case 0: {
		} break;
		case 1: {
			Channels::UnitReply->PlayRandomSample(unit->unitDef->sounds.select, unit);
		} break;
		default: {
			Channels::UserInterface->PlaySample(soundMultiselID);
		} break;
	}
}


void CSelectedUnitsHandler::HandleSingleUnitClickSelection(CUnit* unit, bool doInViewTest, bool selectType)
{
	//FIXME make modular?
	if (unit == nullptr)
		return;
	if (unit->team != gu->myTeam && !gu->spectatingFullSelect && gs->godMode == 0)
		return;

	if (!selectType) {
		if (KeyInput::GetKeyModState(KMOD_CTRL) && (selectedUnits.find(unit->id) != selectedUnits.end())) {
			RemoveUnit(unit);
		} else {
			AddUnit(unit);
		}
	} else {
		const CPlayer* myPlayer = gu->GetMyPlayer();

		// double click, select all units of same type (on screen, unless CTRL is pressed)
		int minTeam = gu->myTeam;
		int maxTeam = gu->myTeam;

		if (gu->spectatingFullSelect || gs->godMode != 0) {
			minTeam = 0;
			maxTeam = teamHandler.ActiveTeams() - 1;
		}

		for (int team = minTeam; team <= maxTeam; team++) {
			if (!gu->spectatingFullSelect && !myPlayer->CanControlTeam(team))
				continue;

			for (CUnit* u: unitHandler.GetUnitsByTeam(team)) {
				if (u->unitDef->id != unit->unitDef->id)
					continue;

				if (!doInViewTest || KeyInput::GetKeyModState(KMOD_CTRL) || camera->InView((u)->midPos))
					AddUnit(u);
			}
		}
	}

	Channels::UnitReply->PlayRandomSample(unit->unitDef->sounds.select, unit);
}



void CSelectedUnitsHandler::AddUnit(CUnit* unit)
{
	// if unit is being transported, we should not be able to select it
	const CUnit* trans = unit->GetTransporter();

	if (trans != nullptr && trans->unitDef->IsTransportUnit() && !trans->unitDef->isFirePlatform)
		return;

	if (unit->noSelect)
		return;

	if (selectedUnits.insert(unit->id).second)
		AddDeathDependence(unit, DEPENDENCE_SELECTED);

	selectionChanged = true;
	possibleCommandsChanged = true;

	const CGroup* g = unit->GetGroup();

	if (g == nullptr || g->id != selectedGroup)
		selectedGroup = -1;

	unit->isSelected = true;
}


void CSelectedUnitsHandler::RemoveUnit(CUnit* unit)
{
	if (selectedUnits.erase(unit->id))
		DeleteDeathDependence(unit, DEPENDENCE_SELECTED);

	selectionChanged = true;
	possibleCommandsChanged = true;
	selectedGroup = -1;
	unit->isSelected = false;
}


void CSelectedUnitsHandler::ClearSelected()
{
	for (const int unitID: selectedUnits) {
		CUnit* u = unitHandler.GetUnit(unitID);

		// not possible unless ::RemoveUnit is not called when it should
		if (u == nullptr) {
			assert(false);
			continue;
		}

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
	selectedGroup = num;
	CGroup* group = uiGroupHandlers[gu->myTeam].GetGroup(num);

	for (const int unitID: group->units) {
		CUnit* u = unitHandler.GetUnit(unitID);

		if (!u->noSelect) {
			u->isSelected = true;
			selectedUnits.insert(u->id);
			AddDeathDependence(u, DEPENDENCE_SELECTED);
		}
	}

	selectionChanged = true;
	possibleCommandsChanged = true;
}


void CSelectedUnitsHandler::SelectUnits(const std::string& line)
{
	for (const std::string& arg : CSimpleParser::Tokenize(line, 0)) {
		if (arg == "clear") {
			selectedUnitsHandler.ClearSelected();
		} else if ((arg[0] == '+') || (arg[0] == '-')) {
			char* endPtr;
			const char* startPtr = arg.c_str() + 1;
			const int unitIndex = strtol(startPtr, &endPtr, 10);
			if (endPtr == startPtr)
				continue; // bad number

			if ((unitIndex < 0) || (static_cast<unsigned int>(unitIndex) >= unitHandler.MaxUnits()))
				continue; // bad index

			CUnit* unit = unitHandler.GetUnit(unitIndex);
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
	static spring::unordered_set<int> unitIDs;
	static int lastID = -1;

	if (command == "restore") {
		ClearSelected();

		for (const int unitID: unitIDs) {
			CUnit* unit = unitHandler.GetUnit(unitID);

			if (unit == nullptr)
				continue;

			AddUnit(unit);
		}

		return;
	}

	if (selectedUnits.size() >= 2) {
		// assign the cycle units
		unitIDs.clear();

		for (const int unitID: selectedUnits) {
			const CUnit* u = unitHandler.GetUnit(unitID);
			unitIDs.insert(u->id);
		}

		ClearSelected();
		AddUnit(unitHandler.GetUnit(lastID = *unitIDs.begin()));
		return;
	}

	// clean the list
	spring::unordered_set<int> tmpSet;
	for (const int unitID: unitIDs) {
		if (unitHandler.GetUnit(unitID) == nullptr)
			continue;
		tmpSet.insert(unitID);
	}

	unitIDs = std::move(tmpSet);

	if ((lastID >= 0) && (unitHandler.GetUnit(lastID) == nullptr))
		lastID = -1;

	// selectedUnits size is 0 or 1
	ClearSelected();

	if (unitIDs.empty())
		return;

	auto fit = unitIDs.find(lastID);

	if (fit == unitIDs.end()) {
		AddUnit(unitHandler.GetUnit(lastID = *unitIDs.begin()));
		return;
	}

	if ((++fit) != unitIDs.end()) {
		AddUnit(unitHandler.GetUnit(lastID = *fit));
		return;
	}

	AddUnit(unitHandler.GetUnit(lastID = *unitIDs.begin()));
}


void CSelectedUnitsHandler::Draw()
{
	glAttribStatePtr->DisableDepthMask();
	glAttribStatePtr->DisableDepthTest();
	glAttribStatePtr->EnableBlendMask(); // for line smoothing
	glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glAttribStatePtr->PolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	SColor udColor(cmdColors.unitBox);
	SColor mdColor(cmdColors.unitBox);

	mdColor.r = 255 - mdColor.r;
	mdColor.g = 255 - mdColor.g;
	mdColor.b = 255 - mdColor.b;

	GL::RenderDataBufferC* buffer = GL::GetRenderBufferC();
	Shader::IProgramObject* shader = buffer->GetShader();
	GL::WideLineAdapterC* wla = GL::GetWideLineAdapterC();

	shader->Enable();
	shader->SetUniformMatrix4x4<float>("u_movi_mat", false, camera->GetViewMatrix());
	shader->SetUniformMatrix4x4<float>("u_proj_mat", false, camera->GetProjectionMatrix());
	wla->Setup(buffer, globalRendering->viewSizeX, globalRendering->viewSizeY, cmdColors.UnitBoxLineWidth(), camera->GetViewProjectionMatrix());

	if (udColor.a > 0) {
		const auto* unitSet = &selectedUnits;

		// note: units in this set are not necessarily all selected themselves (eg.
		// if autoAddBuiltUnitsToSelectedGroup is true) so we check IsUnitSelected
		// for each
		if (selectedGroup != -1) {
			const CGroupHandler* gh = &uiGroupHandlers[gu->myTeam];
			const CGroup* g = gh->GetGroup(selectedGroup);

			unitSet = &g->units;
		}

		for (const int unitID: *unitSet) {
			const CUnit* unit = unitHandler.GetUnit(unitID);
			const MoveDef* moveDef = unit->moveDef;

			if (unit->isIcon)
				continue;
			if (!IsUnitSelected(unit))
				continue;

			const float3& drawPos = unit->drawPos;

			{
				const int uhxsize = (unit->xsize * SQUARE_SIZE) >> 1;
				const int uhzsize = (unit->zsize * SQUARE_SIZE) >> 1;

				// UnitDef footprint corners (rotated)
				const float3 udVerts[4] = {
					{drawPos.x + uhxsize * 1.0f, drawPos.y, drawPos.z + uhzsize * 1.0f},
					{drawPos.x - uhxsize * 1.0f, drawPos.y, drawPos.z + uhzsize * 1.0f},
					{drawPos.x - uhxsize * 1.0f, drawPos.y, drawPos.z - uhzsize * 1.0f},
					{drawPos.x + uhxsize * 1.0f, drawPos.y, drawPos.z - uhzsize * 1.0f},
				};

				wla->SafeAppend({udVerts[0], udColor});
				wla->SafeAppend({udVerts[1], udColor});
				wla->SafeAppend({udVerts[2], udColor});
				wla->SafeAppend({udVerts[3], udColor});
			}


			if (moveDef == nullptr)
				continue;
			if (!globalRendering->drawDebug)
				continue;

			{
				const int mhxsize = (moveDef->xsize * SQUARE_SIZE) >> 1;
				const int mhzsize = (moveDef->zsize * SQUARE_SIZE) >> 1;

				// MoveDef footprint corners
				const float3 mdVerts[4] = {
					{drawPos.x + mhxsize * 1.0f, drawPos.y, drawPos.z + mhzsize * 1.0f},
					{drawPos.x - mhxsize * 1.0f, drawPos.y, drawPos.z + mhzsize * 1.0f},
					{drawPos.x - mhxsize * 1.0f, drawPos.y, drawPos.z - mhzsize * 1.0f},
					{drawPos.x + mhxsize * 1.0f, drawPos.y, drawPos.z - mhzsize * 1.0f},
				};

				wla->SafeAppend({mdVerts[0], mdColor});
				wla->SafeAppend({mdVerts[1], mdColor});
				wla->SafeAppend({mdVerts[2], mdColor});
				wla->SafeAppend({mdVerts[3], mdColor});
			}
		}

		wla->Submit(GL_QUADS);
	}

	if (cmdColors.buildBox[3] > 0.0f) {
		const auto& guiCommands = guihandler->commands;

		const bool shiftPressed = (cmdColors.BuildBoxesOnShift() && KeyInput::GetKeyModState(KMOD_SHIFT));
		const bool buildQueued = (size_t(guihandler->inCommand) < guiCommands.size() && guiCommands[guihandler->inCommand].id < 0);

		// draw queued build sites if a GUI build-icon has been
		// clicked or whenever the shift key is being held down
		if (!selectedUnits.empty() && (shiftPressed || buildQueued)) {
			for (const auto bi: unitHandler.GetBuilderCAIs()) {
				const CBuilderCAI* builderCAI = bi.second;
				const CUnit* builder = builderCAI->owner;

				if (builder->team == gu->myTeam) {
					commandDrawer->SetBuildQueueSquareColor(cmdColors.buildBox);
					commandDrawer->DrawQueuedBuildingSquares(builderCAI);
					continue;
				}
				if (teamHandler.AlliedTeams(builder->team, gu->myTeam)) {
					commandDrawer->SetBuildQueueSquareColor(cmdColors.allyBuildBox);
					commandDrawer->DrawQueuedBuildingSquares(builderCAI);
				}
			}
		}
	}

	shader->Disable();

	glAttribStatePtr->PolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glAttribStatePtr->DisableBlendMask();
	glAttribStatePtr->EnableDepthTest();
	glAttribStatePtr->EnableDepthMask();
}


void CSelectedUnitsHandler::DependentDied(CObject* o)
{
	selectedUnits.erase(static_cast<CUnit*>(o)->id);

	selectionChanged = true;
	possibleCommandsChanged = true;
}


// handles NETMSG_SELECT's
void CSelectedUnitsHandler::NetSelect(std::vector<int>& s, int playerId)
{
	assert(unsigned(playerId) < netSelected.size());
	netSelected[playerId] = s;
}

// handles NETMSG_COMMAND's
void CSelectedUnitsHandler::NetOrder(Command& c, int playerId)
{
	assert(unsigned(playerId) < netSelected.size());

	if (netSelected[playerId].empty())
		return;

	selectedUnitsAI.GiveCommandNet(c, playerId);
	eoh->PlayerCommandGiven(netSelected[playerId], c, playerId);
}

void CSelectedUnitsHandler::ClearNetSelect(int playerId)
{
	netSelected[playerId].clear();
}

// handles NETMSG_AICOMMAND{S}'s sent by AICallback / LuaUnsyncedCtrl (!)
void CSelectedUnitsHandler::AINetOrder(int unitID, int aiTeamID, int playerID, const Command& c)
{
	CUnit* unit = unitHandler.GetUnit(unitID);

	if (unit == nullptr)
		return;

	const CPlayer* player = playerHandler.Player(playerID);

	if (player == nullptr)
		return;

	// no warning; will result in false bug reports due to latency between
	// time of giving valid orders on units which then change team through
	// e.g. LuaRules
	// AI's are hosted by players, but do not have any Player representation
	// themselves and should not be automatically controllable by their host
	// on the other hand they should always be able to control their OWN team
	if ((aiTeamID == MAX_TEAMS && !player->CanControlTeam(unit->team)) || (aiTeamID != MAX_TEAMS && aiTeamID != unit->team))
		return;

	// always pulled from net, synced command by definition
	// (fromSynced determines whether CMD_UNLOAD_UNITS uses
	// synced or unsynced randomized position sampling, etc)
	unit->commandAI->GiveCommand(c, playerID, true, false);
}


/******************************************************************************/
//
//  GetDefaultCmd() and friends
//

static bool targetIsEnemy = false;
static const CUnit* targetUnit = nullptr;
static const CFeature* targetFeature = nullptr;


static inline bool IsBetterLeader(const UnitDef* newDef, const UnitDef* oldDef)
{
	// There is a lot more that could be done here to make better
	// selections, but the users may prefer simplicity over smarts.

	if (targetUnit != nullptr) {
		if (targetIsEnemy) {
			const bool newCanDamage = newDef->CanDamage();
			const bool oldCanDamage = oldDef->CanDamage();

			if ( newCanDamage && !oldCanDamage)
				return true;
			if (!newCanDamage &&  oldCanDamage)
				return false;
			if (!targetUnit->unitDef->CanDamage()) {
				if ( newDef->canReclaim && !oldDef->canReclaim)
					return true;
				if (!newDef->canReclaim &&  oldDef->canReclaim)
					return false;
			}
		} else { // targetIsAlly
			if (targetUnit->health < targetUnit->maxHealth) {
				if ( newDef->canRepair && !oldDef->canRepair)
					return true;
				if (!newDef->canRepair &&  oldDef->canRepair)
					return false;
			}

			const bool newCanLoad = (newDef->transportCapacity > 0);
			const bool oldCanLoad = (oldDef->transportCapacity > 0);

			if ( newCanLoad && !oldCanLoad)
				return true;
			if (!newCanLoad &&  oldCanLoad)
				return false;
			if ( newDef->canGuard && !oldDef->canGuard)
				return true;
			if (!newDef->canGuard &&  oldDef->canGuard)
				return false;
		}
	}
	else if (targetFeature != nullptr) {
		if (targetFeature->udef != nullptr) {
			if ( newDef->canResurrect && !oldDef->canResurrect)
				return true;
			if (!newDef->canResurrect &&  oldDef->canResurrect)
				return false;
		}
		if ( newDef->canReclaim && !oldDef->canReclaim)
			return true;
		if (!newDef->canReclaim &&  oldDef->canReclaim)
			return false;
	}

	return (newDef->speed > oldDef->speed); // CMD_MOVE?
}


// CALLINFO:
// DrawMapStuff --> CGuiHandler::GetDefaultCommand --> GetDefaultCmd
// CMouseHandler::DrawCursor --> DrawCentroidCursor --> CGuiHandler::GetDefaultCommand --> GetDefaultCmd
// LuaUnsyncedRead::GetDefaultCommand --> CGuiHandler::GetDefaultCommand --> GetDefaultCmd
int CSelectedUnitsHandler::GetDefaultCmd(const CUnit* unit, const CFeature* feature)
{
	// return the default if there are no units selected
	if (selectedUnits.empty())
		return CMD_STOP;

	// setup the locals for IsBetterLeader()
	targetUnit = unit;
	targetFeature = feature;

	if (targetUnit != nullptr)
		targetIsEnemy = !teamHandler.Ally(gu->myAllyTeam, targetUnit->allyteam);

	// find the best leader to pick the command
	const CUnit* leaderUnit = unitHandler.GetUnit(*selectedUnits.begin());
	const UnitDef* leaderDef = leaderUnit->unitDef;

	for (const int unitID: selectedUnits) {
		const CUnit* testUnit = unitHandler.GetUnit(unitID);
		const UnitDef* testDef = testUnit->unitDef;

		if (testDef == leaderDef)
			continue;

		if (!IsBetterLeader(testDef, leaderDef))
			continue;

		leaderDef = testDef;
		leaderUnit = testUnit;
	}

	int cmd = leaderUnit->commandAI->GetDefaultCmd(unit, feature);
	eventHandler.DefaultCommand(unit, feature, cmd);
	return cmd;
}


/******************************************************************************/

void CSelectedUnitsHandler::PossibleCommandChange(CUnit* sender)
{
	possibleCommandsChanged |= (sender == nullptr || selectedUnits.find(sender->id) != selectedUnits.end());
}

// CALLINFO:
// CGame::Draw --> DrawCommands
// CMiniMap::DrawForReal --> DrawCommands
void CSelectedUnitsHandler::DrawCommands(bool onMiniMap)
{
	lineDrawer.Configure(cmdColors.UseColorRestarts(),
	                     cmdColors.UseRestartColor(),
	                     cmdColors.restart,
	                     cmdColors.RestartAlpha());
	lineDrawer.SetupLineStipple();
	lineDrawer.SetWidth(CLineDrawer::LineWidth::QueuedCmd);

	glAttribStatePtr->DisableDepthTest();
	glAttribStatePtr->EnableBlendMask();
	glAttribStatePtr->BlendFunc((GLenum) cmdColors.QueuedBlendSrc(), (GLenum) cmdColors.QueuedBlendDst());

	if (selectedGroup != -1) {
		const auto& groupHandler = uiGroupHandlers[gu->myTeam];
		const auto& groupUnits = groupHandler.GetGroup(selectedGroup)->units;

		for (const int unitID: groupUnits) {
			commandDrawer->Draw((unitHandler.GetUnit(unitID))->commandAI, onMiniMap);
		}
	} else {
		for (const int unitID: selectedUnits) {
			commandDrawer->Draw((unitHandler.GetUnit(unitID))->commandAI, onMiniMap);
		}
	}

	// draw the commands from AIs
	waitCommandsAI.DrawCommands();

	lineDrawer.SetWidth(CLineDrawer::LineWidth::Default);

	glAttribStatePtr->EnableDepthTest();
}


// CALLINFO:
// CTooltipConsole::Draw --> CMouseHandler::GetCurrentTooltip
// LuaUnsyncedRead::GetCurrentTooltip --> CMouseHandler::GetCurrentTooltip
// CMouseHandler::GetCurrentTooltip --> CMiniMap::GetToolTip --> GetTooltip
// CMouseHandler::GetCurrentTooltip --> GetTooltip
std::string CSelectedUnitsHandler::GetTooltip()
{
	std::string s;

	{
		if (selectedUnits.empty())
			return s;

		const CUnit* unit = unitHandler.GetUnit(*selectedUnits.begin());
		const CTeam* team = nullptr;

		// show the player name instead of unit name if it has FBI tag showPlayerName
		if (unit->unitDef->showPlayerName) {
			team = teamHandler.Team(unit->team);
			s = team->GetControllerName();
		} else {
			s = unitToolTipMap.Get(unit->id);
		}
	}

	const std::string custom = std::move(eventHandler.WorldTooltip(nullptr, nullptr, nullptr));
	if (!custom.empty())
		return custom;

	{
		#define NO_TEAM -32
		#define MULTI_TEAM -64
		int ctrlTeam = NO_TEAM;

		SUnitStats stats;

		for (const int unitID: selectedUnits) {
			const CUnit* unit = unitHandler.GetUnit(unitID);
			stats.AddUnit(unit, false);

			if (ctrlTeam == NO_TEAM) {
				ctrlTeam = unit->team;
			} else if (ctrlTeam != unit->team) {
				ctrlTeam = MULTI_TEAM;
			}
		}

		s += CTooltipConsole::MakeUnitStatsString(stats);

		const char* ctrlName = "";

		if (ctrlTeam == MULTI_TEAM) {
			ctrlName = "(Multiple teams)";
		} else if (ctrlTeam != NO_TEAM) {
			ctrlName = teamHandler.Team(ctrlTeam)->GetControllerName();
		}

		s += "\n\xff\xff\xff\xff";
		s += ctrlName;
		return s;
	}
}


void CSelectedUnitsHandler::SetCommandPage(int page)
{
	for (const int unitID: selectedUnits) {
		CUnit* u = unitHandler.GetUnit(unitID);
		CCommandAI* c = u->commandAI;
		c->lastSelectedCommandPage = page;
	}
}



void CSelectedUnitsHandler::SendCommand(const Command& c)
{
	if (selectionChanged) {
		// send new selection; first gather unit IDs
		selectedUnitIDs.clear();
		selectedUnitIDs.resize(selectedUnits.size(), 0);

		std::copy(selectedUnits.begin(), selectedUnits.end(), selectedUnitIDs.begin());

		clientNet->Send(CBaseNetProtocol::Get().SendSelect(gu->myPlayerNum, selectedUnitIDs));
		selectionChanged = false;
	}

	clientNet->Send(CBaseNetProtocol::Get().SendCommand(gu->myPlayerNum, c.GetID(), c.GetTimeOut(), c.GetOpts(), c.GetNumParams(), c.GetParams()));
}


// despite the NETMSG_AICOMMANDS packet-id, this only services LuaUnsyncedCtrl
void CSelectedUnitsHandler::SendCommandsToUnits(const std::vector<int>& unitIDs, const std::vector<Command>& commands, bool pairwise)
{
	// do not waste bandwidth (units can be selected
	// by any spectator, but not given orders without
	// god-mode)
	// note: clients verify this every NETMSG_SELECT
	if (gu->spectating && gs->godMode == 0)
		return;

	const unsigned unitIDCount  = unitIDs.size();
	const unsigned commandCount = commands.size();

	if ((unitIDCount == 0) || (commandCount == 0))
		return;

	uint32_t totalParams = 0;

	// if all commands share the same ID / options / number of parameters,
	// insert only these values into the packet to save a bit of bandwidth
	int32_t refCmdID = commands[0].GetID();
	uint8_t refCmdOpts = commands[0].GetOpts();
	int32_t refCmdSize = commands[0].GetNumParams();

	for (unsigned int c = 0; c < commandCount; c++) {
		totalParams += commands[c].GetNumParams();

		if (refCmdID != 0 && refCmdID != commands[c].GetID())
			refCmdID = 0;
		if (refCmdOpts != 0xFF && refCmdOpts != commands[c].GetOpts())
			refCmdOpts = 0xFF;
		if (refCmdSize != 0xFFFF && refCmdSize != commands[c].GetNumParams())
			refCmdSize = 0xFFFF;
	}

	unsigned int optBytesPerCmd = 0;
	unsigned int totalPacketLen = 0;

	// optional data per command (cmdID, cmdOpts, #cmdParams)
	optBytesPerCmd += (sizeof(uint32_t) * (refCmdID   == 0     ));
	optBytesPerCmd += (sizeof(uint8_t ) * (refCmdOpts == 0xFF  ));
	optBytesPerCmd += (sizeof(uint16_t) * (refCmdSize == 0xFFFF));

	// msg type, msg size
	totalPacketLen += (sizeof(uint8_t) + sizeof(static_cast<uint16_t>(totalPacketLen)));
	// player ID, AI ID, pairwise
	totalPacketLen += (sizeof(uint8_t) * 3);
	totalPacketLen += (
		sizeof(static_cast<uint32_t>(refCmdID  )) +
		sizeof(static_cast<uint8_t >(refCmdOpts)) +
		sizeof(static_cast<uint16_t>(refCmdSize))
	);

	totalPacketLen += sizeof(static_cast<uint16_t>(unitIDCount));
	totalPacketLen += (unitIDCount * sizeof(uint16_t));

	totalPacketLen += sizeof(static_cast<uint16_t>(commandCount));
	totalPacketLen += (commandCount * optBytesPerCmd);
	totalPacketLen += (totalParams * sizeof(float)); // params are floats

	if (totalPacketLen > 8192) {
		LOG_L(L_WARNING, "[%s] discarded oversized (len=%i) NETMSG_AICOMMANDS packet", __func__, totalPacketLen);
		return; // do not send oversized packets
	}

	netcode::PackPacket* packet = new netcode::PackPacket(totalPacketLen);
	*packet << static_cast<uint8_t >(NETMSG_AICOMMANDS)
	        << static_cast<uint16_t>(totalPacketLen)

	        << static_cast<uint8_t>(gu->myPlayerNum)
	        << static_cast<uint8_t>(MAX_AIS)
	        // << static_cast<uint8_t>(MAX_TEAMS)

	        << static_cast<uint8_t >(pairwise)
	        << static_cast<uint32_t>(refCmdID)
	        << static_cast<uint8_t >(refCmdOpts)
	        << static_cast<uint16_t>(refCmdSize);

	// NOTE: does not check for invalid unitIDs
	*packet << static_cast<uint16_t>(unitIDCount);
	for (const int unitID: unitIDs) {
		*packet << static_cast<int16_t>(unitID);
	}

	*packet << static_cast<uint16_t>(commandCount);

	for (unsigned int i = 0; i < commandCount; ++i) {
		const Command& cmd = commands[i];

		if (refCmdID == 0)
			*packet << static_cast<uint32_t>(cmd.GetID());
		if (refCmdOpts == 0xFF)
			*packet << static_cast<uint8_t>(cmd.GetOpts());
		if (refCmdSize == 0xFFFF)
			*packet << static_cast<uint16_t>(cmd.GetNumParams());

		for (unsigned int j = 0, n = cmd.GetNumParams(); j < n; j++) {
			*packet << cmd.GetParam(j);
		}
	}

	clientNet->Send(std::shared_ptr<netcode::RawPacket>(packet));
}
