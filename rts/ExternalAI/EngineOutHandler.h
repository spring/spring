/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef ENGINE_OUT_HANDLER_H
#define ENGINE_OUT_HANDLER_H

#include "SkirmishAIWrapper.h"
#include "System/Object.h"
#include "Sim/Misc/GlobalConstants.h"

#include <array>
#include <vector>
#include <string>

struct Command;
class float3;
class CUnit;
class CGroup;
struct WeaponDef;
class SkirmishAIKey;
class CSkirmishAIWrapper;
struct SSkirmishAICallback;


class CEngineOutHandler {
	CR_DECLARE_STRUCT(CEngineOutHandler)


public:
	static CEngineOutHandler* GetInstance();

	static void Create();
	static void Destroy();

	void Init() { activeSkirmishAIs.reserve(16); }
	void Kill() {
		PreDestroy();

		// release leftover active AI's
		while (!activeSkirmishAIs.empty()) {
			DestroySkirmishAI(activeSkirmishAIs.back());
		}

		activeSkirmishAIs.clear();
	}

	/** Called just before all the units are destroyed. */
	void PreDestroy();

	void Update();

	/** Group should return false if it doenst want the unit for some reason. */
	bool UnitAddedToGroup(const CUnit& unit, const CGroup& group);
	/** No way to refuse giving up a unit. */
	void UnitRemovedFromGroup(const CUnit& unit, const CGroup& group);
	void UnitEnteredLos(const CUnit& unit, int allyTeamId);
	void UnitLeftLos(const CUnit& unit, int allyTeamId);
	void UnitEnteredRadar(const CUnit& unit, int allyTeamId);
	void UnitLeftRadar(const CUnit& unit, int allyTeamId);
	void UnitIdle(const CUnit& unit);
	void UnitCreated(const CUnit& unit, const CUnit* builder);
	void UnitFinished(const CUnit& unit);
	void UnitDestroyed(const CUnit& destroyed, const CUnit* attacker);
	void UnitDamaged(const CUnit& damaged, const CUnit* attacker, float damage, int weaponDefID, int projectileID, bool paralyzer);
	void UnitMoveFailed(const CUnit& unit);
	void UnitCaptured(const CUnit& unit, int oldTeam, int newTeam);
	void UnitGiven(const CUnit& unit, int oldTeam, int newTeam);

	void SeismicPing(int allyTeamId, const CUnit& unit, const float3& pos, float strength);
	void WeaponFired(const CUnit& unit, const WeaponDef& def);
	void PlayerCommandGiven(const std::vector<int>& selectedUnitIds, const Command& c, int playerId);

	/**
	 * A specific unit has finished a specific command,
	 * might be a good idea to give new orders to it.
	*/
	void CommandFinished(const CUnit& unit, const Command& command);
	void SendChatMessage(const char* msg, int playerId);

	/// send a raw string from unsynced Lua to one or all active skirmish AI's
	bool SendLuaMessages(int aiTeam, const char* inData, std::vector<const char*>& outData);


	// Skirmish AI stuff
	void CreateSkirmishAI(const uint8_t skirmishAIId);
	/**
	 * Sets a local Skirmish AI to block events.
	 * Do not call this if you want to kill a local AI, but use
	 * the Skirmish AI Handler instead.
	 * @param skirmishAIId index of the AI for which to block events
	 * @see CSkirmishAIHandler::SetLocalKillFlag()
	 * @see DestroySkirmishAI()
	 */
	void BlockSkirmishAIEvents(const uint8_t skirmishAIId) { hostSkirmishAIs[skirmishAIId].SetBlockEvents(true); }
	/**
	 * Destructs a local Skirmish AI for real.
	 * Do not call this if you want to kill a local AI, but use
	 * the Skirmish AI Handler instead.
	 * @param skirmishAIId index of the AI to destroy
	 * @see BlockSkirmishAIEvents()
	 * @see CSkirmishAIHandler::SetLocalKillFlag()
	 */
	void DestroySkirmishAI(const uint8_t skirmishAIId);

	void Load(std::istream* s, const uint8_t skirmishAIId);
	void Save(std::ostream* s, const uint8_t skirmishAIId);

private:
	/// Contains all local Skirmish AIs, indexed by their ID
	std::array<CSkirmishAIWrapper, MAX_AIS > hostSkirmishAIs;

	/**
	 * Array mapping team IDs to local Skirmish AI instances.
	 * There can be multiple Skirmish AIs per team.
	 */
	std::array<std::vector<uint8_t>, MAX_TEAMS> teamSkirmishAIs;

	std::vector<uint8_t> activeSkirmishAIs;
};

#define eoh CEngineOutHandler::GetInstance()

#endif // ENGINE_OUT_HANDLER_H
