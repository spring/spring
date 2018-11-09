/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include <cstdio>
#include <cerrno>

#ifndef _MSC_VER	// this header file does not exist for the microsoft compiler
 #include <unistd.h>
#endif

#include <string>
#include <array>

#include "LuaIO.h"

#if !defined UNITSYNC && !defined DEDICATED && !defined BUILDING_AI
#include "LuaHandle.h"
#endif // !defined UNITSYNC && !defined DEDICATED && !defined BUILDING_AI
#include "LuaInclude.h"
#include "System/FileSystem/DataDirsAccess.h"
#include "System/FileSystem/FileSystem.h"
#include "System/StringUtil.h"


/******************************************************************************/
/******************************************************************************/

static bool IsSafePath(const std::string& path)
{
	// keep searches within the Spring directory
	if ((path[0] == '/') || (path[0] == '\\') ||
	    ((path.size() >= 2) && (path[1] == ':'))) {
		return false;
	}
	if ((path.find("..") != std::string::npos) ||
		(path.find("springsettings.cfg") != std::string::npos) || //don't allow to change config file
		(path.find(".springrc") != std::string::npos) ||
		(path.find("springrc") != std::string::npos)
	) {
		return false;
	}

	return true;
}


/******************************************************************************/
/******************************************************************************/

bool LuaIO::IsSimplePath(const std::string& path)
{
	// keep searches within the Spring directory
	if ((path[0] == '/') || (path[0] == '\\') || ((path.size() >= 2) && (path[1] == ':')))
		return false;

	return (path.find("..") == std::string::npos);
}


bool LuaIO::SafeExecPath(const std::string& path)
{
	return false; // don't allow execution of external programs, yet
}


bool LuaIO::SafeReadPath(const std::string& path)
{
	return dataDirsAccess.InReadDir(path);
}


bool LuaIO::SafeWritePath(const std::string& path)
{
	const std::array<std::string, 5> exeFiles = {"exe", "dll", "so", "bat", "com"};
	const std::string ext = FileSystem::GetExtension(path);

	if (std::find(std::begin(exeFiles), std::end(exeFiles), ext) != exeFiles.end())
		return false;

	return dataDirsAccess.InWriteDir(path);
}


/******************************************************************************/
/******************************************************************************/

FILE* LuaIO::fopen(lua_State* L, const char* path, const char* mode)
{
	// check the mode string
	const std::string modeStr = StringToLower(mode);
	if (modeStr.find_first_not_of("rwabt+") != std::string::npos) {
		errno = EINVAL;
		return nullptr;
	}
	if (!IsSafePath(path)) {
		errno = EPERM; //EACCESS?
		return nullptr;
	}
	return ::fopen(path, mode);
}


FILE* LuaIO::popen(lua_State* L, const char* command, const char* type)
{
	// check the type string
	const std::string typeStr = StringToLower(type);
	if (typeStr.find_first_not_of("rw") != std::string::npos) {
		errno = EINVAL;
		return nullptr;
	}
	errno = EINVAL;
	return nullptr;
}


int LuaIO::pclose(lua_State* L, FILE* stream)
{
	errno = ECHILD;
	return -1;
}


int LuaIO::system(lua_State* L, const char* command)
{
	luaL_error(L, "the system() call is not available");
	return -1; //
}


int LuaIO::remove(lua_State* L, const char* pathname)
{
	if (!SafeWritePath(pathname)
		|| !IsSafePath(pathname)) {
		errno = EPERM; //EACCESS?
		return -1;
	}
	return ::remove(pathname);
}


int LuaIO::rename(lua_State* L, const char* oldpath, const char* newpath)
{
	if (!SafeWritePath(oldpath) || !SafeWritePath(newpath)
		|| !IsSafePath(oldpath) || !IsSafePath(newpath)) {
		errno = EPERM; //EACCESS?
		return -1;
	}
	return ::rename(oldpath, newpath);
}


/******************************************************************************/
/******************************************************************************/
