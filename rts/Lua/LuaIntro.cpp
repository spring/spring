/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaIntro.h"

#include "LuaInclude.h"
#include "LuaArchive.h"
#include "LuaUnsyncedCtrl.h"
#include "LuaCallInCheck.h"
#include "LuaConstGL.h"
#include "LuaConstCMD.h"
#include "LuaConstCMDTYPE.h"
#include "LuaConstEngine.h"
#include "LuaConstGame.h"
#include "LuaConstPlatform.h"
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
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/Log/ILog.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Threading/SpringThreading.h"
#include "System/StringUtil.h"


CLuaIntro* luaIntro = nullptr;

/******************************************************************************/
/******************************************************************************/

static spring::mutex m_singleton;

DECL_LOAD_HANDLER(CLuaIntro, luaIntro)
DECL_FREE_HANDLER(CLuaIntro, luaIntro)


/******************************************************************************/

CLuaIntro::CLuaIntro()
: CLuaHandle("LuaIntro", LUA_HANDLE_ORDER_INTRO, true, false)
{
	luaIntro = this;

	if (!IsValid())
		return;

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
	    !AddEntriesToTable(L, "Spring",    LoadUnsyncedCtrlFunctions)           ||
	    !AddEntriesToTable(L, "Spring",    LoadUnsyncedReadFunctions)           ||
	    !AddEntriesToTable(L, "Spring",    LoadSyncedReadFunctions  )           ||

	    !AddEntriesToTable(L, "VFS",       LuaVFS::PushUnsynced)                ||
	    !AddEntriesToTable(L, "VFS",       LuaZipFileReader::PushUnsynced)      ||
	    !AddEntriesToTable(L, "VFS",       LuaZipFileWriter::PushUnsynced)      ||
	    !AddEntriesToTable(L, "VFS",         LuaArchive::PushEntries)           ||
	    !AddEntriesToTable(L, "Script",      LuaScream::PushEntries)            ||
	    // !AddEntriesToTable(L, "Script",      LuaInterCall::PushEntriesUnsynced) ||
	    !AddEntriesToTable(L, "gl",          LuaOpenGL::PushEntries)            ||
	    !AddEntriesToTable(L, "GL",          LuaConstGL::PushEntries)           ||
	    !AddEntriesToTable(L, "Engine",      LuaConstEngine::PushEntries)       ||
	    !AddEntriesToTable(L, "Platform",    LuaConstPlatform::PushEntries)       ||
	    !AddEntriesToTable(L, "Game",        LuaConstGame::PushEntries)         ||
	    !AddEntriesToTable(L, "CMD",         LuaConstCMD::PushEntries)          ||
	    !AddEntriesToTable(L, "CMDTYPE",     LuaConstCMDTYPE::PushEntries)      ||
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
	luaIntro = nullptr;
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
	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedCtrl, Echo);
	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedCtrl, Log);

	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedCtrl, LoadSoundDef);
	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedCtrl, PlaySoundFile);
	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedCtrl, PlaySoundStream);
	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedCtrl, StopSoundStream);
	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedCtrl, PauseSoundStream);
	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedCtrl, SetSoundStreamVolume);
	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedCtrl, SetSoundEffectParams);

	//REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedCtrl, SetTeamColor);

	//REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedCtrl, AssignMouseCursor);
	//REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedCtrl, ReplaceMouseCursor);

	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedCtrl, ExtractModArchiveFile);

	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedCtrl, SetConfigInt);
	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedCtrl, SetConfigFloat);
	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedCtrl, SetConfigString);

	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedCtrl, CreateDir);

	//REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedCtrl, SetMouseCursor);
	//REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedCtrl, WarpMouse);

	//REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedCtrl, Restart);
	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedCtrl, SetWMIcon);
	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedCtrl, SetWMCaption);

	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedCtrl, SetLogSectionFilterLevel);

	return true;
}


bool CLuaIntro::LoadUnsyncedReadFunctions(lua_State* L)
{
	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedRead, IsReplay);

	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedRead, GetViewGeometry);
	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedRead, GetWindowGeometry);
	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedRead, GetScreenGeometry);
	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedRead, GetMiniMapDualScreen);

	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedRead, GetTeamColor);
	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedRead, GetTeamOrigColor);

	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedRead, GetLocalPlayerID);
	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedRead, GetLocalTeamID);
	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedRead, GetLocalAllyTeamID);
	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedRead, GetSpectatingState);

	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedRead, GetTimer);
	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedRead, DiffTimers);

	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedRead, GetSoundStreamTime);
	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedRead, GetSoundEffectParams);

	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedRead, GetMouseState);
	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedRead, GetMouseCursor);

	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedRead, GetKeyState);
	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedRead, GetModKeyState);
	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedRead, GetPressedKeys);
	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedRead, GetInvertQueueKey);

	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedRead, GetKeyCode);
	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedRead, GetKeySymbol);
	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedRead, GetKeyBindings);
	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedRead, GetActionHotKeys);

	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedRead, GetLogSections);

	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedRead, GetConfigInt);
	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedRead, GetConfigFloat);
	REGISTER_SCOPED_LUA_CFUNC(LuaUnsyncedRead, GetConfigString);
	return true;
}


bool CLuaIntro::LoadSyncedReadFunctions(lua_State* L)
{
	REGISTER_SCOPED_LUA_CFUNC(LuaSyncedRead, AreHelperAIsEnabled);
	REGISTER_SCOPED_LUA_CFUNC(LuaSyncedRead, FixedAllies);

	REGISTER_SCOPED_LUA_CFUNC(LuaSyncedRead, GetGaiaTeamID);

	REGISTER_SCOPED_LUA_CFUNC(LuaSyncedRead, GetMapOptions);
	REGISTER_SCOPED_LUA_CFUNC(LuaSyncedRead, GetModOptions);

	REGISTER_SCOPED_LUA_CFUNC(LuaSyncedRead, GetSideData);

	REGISTER_SCOPED_LUA_CFUNC(LuaSyncedRead, GetAllyTeamStartBox);
	REGISTER_SCOPED_LUA_CFUNC(LuaSyncedRead, GetTeamStartPosition);

	REGISTER_SCOPED_LUA_CFUNC(LuaSyncedRead, GetPlayerList);
	REGISTER_SCOPED_LUA_CFUNC(LuaSyncedRead, GetTeamList);
	REGISTER_SCOPED_LUA_CFUNC(LuaSyncedRead, GetAllyTeamList);

	REGISTER_SCOPED_LUA_CFUNC(LuaSyncedRead, GetPlayerInfo);
	REGISTER_SCOPED_LUA_CFUNC(LuaSyncedRead, GetAIInfo);

	REGISTER_SCOPED_LUA_CFUNC(LuaSyncedRead, GetTeamInfo);
	REGISTER_SCOPED_LUA_CFUNC(LuaSyncedRead, GetTeamLuaAI);

	REGISTER_SCOPED_LUA_CFUNC(LuaSyncedRead, GetAllyTeamInfo);
	REGISTER_SCOPED_LUA_CFUNC(LuaSyncedRead, AreTeamsAllied);
	REGISTER_SCOPED_LUA_CFUNC(LuaSyncedRead, ArePlayersAllied);

	//REGISTER_SCOPED_LUA_CFUNC(LuaSyncedRead, GetGroundHeight);
	//REGISTER_SCOPED_LUA_CFUNC(LuaSyncedRead, GetGroundOrigHeight);
	//REGISTER_SCOPED_LUA_CFUNC(LuaSyncedRead, GetGroundNormal);
	//REGISTER_SCOPED_LUA_CFUNC(LuaSyncedRead, GetGroundInfo);
	//REGISTER_SCOPED_LUA_CFUNC(LuaSyncedRead, GetGroundExtremes);
	return true;
}


string CLuaIntro::LoadFile(const string& filename) const
{
	CFileHandler f(filename, SPRING_VFS_RAW_FIRST);

	string code;
	if (!f.LoadStringData(code))
		code.clear();

	return code;
}


/******************************************************************************/
/******************************************************************************/

void CLuaIntro::DrawLoadScreen()
{
	LUA_CALL_IN_CHECK(L);
	luaL_checkstack(L, 2, __func__);
	static const LuaHashString cmdStr(__func__);
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
	luaL_checkstack(L, 4, __func__);
	static const LuaHashString cmdStr(__func__);
	if (!cmdStr.GetGlobalFunc(L))
		return;

	lua_pushsstring(L, msg);
	lua_pushboolean(L, replace_lastline);

	// call the routine
	RunCallIn(L, cmdStr, 2, 0);
}


// Don't call GamePreload since it may be called concurrent
// with other callins during loading.
void CLuaIntro::GamePreload()
{ }

/******************************************************************************/
/******************************************************************************/
