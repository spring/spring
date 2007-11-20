#ifndef LUA_VFS_H
#define LUA_VFS_H
// LuaVFS.h: interface for the LuaVFS class.
//
//////////////////////////////////////////////////////////////////////

#include <string>
using std::string;

struct lua_State;


class LuaVFS {
	public:
		static bool PushSynced(lua_State* L);
		static bool PushUnsynced(lua_State* L);

	private:
		static const string GetModes(lua_State* L, int index, bool synced);

		static bool PushCommon(lua_State* L);

		static int Include(lua_State* L, bool synced);
		static int LoadFile(lua_State* L, bool synced);
		static int FileExists(lua_State* L, bool synced);
		static int DirList(lua_State* L, bool synced);
		static int SubDirs(lua_State* L, bool synced);

		static int SyncInclude(lua_State* L);
		static int SyncLoadFile(lua_State* L);
		static int SyncFileExists(lua_State* L);
		static int SyncDirList(lua_State* L);
		static int SyncSubDirs(lua_State* L);

		static int UnsyncInclude(lua_State* L);
		static int UnsyncLoadFile(lua_State* L);
		static int UnsyncFileExists(lua_State* L);
		static int UnsyncDirList(lua_State* L);
		static int UnsyncSubDirs(lua_State* L);

		static int UseArchive(lua_State* L);

		// string packing utilities
		static int PackU8(lua_State* L);
		static int PackU16(lua_State* L);
		static int PackU32(lua_State* L);
		static int PackS8(lua_State* L);
		static int PackS16(lua_State* L);
		static int PackS32(lua_State* L);
		static int PackF32(lua_State* L);
		static int UnpackU8(lua_State* L);
		static int UnpackU16(lua_State* L);
		static int UnpackU32(lua_State* L);
		static int UnpackS8(lua_State* L);
		static int UnpackS16(lua_State* L);
		static int UnpackS32(lua_State* L);
		static int UnpackF32(lua_State* L);
};


#endif /* LUA_VFS_H */
