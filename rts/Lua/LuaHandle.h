#ifndef LUA_HANDLE_H
#define LUA_HANDLE_H
// LuaHandle.h: interface for the CLuaHandle class.
//
//////////////////////////////////////////////////////////////////////

#include <string>
#include <vector>
#include <set>
using std::string;
using std::vector;
using std::set;
#include <boost/cstdint.hpp>

#include "EventClient.h"
//FIXME#include "LuaArrays.h"
#include "LuaShaders.h"
#include "LuaTextures.h"
#include "LuaFBOs.h"
#include "LuaRBOs.h"
//FIXME#include "LuaVBOs.h"
#include "LuaDisplayLists.h"


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
			if (ctrlTeam < 0) {
				return (ctrlTeam == AllAccessTeam);
			} else {
				return (ctrlTeam == team);
			}
		}
		inline bool CanReadAllyTeam(int allyTeam) {
			if (readAllyTeam < 0) {
				return (readAllyTeam == AllAccessTeam);
			} else {
				return (readAllyTeam == allyTeam);
			}
		}

		bool GetSynced()       const { return synced; }
		bool GetUserMode()     const { return userMode; }
		bool GetFullRead()     const { return fullRead; }
		bool GetFullCtrl()     const { return fullCtrl; }
		int  GetReadTeam()     const { return readTeam; }
		int  GetReadAllyTeam() const { return readAllyTeam; }
		int  GetCtrlTeam()     const { return ctrlTeam; }
		int  GetSelectTeam()   const { return selectTeam; }

		bool WantsToDie() const { return killMe; }

//FIXME		LuaArrays& GetArrays() { return arrays; }
		LuaShaders& GetShaders() { return shaders; }
		LuaTextures& GetTextures() { return textures; }
//FIXME		LuaVBOs& GetVBOs() { return vbos; }
		LuaFBOs& GetFBOs() { return fbos; }
		LuaRBOs& GetRBOs() { return rbos; }
		CLuaDisplayLists& GetDisplayLists() { return displayLists; }

	public:
		const bool userMode;

	public: // call-ins
		virtual bool HasCallIn(const string& name) { return false; } // FIXME
		bool WantsEvent(const string& name)  { return HasCallIn(name); } // FIXME
		virtual bool SyncedUpdateCallIn(const string& name) { return false; }
		virtual bool UnsyncedUpdateCallIn(const string& name) { return false; }

		void Shutdown();

		void GamePreload();
		void GameStart();
		void GameOver();
		void TeamDied(int teamID);
		void TeamChanged(int teamID);
		void PlayerChanged(int playerID);

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

		// LuaHandleSynced wraps this to set allowChanges
		virtual bool RecvLuaMsg(const string& msg, int playerID);

		void Update();

		bool KeyPress(unsigned short key, bool isRepeat);
		bool KeyRelease(unsigned short key);
		bool MouseMove(int x, int y, int dx, int dy, int button);
		bool MousePress(int x, int y, int button);
		int  MouseRelease(int x, int y, int button); // return a cmd index, or -1
		bool MouseWheel(bool up, float value);
		bool IsAbove(int x, int y);
		string GetTooltip(int x, int y);

		bool DefaultCommand(const CUnit* unit, const CFeature* feature, int& cmd);

		bool ConfigCommand(const string& command);

		bool CommandNotify(const Command& cmd);

		bool AddConsoleLine(const string& msg, const CLogSubsystem&);

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

	protected:
		CLuaHandle(const string& name, int order, bool userMode);
		virtual ~CLuaHandle();

		void KillLua();

		void SetActiveHandle();
		void SetActiveHandle(CLuaHandle*);

		bool AddBasicCalls();
		bool LoadCode(const string& code, const string& debug);
		bool AddEntriesToTable(lua_State* L, const char* name,
		                       bool (*entriesFunc)(lua_State*));

		/// returns stack index of traceback function
		int SetupTraceback();
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
		bool PushUnsyncedCallIn(const LuaHashString& hs);

		inline bool CheckModUICtrl() { return modUICtrl || userMode; }

	protected:
		lua_State* L;

		bool killMe;
		string killMsg;

		bool synced; // FIXME -- remove this once the lua_State split is done
		             //          (use the constant CEventClient version ...)

		bool fullCtrl;
		bool fullRead;
		bool printTracebacks;

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
		static const CLuaHandle* GetActiveHandle() { return activeHandle; }

		static bool ActiveCanCtrlTeam(int team) {
			return activeHandle->CanCtrlTeam(team);
		}
		static bool ActiveCanReadAllyTeam(int allyTeam) {
			return activeHandle->CanReadAllyTeam(allyTeam);
		}

		static const bool& GetActiveFullRead()     { return activeFullRead; }
		static const int&  GetActiveReadAllyTeam() { return activeReadAllyTeam; }
//FIXME		static LuaArrays&   GetActiveArrays()   { return activeHandle->arrays; }
		static LuaShaders&  GetActiveShaders()  { return activeHandle->shaders; }
		static LuaTextures& GetActiveTextures() { return activeHandle->textures; }
//FIXME		static LuaVBOs&     GetActiveVBOs()     { return activeHandle->vbos; }
		static LuaFBOs&     GetActiveFBOs()     { return activeHandle->fbos; }
		static LuaRBOs&     GetActiveRBOs()     { return activeHandle->rbos; }
		static CLuaDisplayLists& GetActiveDisplayLists() {
			return activeHandle->displayLists;
		}

		static void SetDevMode(bool value) { devMode = value; }
		static bool GetDevMode() { return devMode; }

		static void SetModUICtrl(bool value) { modUICtrl = value; }
		static bool GetModUICtrl() { return modUICtrl; }

		static void HandleLuaMsg(int playerID, int script, int mode,
			const std::vector<boost::uint8_t>& msg);

	protected: // static
		static CLuaHandle* activeHandle;
		static bool activeFullRead;
		static int  activeReadAllyTeam;

		static bool devMode; // allows real file access
		static bool modUICtrl; // allows non-user scripts to use UI controls

		// FIXME: because CLuaUnitScript needs to access RunCallIn / activeHandle
		friend class CLuaUnitScript;
};


inline void CLuaHandle::SetActiveHandle()
{
	activeHandle       = this;
	activeFullRead     = this->fullRead;
	activeReadAllyTeam = this->readAllyTeam;
}


inline void CLuaHandle::SetActiveHandle(CLuaHandle* lh)
{
	activeHandle = lh;
	if (lh) {
		activeFullRead     = lh->fullRead;
		activeReadAllyTeam = lh->readAllyTeam;
	}
}


#endif /* LUA_HANDLE_H */
