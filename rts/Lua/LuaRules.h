/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_RULES_H
#define LUA_RULES_H

#include <string>
using std::string;
#include <vector>
using std::vector;
#include <map>
using std::map;

#include "LuaHandleSynced.h"

#define MAX_LUA_COB_ARGS 10


class CUnit;
class CFeature;
class CProjectile;
class CWeapon;
struct UnitDef;
struct FeatureDef;
struct Command;
struct BuildInfo;
struct lua_State;


class CLuaRules : public CLuaHandleSynced
{
	public:
		static void LoadHandler();
		static void FreeHandler();

	public: // call-ins
		bool SyncedUpdateCallIn(lua_State *L, const string& name);
		bool UnsyncedUpdateCallIn(lua_State *L, const string& name);

		bool CommandFallback(const CUnit* unit, const Command& cmd);
		bool AllowCommand(const CUnit* unit, const Command& cmd, bool fromSynced);
		bool AllowUnitCreation(const UnitDef* unitDef,
		                       const CUnit* builder, const BuildInfo* buildInfo);
		bool AllowUnitTransfer(const CUnit* unit, int newTeam, bool capture);
		bool AllowUnitBuildStep(const CUnit* builder, const CUnit* unit, float part);
		bool AllowFeatureCreation(const FeatureDef* featureDef, int allyTeamID,
		                          const float3& pos);
		bool AllowFeatureBuildStep(const CUnit* builder, const CFeature* feature,
		                           float part);
		bool AllowResourceLevel(int teamID, const string& type, float level);
		bool AllowResourceTransfer(int oldTeam, int newTeam,
		                           const string& type, float amount);
		bool AllowDirectUnitControl(int playerID, const CUnit* unit);
		bool AllowStartPosition(int playerID, const float3& pos);

		bool TerraformComplete(const CUnit* unit, const CUnit* build);

		bool MoveCtrlNotify(const CUnit* unit, int data);

		void Cob2Lua(const LuaHashString& funcName, const CUnit* unit,
		             int& argsCount, int args[MAX_LUA_COB_ARGS]);

		int AllowWeaponTargetCheck(
			unsigned int attackerID,
			unsigned int attackerWeaponNum,
			unsigned int attackerWeaponDefID
		);
		bool AllowWeaponTarget(
			unsigned int attackerID,
			unsigned int targetID,
			unsigned int attackerWeaponNum,
			unsigned int attackerWeaponDefID,
			float* targetPriority
		);
		bool AllowWeaponInterceptTarget(
			const CUnit* interceptorUnit,
			const CWeapon* interceptorWeapon,
			const CProjectile* interceptorTarget
		);

		bool UnitPreDamaged(
			const CUnit* unit,
			const CUnit* attacker,
			float damage,
			int weaponDefID,
			int projectileID,
			bool paralyzer,
			float* newDamage,
			float* impulseMult);

		bool ShieldPreDamaged(const CProjectile*, const CWeapon*, const CUnit*, bool);

		// unsynced
		bool DrawUnit(const CUnit* unit);
		bool DrawFeature(const CFeature* feature);
		bool DrawShield(const CUnit* unit, const CWeapon* weapon);
		bool DrawProjectile(const CProjectile* projectile);

	private:
		CLuaRules();
		~CLuaRules();

	protected:
		bool AddSyncedCode(lua_State *L);
		bool AddUnsyncedCode(lua_State *L);

		int UnpackCobArg(lua_State* L);

	protected: // call-outs
		static int PermitHelperAIs(lua_State* L);

	private:
		bool haveCommandFallback;
		bool haveAllowCommand;
		bool haveAllowUnitCreation;
		bool haveAllowUnitTransfer;
		bool haveAllowUnitBuildStep;
		bool haveAllowFeatureCreation;
		bool haveAllowFeatureBuildStep;
		bool haveAllowResourceLevel;
		bool haveAllowResourceTransfer;
		bool haveAllowDirectUnitControl;
		bool haveAllowStartPosition;
		bool haveMoveCtrlNotify;
		bool haveTerraformComplete;
		bool haveUnitPreDamaged;
		bool haveShieldPreDamaged;
		bool haveAllowWeaponTargetCheck;
		bool haveAllowWeaponTarget;
		bool haveAllowWeaponInterceptTarget;

		bool haveDrawUnit;
		bool haveDrawFeature;
		bool haveDrawShield;
		bool haveDrawProjectile;

	private:
		static const int* currentCobArgs;
};


extern CLuaRules* luaRules;


#endif /* LUA_RULES_H */
