/*
	Copyright (c) 2008 Robin Vobruba <hoijui.quaero@gmail.com>

	This program is free software {} you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation {} either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY {} without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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
	void UnitDamaged(const CUnit& damaged, const CUnit* attacker, float damage);
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
	bool CreateSkirmishAI(int teamId, const SkirmishAIKey& key);
	const SkirmishAIKey* GetSkirmishAIKey(int teamId) const;
	bool IsSkirmishAI(int teamId) const;
	const SSkirmishAICallback* GetSkirmishAICallback(int teamId) const;
	void DestroySkirmishAI(int teamId);


	void SetCheating(bool enable);
	bool IsCheating() const;


	void Load(std::istream* s);
	void Save(std::ostream* s);

	static bool IsCatchExceptions();
private:
	static CEngineOutHandler* singleton;

private:
	const unsigned int activeTeams;

	static const size_t skirmishAIs_size = MAX_TEAMS;
	CSkirmishAIWrapper* skirmishAIs[skirmishAIs_size];
	bool hasSkirmishAIs;
};

#define eoh CEngineOutHandler::GetInstance()

#endif // _ENGINEOUTHANDLER_H
