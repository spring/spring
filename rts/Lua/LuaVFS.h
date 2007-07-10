#ifndef LUA_VFS_H
#define LUA_VFS_H
// LuaVFS.h: interface for the LuaVFS class.
//
//////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif


struct lua_State;


class LuaVFS {
	public:
		static bool PushSynced(lua_State* L);
		static bool PushUnsynced(lua_State* L);

		enum AccessMode {
			RAW_ONLY  = 0,
			ZIP_ONLY  = 1,
			RAW_FIRST = 2,
			ZIP_FIRST = 3,
			LAST_MODE = ZIP_FIRST
		};

	private:
		static AccessMode GetMode(lua_State* L, int index, bool synced);

		static int Include(lua_State* L, bool synced);
		static int LoadFile(lua_State* L, bool synced);
		static int FileExists(lua_State* L, bool synced);
		static int DirList(lua_State* L, bool synced);

		static int SyncInclude(lua_State* L);
		static int SyncLoadFile(lua_State* L);
		static int SyncFileExists(lua_State* L);
		static int SyncDirList(lua_State* L);

		static int UnsyncInclude(lua_State* L);
		static int UnsyncLoadFile(lua_State* L);
		static int UnsyncFileExists(lua_State* L);
		static int UnsyncDirList(lua_State* L);

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
