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

class CLuaHandleSynced : public CLuaHandle
{
	public:
		static const LuaRulesParams::Params&  GetGameParams() {return gameParams;};
		static const LuaRulesParams::HashMap& GetGameParamsMap() {return gameParamsMap;};

	public:
		bool Initialize(const string& syncData);
		string GetSyncData();

		inline bool GetAllowChanges() const { return IsDrawCallIn() ? false : allowChanges; }
		inline void SetAllowChanges(bool ac, bool all = false) { if(!IsDrawCallIn() || all) allowChanges = ac; }

	public: // call-ins
		bool HasCallIn(lua_State *L, const string& name);
		virtual bool SyncedUpdateCallIn(lua_State *L, const string& name);
		virtual bool UnsyncedUpdateCallIn(lua_State *L, const string& name);

		virtual void GameFrame(int frameNumber);
		bool GotChatMsg(const string& msg, int playerID);
		bool RecvLuaMsg(const string& msg, int playerID);
		virtual void RecvFromSynced(int args); // not an engine call-in

		bool SyncedActionFallback(const string& line, int playerID);

	public: // custom call-in
		bool HasSyncedXCall(const string& funcName);
		bool HasUnsyncedXCall(const string& funcName);
		int XCall(lua_State* L, lua_State* srcState, const string& funcName);
		int SyncedXCall(lua_State* srcState, const string& funcName);
		int UnsyncedXCall(lua_State* srcState, const string& funcName);

	protected:
		CLuaHandleSynced(const string& name, int order);
		virtual ~CLuaHandleSynced();
		void Init(const string& syncedFile,
		          const string& unsyncedFile,
		          const string& modes);
		bool SetupSynced(lua_State *L, const string& code, const string& filename);
		bool SetupUnsynced(lua_State *L, const string& code, const string& filename);

		// hooks to add code during initialization
		virtual bool AddSyncedCode(lua_State *L) = 0;
		virtual bool AddUnsyncedCode(lua_State *L) = 0;

		string LoadFile(const string& filename, const string& modes) const;

		bool CopyGlobalToUnsynced(lua_State *L, const char* name);
		bool SetupUnsyncedFunction(lua_State *L, const char* funcName);
		bool LoadUnsyncedCode(lua_State *L, const string& code, const string& debug);
		bool SyncifyRandomFuncs(lua_State *L);
		bool CopyRealRandomFuncs(lua_State *L);
		bool LightCopyTable(lua_State *L, int dstIndex, int srcIndex);

	protected:
		static CLuaHandleSynced* GetActiveHandle() {
			return dynamic_cast<CLuaHandleSynced*>(CLuaHandle::GetActiveHandle());
		}

	private:
		bool allowChanges;
	protected:
		bool teamsLocked; // disables CallAsTeam()
		map<string, string> textCommands; // name, help

	private: // call-outs
		static int SyncedRandom(lua_State* L);

		static int LoadStringData(lua_State* L);

		static int CallAsTeam(lua_State* L);

		static int AllowUnsafeChanges(lua_State* L);

		static int AddSyncedActionFallback(lua_State* L);
		static int RemoveSyncedActionFallback(lua_State* L);

		static int GetWatchWeapon(lua_State* L);
		static int SetWatchWeapon(lua_State* L);

	private:
		//FIXME: add to CREG?
		static LuaRulesParams::Params  gameParams;
		static LuaRulesParams::HashMap gameParamsMap;
		friend class LuaSyncedCtrl;
};


#endif /* LUA_HANDLE_SYNCED */
