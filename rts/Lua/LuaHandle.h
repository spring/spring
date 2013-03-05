/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_HANDLE_H
#define LUA_HANDLE_H
#include <boost/cstdint.hpp>

#include "System/EventClient.h"
//FIXME#include "LuaArrays.h"
#include "LuaCallInCheck.h"
#include "LuaContextData.h"
#include "LuaUtils.h"
#include "System/Platform/Threading.h"

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


class CLuaHandle : public CEventClient
{
	public:
		void CheckStack();
		int GetCallInErrors() const { return callinErrors; }
		void ResetCallinErrors() { callinErrors = 0; }

	public:
#define GET_CONTEXT_DATA(v) ((L ? L : GetActiveState())->lcd->v)
#define GET_ACTIVE_CONTEXT_DATA(v) (GetActiveState()->lcd->v)
#define GET_HANDLE_CONTEXT_DATA(v) (L->lcd->v)
#define SET_ACTIVE_CONTEXT_DATA(v) if(all) { D_Sim.v = _##v; D_Draw.v = _##v; } else GET_ACTIVE_CONTEXT_DATA(v) = _##v
#define SET_HANDLE_CONTEXT_DATA(v) GET_HANDLE_CONTEXT_DATA(v) = _##v

		void SetFullRead(bool _fullRead, bool all = false) { SET_ACTIVE_CONTEXT_DATA(fullRead); }
		bool GetFullRead() const { return GET_ACTIVE_CONTEXT_DATA(fullRead); } // virtual function in CEventClient
		static void SetHandleFullRead(const lua_State* L, bool _fullRead) { SET_HANDLE_CONTEXT_DATA(fullRead); }
		static bool GetHandleFullRead(const lua_State* L) { return GET_HANDLE_CONTEXT_DATA(fullRead); }

		void SetFullCtrl(bool _fullCtrl, bool all = false) { SET_ACTIVE_CONTEXT_DATA(fullCtrl); }
		bool GetFullCtrl() const { return GET_ACTIVE_CONTEXT_DATA(fullCtrl); }
		static void SetHandleFullCtrl(const lua_State* L, bool _fullCtrl) { SET_HANDLE_CONTEXT_DATA(fullCtrl); }
		static bool GetHandleFullCtrl(const lua_State* L) { return GET_HANDLE_CONTEXT_DATA(fullCtrl); }

		void SetCtrlTeam(int _ctrlTeam, bool all = false) { SET_ACTIVE_CONTEXT_DATA(ctrlTeam); }
		int GetCtrlTeam() const { return GET_ACTIVE_CONTEXT_DATA(ctrlTeam); }
		static void SetHandleCtrlTeam(const lua_State* L, int _ctrlTeam) { SET_HANDLE_CONTEXT_DATA(ctrlTeam); }
		static int GetHandleCtrlTeam(const lua_State* L) { return GET_HANDLE_CONTEXT_DATA(ctrlTeam); }

		void SetReadTeam(int _readTeam, bool all = false) { SET_ACTIVE_CONTEXT_DATA(readTeam); }
		int GetReadTeam() const { return GET_ACTIVE_CONTEXT_DATA(readTeam); }
		static void SetHandleReadTeam(const lua_State* L, int _readTeam) { SET_HANDLE_CONTEXT_DATA(readTeam); }
		static int GetHandleReadTeam(const lua_State* L) { return GET_HANDLE_CONTEXT_DATA(readTeam); }

		void SetReadAllyTeam(int _readAllyTeam, bool all = false) { SET_ACTIVE_CONTEXT_DATA(readAllyTeam); }
		int GetReadAllyTeam() const { return GET_ACTIVE_CONTEXT_DATA(readAllyTeam); } // virtual function in CEventClient
		static void SetHandleReadAllyTeam(const lua_State* L, int _readAllyTeam) { SET_HANDLE_CONTEXT_DATA(readAllyTeam); }
		static int GetHandleReadAllyTeam(const lua_State* L) { return GET_HANDLE_CONTEXT_DATA(readAllyTeam); }

		void SetSelectTeam(int _selectTeam, bool all = false) { SET_ACTIVE_CONTEXT_DATA(selectTeam); }
		int GetSelectTeam() const { return GET_ACTIVE_CONTEXT_DATA(selectTeam); }
		static void SetHandleSelectTeam(const lua_State* L, int _selectTeam) { SET_HANDLE_CONTEXT_DATA(selectTeam); }
		static int GetHandleSelectTeam(const lua_State* L) { return GET_HANDLE_CONTEXT_DATA(selectTeam); }

		void SetSynced(bool _synced, bool all = false) { SET_ACTIVE_CONTEXT_DATA(synced); }
		bool GetSynced() const { return GET_ACTIVE_CONTEXT_DATA(synced); }
		static void SetHandleSynced(const lua_State* L, bool _synced) { SET_HANDLE_CONTEXT_DATA(synced); }
		static bool GetHandleSynced(const lua_State* L) { return GET_HANDLE_CONTEXT_DATA(synced); }

		static CLuaHandle* GetHandle(lua_State* L) { return GET_HANDLE_CONTEXT_DATA(owner); }

		static bool GetHandleUserMode(const lua_State* L) { return GET_HANDLE_CONTEXT_DATA(owner->GetUserMode()); }
		bool GetUserMode() const { return userMode; }

		static bool CheckModUICtrl(lua_State* L) { return GetModUICtrl() || GetHandleUserMode(L); }
		bool CheckModUICtrl() const { return GetModUICtrl() || GetUserMode(); }

		void SetRunning(lua_State* L, const bool _running) { GET_HANDLE_CONTEXT_DATA(running) += (_running) ? +1 : -1; assert( GET_HANDLE_CONTEXT_DATA(running) >= 0); }
		bool IsRunning() const { return (GET_ACTIVE_CONTEXT_DATA(running) > 0); }

//FIXME		LuaArrays& GetArrays(const lua_State* L = NULL) { return GET_CONTEXT_DATA(arrays); }
		LuaShaders& GetShaders(const lua_State* L = NULL) { return GET_CONTEXT_DATA(shaders); }
		LuaTextures& GetTextures(const lua_State* L = NULL) { return GET_CONTEXT_DATA(textures); }
//FIXME		LuaVBOs& GetVBOs(const lua_State* L = NULL) { return GET_CONTEXT_DATA(vbos); }
		LuaFBOs& GetFBOs(const lua_State* L = NULL) { return GET_CONTEXT_DATA(fbos); }
		LuaRBOs& GetRBOs(const lua_State* L = NULL) { return GET_CONTEXT_DATA(rbos); }
		CLuaDisplayLists& GetDisplayLists(const lua_State* L = NULL) { return GET_CONTEXT_DATA(displayLists); }

	public: // call-ins
		bool WantsEvent(const string& name)  {
			BEGIN_ITERATE_LUA_STATES();
			{
				GML_DRCMUTEX_LOCK(lua); // WantsEvent

				if (HasCallIn(L, name))
					return true;
			}
			END_ITERATE_LUA_STATES();
			return false;
		}

		virtual bool HasCallIn(lua_State* L, const string& name) { return false; } // FIXME
		virtual bool SyncedUpdateCallIn(lua_State* L, const string& name) { return false; }
		virtual bool UnsyncedUpdateCallIn(lua_State* L, const string& name) { return false; }

		void Shutdown();

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
		void UnitCmdDone(const CUnit* unit, int cmdID, int cmdTag);
		void UnitDamaged(const CUnit* unit, const CUnit* attacker,
		                 float damage, int weaponID, bool paralyzer);
		void UnitExperience(const CUnit* unit, float oldExperience);

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

		void ProjectileCreated(const CProjectile* p);
		void ProjectileDestroyed(const CProjectile* p);

		bool Explosion(int weaponID, const float3& pos, const CUnit* owner);

		void StockpileChanged(const CUnit* owner,
		                      const CWeapon* weapon, int oldCount);

		// LuaHandleSynced wraps this to set allowChanges
		virtual bool RecvLuaMsg(const string& msg, int playerID);

		void Save(zipFile archive);

		void UnsyncedHeightMapUpdate(const SRectangle& rect);
		void Update();

		bool KeyPress(unsigned short key, bool isRepeat);
		bool KeyRelease(unsigned short key);
		bool MouseMove(int x, int y, int dx, int dy, int button);
		bool MousePress(int x, int y, int button);
		int  MouseRelease(int x, int y, int button); // return a cmd index, or -1
		bool MouseWheel(bool up, float value);
		bool JoystickEvent(const std::string& event, int val1, int val2);
		bool IsAbove(int x, int y);
		string GetTooltip(int x, int y);

		bool DefaultCommand(const CUnit* unit, const CFeature* feature, int& cmd);

		bool ConfigCommand(const string& command);

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

		void GameProgress(int frameNum);

	public: // custom call-in  (inter-script calls)
		virtual bool HasSyncedXCall(const string& funcName) { return false; }
		virtual bool HasUnsyncedXCall(lua_State* srcState, const string& funcName) { return false; }
		virtual int SyncedXCall(lua_State* srcState, const string& funcName) {
			return 0;
		}
		virtual int UnsyncedXCall(lua_State* srcState, const string& funcName) {
			return 0;
		}

		struct DelayDataDump {
			std::vector<LuaUtils::ShallowDataDump> data;
			std::vector<LuaUtils::DataDump> dump;
			bool xcall;
		};

		/// @return true if any calls were processed, false otherwise
		bool ExecuteCallsFromSynced(bool forced = true);
		virtual void RecvFromSynced(lua_State* srcState, int args);
		void RecvFromSim(int args);
		void DelayRecvFromSynced(lua_State* srcState, int args);
		std::vector<DelayDataDump> delayedCallsFromSynced;
		static int SendToUnsynced(lua_State* L);

		void UpdateThreading();

	protected:
		CLuaHandle(const string& name, int order, bool userMode);
		virtual ~CLuaHandle();

		void KillLua();

		bool AddBasicCalls(lua_State* L);
		bool LoadCode(lua_State* L, const string& code, const string& debug);
		bool AddEntriesToTable(lua_State* L, const char* name, bool (*entriesFunc)(lua_State*));

		/// returns stack index of traceback function
		int SetupTraceback(lua_State* L);
		/// returns error code and sets traceback on error
		int  RunCallInTraceback(const LuaHashString* hs, int inArgs, int outArgs, int errfuncIndex, std::string& traceback);
		/// returns false and prints message to log on error
		bool RunCallInTraceback(const LuaHashString& hs, int inArgs, int outArgs, int errfuncIndex);
		/// returns error code and sets errormessage on error
		int  RunCallIn(int inArgs, int outArgs, std::string& errormessage);
		/// returns false and prints message to log on error
		bool RunCallIn(const LuaHashString& hs, int inArgs, int outArgs);
		bool RunCallInUnsynced(const LuaHashString& hs, int inArgs, int outArgs);

		void LosCallIn(const LuaHashString& hs, const CUnit* unit, int allyTeam);
		void UnitCallIn(const LuaHashString& hs, const CUnit* unit);
		bool PushUnsyncedCallIn(lua_State* L, const LuaHashString& hs);

	protected:
		// MT stuff

		bool singleState;
		// LUA_MT_OPT inserted below mainly so that compiler can optimize code
		bool SingleState() const { return !(LUA_MT_OPT & LUA_STATE) || singleState; } // Is this handle using a single Lua state?
		bool copyExportTable;
		bool CopyExportTable() const { return (LUA_MT_OPT & LUA_STATE) && copyExportTable; } // Copy the table _G.EXPORT --> SYNCED.EXPORT between dual states?
		static bool useDualStates;
		static bool UseDualStates() { return (LUA_MT_OPT & LUA_STATE) && useDualStates; } // Is Lua handle splitting enabled (globally)?
		bool useEventBatch;
		bool UseEventBatch() const { return (LUA_MT_OPT & LUA_BATCH) && useEventBatch; } // Use event batch to forward "synced" luaui events into draw thread?
		bool purgeCallsFromSyncedBatch;
		bool PurgeCallsFromSyncedBatch() const { return (LUA_MT_OPT & LUA_STATE) && purgeCallsFromSyncedBatch; } // Automatically clean deleted objects/IDs from the SendToUnsynced/XCall batch

		lua_State* ForceUnsyncedState() { lua_State* L_Prev = L_Sim; if (!SingleState() && Threading::IsSimThread()) L_Sim = L_Draw; return L_Prev; }
		void RestoreState(lua_State* L_Prev) { if (!SingleState() && Threading::IsSimThread()) L_Sim = L_Prev; }

		lua_State* GetActiveState() {
			return (SingleState() || Threading::IsSimThread()) ? L_Sim : L_Draw;
		}
		const lua_State* GetActiveState() const {
			return (SingleState() || Threading::IsSimThread()) ? L_Sim : L_Draw;
		}

		bool IsValid() const { return (L_Sim != NULL) && (L_Draw != NULL); }

	protected:
		bool userMode;
		
		lua_State* L_Sim;
		lua_State* L_Draw;

		luaContextData D_Sim;
		luaContextData D_Draw;

		bool killMe;
		string killMsg;

		vector<bool> watchUnitDefs;
		vector<bool> watchFeatureDefs;
		vector<bool> watchWeaponDefs; // for the Explosion call-in

		int callinErrors;

	protected: // call-outs
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
		static int CallOutSyncedUpdateCallIn(lua_State* L);
		static int CallOutUnsyncedUpdateCallIn(lua_State* L);

	public: // static
//FIXME		static LuaArrays& GetActiveArrays(lua_State* L)   { return GET_HANDLE_CONTEXT_DATA(arrays); }
		static inline LuaShaders& GetActiveShaders(lua_State* L)  { return GET_HANDLE_CONTEXT_DATA(shaders); }
		static inline LuaTextures& GetActiveTextures(lua_State* L) { return GET_HANDLE_CONTEXT_DATA(textures); }
//FIXME		static LuaVBOs& GetActiveVBOs(lua_State* L)     { return GET_HANDLE_CONTEXT_DATA(vbos); }
		static inline LuaFBOs& GetActiveFBOs(lua_State* L) { return GET_HANDLE_CONTEXT_DATA(fbos); }
		static inline LuaRBOs& GetActiveRBOs(lua_State* L)     { return GET_HANDLE_CONTEXT_DATA(rbos); }
		static inline CLuaDisplayLists& GetActiveDisplayLists(lua_State* L) { return GET_HANDLE_CONTEXT_DATA(displayLists); }

		static void SetDevMode(bool value) { devMode = value; }
		static bool GetDevMode() { return devMode; }

		static void SetModUICtrl(bool value) { modUICtrl = value; }
		static bool GetModUICtrl() { return modUICtrl; }

		static void HandleLuaMsg(int playerID, int script, int mode,
			const std::vector<boost::uint8_t>& msg);
		static bool IsDrawCallIn() {
			return (LUA_MT_OPT & LUA_MUTEX) && !Threading::IsSimThread();
		}
		void ExecuteUnitEventBatch();
		void ExecuteFeatEventBatch();
		void ExecuteObjEventBatch();
		void ExecuteProjEventBatch();
		void ExecuteFrameEventBatch();
		void ExecuteLogEventBatch();

	protected: // static
		static bool devMode; // allows real file access
		static bool modUICtrl; // allows non-user scripts to use UI controls

		std::vector<LuaUnitEvent> luaUnitEventBatch;
		std::vector<LuaFeatEvent> luaFeatEventBatch;
		std::vector<LuaObjEvent> luaObjEventBatch;
		std::vector<LuaProjEvent> luaProjEventBatch;
		std::vector<int> luaFrameEventBatch;
		std::vector<LuaLogEvent> luaLogEventBatch;

		// FIXME: because CLuaUnitScript needs to access RunCallIn
		friend class CLuaUnitScript;
};


inline bool CLuaHandle::RunCallIn(const LuaHashString& hs, int inArgs, int outArgs)
{
	return RunCallInTraceback(hs, inArgs, outArgs, 0);
}


inline bool CLuaHandle::RunCallInUnsynced(const LuaHashString& hs, int inArgs, int outArgs)
{
	SetSynced(false);
	const bool retval = RunCallIn(hs, inArgs, outArgs);
	SetSynced(!userMode);
	return retval;
}



/******************************************************************************/
/******************************************************************************/


#endif /* LUA_HANDLE_H */

