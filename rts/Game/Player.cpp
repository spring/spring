/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include <assert.h>
#include <SDL_mouse.h>

#include "mmgr.h"

#include "ExternalAI/SkirmishAIHandler.h"
#include "Game/Player.h"
#include "Game/PlayerHandler.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/GameHelper.h"
#include "Game/UI/MouseHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Units/COB/CobInstance.h"
#include "System/myMath.h"
#include "System/EventHandler.h"
#include "System/GlobalUnsynced.h"
#include "System/NetProtocol.h"

CR_BIND(CPlayer,);

CR_REG_METADATA(CPlayer, (
				CR_MEMBER(name),
				CR_MEMBER(countryCode),
				CR_MEMBER(rank),
				CR_MEMBER(spectator),
				CR_MEMBER(team),

				CR_MEMBER(active),
				CR_MEMBER(playerNum),
//				CR_MEMBER(readyToStart),
//				CR_MEMBER(cpuUsage),
//				CR_MEMBER(ping),
//				CR_MEMBER(currentStats),

//				CR_MEMBER(controlledTeams),
				CR_RESERVED(32)
				));


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPlayer::CPlayer()
{
	memset(&currentStats, 0, sizeof(Statistics));

	active   = false;
	cpuUsage = 0;
	ping     = 0;

	myControl.forward = 0;
	myControl.back    = 0;
	myControl.left    = 0;
	myControl.right   = 0;

	myControl.mouse1       = 0;
	myControl.mouse2       = 0;
	myControl.viewDir      = float3(0, 0, 1);
	myControl.targetPos    = float3(0, 0, 1);
	myControl.targetDist   = 1000;
	myControl.target       = 0;
	myControl.myController = this;
}

CPlayer::~CPlayer()
{
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
	if (dccs.playerControlledUnit)
		StopControllingUnit();

	if (playerHandler->Player(gu->myPlayerNum) == this) { //TODO bad hack
		gu->spectating           = true;
		gu->spectatingFullView   = true;
		gu->spectatingFullSelect = true;
	}
	eventHandler.PlayerChanged(playerNum);
}

void CPlayer::GameFrame(int frameNum)
{
	if (!active || !dccs.playerControlledUnit)
		return;

	CUnit* unit = dccs.playerControlledUnit;
	DirectControlStruct* dc = &myControl;

	const int piece = unit->script->AimFromWeapon(0);
	const float3 relPos = unit->script->GetPiecePos(piece);
	float3 pos = unit->pos +
		unit->frontdir * relPos.z +
		unit->updir    * relPos.y +
		unit->rightdir * relPos.x;
	pos += UpVector * 7;

	dccs.oldDCpos = pos;

	CUnit* hit;
	float dist = helper->TraceRayTeam(pos, dc->viewDir, unit->maxRange, hit, 1, unit, teamHandler->AllyTeam(team));
	dc->target = hit;

	// prevent dgunning using FPS view if outside dgun-limit
	if (uh->limitDgun && unit->unitDef->isCommander && unit->pos.SqDistance(teamHandler->Team(unit->team)->startPos) > Square(uh->dgunRadius)) {
		return;
	}

	if (hit) {
		dc->targetDist = dist;
		dc->targetPos = hit->pos;
		if (!dc->mouse2) {
			unit->AttackUnit(hit, true);
		}
	} else {
		if (dist > unit->maxRange * 0.95f)
			dist = unit->maxRange * 0.95f;

		dc->targetDist = dist;
		dc->targetPos = pos + dc->viewDir * dc->targetDist;

		if (!dc->mouse2) {
			unit->AttackGround(dc->targetPos, true);
			for (std::vector<CWeapon*>::iterator wi = unit->weapons.begin(); wi != unit->weapons.end(); ++wi) {
				float d = dc->targetDist;
				if (d > (*wi)->range * 0.95f)
					d = (*wi)->range * 0.95f;
				float3 p = pos + dc->viewDir * d;
				(*wi)->AttackGround(p, true);
			}
		}
	}
}

void CPlayer::StopControllingUnit()
{
	if(!dccs.playerControlledUnit)
		return;

	CUnit* unit = dccs.playerControlledUnit;
	unit->directControl = 0;
	unit->AttackUnit(0, true);
	if (gu->directControl == dccs.playerControlledUnit) {
		assert(playerHandler->Player(gu->myPlayerNum) == this);
		gu->directControl = 0;

		/* Switch back to the camera we were using before. */
		camHandler->PopMode();

		if (mouse->locked && !mouse->wasLocked) {
			mouse->locked = false;
			mouse->ShowMouse();
		}
	}

	dccs.playerControlledUnit = 0;
}



void DirectControlClientState::SendStateUpdate(bool* camMove) {
	unsigned char state = 0;

	if (camMove[0]) { state |= (1 << 0); }
	if (camMove[1]) { state |= (1 << 1); }
	if (camMove[2]) { state |= (1 << 2); }
	if (camMove[3]) { state |= (1 << 3); }
	if (mouse->buttons[SDL_BUTTON_LEFT].pressed)  { state |= (1 << 4); }
	if (mouse->buttons[SDL_BUTTON_RIGHT].pressed) { state |= (1 << 5); }

	shortint2 hp = GetHAndPFromVector(camera->forward);

	if (hp.x != oldHeading || hp.y != oldPitch || state != oldState) {
		oldHeading = hp.x;
		oldPitch   = hp.y;
		oldState   = state;

		net->Send(CBaseNetProtocol::Get().SendDirectControlUpdate(gu->myPlayerNum, state, hp.x, hp.y));
	}
}
