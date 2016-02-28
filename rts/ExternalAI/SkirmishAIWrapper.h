/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SKIRMISH_AI_WRAPPER_H
#define SKIRMISH_AI_WRAPPER_H

#include "System/Object.h"
#include "SkirmishAIKey.h"
#include "System/Platform/SharedLib.h"

#include <map>
#include <string>
#include <memory>

class CAICallback;
class CAICheats;
class CSkirmishAILibrary;
struct SSkirmishAICallback;

struct Command;
class float3;


/**
 * Acts as an OO wrapper for a Skirmish AI instance.
 * Basically converts function calls to AIEvents,
 * which are then sent ot the AI.
 */
class CSkirmishAIWrapper : public CObject {
private:
	CR_DECLARE(CSkirmishAIWrapper)

	void CreateCallback();

public:
	/// used only by creg
	CSkirmishAIWrapper();

	CSkirmishAIWrapper(const int skirmishAIId);
	~CSkirmishAIWrapper();

	void Serialize(creg::ISerializer *s);
	void PostLoad();


	/**
	 * Initialize the AI instance.
	 * This calls the native init() method, the InitAIEvent is sent afterwards.
	 */
	void Init();

	/// @see SReleaseEvent in Interface/AISEvents.h
	void Release(int reason = 0 /* = unspecified */);

	/** Called just before all the units are destroyed. */
	void PreDestroy();

	/**
	 * No events are forwarded to the Skirmish AI plugin
	 * after this method has been called.
	 * Do not call this if you want to kill a local AI, but use
	 * the Skirmish AI Handler instead.
	 * @see CSkirmishAIHandler::SetLocalSkirmishAIDieing()
	 */
	void Dieing() { dieing = true; }


	// AI Events
	void Load(std::istream *s);
	void Save(std::ostream *s);

	void UnitIdle(int unitId);
	void UnitCreated(int unitId, int builderId);
	void UnitFinished(int unitId);
	void UnitDestroyed(int unitId, int attackerUnitId);
	void UnitDamaged(int unitId, int attackerUnitId, float damage, const float3& dir, int weaponDefId, bool paralyzer);
	void UnitMoveFailed(int unitId);
	void UnitGiven(int unitId, int oldTeam, int newTeam);
	void UnitCaptured(int unitId, int oldTeam, int newTeam);
	void EnemyCreated(int unitId);
	void EnemyFinished(int unitId);
	void EnemyEnterLOS(int unitId);
	void EnemyLeaveLOS(int unitId);
	void EnemyEnterRadar(int unitId);
	void EnemyLeaveRadar(int unitId);
	void EnemyDestroyed(int enemyUnitId, int attackerUnitId);
	void EnemyDamaged(int enemyUnitId, int attackerUnitId, float damage, const float3& dir, int weaponDefId, bool paralyzer);
	void Update(int frame);
	void SendChatMessage(const char* msg, int fromPlayerId);
	void SendLuaMessage(const char* inData, const char** outData);
	void WeaponFired(int unitId, int weaponDefId);
	void PlayerCommandGiven(const std::vector<int>& selectedUnits, const Command& c, int playerId);
	void CommandFinished(int unitId, int commandId, int commandTopicId);
	void SeismicPing(int allyTeam, int unitId, const float3& pos, float strength);

	int GetSkirmishAIID() const { return skirmishAIId; }
	int GetTeamId() const { return teamId; }
	const SkirmishAIKey& GetKey() const { return key; }

	void SetCheatEventsEnabled(bool enable) { cheatEvents = enable; }
	bool IsCheatEventsEnabled() const { return cheatEvents; }

private:
	bool LoadSkirmishAI(bool postLoad);

	/**
	 * CAUTION: takes C AI Interface events, not engine C++ ones!
	 */
	int HandleEvent(int topic, const void* data) const;

private:
	SkirmishAIKey key;

	const CSkirmishAILibrary* library;
	const SSkirmishAICallback* sCallback;

	std::unique_ptr<CAICallback> callback;
	std::unique_ptr<CAICheats> cheats;

	std::string timerName;


	int skirmishAIId;
	int teamId;

	bool initialized;
	bool released;
	bool cheatEvents;

	bool initOk;
	bool dieing;
};

#endif // SKIRMISH_AI_WRAPPER_H
