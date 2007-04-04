#ifndef LUA_HANDLE_SYNCED
#define LUA_HANDLE_SYNCED
// LuaHandleSynced.h: interface for the CLuaHandleSynced class.
//
//////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include <string>
using std::string;


#include "LuaHandle.h"


struct lua_State;


class CLuaHandleSynced : public CLuaHandle
{
	public:
		bool SetupSynced(const string& code, const string& filename);
		bool SetupUnsynced(const string& code, const string& filename);

		bool Initialize(const string& syncData);
		string GetSyncData();

	public: // call-ins
		bool HasCallIn(const string& callInName);

		void GameFrame(int frameNumber);
		bool GotChatMsg(const string& msg, int playerID);
		void RecvFromSynced(int args); // not an engine call-in

	protected:
		CLuaHandleSynced(const string& name, int order,
		                 LuaCobCallback callback,
		                 const string& msgPrefix);
		virtual ~CLuaHandleSynced();
		void Init(const string& syncedFile,
		          const string& unsyncedFile);

		// hooks to add code during initialization
		virtual bool AddSyncedCode() = 0;
		virtual bool AddUnsyncedCode() = 0;

		string LoadFile(const string& filename) const;
		string LoadMapFile(const string& filename) const;
		string LoadModFile(const string& filename) const;

		bool CopyGlobalToUnsynced(const char* name);
		bool SetupUnsyncedFunction(const char* funcName);
		bool LoadUnsyncedCode(const string& code, const string& debug);
		bool SyncifyRandomFuncs();
		bool CopyRealRandomFuncs();
		bool LightCopyTable(int dstIndex, int srcIndex);

	protected:
		static CLuaHandleSynced* GetActiveHandle() {
			return dynamic_cast<CLuaHandleSynced*>(activeHandle);
		}

	protected:
		bool teamsLocked; // disables CallAsTeam()
		const string messagePrefix;

	private:
		void KillLua();
		void SendCallbacks();

	private: // call-outs
		static int SyncedRandom(lua_State* L);

		static int Include(lua_State* L);
		static int LoadString(lua_State* L);

		static int SendToUnsynced(lua_State* L);

		static int CallAsTeam(lua_State* L);

		static int UpdateCallIn(lua_State* L);
};


#endif /* LUA_HANDLE_SYNCED */
