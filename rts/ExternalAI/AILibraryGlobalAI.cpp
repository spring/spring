/*
	Copyright 2008  Nicolas Wu
	
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "AISCommands.h"


#include "AILibraryGlobalAI.h"
#include "AISEvents.h"
#include "GlobalAICallback.h"
//#include "SGlobalAICallback.h"
#include "SAICallback.h"
#include "float3.h"
#include "SAIFloat3.h"

#include <iostream>

//SGlobalAICallback* initSGlobalAICallback(int teamId, IGlobalAICallback* globalAICallback);
SAICallback* initSAICallback(int teamId, IGlobalAICallback* aiGlobalCallback); // defined in "SGlobalAICallback.cpp"

CAILibraryGlobalAI::CAILibraryGlobalAI(const char* libName, int team)  : CAILibrary(libName, team)
{
	CGlobalAI::team = team;
	
	ai = this;  
	callback = new CGlobalAICallback(this);
	InitAI(callback, team);
}

CAILibraryGlobalAI::~CAILibraryGlobalAI() {
	delete callback;
	ai = 0;
}

// This is the inherited interface from CGlobalAI, we don't
// wish to do anything here.

void CAILibraryGlobalAI::LoadAILib(int, const char*, bool) {
}
void CAILibraryGlobalAI::Serialize(creg::ISerializer *s) {
}
void CAILibraryGlobalAI::PostLoad() {
}
void CAILibraryGlobalAI::Load(std::istream *s) {
}
void CAILibraryGlobalAI::PreDestroy() {} 

// This is the inherited interface from IGlobalAI
void CAILibraryGlobalAI::InitAI(IGlobalAICallback* callback, int team) {
//	SGlobalAICallback* c_callback = initSGlobalAICallback(team, callback);
	SAICallback* c_callback = initSAICallback(team, callback);
	SInitEvent event = {team, c_callback};
	handleEvent(EVENT_INIT, &event);
}

void CAILibraryGlobalAI::Update() {
	SUpdateEvent event = {0};
	handleEvent(EVENT_UPDATE, &event);
}

void CAILibraryGlobalAI::Load(IGlobalAICallback* callback, std::istream *s) {
	//TODO
}

void CAILibraryGlobalAI::Save(std::ostream *s) {
	//TODO
}

// NOTE: this isn't the same as handleEvent, it's the one that's been inherited.
int CAILibraryGlobalAI::HandleEvent(int aiEventId, const void *data) {

	switch (aiEventId) {
		case AI_EVENT_UNITGIVEN:
		{
			ChangeTeamEvent* evt = (ChangeTeamEvent*) data;
			SUnitGivenEvent sEvent = {evt->unit, evt->oldteam, evt->newteam};
			return handleEvent(EVENT_UNIT_GIVEN, &sEvent);
		}
		case AI_EVENT_UNITCAPTURED:
		{
			ChangeTeamEvent* evt = (ChangeTeamEvent*) data;
			SUnitCapturedEvent sEvent = {evt->unit, evt->oldteam, evt->newteam};
			return handleEvent(EVENT_UNIT_CAPTURED, &sEvent);
		}
		case AI_EVENT_WEAPON_FIRED:
		{
			WeaponFireEvent* evt = (WeaponFireEvent*) data;
			SWeaponFiredEvent sEvent = {evt->unit, evt->def->id};
			return handleEvent(EVENT_WEAPON_FIRED, &sEvent);
		}
		case AI_EVENT_PLAYER_COMMAND:
		{
			PlayerCommandEvent* evt = (PlayerCommandEvent*) data;
			int numUnits = evt->units.size();
			int unitIds[numUnits];
			int i;
            for (i=0; i < numUnits; ++i) {
                unitIds[i] = evt->units.at(i);
            }
            int sCommandId;
            void* sCommandData = mallocSUnitCommand(-1, -1, &(evt->command), &sCommandId);
			SPlayerCommandEvent sEvent = {unitIds, numUnits, sCommandId, sCommandData, evt->player};
			return handleEvent(EVENT_PLAYER_COMMAND, &sEvent);
		}
		case AI_EVENT_SEISMIC_PING:
		{
			SeismicPingEvent* evt = (SeismicPingEvent*) data;
			SSeismicPingEvent sEvent = {evt->pos.toSAIFloat3(), evt->strength};
			return handleEvent(EVENT_SEISMIC_PING, &sEvent);
		}
		default:
			return 0;
	}
}


void CAILibraryGlobalAI::GotChatMsg(const char* message,int player) {
	SMessageEvent event = {player, message};
	handleEvent(EVENT_MESSAGE, &event);
}

void CAILibraryGlobalAI::UnitCreated(int unit) {
	SUnitCreatedEvent event = {unit};
	handleEvent(EVENT_UNIT_CREATED, &event);
}

void CAILibraryGlobalAI::UnitFinished(int unit) {
	SUnitFinishedEvent event = {unit};
	handleEvent(EVENT_UNIT_FINISHED, &event);
}

void CAILibraryGlobalAI::UnitIdle(int unit) {
	SUnitIdleEvent event = {unit};
	handleEvent(EVENT_UNIT_IDLE, &event);
}

void CAILibraryGlobalAI::UnitMoveFailed(int unit) {
	SUnitMoveFailedEvent event = {unit};
	handleEvent(EVENT_UNIT_MOVE_FAILED, &event);
}

void CAILibraryGlobalAI::UnitDamaged(int unit,int attacker,float damage, float3 dir) {
	SUnitDamagedEvent event = {unit, attacker, damage, dir.toSAIFloat3()};
	handleEvent(EVENT_UNIT_DAMAGED, &event);
}

void CAILibraryGlobalAI::UnitDestroyed(int unit, int attacker) {
	SUnitDestroyedEvent event = {unit, attacker};
	handleEvent(EVENT_UNIT_DESTROYED, &event);
}

void CAILibraryGlobalAI::EnemyEnterLOS(int enemy) {
	SEnemyEnterLOSEvent event = {enemy};
	handleEvent(EVENT_ENEMY_ENTER_LOS, &event);
}

void CAILibraryGlobalAI::EnemyLeaveLOS(int enemy) {
	SEnemyLeaveLOSEvent event = {enemy};
	handleEvent(EVENT_ENEMY_LEAVE_LOS, &event);
}

void CAILibraryGlobalAI::EnemyEnterRadar(int enemy) {
	SEnemyEnterRadarEvent event = {enemy};
	handleEvent(EVENT_ENEMY_ENTER_RADAR, &event);
}
void CAILibraryGlobalAI::EnemyLeaveRadar(int enemy) {
	SEnemyLeaveRadarEvent event = {enemy};
	handleEvent(EVENT_ENEMY_LEAVE_RADAR, &event);
}

void CAILibraryGlobalAI::EnemyDamaged(int enemy,int attacker,float damage,float3 dir) {
	SEnemyDamagedEvent event = {enemy, attacker, damage, dir.toSAIFloat3()};
	handleEvent(EVENT_ENEMY_DAMAGED, &event);
}

void CAILibraryGlobalAI::EnemyDestroyed(int enemy, int attacker) {
	SEnemyDestroyedEvent event = {enemy, attacker};
	handleEvent(EVENT_ENEMY_DESTROYED, &event);
}
