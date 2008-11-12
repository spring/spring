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

//#ifndef _GROUPAIWRAPPER_H
//#define _GROUPAIWRAPPER_H
//
//#include "Object.h"
//#include "IGroupAI.h"
//#include "GlobalAICallback.h"
//#include "Interface/SAICallback.h"
//#include "Interface/SAIInterfaceLibrary.h"
//#include "Platform/SharedLib.h"
//
//#include <map>
//#include <string>
//
//class CAICallback;
//struct Command;
//struct float3;
//
//class CGroupAIWrapper : public CObject, public IGroupAI {
//public:
//	CR_DECLARE(CGroupAIWrapper);
//
//	CGroupAIWrapper(int teamId, int groupId, const SGAIKey& key,
//			const std::map<std::string, std::string>& options = (std::map<std::string, std::string>()));
//	~CGroupAIWrapper();
//
//	void Serialize(creg::ISerializer *s);
//	void PostLoad();
//
//	// AI Events
//	virtual void Load(std::istream *s);
//	virtual void Save(std::ostream *s);
//
//	// Events in common with Skirmish AIs
//	virtual void UnitIdle(int unitId);
//	virtual void UnitCreated(int unitId);
//	virtual void UnitFinished(int unitId);
//	virtual void UnitDestroyed(int unitId, int attackerUnitId);
//	virtual void UnitDamaged(int unitId, int attackerUnitId, float damage, const float3& dir);
//	virtual void UnitMoveFailed(int unitId);
//	virtual void UnitGiven(int unitId, int oldTeam, int newTeam);
//	virtual void UnitCaptured(int unitId, int oldTeam, int newTeam);
//	virtual void EnemyEnterLOS(int unitId);
//	virtual void EnemyLeaveLOS(int unitId);
//	virtual void EnemyEnterRadar(int unitId);
//	virtual void EnemyLeaveRadar(int unitId);
//	virtual void EnemyDestroyed(int enemyUnitId, int attackerUnitId);
//	virtual void EnemyDamaged(int enemyUnitId, int attackerUnitId, float damage, const float3& dir);
//	virtual void Update(int frame);
////	virtual void Update();				//called once a frame (30 times a second)
//	virtual void GotChatMsg(const char* msg, int fromPlayerId);
//	virtual void WeaponFired(int unitId, int weaponDefId);
//	virtual void PlayerCommandGiven(const std::vector<int>& selectedUnits, const Command& c, int playerId);
////	virtual void GiveCommand(Command* c);		//the group is given a command by the player
//	virtual void SeismicPing(int allyTeam, int unitId, const float3& pos, float strength);
//
//	// Group AI specific Events
//	virtual bool AddUnit(int unitId);		//group should return false if it doenst want the unit for some reason
//	virtual void RemoveUnit(int unitId);		//no way to refuse giving up a unit
//	//the ai tells the interface what commands it can take (note that it returns a reference so it must keep the vector in memory itself)
//	virtual const std::vector<CommandDescription>& GetPossibleCommands();
//	virtual int GetDefaultCmd(int unitId);	//the default command for the ai given that the mouse pointer hovers above unit unitid (or no unit if unitid=0)
//	virtual void CommandFinished(int unitId, int commandTopic);	//a specific unit has finished a specific command, might be a good idea to give new orders to it
//	virtual void DrawCommands();				//the place to use the LineDrawer interface functions
//
//
//	virtual void PreDestroy(); // called just before all the units are destroyed
//
//	virtual int GetTeamId() const;
//	virtual int GetGroupId() const;
//
//	virtual void SetCheatEventsEnabled(bool enable);
//	virtual bool IsCheatEventsEnabled() const;
//
//
//	/**
//	 * Inherited form IGroupAI.
//	 * CAUTION: takes C AI Interface events, not engine C++ ones!
//	 */
//	virtual int HandleEvent(int topic, const void* data) const;
//
//private:
//	virtual void Init();
//	virtual void Release();
//
//private:
//	int teamId;
//	int groupId;
//	bool cheatEvents;
//
////	bool loadSupported;
//
//	IGroupAI* ai;
//	CGroupAICallback* callback;
//	SAICallback* c_callback;
//	SGAIKey key;
//	std::vector<std::string> optionKeys;
//	std::vector<std::string> optionValues;
//	const char** optionKeys_c;
//	const char** optionValues_c;
//
//private:
//	void LoadGroupAI(bool postLoad);
//};
//
//#endif // _GROUPAIWRAPPER_H
