#ifndef LUA_RULES_H
#define LUA_RULES_H
// LuaRules.h: interface for the CLuaRules class.
//
//////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include <string>
using std::string;
#include <vector>
using std::vector;
#include <map>
using std::map;

#include "LuaHandleSynced.h"


class CUnit;
class CFeature;
struct UnitDef;
struct FeatureDef;
struct Command;
struct lua_State;


class CLuaRules : public CLuaHandleSynced
{
	public:
		static void LoadHandler();
		static void FreeHandler();

		static bool SetConfigString(const string& cfg);
		static const string& GetConfigString() { return configString; }

		static const vector<float>&    GetGameParams();
		static const map<string, int>& GetGameParamsMap();

		const map<string, string>& GetInfoMap() const { return infoMap; }

	public: // call-ins
		bool SyncedUpdateCallIn(const string& name);
		bool UnsyncedUpdateCallIn(const string& name);

		bool CommandFallback(const CUnit* unit, const Command& cmd);
		bool AllowCommand(const CUnit* unit, const Command& cmd);
		bool AllowUnitCreation(const UnitDef* unitDef,
		                       const CUnit* builder, const float3* pos);
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

	private:
		CLuaRules();
		~CLuaRules();

	protected:
		bool AddSyncedCode();
		bool AddUnsyncedCode();

		static void SetRulesParam(lua_State* L, const char* caller, int offset,
		                          vector<float>& params,
		                          map<string, int>& paramsMap);
		static void CreateRulesParams(lua_State* L, const char* caller, int offset,
		                              vector<float>& params,
		                              map<string, int>& paramsMap);

	protected: // call-outs
		static int GetConfigString(lua_State* L);

		static int PermitHelperAIs(lua_State* L);

		static int SetRulesInfoMap(lua_State* L);

		static int SetUnitRulesParam(lua_State* L);
		static int CreateUnitRulesParams(lua_State* L);

		static int SetTeamRulesParam(lua_State* L);
		static int CreateTeamRulesParams(lua_State* L);

		static int SetGameRulesParam(lua_State* L);
		static int CreateGameRulesParams(lua_State* L);

	protected:
		static void CobCallback(int retCode, void* p1, void* p2);

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

		map<string, string> infoMap;

	private:
		static string configString;
		static vector<float>    gameParams;
		static map<string, int> gameParamsMap;
};


extern CLuaRules* luaRules;


#endif /* LUA_RULES_H */
