/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include <assert.h>

#include "mmgr.h"

#include "ExternalAI/SkirmishAIHandler.h"
#include "Game/Player.h"
#include "Game/PlayerHandler.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/SelectedUnits.h"
#include "Game/UI/MouseHandler.h"
#include "Lua/LuaRules.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "System/myMath.h"
#include "System/EventHandler.h"
#include "System/GlobalUnsynced.h"
#include "System/LogOutput.h"

CR_BIND(CPlayer,);

CR_REG_METADATA(CPlayer, (
	CR_MEMBER(name),
	CR_MEMBER(countryCode),
	CR_MEMBER(rank),
	CR_MEMBER(spectator),
	CR_MEMBER(team),

	CR_MEMBER(active),
	CR_MEMBER(playerNum),
//	CR_MEMBER(readyToStart),
//	CR_MEMBER(cpuUsage),
//	CR_MEMBER(ping),
//	CR_MEMBER(currentStats),

//	CR_MEMBER(controlledTeams),
	CR_RESERVED(32)
));


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPlayer::CPlayer()
	: PlayerBase()
	, active(false)
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
		for (int t = 0; t < teamHandler->ActiveTeams(); t++) {
			controlledTeams.insert(t);
		}
		return;
	}

	if (!spectator) {
		// my team
		controlledTeams.insert(team);
	}

	// AI teams
	const CSkirmishAIHandler::id_ai_t aiIds = skirmishAIHandler.GetAllSkirmishAIs();
	for (CSkirmishAIHandler::id_ai_t::const_iterator ai = aiIds.begin(); ai != aiIds.end(); ++ai) {
		const bool isHostedByUs   = (ai->second.hostPlayer == playerNum);
		if (isHostedByUs) {
			controlledTeams.insert(ai->second.team);
		}
	}
}


void CPlayer::UpdateControlledTeams()
{
	for (int p = 0; p < playerHandler->ActivePlayers(); p++) {
		CPlayer* player = playerHandler->Player(p);
		if (player) {
			player->SetControlledTeams();
		}
	}
}


void CPlayer::StartSpectating()
{
	if (spectator) {
		return;
	}

	spectator = true;

	if (gu->myPlayerNum == this->playerNum) { //TODO bad hack
		gu->spectating           = true;
		gu->spectatingFullView   = true;
		gu->spectatingFullSelect = true;
	}

	StopControllingUnit();
	eventHandler.PlayerChanged(playerNum);
}

void CPlayer::GameFrame(int frameNum)
{
	if (!active || (fpsController.GetControllee() == NULL)) {
		return;
	}

	fpsController.Update();
}



void CPlayer::StartControllingUnit()
{
	CUnit* curControlleeUnit = fpsController.GetControllee();
	CUnit* newControlleeUnit = NULL;

	if (curControlleeUnit != NULL) {
		// player released control
		StopControllingUnit();
	} else {
		// player took control
		const std::vector<int>& ourSelectedUnits = selectedUnits.netSelected[this->playerNum];

		if (ourSelectedUnits.empty()) {
			return;
		}

		// pick the first unit we have selected
		newControlleeUnit = uh->GetUnit(ourSelectedUnits[0]);

		if (newControlleeUnit == NULL || newControlleeUnit->weapons.empty()) {
			return;
		}


		if (newControlleeUnit->fpsControlPlayer != NULL) {
			if (this->playerNum == gu->myPlayerNum) {
				logOutput.Print(
					"player %d (%s) is already controlling unit %d",
					newControlleeUnit->fpsControlPlayer->playerNum,
					newControlleeUnit->fpsControlPlayer->name.c_str(),
					newControlleeUnit->id
				);
			}
		}
		else if (luaRules == NULL || luaRules->AllowDirectUnitControl(this->playerNum, newControlleeUnit)) {
			newControlleeUnit->fpsControlPlayer = this;
			fpsController.SetControlleeUnit(newControlleeUnit);

			if (this->playerNum == gu->myPlayerNum) {
				// update the unsynced state
				gu->fpsMode = true;
				mouse->wasLocked = mouse->locked;

				if (!mouse->locked) {
					mouse->locked = true;
					mouse->HideMouse();
				}
				camHandler->PushMode();
				camHandler->SetCameraMode(0);
				selectedUnits.ClearSelected();
			}
		}
	}
}

void CPlayer::StopControllingUnit()
{
	if (fpsController.GetControllee() == NULL || mouse == NULL) {
		return;
	}

	CPlayer* that = gu->GetMyPlayer();
	CUnit* thatUnit = that->fpsController.GetControllee();
	CUnit* thisUnit = this->fpsController.GetControllee();

	thisUnit->fpsControlPlayer = NULL;
	thisUnit->AttackUnit(NULL, true, true);

	if (thatUnit == thisUnit) {
		// update the  unsynced state
		gu->fpsMode = false;
		assert(gu->myPlayerNum == this->playerNum);

		// switch back to the camera we were using before
		camHandler->PopMode();

		if (mouse->locked && !mouse->wasLocked) {
			mouse->locked = false;
			mouse->ShowMouse();
		}
	}

	fpsController.SetControlleeUnit(NULL);
}
