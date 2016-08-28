/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <mutex>
#include <boost/thread/mutex.hpp>

#include "LuaIntro.h"

#include "LuaInclude.h"
#include "LuaArchive.h"
#include "LuaUnsyncedCtrl.h"
#include "LuaCallInCheck.h"
#include "LuaConstGL.h"
#include "LuaConstCMD.h"
#include "LuaConstCMDTYPE.h"
#include "LuaConstGame.h"
#include "LuaInterCall.h"
#include "LuaUnsyncedRead.h"
#include "LuaScream.h"
#include "LuaSyncedRead.h"
#include "LuaOpenGL.h"
#include "LuaUtils.h"
#include "LuaVFS.h"
#include "LuaIO.h"
#include "LuaZip.h"
#include "Rendering/IconHandler.h"
#include "System/EventHandler.h"
#include "System/Log/ILog.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Config/ConfigHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Util.h"


CLuaIntro* LuaIntro = NULL;

/******************************************************************************/
/******************************************************************************/

static boost::mutex m_singleton;

DECL_LOAD_HANDLER(CLuaIntro, LuaIntro)
DECL_FREE_HANDLER(CLuaIntro, LuaIntro)


/******************************************************************************/

CLuaIntro::CLuaIntro()
: CLuaHandle("LuaIntro", LUA_HANDLE_ORDER_INTRO, true, false)
{
	LuaIntro = this;

	if (!IsValid()) {
		return;
	}

	const std::string file = "LuaIntro/main.lua";

	const string code = LoadFile(file);
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
	    !AddEntriesToTable(L, "Spring",    LoadUnsyncedCtrlFunctions) ||
	    !AddEntriesToTable(L, "Spring",    LoadUnsyncedReadFunctions) ||
	    !AddEntriesToTable(L, "Spring",    LoadSyncedReadFunctions) ||

	    !AddEntriesToTable(L, "VFS",       LuaVFS::PushUnsynced)         ||
	    !AddEntriesToTable(L, "VFS",       LuaZipFileReader::PushUnsynced) ||
	    !AddEntriesToTable(L, "VFS",       LuaZipFileWriter::PushUnsynced) ||
	    !AddEntriesToTable(L, "VFS",         LuaArchive::PushEntries)      ||
	    !AddEntriesToTable(L, "Script",      LuaScream::PushEntries)       ||
	    //!AddEntriesToTable(L, "Script",      LuaInterCall::PushEntriesUnsynced) ||
	    //!AddEntriesToTable(L, "Script",      LuaLobby::PushEntries)        ||
	    !AddEntriesToTable(L, "gl",          LuaOpenGL::PushEntries)       ||
	    !AddEntriesToTable(L, "GL",          LuaConstGL::PushEntries)      ||
	    !AddEntriesToTable(L, "Game",        LuaConstGame::PushEntries)    ||
	    !AddEntriesToTable(L, "CMD",         LuaConstCMD::PushEntries)     ||
	    !AddEntriesToTable(L, "CMDTYPE",     LuaConstCMDTYPE::PushEntries) ||
	    !AddEntriesToTable(L, "LOG",         LuaUtils::PushLogEntries)
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

	lua_settop(L, 0);

	// register for call-ins
	eventHandler.AddClient(this);
}


CLuaIntro::~CLuaIntro()
{
	LuaIntro = NULL;
}


bool CLuaIntro::RemoveSomeOpenGLFunctions(lua_State* L)
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


bool CLuaIntro::LoadUnsyncedCtrlFunctions(lua_State* L)
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

	//REGISTER_LUA_CFUNC(AssignMouseCursor);
	//REGISTER_LUA_CFUNC(ReplaceMouseCursor);

	REGISTER_LUA_CFUNC(ExtractModArchiveFile);

	REGISTER_LUA_CFUNC(GetConfigInt);
	REGISTER_LUA_CFUNC(SetConfigInt);
	REGISTER_LUA_CFUNC(GetConfigString);
	REGISTER_LUA_CFUNC(SetConfigString);

	REGISTER_LUA_CFUNC(CreateDir);

	//REGISTER_LUA_CFUNC(SetMouseCursor);
	//REGISTER_LUA_CFUNC(WarpMouse);

	//REGISTER_LUA_CFUNC(Restart);
	REGISTER_LUA_CFUNC(SetWMIcon);
	REGISTER_LUA_CFUNC(SetWMCaption);

	REGISTER_LUA_CFUNC(SetLogSectionFilterLevel);

	#undef REGISTER_LUA_CFUNC
	return true;
}


bool CLuaIntro::LoadUnsyncedReadFunctions(lua_State* L)
{
	#define REGISTER_LUA_CFUNC(x) \
		lua_pushstring(L, #x);      \
		lua_pushcfunction(L, LuaUnsyncedRead::x);    \
		lua_rawset(L, -3)

	REGISTER_LUA_CFUNC(IsReplay);

	REGISTER_LUA_CFUNC(GetViewGeometry);
	REGISTER_LUA_CFUNC(GetWindowGeometry);
	REGISTER_LUA_CFUNC(GetScreenGeometry);
	REGISTER_LUA_CFUNC(GetMiniMapDualScreen);

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

	#undef REGISTER_LUA_CFUNC
	return true;
}


bool CLuaIntro::LoadSyncedReadFunctions(lua_State* L)
{
	#define REGISTER_LUA_CFUNC(x) \
		lua_pushstring(L, #x);      \
		lua_pushcfunction(L, LuaSyncedRead::x);    \
		lua_rawset(L, -3)

	REGISTER_LUA_CFUNC(AreHelperAIsEnabled);
	REGISTER_LUA_CFUNC(FixedAllies);

	REGISTER_LUA_CFUNC(GetGaiaTeamID);

	REGISTER_LUA_CFUNC(GetMapOptions);
	REGISTER_LUA_CFUNC(GetModOptions);

	REGISTER_LUA_CFUNC(GetSideData);

	REGISTER_LUA_CFUNC(GetAllyTeamStartBox);
	REGISTER_LUA_CFUNC(GetTeamStartPosition);

	REGISTER_LUA_CFUNC(GetPlayerList);
	REGISTER_LUA_CFUNC(GetTeamList);
	REGISTER_LUA_CFUNC(GetAllyTeamList);

	REGISTER_LUA_CFUNC(GetPlayerInfo);
	REGISTER_LUA_CFUNC(GetAIInfo);

	REGISTER_LUA_CFUNC(GetTeamInfo);
	REGISTER_LUA_CFUNC(GetTeamLuaAI);

	REGISTER_LUA_CFUNC(GetAllyTeamInfo);
	REGISTER_LUA_CFUNC(AreTeamsAllied);
	REGISTER_LUA_CFUNC(ArePlayersAllied);

	//REGISTER_LUA_CFUNC(GetGroundHeight);
	//REGISTER_LUA_CFUNC(GetGroundOrigHeight);
	//REGISTER_LUA_CFUNC(GetGroundNormal);
	//REGISTER_LUA_CFUNC(GetGroundInfo);
	//REGISTER_LUA_CFUNC(GetGroundExtremes);

	#undef REGISTER_LUA_CFUNC
	return true;
}


string CLuaIntro::LoadFile(const string& filename) const
{
	CFileHandler f(filename, SPRING_VFS_RAW_FIRST);

	string code;
	if (!f.LoadStringData(code)) {
		code.clear();
	}
	return code;
}


/******************************************************************************/
/******************************************************************************/

void CLuaIntro::DrawLoadScreen()
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 2, __FUNCTION__);
	static const LuaHashString cmdStr("DrawLoadScreen");
	if (!cmdStr.GetGlobalFunc(L)) {
		//LuaOpenGL::DisableCommon(LuaOpenGL::DRAW_SCREEN);
		return; // the call is not defined
	}

	LuaOpenGL::SetDrawingEnabled(L, true);
	LuaOpenGL::EnableCommon(LuaOpenGL::DRAW_SCREEN);

	// call the routine
	RunCallIn(L, cmdStr, 0, 0);

	LuaOpenGL::DisableCommon(LuaOpenGL::DRAW_SCREEN);
	LuaOpenGL::SetDrawingEnabled(L, false);
}


void CLuaIntro::LoadProgress(const std::string& msg, const bool replace_lastline)
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 4, __FUNCTION__);
	static const LuaHashString cmdStr("LoadProgress");
	if (!cmdStr.GetGlobalFunc(L)) {
		return;
	}

	lua_pushsstring(L, msg);
	lua_pushboolean(L, replace_lastline);

	// call the routine
	RunCallIn(L, cmdStr, 2, 0);
}

/******************************************************************************/
/******************************************************************************/
