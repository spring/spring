/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_HANDLE_SYNCED
#define LUA_HANDLE_SYNCED

#include <map>
#include <string>
using std::map;
using std::string;

#include "LuaHandle.h"
#include "LuaRulesParams.h"

struct lua_State;
class LuaSyncedCtrl;
class CLuaHandleSynced;
struct BuildInfo;


class CUnsyncedLuaHandle : public CLuaHandle
{
	friend class CLuaHandleSynced;

	public: // call-ins
		bool DrawUnit(const CUnit* unit);
		bool DrawFeature(const CFeature* feature);
		bool DrawShield(const CUnit* unit, const CWeapon* weapon);
		bool DrawProjectile(const CProjectile* projectile);

	public: // all non-eventhandler callins
		void RecvFromSynced(lua_State* srcState, int args); // not an engine call-in

	protected:
		CUnsyncedLuaHandle(CLuaHandleSynced* base, const string& name, int order);
		virtual ~CUnsyncedLuaHandle();

		bool Init(const string& code, const string& file);

		static CUnsyncedLuaHandle* GetUnsyncedHandle(lua_State* L) {
			assert(dynamic_cast<CUnsyncedLuaHandle*>(CLuaHandle::GetHandle(L)));
			return static_cast<CUnsyncedLuaHandle*>(CLuaHandle::GetHandle(L));
		}

	protected:
		CLuaHandleSynced& base;
};



class CSyncedLuaHandle : public CLuaHandle
{
	friend class CLuaHandleSynced;

	public: // call-ins
		bool CommandFallback(const CUnit* unit, const Command& cmd);
		bool AllowCommand(const CUnit* unit, const Command& cmd, bool fromSynced);

		bool AllowUnitCreation(const UnitDef* unitDef, const CUnit* builder, const BuildInfo* buildInfo);
		bool AllowUnitTransfer(const CUnit* unit, int newTeam, bool capture);
		bool AllowUnitBuildStep(const CUnit* builder, const CUnit* unit, float part);
		bool AllowFeatureCreation(const FeatureDef* featureDef, int allyTeamID, const float3& pos);
		bool AllowFeatureBuildStep(const CUnit* builder, const CFeature* feature, float part);
		bool AllowResourceLevel(int teamID, const string& type, float level);
		bool AllowResourceTransfer(int oldTeam, int newTeam, const string& type, float amount);
		bool AllowDirectUnitControl(int playerID, const CUnit* unit);
		bool AllowBuilderHoldFire(const CUnit* unit, int action);
		bool AllowStartPosition(int playerID, unsigned char readyState, const float3& clampedPos, const float3& rawPickPos);

		bool TerraformComplete(const CUnit* unit, const CUnit* build);
		bool MoveCtrlNotify(const CUnit* unit, int data);

		int AllowWeaponTargetCheck(unsigned int attackerID, unsigned int attackerWeaponNum, unsigned int attackerWeaponDefID);
		bool AllowWeaponTarget(
			unsigned int attackerID,
			unsigned int targetID,
			unsigned int attackerWeaponNum,
			unsigned int attackerWeaponDefID,
			float* targetPriority
		);
		bool AllowWeaponInterceptTarget(const CUnit* interceptorUnit, const CWeapon* interceptorWeapon, const CProjectile* interceptorTarget);

		bool UnitPreDamaged(
			const CUnit* unit,
			const CUnit* attacker,
			float damage,
			int weaponDefID,
			int projectileID,
			bool paralyzer,
			float* newDamage,
			float* impulseMult);

		bool FeaturePreDamaged(
			const CFeature* feature,
			const CUnit* attacker,
			float damage,
			int weaponDefID,
			int projectileID,
			float* newDamage,
			float* impulseMult);

		bool ShieldPreDamaged(const CProjectile*, const CWeapon*, const CUnit*, bool);

		bool SyncedActionFallback(const string& line, int playerID);

	protected:
		CSyncedLuaHandle(CLuaHandleSynced* base, const string& name, int order);
		virtual ~CSyncedLuaHandle();

		bool Init(const string& code, const string& file);

		static CSyncedLuaHandle* GetSyncedHandle(lua_State* L) {
			assert(dynamic_cast<CSyncedLuaHandle*>(CLuaHandle::GetHandle(L)));
			return static_cast<CSyncedLuaHandle*>(CLuaHandle::GetHandle(L));
		}

	protected:
		CLuaHandleSynced& base;

		map<string, string> textCommands; // name, help

	private:
		int origNextRef;

	private: // call-outs
		static int SyncedRandom(lua_State* L);
		static int SyncedRandomSeed(lua_State* L);

		static int SyncedNext(lua_State* L);
		static int SyncedPairs(lua_State* L);

		static int SendToUnsynced(lua_State* L);

		static int AddSyncedActionFallback(lua_State* L);
		static int RemoveSyncedActionFallback(lua_State* L);

		static int GetWatchUnitDef(lua_State* L);
		static int SetWatchUnitDef(lua_State* L);
		static int GetWatchFeatureDef(lua_State* L);
		static int SetWatchFeatureDef(lua_State* L);
		static int GetWatchWeaponDef(lua_State* L);
		static int SetWatchWeaponDef(lua_State* L);
};


class CLuaHandleSynced
{
	public: // Non-eventhandler call-ins
		bool GotChatMsg(const string& msg, int playerID) {
			return syncedLuaHandle.GotChatMsg(msg, playerID) ||
				unsyncedLuaHandle.GotChatMsg(msg, playerID);
		}

		bool RecvLuaMsg(const string& msg, int playerID) {
			return syncedLuaHandle.RecvLuaMsg(msg, playerID);
		}

	public:
		void CheckStack() {
			syncedLuaHandle.CheckStack();
			unsyncedLuaHandle.CheckStack();
		}

		static CUnsyncedLuaHandle* GetUnsyncedHandle(lua_State* L) {
			if (CLuaHandle::GetHandleSynced(L)) {
				auto slh = CSyncedLuaHandle::GetSyncedHandle(L);
				return &slh->base.unsyncedLuaHandle;
			} else {
				return CUnsyncedLuaHandle::GetUnsyncedHandle(L);
			}
		}

		static CSyncedLuaHandle* GetSyncedHandle(lua_State* L) {
			if (CLuaHandle::GetHandleSynced(L)) {
				return CSyncedLuaHandle::GetSyncedHandle(L);
			} else {
				auto ulh = CUnsyncedLuaHandle::GetUnsyncedHandle(L);
				return &ulh->base.syncedLuaHandle;
			}
		}

	protected:
		CLuaHandleSynced(const string& name, int order);
		virtual ~CLuaHandleSynced();

		string LoadFile(const string& filename, const string& modes) const;
		void Init(const string& syncedFile, const string& unsyncedFile, const string& modes);

		bool IsValid() const {
			return syncedLuaHandle.IsValid() && unsyncedLuaHandle.IsValid();
		}
		void KillLua() {
			syncedLuaHandle.KillLua();
			unsyncedLuaHandle.KillLua();
		}

	#define SET_PERMISSION(name, type) \
		void Set ## name(const type arg) { \
			syncedLuaHandle.Set ## name(arg); \
			unsyncedLuaHandle.Set ## name(arg); \
		}

		SET_PERMISSION(FullCtrl, bool);
		SET_PERMISSION(FullRead, bool);
		SET_PERMISSION(CtrlTeam, int);
		SET_PERMISSION(ReadTeam, int);
		SET_PERMISSION(ReadAllyTeam, int);
		SET_PERMISSION(SelectTeam, int);

	#undef SET_PERMISSION

	protected:
		friend class CUnsyncedLuaHandle;
		friend class CSyncedLuaHandle;

		// hooks to add code during initialization
		virtual bool AddSyncedCode(lua_State* L) = 0;
		virtual bool AddUnsyncedCode(lua_State* L) = 0;

		// call-outs
		static int LoadStringData(lua_State* L);
		static int CallAsTeam(lua_State* L);

	public:
		CSyncedLuaHandle syncedLuaHandle;
		CUnsyncedLuaHandle unsyncedLuaHandle;

	public:
		static const LuaRulesParams::Params&  GetGameParams() { return gameParams; }
		static const LuaRulesParams::HashMap& GetGameParamsMap() { return gameParamsMap; }

	private:
		//FIXME: add to CREG?
		static LuaRulesParams::Params  gameParams;
		static LuaRulesParams::HashMap gameParamsMap;
		friend class LuaSyncedCtrl;
};


#endif /* LUA_HANDLE_SYNCED */
