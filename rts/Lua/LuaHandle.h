/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_HANDLE_H
#define LUA_HANDLE_H

#include <string>
#include <vector>
#include <set>
using std::string;
using std::vector;
using std::set;
#include <boost/cstdint.hpp>

#include "EventClient.h"
//FIXME#include "LuaArrays.h"
#include "LuaCallInCheck.h"
#include "LuaShaders.h"
#include "LuaTextures.h"
#include "LuaFBOs.h"
#include "LuaRBOs.h"
#include "LuaUtils.h"
//FIXME#include "LuaVBOs.h"
#include "LuaDisplayLists.h"
#include "System/Platform/Threading.h"
#include "System/LogOutput.h"


#define LUA_HANDLE_ORDER_RULES            100
#define LUA_HANDLE_ORDER_UNITS            200
#define LUA_HANDLE_ORDER_GAIA             300
#define LUA_HANDLE_ORDER_RULES_UNSYNCED  1100
#define LUA_HANDLE_ORDER_UNITS_UNSYNCED  1200
#define LUA_HANDLE_ORDER_GAIA_UNSYNCED   1300
#define LUA_HANDLE_ORDER_UI              2000


class CUnit;
class CWeapon;
class CFeature;
class CProjectile;
struct Command;
struct LuaHashString;
struct lua_State;
class CLogSubsystem;


class CLuaHandle : public CEventClient
{
	public:
		void CheckStack();
		int GetCallInErrors() const { return callinErrors; }
		void ResetCallinErrors() { callinErrors = 0; }

	public:
		inline bool CanCtrlTeam(int team) {
			int ctrlTeam = GetCtrlTeam();
			if (ctrlTeam < 0) {
				return (ctrlTeam == AllAccessTeam);
			} else {
				return (ctrlTeam == team);
			}
		}
		inline bool CanReadAllyTeam(int allyTeam) {
			int readAllyTeam = GetReadAllyTeam();
			if (readAllyTeam < 0) {
				return (readAllyTeam == AllAccessTeam);
			} else {
				return (readAllyTeam == allyTeam);
			}
		}

#define SET_CONTEXT_DATA(v) if(all) { D_Sim.v = _##v; D_Draw.v = _##v; } else GetLuaContextData().v = _##v
		inline void SetFullRead(bool _fullRead, bool all = false) { SET_CONTEXT_DATA(fullRead); }
		inline bool GetFullRead() const { return GetLuaContextData().fullRead; }
		inline void SetFullCtrl(bool _fullCtrl, bool all = false) { SET_CONTEXT_DATA(fullCtrl); }
		inline bool GetFullCtrl() const { return GetLuaContextData().fullCtrl; }
		inline void SetCtrlTeam(int _ctrlTeam, bool all = false) { SET_CONTEXT_DATA(ctrlTeam); }
		inline int GetCtrlTeam() const { return GetLuaContextData().ctrlTeam; }
		inline void SetReadTeam(int _readTeam, bool all = false) { SET_CONTEXT_DATA(readTeam); }
		inline int GetReadTeam() const { return GetLuaContextData().readTeam; }
		inline void SetReadAllyTeam(int _readAllyTeam, bool all = false) { SET_CONTEXT_DATA(readAllyTeam); }
		inline int GetReadAllyTeam() const { return GetLuaContextData().readAllyTeam; }
		inline void SetSelectTeam(int _selectTeam, bool all = false) { SET_CONTEXT_DATA(selectTeam); }
		inline int GetSelectTeam() const { return GetLuaContextData().selectTeam; }
		inline void SetSynced(bool _synced, bool all = false) { if(!IsDrawCallIn() || all) synced = _synced; }
		inline bool GetSynced() const { return IsDrawCallIn() ? false : synced; }

		bool GetUserMode()     const { return userMode; }


		bool WantsToDie() const { return killMe; }

//FIXME		LuaArrays& GetArrays() { return arrays; }
		LuaShaders& GetShaders() { return GetLuaContextData().shaders; }
		LuaTextures& GetTextures() { return GetLuaContextData().textures; }
//FIXME		LuaVBOs& GetVBOs() { return vbos; }
		LuaFBOs& GetFBOs() { return GetLuaContextData().fbos; }
		LuaRBOs& GetRBOs() { return GetLuaContextData().rbos; }
		CLuaDisplayLists& GetDisplayLists() { return GetLuaContextData().displayLists; }

	public:
		const bool userMode;

	public: // call-ins
		virtual bool HasCallIn(lua_State *L, const string& name) { return false; } // FIXME
		bool WantsEvent(const string& name)  { return HasCallIn(GetActiveState(), name); } // FIXME
		virtual bool SyncedUpdateCallIn(lua_State *L, const string& name) { return false; }
		virtual bool UnsyncedUpdateCallIn(lua_State *L, const string& name) { return false; }

		void Shutdown();

		void Load(CArchiveBase* archive);

		void GamePreload();
		void GameStart();
		void GameOver(const std::vector<unsigned char>& winningAllyTeams);
		void GamePaused(int playerID, bool paused);
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
		void UnitTaken(const CUnit* unit, int newTeam);
		void UnitGiven(const CUnit* unit, int oldTeam);

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

		void UnitMoveFailed(const CUnit* unit);

		void FeatureCreated(const CFeature* feature);
		void FeatureDestroyed(const CFeature* feature);

		void ProjectileCreated(const CProjectile* p);
		void ProjectileDestroyed(const CProjectile* p);

		bool Explosion(int weaponID, const float3& pos, const CUnit* owner);

		void StockpileChanged(const CUnit* owner,
		                      const CWeapon* weapon, int oldCount);

		virtual void GameFrame(int frameNumber);
		// LuaHandleSynced wraps this to set allowChanges
		virtual bool RecvLuaMsg(const string& msg, int playerID);

		void Save(zipFile archive);

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

		bool AddConsoleLine(const string& msg, const CLogSubsystem& sys);

		bool GroupChanged(int groupID);

		bool GameSetup(const string& state, bool& ready,
		               const map<int, string>& playerStates);

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

	public: // custom call-in  (inter-script calls)
		virtual bool HasSyncedXCall(const string& funcName) { return false; }
		virtual bool HasUnsyncedXCall(const string& funcName) { return false; }
		virtual int SyncedXCall(lua_State* srcState, const string& funcName) {
			return 0;
		}
		virtual int UnsyncedXCall(lua_State* srcState, const string& funcName) {
			return 0;
		}

		struct DelayData {
			int type;
			std::string str;
			float num;
			bool bol;
		};
		struct DelayDataDump {
			std::vector<DelayData> dd;
			std::vector<LuaUtils::DataDump> com;
		};

		void ExecuteRecvFromSynced();
		virtual void RecvFromSynced(int args);
		void RecvFromSim(int args);
		void DelayRecvFromSynced(lua_State* srcState, int args);
		std::vector<DelayDataDump> delayedRecvFromSynced;
		static int SendToUnsynced(lua_State* L);

		void UpdateThreading();

	protected:
		CLuaHandle(const string& name, int order, bool userMode);
		virtual ~CLuaHandle();

		void KillLua();

		inline void SetActiveHandle(bool draw = IsDrawCallIn());
		inline void SetActiveHandle(CLuaHandle* lh, bool draw = IsDrawCallIn());
		bool singleState;
		// DUAL_LUA_STATES inserted below mainly so that compiler can optimize code if DUAL_LUA_STATES = 0
		inline bool SingleState() { return !DUAL_LUA_STATES || singleState; } // Is this handle using a single Lua state?
		bool copyExportTable;
		inline bool CopyExportTable() { return DUAL_LUA_STATES && copyExportTable; } // Copy the table _G.EXPORT --> SYNCED.EXPORT between dual states?
		static bool useDualStates;
		static inline bool UseDualStates() { return DUAL_LUA_STATES && useDualStates; } // Is Lua handle splitting enabled (globally)?
		bool useEventBatch;
		inline bool UseEventBatch() { return DUAL_LUA_STATES && useEventBatch; } // Use event batch to forward "synced" luaui events into draw thread?

		inline lua_State *GetActiveState() {
			return (SingleState() || Threading::IsSimThread()) ? L_Sim : L_Draw;
		}

		bool AddBasicCalls(lua_State *L);
		bool LoadCode(lua_State *L, const string& code, const string& debug);
		bool AddEntriesToTable(lua_State* L, const char* name,
		                       bool (*entriesFunc)(lua_State*));

		/// returns stack index of traceback function
		int SetupTraceback(lua_State *L);
		/// returns error code and sets traceback on error
		int  RunCallInTraceback(int inArgs, int outArgs, int errfuncIndex, std::string& traceback);
		/// returns false and prints message to log on error
		bool RunCallInTraceback(const LuaHashString& hs, int inArgs, int outArgs, int errfuncIndex);
		/// returns error code and sets errormessage on error
		int  RunCallIn(int inArgs, int outArgs, std::string& errormessage);
		/// returns false and prints message to log on error
		bool RunCallIn(const LuaHashString& hs, int inArgs, int outArgs);
		bool RunCallInUnsynced(const LuaHashString& hs, int inArgs, int outArgs);

		void LosCallIn(const LuaHashString& hs, const CUnit* unit, int allyTeam);
		void UnitCallIn(const LuaHashString& hs, const CUnit* unit);
		bool PushUnsyncedCallIn(lua_State *L, const LuaHashString& hs);

		inline bool CheckModUICtrl() { return modUICtrl || userMode; }

		inline bool IsValid() const { return (L_Sim != NULL) && (L_Draw != NULL); }

	protected:

		bool synced; // FIXME -- remove this once the lua_State split is done
		//          (use the constant CEventClient version ...)

		struct luaContextData {
			luaContextData() : fullCtrl(false), fullRead(false), ctrlTeam(CEventClient::NoAccessTeam), 
								readTeam(0), readAllyTeam(0), selectTeam(CEventClient::NoAccessTeam) {}
			bool fullCtrl;
			bool fullRead;
			int  ctrlTeam;
			int  readTeam;
			int  readAllyTeam;
			int  selectTeam;
			//FIXME		LuaArrays arrays;
			LuaShaders shaders;
			LuaTextures textures;
			//FIXME		LuaVBOs vbos;
			LuaFBOs fbos;
			LuaRBOs rbos;
			CLuaDisplayLists displayLists;
		};

		lua_State* L_Sim;
		lua_State* L_Draw;

		luaContextData D_Sim;
		luaContextData D_Draw;

		const luaContextData &GetLuaContextData(bool draw = IsDrawCallIn()) const { return draw ? D_Draw : D_Sim; }
		luaContextData &GetLuaContextData(bool draw = IsDrawCallIn()) { return draw ? D_Draw : D_Sim; }

		bool killMe;
		string killMsg;

		bool printTracebacks;

		vector<bool> watchWeapons; // for the Explosion call-in

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
		static inline CLuaHandle* GetActiveHandle(bool draw = IsDrawCallIn()) {
			return GetStaticLuaContextData(draw).activeHandle;
		}

		static inline bool ActiveCanCtrlTeam(int team) {
			return GetActiveHandle()->CanCtrlTeam(team);
		}
		static inline bool ActiveCanReadAllyTeam(int allyTeam) {
			return GetActiveHandle()->CanReadAllyTeam(allyTeam);
		}

		static inline const bool GetActiveFullRead() { return GetStaticLuaContextData().activeFullRead; }
		static inline void SetActiveFullRead(bool fr) {	GetStaticLuaContextData().activeFullRead = fr; }
		static inline const int GetActiveReadAllyTeam() { return GetStaticLuaContextData().activeReadAllyTeam; }
		static inline void SetActiveReadAllyTeam(int rat) { GetStaticLuaContextData().activeReadAllyTeam = rat; }

		static inline luaContextData &GetActiveLuaContextData(lua_State *L) {
			CLuaHandle *ah = GetActiveHandle();
			return ah->GetLuaContextData(L == ah->L_Draw);
		}
//FIXME		static LuaArrays& GetActiveArrays(lua_State *L)   { return GetActiveLuaContextData(L).arrays; }
		static inline LuaShaders& GetActiveShaders(lua_State *L)  { return GetActiveLuaContextData(L).shaders; }
		static inline LuaTextures& GetActiveTextures(lua_State *L) { return GetActiveLuaContextData(L).textures; }
//FIXME		static LuaVBOs& GetActiveVBOs(lua_State *L)     { return GetActiveLuaContextData(L).vbos; }
		static inline LuaFBOs& GetActiveFBOs(lua_State *L) { return GetActiveLuaContextData(L).fbos; }
		static inline LuaRBOs& GetActiveRBOs(lua_State *L)     { return GetActiveLuaContextData(L).rbos; }
		static inline CLuaDisplayLists& GetActiveDisplayLists(lua_State *L) { return GetActiveLuaContextData(L).displayLists; }

		static void SetDevMode(bool value) { devMode = value; }
		static bool GetDevMode() { return devMode; }

		static void SetModUICtrl(bool value) { modUICtrl = value; }
		static bool GetModUICtrl() { return modUICtrl; }

		static void HandleLuaMsg(int playerID, int script, int mode,
			const std::vector<boost::uint8_t>& msg);
		static inline bool IsDrawCallIn() {
			return !Threading::IsSimThread();
		}
		void ExecuteUnitEventBatch();
		void ExecuteFeatEventBatch();
		void ExecuteProjEventBatch();
		void ExecuteFrameEventBatch();
		void ExecuteMiscEventBatch();

	protected: // static
		static bool devMode; // allows real file access
		static bool modUICtrl; // allows non-user scripts to use UI controls

		std::vector<LuaUnitEvent>luaUnitEventBatch;
		std::vector<LuaFeatEvent>luaFeatEventBatch;
		std::vector<LuaProjEvent>luaProjEventBatch;
		std::vector<int>luaFrameEventBatch;
		std::vector<LuaMiscEvent>luaMiscEventBatch;
		bool execUnitBatch;
		bool execFeatBatch;
		bool execProjBatch;
		bool execFrameBatch;
		bool execMiscBatch;

		// FIXME: because CLuaUnitScript needs to access RunCallIn / activeHandle
		friend class CLuaUnitScript;
	private:
		struct staticLuaContextData {
			staticLuaContextData() : activeFullRead(false), activeReadAllyTeam(CEventClient::NoAccessTeam), activeHandle(NULL) {}
			bool activeFullRead;
			int  activeReadAllyTeam;
			CLuaHandle* activeHandle;
		};
		static staticLuaContextData S_Sim;
		static staticLuaContextData S_Draw;

		static staticLuaContextData &GetStaticLuaContextData(bool draw = IsDrawCallIn()) { return draw ? S_Draw : S_Sim; }
};


inline void CLuaHandle::SetActiveHandle(bool draw)
{
	SetActiveHandle(this, draw);
}

inline void CLuaHandle::SetActiveHandle(CLuaHandle* lh, bool draw)
{
	GetStaticLuaContextData(draw).activeHandle = lh;
	if (lh) {
		GetStaticLuaContextData(draw).activeFullRead = GetLuaContextData(draw).fullRead;
		GetStaticLuaContextData(draw).activeReadAllyTeam = GetLuaContextData(draw).readAllyTeam;
	}
}


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


#endif /* LUA_HANDLE_H */
