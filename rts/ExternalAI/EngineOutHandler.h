/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef ENGINE_OUT_HANDLER_H
#define ENGINE_OUT_HANDLER_H

#include "System/Object.h"
#include "Sim/Misc/GlobalConstants.h"

#include <memory>

#include <map>
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


void handleAIException(const char* description);

class CEngineOutHandler : public CObject {
	CR_DECLARE(CEngineOutHandler)


public:
	static CEngineOutHandler* GetInstance();
	~CEngineOutHandler();

	static void Create();
	static void Destroy();

	void PostLoad();
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
	void CreateSkirmishAI(const size_t skirmishAIId);
	/**
	 * Sets a local Skirmish AI dieing.
	 * Do not call this if you want to kill a local AI, but use
	 * the Skirmish AI Handler instead.
	 * @param skirmishAIId index of the AI to mark as dieing
	 * @see CSkirmishAIHandler::SetLocalSkirmishAIDieing()
	 * @see DestroySkirmishAI()
	 */
	void SetSkirmishAIDieing(const size_t skirmishAIId);
	/**
	 * Destructs a local Skirmish AI for real.
	 * Do not call this if you want to kill a local AI, but use
	 * the Skirmish AI Handler instead.
	 * @param skirmishAIId index of the AI to destroy
	 * @see SetSkirmishAIDieing()
	 * @see CSkirmishAIHandler::SetLocalSkirmishAIDieing()
	 */
	void DestroySkirmishAI(const size_t skirmishAIId);

	void Load(std::istream* s);
	void Save(std::ostream* s);

private:
	typedef std::vector<uint8_t> ids_t;
	typedef std::map<uint8_t, std::unique_ptr<CSkirmishAIWrapper> > id_ai_t;
	typedef std::map<int, ids_t> team_ais_t;

	/// Contains all local Skirmish AIs, indexed by their ID
	id_ai_t id_skirmishAI;

	/**
	 * Array mapping team IDs to local Skirmish AI instances.
	 * There can be multiple Skirmish AIs per team.
	 */
	team_ais_t team_skirmishAIs;
};

#define eoh CEngineOutHandler::GetInstance()

#endif // ENGINE_OUT_HANDLER_H
