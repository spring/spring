/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_HANDLE_H
#define LUA_HANDLE_H
#include <boost/cstdint.hpp>

#include "System/EventClient.h"
//FIXME#include "LuaArrays.h"
#include "LuaContextData.h"
#include "LuaHashString.h"
#include "lib/lua/include/LuaInclude.h" //FIXME needed for GetLuaContextData

#include <string>
#include <vector>
#include <set>
using std::string;
using std::vector;
using std::set;


#define LUA_HANDLE_ORDER_RULES            100
#define LUA_HANDLE_ORDER_UNITS            200
#define LUA_HANDLE_ORDER_GAIA             300
#define LUA_HANDLE_ORDER_RULES_UNSYNCED  1100
#define LUA_HANDLE_ORDER_UNITS_UNSYNCED  1200
#define LUA_HANDLE_ORDER_GAIA_UNSYNCED   1300
#define LUA_HANDLE_ORDER_UI              2000
#define LUA_HANDLE_ORDER_INTRO           3000


class CUnit;
class CWeapon;
class CFeature;
class CProjectile;
struct Command;
struct SRectangle;
struct LuaHashString;
struct lua_State;
class LuaRBOs;
class LuaFBOs;
class LuaTextures;
class LuaShaders;
class CLuaDisplayLists;
class CLuaRules;


class CLuaHandle : public CEventClient
{
	public:
		void CheckStack();
		int GetCallInErrors() const { return callinErrors; }
		void ResetCallinErrors() { callinErrors = 0; }

	public:
	#define PERMISSIONS_FUNCS(Name, type, dataArg) \
		void Set ## Name(type _ ## dataArg) { GetLuaContextData(L)->dataArg = _ ## dataArg; } \
		type Get ## Name() const { return GetLuaContextData(L)->dataArg; } \
		static void SetHandle ## Name(const lua_State* L, type _ ## dataArg) { GetLuaContextData(L)->dataArg = _ ## dataArg;; } \
		static type GetHandle ## Name(const lua_State* L) { return GetLuaContextData(L)->dataArg; }

		PERMISSIONS_FUNCS(FullRead,     bool, fullRead); // virtual function in CEventClient
		PERMISSIONS_FUNCS(FullCtrl,     bool, fullCtrl);
		PERMISSIONS_FUNCS(CtrlTeam,     int,  ctrlTeam);
		PERMISSIONS_FUNCS(ReadTeam,     int,  readTeam);
		PERMISSIONS_FUNCS(ReadAllyTeam, int,  readAllyTeam); // virtual function in CEventClient
		PERMISSIONS_FUNCS(SelectTeam,   int,  selectTeam);

	#undef PERMISSIONS_FUNCS

		static bool GetHandleSynced(const lua_State* L) { return GetLuaContextData(L)->synced; }

		bool GetUserMode() const { return userMode; }
		static bool GetHandleUserMode(const lua_State* L) { return GetLuaContextData(L)->owner->GetUserMode(); }

		bool CheckModUICtrl() const { return GetModUICtrl() || GetUserMode(); }
		static bool CheckModUICtrl(lua_State* L) { return GetModUICtrl() || GetHandleUserMode(L); }

		static int GetHandleAllowChanges(const lua_State* L) { return GetLuaContextData(L)->allowChanges; }

		static CLuaHandle* GetHandle(lua_State* L) { return GetLuaContextData(L)->owner; }

		static void SetHandleRunning(lua_State* L, const bool _running) {
			GetLuaContextData(L)->running += (_running) ? +1 : -1;
			assert( GetLuaContextData(L)->running >= 0);
		}
		static bool IsHandleRunning(lua_State* L) { return (GetLuaContextData(L)->running > 0); }
		bool IsRunning() const { return IsHandleRunning(L); }

		bool IsValid() const { return (L != NULL); }

		//FIXME needed by LuaSyncedTable (can be solved cleaner?)
		lua_State* GetLuaState() const { return L; }

		LuaShaders& GetShaders(const lua_State* L = NULL) { return GetLuaContextData(L)->shaders; }
		LuaTextures& GetTextures(const lua_State* L = NULL) { return GetLuaContextData(L)->textures; }
		LuaFBOs& GetFBOs(const lua_State* L = NULL) { return GetLuaContextData(L)->fbos; }
		LuaRBOs& GetRBOs(const lua_State* L = NULL) { return GetLuaContextData(L)->rbos; }
		CLuaDisplayLists& GetDisplayLists(const lua_State* L = NULL) { return GetLuaContextData(L)->displayLists; }

	public: // call-ins
		bool WantsEvent(const string& name) { return HasCallIn(L, name); }
		virtual bool HasCallIn(lua_State* L, const string& name);
		virtual bool UpdateCallIn(lua_State* L, const string& name);

		void Load(IArchive* archive);

		void GamePreload();
		void GameStart();
		void GameOver(const std::vector<unsigned char>& winningAllyTeams);
		void GamePaused(int playerID, bool paused);
		void GameFrame(int frameNum);
		void GameID(const unsigned char* gameID, unsigned int numBytes);

		void TeamDied(int teamID);
		void TeamChanged(int teamID);
		void PlayerChanged(int playerID);
		void PlayerAdded(int playerID);
		void PlayerRemoved(int playerID, int reason);

		void UnitCreated(const CUnit* unit, const CUnit* builder);
		void UnitFinished(const CUnit* unit);
		void UnitFromFactory(const CUnit* unit, const CUnit* factory,
		                     bool userOrders);
		void UnitDestroyed(const CUnit* unit, const CUnit* attacker);
		void UnitTaken(const CUnit* unit, int oldTeam, int newTeam);
		void UnitGiven(const CUnit* unit, int oldTeam, int newTeam);

		void UnitIdle(const CUnit* unit);
		void UnitCommand(const CUnit* unit, const Command& command);
		void UnitCmdDone(const CUnit* unit, const Command& command);
		void UnitDamaged(
			const CUnit* unit,
			const CUnit* attacker,
			float damage,
			int weaponDefID,
			int projectileID,
			bool paralyzer);
		void UnitExperience(const CUnit* unit, float oldExperience);
		void UnitHarvestStorageFull(const CUnit* unit);

		void UnitSeismicPing(const CUnit* unit, int allyTeam,
		                     const float3& pos, float strength);
		void UnitEnteredRadar(const CUnit* unit, int allyTeam);
		void UnitEnteredLos(const CUnit* unit, int allyTeam);
		void UnitLeftRadar(const CUnit* unit, int allyTeam);
		void UnitLeftLos(const CUnit* unit, int allyTeam);

		void UnitEnteredWater(const CUnit* unit);
		void UnitEnteredAir(const CUnit* unit);
		void UnitLeftWater(const CUnit* unit);
		void UnitLeftAir(const CUnit* unit);

		void UnitLoaded(const CUnit* unit, const CUnit* transport);
		void UnitUnloaded(const CUnit* unit, const CUnit* transport);

		void UnitCloaked(const CUnit* unit);
		void UnitDecloaked(const CUnit* unit);

		void UnitUnitCollision(const CUnit* collider, const CUnit* collidee);
		void UnitFeatureCollision(const CUnit* collider, const CFeature* collidee);
		void UnitMoveFailed(const CUnit* unit);

		void FeatureCreated(const CFeature* feature);
		void FeatureDestroyed(const CFeature* feature);
		void FeatureDamaged(
			const CFeature* feature,
			const CUnit* attacker,
			float damage,
			int weaponDefID,
			int projectileID);

		void ProjectileCreated(const CProjectile* p);
		void ProjectileDestroyed(const CProjectile* p);

		bool Explosion(int weaponID, int projectileID, const float3& pos, const CUnit* owner);

		void StockpileChanged(const CUnit* owner,
		                      const CWeapon* weapon, int oldCount);

		void Save(zipFile archive);

		void UnsyncedHeightMapUpdate(const SRectangle& rect);
		void Update();

		bool KeyPress(int key, bool isRepeat);
		bool KeyRelease(int key);
		bool TextInput(const std::string& utf8);
		bool MouseMove(int x, int y, int dx, int dy, int button);
		bool MousePress(int x, int y, int button);
		void MouseRelease(int x, int y, int button);
		bool MouseWheel(bool up, float value);
		bool JoystickEvent(const std::string& event, int val1, int val2);
		bool IsAbove(int x, int y);
		string GetTooltip(int x, int y);

		bool DefaultCommand(const CUnit* unit, const CFeature* feature, int& cmd);

		bool CommandNotify(const Command& cmd);

		bool AddConsoleLine(const string& msg, const string& section, int level);

		bool GroupChanged(int groupID);

		bool GameSetup(const string& state, bool& ready,
		               const map<int, string>& playerStates);

		const char* RecvSkirmishAIMessage(int aiID, const char* data, int inSize);

		string WorldTooltip(const CUnit* unit,
		                    const CFeature* feature,
		                    const float3* groundPos);

		bool MapDrawCmd(int playerID, int type,
		                const float3* pos0,
		                const float3* pos1,
		                const string* labe);

		void ViewResize();

		void DrawGenesis();
		void DrawWorld();
		void DrawWorldPreUnit();
		void DrawWorldShadow();
		void DrawWorldReflection();
		void DrawWorldRefraction();
		void DrawScreenEffects();
		void DrawScreen();
		void DrawInMiniMap();
		void DrawInMiniMapBackground();

		void GameProgress(int frameNum);
		//FIXME void MetalMapChanged(const int x, const int z);

		void CollectGarbage();

	public: // Non-eventhandler call-ins
		void Shutdown();
		bool GotChatMsg(const string& msg, int playerID);
		bool RecvLuaMsg(const string& msg, int playerID);

	public: // custom call-in  (inter-script calls)
		bool HasXCall(const string& funcName) { return HasCallIn(L, funcName); }
		int XCall(lua_State* srcState, const string& funcName);

	protected:
		CLuaHandle(const string& name, int order, bool userMode, bool synced);
		virtual ~CLuaHandle();

		void KillLua();

		static void PushTracebackFuncToRegistry(lua_State* L);

		bool AddBasicCalls(lua_State* L);
		bool LoadCode(lua_State* L, const string& code, const string& debug);
		static bool AddEntriesToTable(lua_State* L, const char* name, bool (*entriesFunc)(lua_State*));

		/// returns error code and sets traceback on error
		int  RunCallInTraceback(lua_State* L, const LuaHashString* hs, int inArgs, int outArgs, int errFuncIndex, std::string& tracebackMsg, bool popErrFunc);
		/// returns false and prints message to log on error
		bool RunCallInTraceback(lua_State* L, const LuaHashString& hs, int inArgs, int outArgs, int errFuncIndex, bool popErrFunc = true);
		/// returns false and and sets errormessage on error
		bool RunCallIn(lua_State* L, int inArgs, int outArgs, std::string& errormessage);
		/// returns false and prints message to log on error
		bool RunCallIn(lua_State* L, const LuaHashString& hs, int inArgs, int outArgs);

		void LosCallIn(const LuaHashString& hs, const CUnit* unit, int allyTeam);
		void UnitCallIn(const LuaHashString& hs, const CUnit* unit);

		void RunDrawCallIn(const LuaHashString& hs);

	protected:
		bool userMode;

		lua_State* L;
		luaContextData D;

		lua_State* L_GC;

		bool killMe;
		string killMsg;

		vector<bool> watchUnitDefs;
		vector<bool> watchFeatureDefs;
		vector<bool> watchWeaponDefs; // for the Explosion call-in

		int callinErrors;

	private: // call-outs
		static int KillActiveHandle(lua_State* L);
		static int CallOutGetName(lua_State* L);
		static int CallOutGetSynced(lua_State* L);
		static int CallOutGetFullCtrl(lua_State* L);
		static int CallOutGetFullRead(lua_State* L);
		static int CallOutGetCtrlTeam(lua_State* L);
		static int CallOutGetReadTeam(lua_State* L);
		static int CallOutGetReadAllyTeam(lua_State* L);
		static int CallOutGetSelectTeam(lua_State* L);
		static int CallOutGetGlobal(lua_State* L);
		static int CallOutGetRegistry(lua_State* L);
		static int CallOutGetCallInList(lua_State* L);
		static int CallOutUpdateCallIn(lua_State* L);
		static int CallOutIsEngineMinVersion(lua_State* L);

	public: // static
		static inline LuaShaders& GetActiveShaders(lua_State* L)  { return GetLuaContextData(L)->shaders; }
		static inline LuaTextures& GetActiveTextures(lua_State* L) { return GetLuaContextData(L)->textures; }
		static inline LuaFBOs& GetActiveFBOs(lua_State* L) { return GetLuaContextData(L)->fbos; }
		static inline LuaRBOs& GetActiveRBOs(lua_State* L)     { return GetLuaContextData(L)->rbos; }
		static inline CLuaDisplayLists& GetActiveDisplayLists(lua_State* L) { return GetLuaContextData(L)->displayLists; }

		static void SetDevMode(bool value) { devMode = value; }
		static bool GetDevMode() { return devMode; }

		static void SetModUICtrl(bool value) { modUICtrl = value; }
		static bool GetModUICtrl() { return modUICtrl; }

		static void HandleLuaMsg(int playerID, int script, int mode,
			const std::vector<boost::uint8_t>& msg);

	protected: // static
		static bool devMode; // allows real file access
		static bool modUICtrl; // allows non-user scripts to use UI controls

		// FIXME: because CLuaUnitScript needs to access RunCallIn
		friend class CLuaUnitScript;

		// FIXME needs access to L & RunCallIn
		friend class CLuaRules;
};


inline bool CLuaHandle::RunCallIn(lua_State* L, const LuaHashString& hs, int inArgs, int outArgs)
{
	return RunCallInTraceback(L, hs, inArgs, outArgs, 0, false);
}


inline bool CLuaHandle::RunCallIn(lua_State* L, int inArgs, int outArgs, std::string& errorMsg)
{
	return RunCallInTraceback(L, NULL, inArgs, outArgs, 0, errorMsg, false);
}


/******************************************************************************/
/******************************************************************************/


#endif /* LUA_HANDLE_H */

