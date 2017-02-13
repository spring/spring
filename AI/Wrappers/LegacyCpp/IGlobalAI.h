/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef I_GLOBAL_AI_H
#define I_GLOBAL_AI_H

#include <cstdlib>
#include <cstring>

#include "aibase.h"
#include "Sim/Units/CommandAI/Command.h"
#include "System/float3.h"

#define GLOBAL_AI_INTERFACE_VERSION (19 + AI_INTERFACE_GENERATED_VERSION)

#define AI_EVENT_UNITGIVEN       1 // ChangeTeamEvent
#define AI_EVENT_UNITCAPTURED    2 // ChangeTeamEvent
#define AI_EVENT_WEAPON_FIRED    3 // WeaponFireEvent
#define AI_EVENT_PLAYER_COMMAND  4 // PlayerCommandEvent
#define AI_EVENT_SEISMIC_PING    5 // SeismicPingEvent


namespace springLegacyAI {

class IGlobalAICallback;
struct WeaponDef;

class IGlobalAI
{
public:
	struct ChangeTeamEvent {
		int unit, newteam, oldteam;
	};
	struct WeaponFireEvent {
		int unit;
		const WeaponDef* def;
	};
	struct PlayerCommandEvent {
		std::vector<int> units;
		Command command;
		int player;
	};
	struct SeismicPingEvent {
		float3 pos;
		float strength;
	};

	/// Called only once, right after the constructor was called
	virtual void InitAI(IGlobalAICallback* callback, int team) = 0;
	/// Called only once, right before the destructor is called
	virtual void ReleaseAI() {}

	/// called when a new unit is created on ai team
	virtual void UnitCreated(int unit, int builder) = 0;
	/// called when an unit has finished building
	virtual void UnitFinished(int unit) = 0;
	/// called when a unit is destroyed
	virtual void UnitDestroyed(int unit, int attacker) = 0;

	/// called when a new enemy unit is created
	virtual void EnemyCreated(int enemy) {}
	/// called when a new enemy unit has finished building
	virtual void EnemyFinished(int enemy) {}

	virtual void EnemyEnterLOS(int enemy) = 0;
	virtual void EnemyLeaveLOS(int enemy) = 0;

	/**
	 * called when an enemy enter radar coverage
	 * (los always count as radar coverage to)
	 */
	virtual void EnemyEnterRadar(int enemy) = 0;
	/**
	 * called when an enemy leave radar coverage
	 * (los always count as radar coverage to)
	 */
	virtual void EnemyLeaveRadar(int enemy) = 0;

	/// called when an enemy inside los or radar is damaged
	virtual void EnemyDamaged(int damaged, int attacker, float damage, float3 dir) = 0;
	/**
	 * will be called if an enemy inside los or radar dies
	 * (note that leave los etc will not be called then)
	 */
	virtual void EnemyDestroyed(int enemy, int attacker) = 0;

	/// called when a unit go idle and is not assigned to any group
	virtual void UnitIdle(int unit) = 0;


	/**
	 * @deprecated
	 * @see #RecvChatMessage
	 */
	virtual void GotChatMsg(const char* message, int player) {}


	/// called when a player sends a chat message
	virtual void RecvChatMessage(const char* message, int player) { GotChatMsg(message, player); }

	/// called when a Lua widget or unsynced gadget sends a message to this AI
	/// do not use outData, it is always NULL for interface-technical reasons
	virtual void RecvLuaMessage(const char* inData, const char** outData) { /* *outData = inData; */ }


	/// called when one of your units are damaged
	virtual void UnitDamaged(int damaged, int attacker, float damage, float3 dir) = 0;
	/// called when a ground unit failed to reach it's destination
	virtual void UnitMoveFailed(int unit) = 0;

	/// general messaging function to be used for future API extensions.
	virtual int HandleEvent(int msg, const void *data) = 0;

	/// called every frame
	virtual void Update() = 0;

	/// load ai from file
	virtual void Load(IGlobalAICallback* callback, std::istream* s) {}
	/// save ai to file
	virtual void Save(std::ostream* s) {}

	// use virtual instead of pure virtual,
	// because pure virtual is not well supported
	// among different OSs and compilers,
	// and pure virtual has no advantage
	// if we have other pure virtual functions
	// in the class
	virtual ~IGlobalAI() {}
};

} // namespace springLegacyAI

#endif // I_GLOBAL_AI_H
