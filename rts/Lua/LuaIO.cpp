#include "StdAfx.h"
// LuaIO.cpp: implementation of the LuaIO class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <errno.h>

#ifndef _MSC_VER	// this header file does not exist for the microsoft compiler
 #include <unistd.h>
#endif

#include <string>
using std::string;

#include "mmgr.h"

#include "LuaIO.h"

#if !defined UNITSYNC && !defined DEDICATED && !defined BUILDING_AI
#include "LuaHandle.h"
#endif // !defined UNITSYNC && !defined DEDICATED && !defined BUILDING_AI
#include "LuaInclude.h"
#include "FileSystem/FileSystem.h"
#include "Util.h"


/******************************************************************************/
/******************************************************************************/

static bool IsSafePath(const string& path)
{
	// keep searches within the Spring directory
	if ((path[0] == '/') || (path[0] == '\\') ||
	    ((path.size() >= 2) && (path[1] == ':'))) {
		return false;
	}
	if (path.find("..") != string::npos) {
		return false;
	}
	return true;
}


/******************************************************************************/
/******************************************************************************/

bool LuaIO::IsSimplePath(const string& path)
{
	// keep searches within the Spring directory
	if ((path[0] == '/') || (path[0] == '\\') ||
	    ((path.size() >= 2) && (path[1] == ':'))) {
		return false;
	}
	if (path.find("..") != string::npos) {
		return false;
	}
	return true;
}


bool LuaIO::SafeExecPath(const string& path)
{
	return false; // don't allow execution of external programs, yet
}


bool LuaIO::SafeReadPath(const string& path)
{
	return filesystem.InReadDir(path);
}


bool LuaIO::SafeWritePath(const string& path)
{
	string prefix = ""; // FIXME
#if !defined UNITSYNC && !defined DEDICATED && !defined BUILDING_AI
	const CLuaHandle* lh = CLuaHandle::GetActiveHandle();
	if (lh != NULL) {
		prefix = lh->GetName() + "/" + "Write";
	}
#endif // !defined UNITSYNC && !defined DEDICATED && !defined BUILDING_AI
	const size_t numExtensions = 5;
	const char* exeFiles[numExtensions] = {"exe", "dll", "so", "bat", "com"};
	const string ext = GetFileExt(path);
	for (size_t i = 0; i < numExtensions; ++i)
	{
		if (ext == exeFiles[i])
			return false;
	}
	return filesystem.InWriteDir(path, prefix);
}


/******************************************************************************/
/******************************************************************************/

FILE* LuaIO::fopen(lua_State* L, const char* path, const char* mode)
{
	// check the mode string
	const string modeStr = StringToLower(mode);
	if (modeStr.find_first_not_of("rwabt+") != string::npos) {
		errno = EINVAL;
		return NULL;
	}
	if (!IsSafePath(path)) {
		errno = EPERM; //EACCESS?
		return NULL;
	}
	return ::fopen(path, mode);
}


FILE* LuaIO::popen(lua_State* L, const char* command, const char* type)
{
	// check the type string
	const string typeStr = StringToLower(type);
	if (typeStr.find_first_not_of("rw") != string::npos) {
		errno = EINVAL;
		return NULL;
	}
	errno = EINVAL;
	return NULL;
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
	if (!SafeWritePath(pathname)) {
		errno = EPERM; //EACCESS?
		return -1;
	}
	return ::remove(pathname);
}


int LuaIO::rename(lua_State* L, const char* oldpath, const char* newpath)
{
	if (!SafeWritePath(oldpath) || !SafeWritePath(newpath)) {
		errno = EPERM; //EACCESS?
		return -1;
	}
	return ::rename(oldpath, newpath);
}


/******************************************************************************/
/******************************************************************************/
