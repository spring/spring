/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SKIRMISHAIWRAPPER_H
#define _SKIRMISHAIWRAPPER_H

#include "Object.h"
#include "SkirmishAIKey.h"
#include "Platform/SharedLib.h"

#include <map>
#include <string>

class CAICallback;
class CAICheats;
struct SSkirmishAICallback;
class CSkirmishAI;
struct Command;
class float3;

/**
 * Acts as an OO wrapper for a Skirmish AI instance.
 * Basically converts function calls to AIEvents,
 * which are then sent ot the AI.
 */
class CSkirmishAIWrapper : public CObject {
private:
	CR_DECLARE(CSkirmishAIWrapper);
	/// used only by creg
	CSkirmishAIWrapper();

	void CreateCallback();

public:
	CSkirmishAIWrapper(const size_t skirmishAIId);
	~CSkirmishAIWrapper();

	void Serialize(creg::ISerializer *s);
	void PostLoad();

	// AI Events
	virtual void Load(std::istream *s);
	virtual void Save(std::ostream *s);

	virtual void UnitIdle(int unitId);
	virtual void UnitCreated(int unitId, int builderId);
	virtual void UnitFinished(int unitId);
	virtual void UnitDestroyed(int unitId, int attackerUnitId);
	virtual void UnitDamaged(int unitId, int attackerUnitId, float damage, const float3& dir, int weaponDefId, bool paralyzer);
	virtual void UnitMoveFailed(int unitId);
	virtual void UnitGiven(int unitId, int oldTeam, int newTeam);
	virtual void UnitCaptured(int unitId, int oldTeam, int newTeam);
	virtual void EnemyCreated(int unitId);
	virtual void EnemyFinished(int unitId);
	virtual void EnemyEnterLOS(int unitId);
	virtual void EnemyLeaveLOS(int unitId);
	virtual void EnemyEnterRadar(int unitId);
	virtual void EnemyLeaveRadar(int unitId);
	virtual void EnemyDestroyed(int enemyUnitId, int attackerUnitId);
	virtual void EnemyDamaged(int enemyUnitId, int attackerUnitId, float damage, const float3& dir, int weaponDefId, bool paralyzer);
	virtual void Update(int frame);
	virtual void GotChatMsg(const char* msg, int fromPlayerId);
	virtual void WeaponFired(int unitId, int weaponDefId);
	virtual void PlayerCommandGiven(const std::vector<int>& selectedUnits, const Command& c, int playerId);
	virtual void CommandFinished(int unitId, int commandId, int commandTopicId);
	virtual void SeismicPing(int allyTeam, int unitId, const float3& pos, float strength);

	/** Called just before all the units are destroyed. */
	virtual void PreDestroy();

	virtual int GetTeamId() const;
	virtual const SkirmishAIKey& GetKey() const;
	virtual const SSkirmishAICallback* GetCallback() const;

	virtual void SetCheatEventsEnabled(bool enable);
	virtual bool IsCheatEventsEnabled() const;

	virtual void Init();
	void Dieing();
	/// @see SReleaseEvent in Interface/AISEvents.h
	virtual void Release(int reason = 0 /* = unspecified */);

	size_t GetSkirmishAIID() const { return skirmishAIId; }

private:
	size_t skirmishAIId;
	int teamId;
	bool cheatEvents;

	CSkirmishAI* ai;
	bool initialized, released;
	CAICallback* callback;
	CAICheats* cheats;
	SSkirmishAICallback* c_callback;
	SkirmishAIKey key;
	const struct InfoItem* info;
	unsigned int size_info;

private:
	bool LoadSkirmishAI(bool postLoad);
};

#endif // _SKIRMISHAIWRAPPER_H
