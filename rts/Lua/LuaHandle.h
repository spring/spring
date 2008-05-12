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

//FIXME#include "LuaArrays.h"
#include "LuaShaders.h"
#include "LuaTextures.h"
#include "LuaFBOs.h"
#include "LuaRBOs.h"
//FIXME#include "LuaVBOs.h"
#include "LuaDisplayLists.h"


#define LUA_HANDLE_ORDER_RULES 0
#define LUA_HANDLE_ORDER_GAIA  1
#define LUA_HANDLE_ORDER_UI    2


class CUnit;
class CFeature;
class CWeapon;
struct LuaHashString;
struct lua_State;


typedef void (*LuaCobCallback)(int retCode, void* unitID, void* data);


class CLuaHandle {
	public:
		const string& GetName() const { return name; }
		void CheckStack();
		int GetCallInErrors() const { return callinErrors; }
		void ResetCallinErrors() { callinErrors = 0; }
		

	public:
		enum SpecialTeams {
			NoAccessTeam   = -1,
			AllAccessTeam  = -2,
			MinSpecialTeam = -2
		};

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

		bool WantsToDie()      const { return killMe; }

		const LuaCobCallback  GetCallback() { return cobCallback; }

//FIXME		LuaArrays& GetArrays() { return arrays; }
		LuaShaders& GetShaders() { return shaders; }
		LuaTextures& GetTextures() { return textures; }
//FIXME		LuaVBOs& GetVBOs() { return vbos; }
		LuaFBOs& GetFBOs() { return fbos; }
		LuaRBOs& GetRBOs() { return rbos; }
		CLuaDisplayLists& GetDisplayLists() { return displayLists; }

	public:
		const string name;
		const int order;
		const bool userMode;

	public: // call-ins
		virtual bool HasCallIn(const string& name) { return false; }
		virtual bool SyncedUpdateCallIn(const string& name) { return false; }
		virtual bool UnsyncedUpdateCallIn(const string& name) { return false; }

		void Update();

		void Shutdown();

		void GamePreload();
		void GameStart();
		void GameOver();
		void TeamDied(int teamID);

		void UnitCreated(const CUnit* unit, const CUnit* builder);
		void UnitFinished(const CUnit* unit);
		void UnitFromFactory(const CUnit* unit, const CUnit* factory,
		                     bool userOrders);
		void UnitDestroyed(const CUnit* unit, const CUnit* attacker);
		void UnitTaken(const CUnit* unit, int newTeam);
		void UnitGiven(const CUnit* unit, int oldTeam);

		void UnitIdle(const CUnit* unit);
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

		void UnitLoaded(const CUnit* unit, const CUnit* transport);
		void UnitUnloaded(const CUnit* unit, const CUnit* transport);

		void UnitCloaked(const CUnit* unit);
		void UnitDecloaked(const CUnit* unit);

		void FeatureCreated(const CFeature* feature);
		void FeatureDestroyed(const CFeature* feature);

		void StockpileChanged(const CUnit* owner,
		                      const CWeapon* weapon, int oldCount);

		bool Explosion(int weaponID, const float3& pos, const CUnit* owner);

		// LuaHandleSynced wraps this to set allowChanges
		virtual bool RecvLuaMsg(const string& msg, int playerID);

		bool DefaultCommand(const CUnit* unit, const CFeature* feature, int& cmd);

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
		CLuaHandle(const string& name, int order,
		           bool userMode, LuaCobCallback callback);
		virtual ~CLuaHandle();

		void KillLua();

		void SetActiveHandle();
		void SetActiveHandle(CLuaHandle*);

		bool AddBasicCalls();
		bool LoadCode(const string& code, const string& debug);
		bool AddEntriesToTable(lua_State* L, const char* name,
		                       bool (*entriesFunc)(lua_State*));

		bool RunCallIn(const LuaHashString& hs, int inArgs, int outArgs);
		void LosCallIn(const LuaHashString& hs, const CUnit* unit, int allyTeam);
		bool PushUnsyncedCallIn(const LuaHashString& hs);

	protected:
		lua_State* L;

		bool killMe;
		string killMsg;

		bool synced;

		bool fullCtrl;
		bool fullRead;
		int  ctrlTeam;
		int  readTeam;
		int  readAllyTeam;
		int  selectTeam;

		LuaCobCallback cobCallback;

//FIXME		LuaArrays arrays;
		LuaShaders shaders;
		LuaTextures textures;
//FIXME		LuaVBOs vbos;
		LuaFBOs fbos;
		LuaRBOs rbos;
		CLuaDisplayLists displayLists;

		vector<bool> watchWeapons; // for the Explosion call-in

		struct CobCallbackData {
			CobCallbackData(int rc, int uid, float fd)
			: retCode(rc), unitID(uid), floatData(fd) {}
			int retCode;
			int unitID;
			float floatData;
		};
		vector<CobCallbackData> cobCallbackEntries;

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
		static int CallOutSyncedUpdateCallIn(lua_State* L);
		static int CallOutUnsyncedUpdateCallIn(lua_State* L);

	public: // static
		static const CLuaHandle* GetActiveHandle() { return activeHandle; }

		static bool ActiveCanCtrlTeam(int team) {
			return activeHandle->CanReadAllyTeam(team);
		}
		static bool ActiveCanReadAllyTeam(int allyTeam) {
			return activeHandle->CanReadAllyTeam(allyTeam);
		}

		static const bool& GetActiveFullRead()     { return activeFullRead; }
		static const int&  GetActiveReadAllyTeam() { return activeReadAllyTeam; }
		static const LuaCobCallback GetActiveCallback() {
			return activeHandle->cobCallback;
		}
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

		static void HandleLuaMsg(int playerID, int script, int mode,
		                         const string& msg);

	protected: // static
		static CLuaHandle* activeHandle;
		static bool activeFullRead;
		static int  activeReadAllyTeam;

		static bool devMode; // allows real file access
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
