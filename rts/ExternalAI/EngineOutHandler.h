/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _ENGINEOUTHANDLER_H
#define _ENGINEOUTHANDLER_H

#include "Object.h"
#include "Sim/Misc/GlobalConstants.h"

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
private:
	CR_DECLARE(CEngineOutHandler);

	CEngineOutHandler();
	~CEngineOutHandler();

public:
	/**
	 * This function initialized a singleton instance,
	 * if not yet done by a call to GetInstance()
	 */
	static void Initialize();
	static CEngineOutHandler* GetInstance();
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
	void UnitDamaged(const CUnit& damaged, const CUnit* attacker, float damage, int weaponId, bool paralyzer);
	void UnitMoveFailed(const CUnit& unit);
	void UnitCaptured(const CUnit& unit, int newTeamId);
	void UnitGiven(const CUnit& unit, int oldTeamId);

	void SeismicPing(int allyTeamId, const CUnit& unit, const float3& pos, float strength);
	void WeaponFired(const CUnit& unit, const WeaponDef& def);
	void PlayerCommandGiven(const std::vector<int>& selectedUnitIds, const Command& c, int playerId);
	/**
	 * A specific unit has finished a specific command,
	 * might be a good idea to give new orders to it.
	*/
	void CommandFinished(const CUnit& unit, const Command& command);
	void GotChatMsg(const char* msg, int playerId);


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


	void SetCheating(bool enable);
	bool IsCheating() const;


	void Load(std::istream* s);
	void Save(std::ostream* s);

	/**
	 * Should exceptions thrown by AIs be caught by the engine?
	 * In practice, an exception thrown by an AI should be caught in the AI,
	 * or at least the interface plugin, and should therefore never reach
	 * the engine. The reason for this is, that we do not know if the engine
	 * and the AI are compiled with the same compiler/exception system.
	 * Shorlty: catching AI exceptions in the engine is deprecated
	 */
	static bool IsCatchExceptions();
	/**
	 * This is used inside a catch block for a try guarding a call form
	 * the engine into an AI.
	 * There is a macro defined for the catch part, so you should never have
	 * to call this method directly.
	 * @see CATCH_AI_EXCEPTION
	 */
	static void HandleAIException(const char* description);
	/**
	 * Catches a common set of exceptions thrown by AIs.
	 * Use like this:
	 * <code>
	 * try {
	 *     ai->sendEvent(evt);
	 * } CATCH_AI_EXCEPTION
	 * </code>
	 * @see HandleAIException()
	 */
	#define CATCH_AI_EXCEPTION									\
		catch (const std::exception& e) {						\
			CEngineOutHandler::HandleAIException(e.what());		\
			throw e;											\
		} catch (const std::string& s) {						\
			CEngineOutHandler::HandleAIException(s.c_str());	\
			throw s;											\
		} catch (const char* s) {								\
			CEngineOutHandler::HandleAIException(s);			\
			throw s;											\
		} catch (int err) {										\
			const std::string s = IntToString(err);				\
			CEngineOutHandler::HandleAIException(s.c_str());	\
			throw err;											\
		} catch (...) {											\
			CEngineOutHandler::HandleAIException("Unknown");	\
			throw;												\
		}

private:
	static CEngineOutHandler* singleton;

private:
	typedef std::vector<size_t> ids_t;

	typedef std::map<size_t, CSkirmishAIWrapper*> id_ai_t;
	/// Contains all local Skirmish AIs, indexed by their ID
	id_ai_t id_skirmishAI;

	typedef std::map<int, ids_t> team_ais_t;
	/**
	 * Array mapping team IDs to local Skirmish AI instances.
	 * There can be multiple Skirmish AIs per team.
	 */
	team_ais_t team_skirmishAIs;
};

#define eoh CEngineOutHandler::GetInstance()

#endif // _ENGINEOUTHANDLER_H
