/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cassert>


#include "ExternalAI/SkirmishAIHandler.h"
#include "Player.h"
#include "PlayerHandler.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/GlobalUnsynced.h"
#include "Game/SelectedUnitsHandler.h"
#include "Game/UI/MouseHandler.h"
#include "Game/UI/UnitTracker.h"
#include "Lua/LuaUI.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "System/SpringMath.h"
#include "System/EventHandler.h"
#include "System/Log/ILog.h"
#include "System/creg/STL_Set.h"


CR_BIND_DERIVED(CPlayer, PlayerBase, )
CR_REG_METADATA(CPlayer, (
	CR_MEMBER(active),
	CR_MEMBER(playerNum),
	CR_IGNORED(ping),
	CR_MEMBER(currentStats),
	CR_IGNORED(fpsController),
	CR_MEMBER(controlledTeams)
))


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPlayer::CPlayer()
	: active(false)
	, playerNum(-1)
	, ping(0)
{
	fpsController.SetControllerPlayer(this);
}



void CPlayer::SetControlledTeams()
{
	controlledTeams.clear();

	if (gs->godMode) {
		// anyone can control any unit
		for (int t = 0; t < teamHandler.ActiveTeams(); t++) {
			controlledTeams.insert(t);
		}
		return;
	}

	// my team
	if (!spectator)
		controlledTeams.insert(team);

	// AI teams
	for (const auto& p: skirmishAIHandler.GetAllSkirmishAIs()) {
		const SkirmishAIData& sad = *(p.second);

		if (sad.hostPlayer != playerNum)
			continue;

		controlledTeams.insert(sad.team);
	}
}


void CPlayer::UpdateControlledTeams()
{
	for (int p = 0; p < playerHandler.ActivePlayers(); p++) {
		CPlayer* player = playerHandler.Player(p);

		if (player == nullptr)
			continue;

		player->SetControlledTeams();
	}
}


void CPlayer::StartSpectating()
{
	if (spectator)
		return;

	spectator = true;

	if (gu->myPlayerNum == this->playerNum) {
		// HACK: unsynced code should just listen for the PlayerChanged event
		gu->spectating           = true;
		gu->spectatingFullView   = true;
		gu->spectatingFullSelect = true;

		//FIXME use eventHandler?
		CLuaUI::UpdateTeams();
		selectedUnitsHandler.ClearSelected();
		if (readMap != nullptr)
			readMap->BecomeSpectator();
		unitTracker.Disable();
	}

	StopControllingUnit();
	eventHandler.PlayerChanged(playerNum);
}

void CPlayer::JoinTeam(int newTeam)
{
	// a player that joins a team always stops spectating
	spectator = false;
	team = newTeam;

	if (gu->myPlayerNum == this->playerNum) {
		// HACK: see StartSpectating
		gu->myPlayingTeam = gu->myTeam = newTeam;
		gu->myPlayingAllyTeam = gu->myAllyTeam = teamHandler.AllyTeam(gu->myTeam);

		gu->spectating           = false;
		gu->spectatingFullView   = false;
		gu->spectatingFullSelect = false;

		CLuaUI::UpdateTeams();
		selectedUnitsHandler.ClearSelected();
		unitTracker.Disable();
	}

	eventHandler.PlayerChanged(playerNum);
}

void CPlayer::GameFrame(int frameNum)
{
	if (!active || (fpsController.GetControllee() == nullptr))
		return;

	fpsController.Update();
}



void CPlayer::StartControllingUnit()
{
	CUnit* curControlleeUnit = fpsController.GetControllee();
	CUnit* newControlleeUnit = nullptr;

	if (curControlleeUnit != nullptr) {
		// player released control
		StopControllingUnit();
	} else {
		// player took control
		const std::vector<int>& ourSelectedUnits = selectedUnitsHandler.netSelected[this->playerNum];

		if (ourSelectedUnits.empty())
			return;

		// pick the first unit we have selected
		newControlleeUnit = unitHandler.GetUnit(ourSelectedUnits[0]);

		if (newControlleeUnit == nullptr || newControlleeUnit->weapons.empty())
			return;

		if (newControlleeUnit->fpsControlPlayer != nullptr) {
			if (this->playerNum == gu->myPlayerNum) {
				LOG_L(L_WARNING,
						"player %d (%s) is already controlling unit %d",
						newControlleeUnit->fpsControlPlayer->playerNum,
						newControlleeUnit->fpsControlPlayer->name.c_str(),
						newControlleeUnit->id);
			}

			return;
		}

		if (eventHandler.AllowDirectUnitControl(this->playerNum, newControlleeUnit)) {
			newControlleeUnit->fpsControlPlayer = this;
			fpsController.SetControlleeUnit(newControlleeUnit);
			selectedUnitsHandler.ClearNetSelect(this->playerNum);

			if (this->playerNum == gu->myPlayerNum) {
				// update the unsynced state
				selectedUnitsHandler.ClearSelected();

				gu->fpsMode = true;
				mouse->wasLocked = mouse->locked;

				if (!mouse->locked) {
					mouse->locked = true;
					mouse->HideMouse();
				}
				camHandler->PushMode();
				camHandler->SetCameraMode(0);
			}
		}
	}
}

void CPlayer::StopControllingUnit()
{
	if (fpsController.GetControllee() == nullptr || mouse == nullptr)
		return;

	CPlayer* that = gu->GetMyPlayer();
	CUnit* thatUnit = that->fpsController.GetControllee();
	CUnit* thisUnit = this->fpsController.GetControllee();

	// note: probably better to issue CMD_STOP via thisUnit->commandAI
	thisUnit->AttackUnit(nullptr, true, false, true);
	thisUnit->fpsControlPlayer = nullptr;
	fpsController.SetControlleeUnit(nullptr);
	selectedUnitsHandler.ClearNetSelect(this->playerNum);

	if (thatUnit == thisUnit) {
		// update the unsynced state
		selectedUnitsHandler.ClearSelected();

		gu->fpsMode = false;
		assert(gu->myPlayerNum == this->playerNum);

		// switch back to the camera we were using before
		camHandler->PopMode();

		if (mouse->locked && !mouse->wasLocked) {
			mouse->locked = false;
			mouse->ShowMouse();
		}
	}
}
