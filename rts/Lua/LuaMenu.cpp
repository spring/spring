/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaMenu.h"

#include <mutex>
#include <boost/thread/mutex.hpp>

#include "LuaInclude.h"

#include "LuaArchive.h"
#include "LuaIO.h"
#include "LuaConstGL.h"
#include "LuaOpenGL.h"
#include "LuaScream.h"
#include "LuaUtils.h"
#include "LuaUnitDefs.h"
#include "LuaUnsyncedCtrl.h"
#include "LuaUnsyncedRead.h"
#include "LuaVFS.h"
#include "LuaVFSDownload.h"
#include "LuaZip.h"

#include "Game/GameVersion.h"
#include "System/EventHandler.h"
#include "System/Config/ConfigHandler.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "lib/luasocket/src/luasocket.h"

CLuaMenu* luaMenu = nullptr;

/******************************************************************************/
/******************************************************************************/

static boost::mutex m_singleton;

DECL_LOAD_HANDLER(CLuaMenu, luaMenu)
DECL_FREE_HANDLER(CLuaMenu, luaMenu)


/******************************************************************************/
/******************************************************************************/

CLuaMenu::CLuaMenu()
: CLuaHandle("LuaMenu", LUA_HANDLE_ORDER_MENU, true, false)
{
	luaMenu = this;

	if (!IsValid())
		return;

	const bool luaSocketEnabled = configHandler->GetBool("LuaSocketEnabled");
	const std::string file = "LuaMenu/main.lua";
	const std::string code = LoadFile(file);

	LOG("LuaMenu Entry Point: \"%s\"", file.c_str());
	//LOG("LuaSocket Enabled: %s", (luaSocketEnabled ? "yes": "no" ));

	if (code.empty()) {
		KillLua();
		return;
	}

	// load the standard libraries
	LUA_OPEN_LIB(L, luaopen_base);
	LUA_OPEN_LIB(L, luaopen_io);
	LUA_OPEN_LIB(L, luaopen_os);
	LUA_OPEN_LIB(L, luaopen_math);
	LUA_OPEN_LIB(L, luaopen_table);
	LUA_OPEN_LIB(L, luaopen_string);
	LUA_OPEN_LIB(L, luaopen_debug);

	//initialize luasocket
	if (luaSocketEnabled){
		InitLuaSocket(L);
	}

	// setup the lua IO access check functions
	lua_set_fopen(L, LuaIO::fopen);
	lua_set_popen(L, LuaIO::popen, LuaIO::pclose);
	lua_set_system(L, LuaIO::system);
	lua_set_remove(L, LuaIO::remove);
	lua_set_rename(L, LuaIO::rename);

	// remove a few dangerous calls
	lua_getglobal(L, "io");
	lua_pushstring(L, "popen"); lua_pushnil(L); lua_rawset(L, -3);
	lua_pop(L, 1);
	lua_getglobal(L, "os"); {
		lua_pushliteral(L, "exit");      lua_pushnil(L); lua_rawset(L, -3);
		lua_pushliteral(L, "execute");   lua_pushnil(L); lua_rawset(L, -3);
		//lua_pushliteral(L, "remove");    lua_pushnil(L); lua_rawset(L, -3);
		//lua_pushliteral(L, "rename");    lua_pushnil(L); lua_rawset(L, -3);
		lua_pushliteral(L, "tmpname");   lua_pushnil(L); lua_rawset(L, -3);
		lua_pushliteral(L, "getenv");    lua_pushnil(L); lua_rawset(L, -3);
		//lua_pushliteral(L, "setlocale"); lua_pushnil(L); lua_rawset(L, -3);
	}
	lua_pop(L, 1); // os

	lua_pushvalue(L, LUA_GLOBALSINDEX);

	AddBasicCalls(L); // into Global

	// load the spring libraries
	if (
	    !AddEntriesToTable(L, "Spring",    LoadUnsyncedCtrlFunctions)      ||
	    !AddEntriesToTable(L, "Spring",    LoadUnsyncedReadFunctions)      ||
	    !AddEntriesToTable(L, "Game",      PushGameVersion)                ||

	    !AddEntriesToTable(L, "Script",    LuaScream::PushEntries)         ||
	    !AddEntriesToTable(L, "VFS",       LuaVFS::PushUnsynced)           ||
	    !AddEntriesToTable(L, "VFS",       LuaZipFileReader::PushUnsynced) ||
	    !AddEntriesToTable(L, "VFS",       LuaZipFileWriter::PushUnsynced) ||
	    !AddEntriesToTable(L, "VFS",       LuaArchive::PushEntries)        ||
	    !AddEntriesToTable(L, "gl",        LuaOpenGL::PushEntries)         ||
	    !AddEntriesToTable(L, "GL",        LuaConstGL::PushEntries)        ||
	    !AddEntriesToTable(L, "LOG",       LuaUtils::PushLogEntries)       ||
	    !AddEntriesToTable(L, "VFS",       LuaVFSDownload::PushEntries)
	) {
		KillLua();
		return;
	}

	RemoveSomeOpenGLFunctions(L);

	lua_settop(L, 0);
	if (!LoadCode(L, code, file)) {
		KillLua();
		return;
	}

	// register for call-ins
	eventHandler.AddClient(this);

	lua_settop(L, 0);
}


CLuaMenu::~CLuaMenu()
{
	luaMenu = NULL;
}


string CLuaMenu::LoadFile(const string& name) const
{
	CFileHandler f(name, SPRING_VFS_MENU SPRING_VFS_MOD SPRING_VFS_BASE);

	string code;
	if (!f.LoadStringData(code))
		code.clear();

	return code;
}


void CLuaMenu::InitLuaSocket(lua_State* L) {
	std::string code;
	std::string filename = "socket.lua";
	CFileHandler f(filename);

	LUA_OPEN_LIB(L, luaopen_socket_core);

	if (f.LoadStringData(code)) {
		LoadCode(L, code, filename);
	} else {
		LOG_L(L_ERROR, "Error loading %s", filename.c_str());
	}
}


bool CLuaMenu::RemoveSomeOpenGLFunctions(lua_State* L)
{
	// remove some spring opengl functions that don't work preloading
	lua_getglobal(L, "gl"); {
		#define PUSHNIL(x) lua_pushliteral(L, #x); lua_pushnil(L); lua_rawset(L, -3)
		PUSHNIL(DrawMiniMap);
		PUSHNIL(SlaveMiniMap);
		PUSHNIL(ConfigMiniMap);

		//FIXME create a way to render model files
		PUSHNIL(Unit);
		PUSHNIL(UnitRaw);
		PUSHNIL(UnitShape);
		PUSHNIL(UnitMultMatrix);
		PUSHNIL(UnitPiece);
		PUSHNIL(UnitPieceMatrix);
		PUSHNIL(UnitPieceMultMatrix);
		PUSHNIL(Feature);
		PUSHNIL(FeatureShape);
		PUSHNIL(DrawListAtUnit);
		PUSHNIL(DrawFuncAtUnit);
		PUSHNIL(DrawGroundCircle);
		PUSHNIL(DrawGroundQuad);

		PUSHNIL(GetGlobalTexNames);
		PUSHNIL(GetGlobalTexCoords);
		PUSHNIL(GetShadowMapParams);
	}
	lua_pop(L, 1); // gl
	return true;
}


bool CLuaMenu::LoadUnsyncedCtrlFunctions(lua_State* L)
{
	#define REGISTER_LUA_CFUNC(x) \
		lua_pushstring(L, #x);      \
		lua_pushcfunction(L, LuaUnsyncedCtrl::x);    \
		lua_rawset(L, -3)

	REGISTER_LUA_CFUNC(Echo);
	REGISTER_LUA_CFUNC(Log);

	REGISTER_LUA_CFUNC(LoadSoundDef);
	REGISTER_LUA_CFUNC(PlaySoundFile);
	REGISTER_LUA_CFUNC(PlaySoundStream);
	REGISTER_LUA_CFUNC(StopSoundStream);
	REGISTER_LUA_CFUNC(PauseSoundStream);
	REGISTER_LUA_CFUNC(SetSoundStreamVolume);
	REGISTER_LUA_CFUNC(SetSoundEffectParams);

	//REGISTER_LUA_CFUNC(SetTeamColor);


	REGISTER_LUA_CFUNC(ExtractModArchiveFile);

	REGISTER_LUA_CFUNC(GetConfigInt);
	REGISTER_LUA_CFUNC(SetConfigInt);
	REGISTER_LUA_CFUNC(GetConfigString);
	REGISTER_LUA_CFUNC(SetConfigString);

	REGISTER_LUA_CFUNC(CreateDir);

	REGISTER_LUA_CFUNC(SetWMIcon);
	REGISTER_LUA_CFUNC(SetWMCaption);

	REGISTER_LUA_CFUNC(SetClipboard);
	REGISTER_LUA_CFUNC(AssignMouseCursor);
	REGISTER_LUA_CFUNC(ReplaceMouseCursor);
	REGISTER_LUA_CFUNC(SetMouseCursor);
	REGISTER_LUA_CFUNC(WarpMouse);


	REGISTER_LUA_CFUNC(SetLogSectionFilterLevel);

	REGISTER_LUA_CFUNC(Restart);
	REGISTER_LUA_CFUNC(Reload);
	REGISTER_LUA_CFUNC(Quit);
	REGISTER_LUA_CFUNC(Start);
	#undef REGISTER_LUA_CFUNC
	return true;
}


bool CLuaMenu::LoadUnsyncedReadFunctions(lua_State* L)
{
	#define REGISTER_LUA_CFUNC(x) \
		lua_pushstring(L, #x);      \
		lua_pushcfunction(L, LuaUnsyncedRead::x);    \
		lua_rawset(L, -3)

	REGISTER_LUA_CFUNC(GetViewGeometry);
	REGISTER_LUA_CFUNC(GetWindowGeometry);
	REGISTER_LUA_CFUNC(GetScreenGeometry);
	REGISTER_LUA_CFUNC(GetMiniMapDualScreen);
	REGISTER_LUA_CFUNC(GetDrawFrame);
	REGISTER_LUA_CFUNC(IsGUIHidden);

	REGISTER_LUA_CFUNC(GetTeamColor);
	REGISTER_LUA_CFUNC(GetTeamOrigColor);

	REGISTER_LUA_CFUNC(GetLocalPlayerID);
	REGISTER_LUA_CFUNC(GetLocalTeamID);
	REGISTER_LUA_CFUNC(GetLocalAllyTeamID);
	REGISTER_LUA_CFUNC(GetSpectatingState);

	REGISTER_LUA_CFUNC(GetTimer);
	REGISTER_LUA_CFUNC(DiffTimers);

	REGISTER_LUA_CFUNC(GetSoundStreamTime);
	REGISTER_LUA_CFUNC(GetSoundEffectParams);

	REGISTER_LUA_CFUNC(GetClipboard);
	REGISTER_LUA_CFUNC(GetMouseState);
	REGISTER_LUA_CFUNC(GetMouseCursor);

	REGISTER_LUA_CFUNC(GetKeyState);
	REGISTER_LUA_CFUNC(GetModKeyState);
	REGISTER_LUA_CFUNC(GetPressedKeys);
	REGISTER_LUA_CFUNC(GetInvertQueueKey);

	REGISTER_LUA_CFUNC(GetKeyCode);
	REGISTER_LUA_CFUNC(GetKeySymbol);
	REGISTER_LUA_CFUNC(GetKeyBindings);
	REGISTER_LUA_CFUNC(GetActionHotKeys);

	REGISTER_LUA_CFUNC(GetLogSections);
	REGISTER_LUA_CFUNC(GetConfigParams);

	#undef REGISTER_LUA_CFUNC
	return true;
}

bool CLuaMenu::PushGameVersion(lua_State* L)
{
	LuaPushNamedString(L, "version", SpringVersion::GetSync());
	LuaPushNamedString(L, "versionFull", (!CLuaHandle::GetHandleSynced(L))? SpringVersion::GetFull(): "");
	LuaPushNamedString(L, "versionPatchSet", (!CLuaHandle::GetHandleSynced(L))? SpringVersion::GetPatchSet(): "");
	LuaPushNamedString(L, "buildFlags", (!CLuaHandle::GetHandleSynced(L))? SpringVersion::GetAdditional(): "");
	return true;
}



/******************************************************************************/
/******************************************************************************/
