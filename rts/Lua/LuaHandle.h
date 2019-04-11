/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_HANDLE_H
#define LUA_HANDLE_H
#include <cinttypes>

#include "System/EventClient.h"
//FIXME#include "LuaArrays.h"
#include "LuaContextData.h"
#include "LuaHashString.h"
#include "lib/lua/include/LuaInclude.h" //FIXME needed for GetLuaContextData

#include <string>
#include <vector>

#define LUA_HANDLE_ORDER_RULES            100
#define LUA_HANDLE_ORDER_UNITS            200
#define LUA_HANDLE_ORDER_GAIA             300
#define LUA_HANDLE_ORDER_RULES_UNSYNCED  1100
#define LUA_HANDLE_ORDER_UNITS_UNSYNCED  1200
#define LUA_HANDLE_ORDER_GAIA_UNSYNCED   1300
#define LUA_HANDLE_ORDER_UI              2000
#define LUA_HANDLE_ORDER_INTRO           3000
#define LUA_HANDLE_ORDER_MENU            4000


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
class CLuaRules;


class CLuaHandle : public CEventClient
{
	public:
		void CheckStack();
		int GetCallInErrors() const { return callinErrors; }
		void ResetCallinErrors() { callinErrors = 0; }

	public:
	#define PERMISSIONS_FUNCS(Name, type, dataArg, OVERRIDE) \
		void Set ## Name(type _ ## dataArg) { GetLuaContextData(L)->dataArg = _ ## dataArg; } \
		type Get ## Name() const OVERRIDE { return GetLuaContextData(L)->dataArg; } \
		static void SetHandle ## Name(const lua_State* L, type _ ## dataArg) { GetLuaContextData(L)->dataArg = _ ## dataArg;; } \
		static type GetHandle ## Name(const lua_State* L) { return GetLuaContextData(L)->dataArg; }

		PERMISSIONS_FUNCS(FullRead,     bool, fullRead, override); // virtual function in CEventClient
		PERMISSIONS_FUNCS(FullCtrl,     bool, fullCtrl, );
		PERMISSIONS_FUNCS(CtrlTeam,     int,  ctrlTeam, );
		PERMISSIONS_FUNCS(ReadTeam,     int,  readTeam, );
		PERMISSIONS_FUNCS(ReadAllyTeam, int,  readAllyTeam, override); // virtual function in CEventClient
		PERMISSIONS_FUNCS(SelectTeam,   int,  selectTeam, );

	#undef PERMISSIONS_FUNCS

		static bool GetHandleSynced(const lua_State* L) { return GetLuaContextData(L)->synced; }

		bool GetUserMode() const { return userMode; }

		static bool GetHandleUserMode(lua_State* L) { return (GetHandle(L))->GetUserMode(); }

		static int GetHandleAllowChanges(const lua_State* L) { return GetLuaContextData(L)->allowChanges; }

		static CLuaHandle* GetHandle(lua_State* L) { return (GetLuaContextData(L)->owner); }

		static void SetHandleRunning(lua_State* L, const bool _running) {
			GetLuaContextData(L)->running += (_running) ? +1 : -1;
			assert(GetLuaContextData(L)->running >= 0);
		}
		static bool IsHandleRunning(lua_State* L) { return (GetLuaContextData(L)->running > 0); }

		bool IsRunning() const { return IsHandleRunning(L); }
		bool IsValid() const { return (L != nullptr); }

		// virtual bool PersistOnReload() const { return (GetName() == "LuaMenu"); }
		virtual bool PersistOnReload() const { return false; }

		// used by LuaSyncedTable and creg save
		lua_State* GetLuaState() const { return L; }
		lua_State* GetLuaGCState() const { return L_GC; }
		// used by creg load
		void SetLuaStates(lua_State* L_, lua_State* L_GC_) { L = L_; L_GC = L_GC_; }

#if (!defined(UNITSYNC) && !defined(DEDICATED))
		LuaShaders& GetShaders(const lua_State* L = nullptr) { return GetLuaContextData(L)->shaders; }
		LuaTextures& GetTextures(const lua_State* L = nullptr) { return GetLuaContextData(L)->textures; }
		LuaFBOs& GetFBOs(const lua_State* L = nullptr) { return GetLuaContextData(L)->fbos; }
		LuaRBOs& GetRBOs(const lua_State* L = nullptr) { return GetLuaContextData(L)->rbos; }
#endif

	public: // call-ins
		bool WantsEvent(const std::string& name) override { return HasCallIn(L, name); }
		virtual bool HasCallIn(lua_State* L, const std::string& name) const;
		virtual bool UpdateCallIn(lua_State* L, const std::string& name);

		void Load(IArchive* archive) override;

		void GamePreload() override;
		void GameStart() override;
		void GameOver(const std::vector<unsigned char>& winningAllyTeams) override;
		void GamePaused(int playerID, bool paused) override;
		void GameFrame(int frameNum) override;
		void GameID(const unsigned char* gameID, unsigned int numBytes) override;

		void TeamDied(int teamID) override;
		void TeamChanged(int teamID) override;
		void PlayerChanged(int playerID) override;
		void PlayerAdded(int playerID) override;
		void PlayerRemoved(int playerID, int reason) override;

		void UnitCreated(const CUnit* unit, const CUnit* builder) override;
		void UnitFinished(const CUnit* unit) override;
		void UnitFromFactory(const CUnit* unit, const CUnit* factory, bool userOrders) override;
		void UnitReverseBuilt(const CUnit* unit) override;
		void UnitDestroyed(const CUnit* unit, const CUnit* attacker) override;
		void UnitTaken(const CUnit* unit, int oldTeam, int newTeam) override;
		void UnitGiven(const CUnit* unit, int oldTeam, int newTeam) override;

		void UnitIdle(const CUnit* unit) override;
		void UnitCommand(const CUnit* unit, const Command& command) override;
		void UnitCmdDone(const CUnit* unit, const Command& command) override;
		void UnitDamaged(
			const CUnit* unit,
			const CUnit* attacker,
			float damage,
			int weaponDefID,
			int projectileID,
			bool paralyzer
		) override;
		void UnitStunned(const CUnit* unit, bool stunned) override;
		void UnitExperience(const CUnit* unit, float oldExperience) override;
		void UnitHarvestStorageFull(const CUnit* unit) override;

		void UnitSeismicPing(const CUnit* unit, int allyTeam,
		                     const float3& pos, float strength) override;
		void UnitEnteredRadar(const CUnit* unit, int allyTeam) override;
		void UnitEnteredLos(const CUnit* unit, int allyTeam) override;
		void UnitLeftRadar(const CUnit* unit, int allyTeam) override;
		void UnitLeftLos(const CUnit* unit, int allyTeam) override;

		void UnitEnteredWater(const CUnit* unit) override;
		void UnitEnteredAir(const CUnit* unit) override;
		void UnitLeftWater(const CUnit* unit) override;
		void UnitLeftAir(const CUnit* unit) override;

		void UnitLoaded(const CUnit* unit, const CUnit* transport) override;
		void UnitUnloaded(const CUnit* unit, const CUnit* transport) override;

		void UnitCloaked(const CUnit* unit) override;
		void UnitDecloaked(const CUnit* unit) override;

		bool UnitUnitCollision(const CUnit* collider, const CUnit* collidee) override;
		bool UnitFeatureCollision(const CUnit* collider, const CFeature* collidee) override;
		void UnitMoveFailed(const CUnit* unit) override;

		void RenderUnitDestroyed(const CUnit* unit) override;

		void FeatureCreated(const CFeature* feature) override;
		void FeatureDestroyed(const CFeature* feature) override;
		void FeatureDamaged(
			const CFeature* feature,
			const CUnit* attacker,
			float damage,
			int weaponDefID,
			int projectileID
		) override;

		void ProjectileCreated(const CProjectile* p) override;
		void ProjectileDestroyed(const CProjectile* p) override;

		bool Explosion(int weaponID, int projectileID, const float3& pos, const CUnit* owner) override;

		void StockpileChanged(const CUnit* owner,
		                      const CWeapon* weapon, int oldCount) override;

		void Save(zipFile archive) override;

		void UnsyncedHeightMapUpdate(const SRectangle& rect) override;
		void Update() override;

		bool KeyPress(int key, bool isRepeat) override;
		bool KeyRelease(int key) override;
		bool TextInput(const std::string& utf8) override;
		bool TextEditing(const std::string& utf8, unsigned int start, unsigned int length) override;
		bool MouseMove(int x, int y, int dx, int dy, int button) override;
		bool MousePress(int x, int y, int button) override;
		void MouseRelease(int x, int y, int button) override;
		bool MouseWheel(bool up, float value) override;
		bool IsAbove(int x, int y) override;
		std::string GetTooltip(int x, int y) override;

		bool DefaultCommand(const CUnit* unit, const CFeature* feature, int& cmd) override;

		bool CommandNotify(const Command& cmd) override;

		bool AddConsoleLine(const std::string& msg, const std::string& section, int level) override;

		bool GroupChanged(int groupID) override;

		bool GameSetup(const std::string& state, bool& ready,
		               const std::vector< std::pair<int, std::string> >& playerStates) override;

		const char* RecvSkirmishAIMessage(int aiID, const char* data, int inSize, size_t* outSize);

		std::string WorldTooltip(
			const CUnit* unit,
			const CFeature* feature,
			const float3* groundPos
		) override;

		bool MapDrawCmd(
			int playerID,
			int type,
			const float3* pos0,
			const float3* pos1,
			const std::string* label
		) override;

		void ViewResize() override;

		void SunChanged() override;

		void DrawGenesis() override;
		void DrawWater() override;
		void DrawSky() override;
		void DrawSun() override;
		void DrawGrass() override;
		void DrawTrees() override;
		void DrawWorld() override;
		void DrawWorldPreUnit() override;
		void DrawWorldPreParticles() override;
		void DrawWorldShadow() override;
		void DrawWorldReflection() override;
		void DrawWorldRefraction() override;
		void DrawGroundPreForward() override;
		void DrawGroundPostForward() override;
		void DrawGroundPreDeferred() override;
		void DrawGroundPostDeferred() override;
		void DrawUnitsPostDeferred() override;
		void DrawFeaturesPostDeferred() override;
		void DrawScreenCommon(const LuaHashString& cmdStr);
		void DrawScreenEffects() override;
		void DrawScreenPost()  override;
		void DrawScreen() override;
		void DrawInMiniMap() override;
		void DrawInMiniMapBackground() override;

		void GameProgress(int frameNum) override;
		void Pong(uint8_t pingTag, const spring_time pktSendTime, const spring_time pktRecvTime) override;

		//FIXME void MetalMapChanged(const int x, const int z);

		void CollectGarbage() override;

		void DownloadQueued(int ID, const std::string& archiveName, const std::string& archiveType) override;
		void DownloadStarted(int ID) override;
		void DownloadFinished(int ID) override;
		void DownloadFailed(int ID, int errorID) override;
		void DownloadProgress(int ID, long downloaded, long total) override;

	public: // Non-eventhandler call-ins
		void Shutdown();
		bool GotChatMsg(const std::string& msg, int playerID);
		bool RecvLuaMsg(const std::string& msg, int playerID);

	public: // custom call-in  (inter-script calls)
		bool HasXCall(const std::string& funcName) const { return HasCallIn(L, funcName); }
		int XCall(lua_State* srcState, const char* funcName);

	protected:
		CLuaHandle(const std::string& name, int order, bool userMode, bool synced);
		virtual ~CLuaHandle();

		void KillLua(bool inFreeHandler = false);

		static void PushTracebackFuncToRegistry(lua_State* L);

		bool AddBasicCalls(lua_State* L);
		bool LoadCode(lua_State* L, const std::string& code, const std::string& debug);
		static bool AddEntriesToTable(lua_State* L, const char* name, bool (*entriesFunc)(lua_State*));

		/// returns error code and sets traceback on error
		int  RunCallInTraceback(lua_State* L, const LuaHashString* hs, std::string* ts, int inArgs, int outArgs, int errFuncIndex, bool popErrFunc);
		/// returns false and prints message to log on error
		bool RunCallInTraceback(lua_State* L, const LuaHashString& hs, int inArgs, int outArgs, int errFuncIndex, bool popErrFunc = true);
		/// returns false and and sets errormessage on error
		bool RunCallInLUS(lua_State* L, std::string* ts, int inArgs, int outArgs);
		/// returns false and prints message to log on error
		bool RunCallIn(lua_State* L, const LuaHashString& hs, int inArgs, int outArgs);

		void LosCallIn(const LuaHashString& hs, const CUnit* unit, int allyTeam);
		void UnitCallIn(const LuaHashString& hs, const CUnit* unit);

		void RunDrawCallIn(const LuaHashString& hs);

	protected:
		bool userMode = false;
		bool killMe = false; // set for handles that fail to RunCallIn

		int callinErrors = 0;

		lua_State* L;
		lua_State* L_GC;
		luaContextData D;

		std::string killMsg;

		std::vector<bool> watchUnitDefs;        // callin masks for Unit*Collision, UnitMoveFailed
		std::vector<bool> watchFeatureDefs;     // callin masks for UnitFeatureCollision
		std::vector<bool> watchProjectileDefs;  // callin masks for Projectile*
		std::vector<bool> watchExplosionDefs;   // callin masks for Explosion
		std::vector<bool> watchAllowTargetDefs; // callin masks for AllowWeapon*Target*

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
#if (!defined(UNITSYNC) && !defined(DEDICATED))
		static inline LuaShaders& GetActiveShaders(lua_State* L) { return GetLuaContextData(L)->shaders; }
		static inline LuaTextures& GetActiveTextures(lua_State* L) { return GetLuaContextData(L)->textures; }
		static inline LuaFBOs& GetActiveFBOs(lua_State* L) { return GetLuaContextData(L)->fbos; }
		static inline LuaRBOs& GetActiveRBOs(lua_State* L) { return GetLuaContextData(L)->rbos; }
#endif

		static void SetDevMode(bool value) { devMode = value; }
		static bool GetDevMode() { return devMode; }

		static void HandleLuaMsg(int playerID, int script, int mode, const std::vector<std::uint8_t>& msg);

	protected: // static
		static bool devMode; // allows real file access

		// FIXME: because CLuaUnitScript needs to access RunCallIn
		friend class CLuaUnitScript;

		// FIXME needs access to L & RunCallIn
		friend class CLuaRules;
};


inline bool CLuaHandle::RunCallIn(lua_State* L, const LuaHashString& hs, int inArgs, int outArgs)
{
	return RunCallInTraceback(L, hs, inArgs, outArgs, 0, false);
}

inline bool CLuaHandle::RunCallInLUS(lua_State* L, std::string* ts, int inArgs, int outArgs)
{
	return RunCallInTraceback(L, nullptr, ts, inArgs, outArgs, 0, false);
}


/******************************************************************************/
/******************************************************************************/


#endif /* LUA_HANDLE_H */
