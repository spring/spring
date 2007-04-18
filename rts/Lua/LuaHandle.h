#ifndef LUA_HANDLE_H
#define LUA_HANDLE_H
// LuaHandle.h: interface for the CLuaHandle class.
//
//////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include <string>
#include <vector>
using std::string;
using std::vector;

#include "LuaDisplayLists.h"


#define LUA_HANDLE_ORDER_MOD  0
#define LUA_HANDLE_ORDER_COB  1
#define LUA_HANDLE_ORDER_GAIA 2
#define LUA_HANDLE_ORDER_UI   3


class CUnit;
class CFeature;
struct LuaHashString;
struct lua_State;


typedef void (*LuaCobCallback)(int retCode, void* unitID, void* data);


class CLuaHandle {
	public:
		const string& GetName() const { return name; }

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
		CLuaDisplayLists& GetDisplayLists() { return displayLists; }

		virtual string LoadFile(const string& filename) const;

	public:
		const string name;
		const int order;
		const bool userMode;

	public: // call-ins
		virtual bool HasCallIn(const string& callInName) { return false; }

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
		void UnitDamaged(const CUnit* unit, const CUnit* attacker, float damage);

		void UnitSeismicPing(const CUnit* unit, int allyTeam,
		                     const float3& pos, float strength);
		void UnitEnteredRadar(const CUnit* unit, int allyTeam);
		void UnitEnteredLos(const CUnit* unit, int allyTeam);
		void UnitLeftRadar(const CUnit* unit, int allyTeam);
		void UnitLeftLos(const CUnit* unit, int allyTeam);

		void UnitLoaded(const CUnit* unit, const CUnit* transport);
		void UnitUnloaded(const CUnit* unit, const CUnit* transport);

		void FeatureCreated(const CFeature* feature);
		void FeatureDestroyed(const CFeature* feature);

		void DrawWorld();
		void DrawWorldShadow();
		void DrawWorldReflection();
		void DrawWorldRefraction();
		void DrawScreen();
		void DrawInMiniMap();

	public: // custom call-in
		virtual bool HasUnsyncedXCall(const string& funcName) { return false; }
		virtual int UnsyncedXCall(lua_State* srcState, const string& funcName) {
			return 0;
		}

	protected:
		CLuaHandle(const string& name, int order,
		           bool userMode, LuaCobCallback callback);
		virtual ~CLuaHandle();

		void SetActiveHandle();
		void SetActiveHandle(CLuaHandle*);

		bool AddBasicCalls();
		bool LoadCode(const string& code, const string& debug);
		bool AddEntriesToTable(lua_State* L, const char* name,
		                       bool (*entriesFunc)(lua_State*));

		bool RunCallIn(const LuaHashString& hs, int inArgs, int outArgs);
		void LosCallIn(const LuaHashString& hs, const CUnit* unit, int allyTeam);
		bool LoadDrawCallIn(const LuaHashString& hs);

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

		CLuaDisplayLists displayLists;

		struct CobCallbackData {
			CobCallbackData(int rc, int uid, float fd)
			: retCode(rc), unitID(uid), floatData(fd) {}
			int retCode;
			int unitID;
			float floatData;
		};
		vector<CobCallbackData> cobCallbackEntries;

	protected: // call-outs
		static int KillActiveHandle(lua_State* L);
		static int CallOutGetSynced(lua_State* L);
		static int CallOutGetFullCtrl(lua_State* L);
		static int CallOutGetFullRead(lua_State* L);
		static int CallOutGetCtrlTeam(lua_State* L);
		static int CallOutGetReadTeam(lua_State* L);
		static int CallOutGetReadAllyTeam(lua_State* L);
		static int CallOutGetSelectTeam(lua_State* L);

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
		static CLuaDisplayLists& GetActiveDisplayLists() {
			return activeHandle->displayLists;
		}

		static void SetDevMode(bool value) { devMode = value; }
		static bool GetDevMode() { return devMode; }


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
