#include "StdAfx.h"
// Player.cpp: implementation of the CPlayer class.
//
//////////////////////////////////////////////////////////////////////
#include <assert.h>

#include "mmgr.h"

#include "Player.h"
#include "PlayerHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/GlobalSynced.h"
#include "ExternalAI/Interface/SAIInterfaceLibrary.h"
#ifdef DIRECT_CONTROL_ALLOWED
#  include "UI/MouseHandler.h"
#  include "CameraHandler.h"
#  include "Camera.h"
#endif
#include "EventHandler.h"
#include "GlobalUnsynced.h"

CR_BIND(CPlayer,);

CR_REG_METADATA(CPlayer, (
				CR_MEMBER(active),
				CR_MEMBER(name),
				CR_MEMBER(countryCode),
				CR_MEMBER(rank),
				CR_MEMBER(spectator),
				CR_MEMBER(team),
//				CR_MEMBER(readyToStart),
//				CR_MEMBER(cpuUsage),
//				CR_MEMBER(ping),
				CR_MEMBER(currentStats),
				CR_MEMBER(playerNum),
//				CR_MEMBER(controlledTeams),
				CR_RESERVED(32)
				));

CR_BIND(CPlayer::Statistics,);

CR_REG_METADATA_SUB(CPlayer, Statistics, (
					CR_MEMBER(mousePixels),
					CR_MEMBER(mouseClicks),
					CR_MEMBER(keyPresses),
					CR_MEMBER(numCommands),
					CR_MEMBER(unitCommands),
					CR_RESERVED(16)
					));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPlayer::CPlayer()
{
	PUSH_CODE_MODE;
	ENTER_MIXED;
	currentStats = SAFE_NEW Statistics;
	memset(currentStats, 0, sizeof(Statistics));
	POP_CODE_MODE;

	active = false;
	readyToStart = false;
	cpuUsage = 0;
	ping = 0;


#ifdef DIRECT_CONTROL_ALLOWED
	playerControlledUnit=0;

	myControl.forward=0;
	myControl.back=0;
	myControl.left=0;
	myControl.right=0;

	myControl.mouse1=0;
	myControl.mouse2=0;
	myControl.viewDir=float3(0,0,1);
	myControl.targetPos=float3(0,0,1);
	myControl.targetDist=1000;
	myControl.target=0;
	myControl.myController=this;
#endif
}

CPlayer::~CPlayer()
{
	delete currentStats;
}


void CPlayer::SetControlledTeams()
{
	controlledTeams.clear();

	if (gs->godMode) {
		// anyone can control any unit
		for (int t = 0; t < MAX_TEAMS; t++) {
			controlledTeams.insert(t);
		}
		return;
	}

	if (!spectator) {
		// my team
		controlledTeams.insert(team);
	}

	// AI teams
	for (int t = 0; t < teamHandler->ActiveTeams(); t++) {
		const CTeam* team = teamHandler->Team(t);
		if (team && team->isAI &&
			!SSAIKey_Comparator::IsEmpty(team->skirmishAISpecifier) && // is not empty? -> luaAI does not require client control
			(team->leader == playerNum)) {
			controlledTeams.insert(t);
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
	spectator = true;
	if (playerHandler->Player(gu->myPlayerNum) == this) { //TODO bad hack
		gu->spectating           = true;
		gu->spectatingFullView   = true;
		gu->spectatingFullSelect = true;
	}
	eventHandler.PlayerChanged(playerNum);
}


#ifdef DIRECT_CONTROL_ALLOWED
void CPlayer::StopControllingUnit()
{
	ENTER_UNSYNCED;
	if (gu->directControl == playerControlledUnit) {
		assert(playerHandler->Player(gu->myPlayerNum) == this);
		gu->directControl = 0;

		/* Switch back to the camera we were using before. */
		camHandler->PopMode();

		if (mouse->locked && !mouse->wasLocked){
			mouse->locked = false;
			mouse->ShowMouse();
		}
	}
	ENTER_SYNCED;

	playerControlledUnit = 0;
}
#endif
