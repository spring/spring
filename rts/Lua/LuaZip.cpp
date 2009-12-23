/**
 * @file LuaZip.cpp
 * @author: Tobi Vollebregt
 */

#include "LuaZip.h"
#include "LuaInclude.h"
#include "LuaHashString.h"
#include "System/FileSystem/ArchiveFactory.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/LogOutput.h"
#include "System/Util.h"
#include "lib/minizip/zip.h"
#include <cstring>
#include <string>
#include "mmgr.h"

using std::string;


static int pushresult(lua_State* L, bool result, const char* msg)
{
	lua_pushboolean(L, result);

	if (!result) {
		lua_pushstring(L, msg);
		return 2;
	}

	return 1;
}

/******************************************************************************/
/******************************************************************************/

bool LuaZipFileWriter::PushSynced(lua_State* L)
{
	CreateMetatable(L);

	return true;
}


bool LuaZipFileWriter::PushUnsynced(lua_State* L)
{
	CreateMetatable(L);

	HSTR_PUSH_CFUNC(L, "CreateZipFileWriter", open);

	return true;
}


bool LuaZipFileWriter::CreateMetatable(lua_State* L)
{
	luaL_newmetatable(L, "ZipFileWriter");

	// metatable.__index = metatable
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");

	HSTR_PUSH_CFUNC(L, "__gc",  meta_gc);
	HSTR_PUSH_CFUNC(L, "close", meta_gc);
	HSTR_PUSH_CFUNC(L, "open",  meta_open);
	HSTR_PUSH_CFUNC(L, "write", meta_write);

	lua_pop(L, 1);
	return true;
}


struct ZipFileWriterUserdata {
	zipFile zip;
	bool fileOpen;
	bool dontClose;
};


static bool FileExists(string filename)
{
	CFileHandler test(filename);
	return test.FileExists();
}


bool LuaZipFileWriter::PushNew(lua_State* L, const string& filename, zipFile zip)
{
	lua_checkstack(L, 2);

	ZipFileWriterUserdata* udata = (ZipFileWriterUserdata*)lua_newuserdata(L, sizeof(ZipFileWriterUserdata));
	std::memset(udata, 0, sizeof(ZipFileWriterUserdata));
	luaL_getmetatable(L, "ZipFileWriter");
	lua_setmetatable(L, -2);

	if (zip) {
		udata->zip = zip;
		udata->dontClose = true;
	}
	else {
		string realname = filesystem.LocateFile(filename, FileSystem::WRITE | FileSystem::CREATE_DIRS);
		if (!realname.empty() && !FileExists(realname)) {
			udata->zip = zipOpen(realname.c_str(), APPEND_STATUS_CREATE);
		}
	}

	if (!udata->zip) {
		lua_pop(L, 1);
		lua_pushnil(L);
	}

	return udata->zip != NULL;
}


int LuaZipFileWriter::open(lua_State* L)
{
	PushNew(L, string(luaL_checkstring(L, 1)), NULL);
	return 1;
}


static ZipFileWriterUserdata* towriter(lua_State* L)
{
	return ((ZipFileWriterUserdata*)luaL_checkudata(L, 1, "ZipFileWriter"));
}


int LuaZipFileWriter::meta_gc(lua_State* L)
{
	ZipFileWriterUserdata* f = towriter(L);

	if (f->zip && !f->dontClose) {
		const bool success = (Z_OK == zipClose(f->zip, NULL));
		f->zip = NULL;
		return pushresult(L, success, "close failed");
	}

	return 0;
}


int LuaZipFileWriter::meta_open(lua_State* L)
{
	ZipFileWriterUserdata* f = towriter(L);
	string name = luaL_checkstring(L, 2);

	if (!f->zip) {
		luaL_error(L, "zip not open");
	}

	// zipOpenNewFileInZip closes existing open file in zip.
	f->fileOpen = (Z_OK == zipOpenNewFileInZip(f->zip, name.c_str(), NULL, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_BEST_COMPRESSION));

	return pushresult(L, f->fileOpen, "zipOpenNewFileInZip failed");
}


int LuaZipFileWriter::meta_write(lua_State* L)
{
	ZipFileWriterUserdata* f = towriter(L);

	if (!f->zip) {
		luaL_error(L, "zip not open");
	}
	if (!f->fileOpen) {
		luaL_error(L, "file in zip not open");
	}

	// based on liolib.cpp g_write
	int arg = 2;
	int nargs = lua_gettop(L) - 1;
	int status = 1;
	for (; nargs--; arg++) {
		size_t l;
		const char *s = luaL_checklstring(L, arg, &l);
		status = status && (zipWriteInFileInZip(f->zip, s, l) == Z_OK);
	}
	return pushresult(L, status, "write failed");
}

/******************************************************************************/
/******************************************************************************/

bool LuaZipFileReader::PushSynced(lua_State* L)
{
	CreateMetatable(L);

	return true;
}


bool LuaZipFileReader::PushUnsynced(lua_State* L)
{
	CreateMetatable(L);

	HSTR_PUSH_CFUNC(L, "CreateZipFileReader", open);

	return true;
}


bool LuaZipFileReader::CreateMetatable(lua_State* L)
{
	luaL_newmetatable(L, "ZipFileReader");

	// metatable.__index = metatable
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");

	HSTR_PUSH_CFUNC(L, "__gc",  meta_gc);
	HSTR_PUSH_CFUNC(L, "close", meta_gc);
	HSTR_PUSH_CFUNC(L, "open",  meta_open);
	HSTR_PUSH_CFUNC(L, "read",  meta_read);

	lua_pop(L, 1);
	return true;
}


struct ZipFileReaderUserdata {
	CArchiveBase* archive;
	int file;
	bool dontClose;
};


bool LuaZipFileReader::PushNew(lua_State* L, const string& filename, CArchiveBase* archive)
{
	lua_checkstack(L, 2);

	ZipFileReaderUserdata* udata = (ZipFileReaderUserdata*)lua_newuserdata(L, sizeof(ZipFileReaderUserdata));
	std::memset(udata, 0, sizeof(ZipFileReaderUserdata));
	luaL_getmetatable(L, "ZipFileReader");
	lua_setmetatable(L, -2);

	if (archive) {
		udata->archive = archive;
		udata->dontClose = true;
	}
	else {
		string realname = filesystem.LocateFile(filename);
		if (!realname.empty()) {
			udata->archive = CArchiveFactory::OpenArchive(realname);
		}
	}

	if (!udata->archive) {
		lua_pop(L, 1);
		lua_pushnil(L);
	}

	return udata->archive != NULL;
}


int LuaZipFileReader::open(lua_State* L)
{
	PushNew(L, string(luaL_checkstring(L, 1)), NULL);
	return 1;
}


static ZipFileReaderUserdata* toreader(lua_State* L)
{
	return ((ZipFileReaderUserdata*)luaL_checkudata(L, 1, "ZipFileReader"));
}


int LuaZipFileReader::meta_gc(lua_State* L)
{
	ZipFileReaderUserdata* f = toreader(L);

	if (f->archive && !f->dontClose) {
		SafeDelete(f->archive);
		lua_pushboolean(L, 1);
		return 1;
	}

	return 0;
}


int LuaZipFileReader::meta_open(lua_State* L)
{
	ZipFileReaderUserdata* f = toreader(L);
	string name(luaL_checkstring(L, 2));

	if (!f->archive) {
		luaL_error(L, "zip not open");
	}
	if (f->file) {
		f->archive->CloseFile(f->file);
		f->file = 0;
	}

	f->file = f->archive->OpenFile(name.c_str());

	return pushresult(L, f->file != 0, "open failed");
}


static int test_eof (lua_State *L, ZipFileReaderUserdata* f)
{
	// based on liolib.cpp test_eof
	int c = f->archive->Peek(f->file);
	lua_pushlstring(L, NULL, 0);
	return (c != EOF);
}


static int read_chars(lua_State *L, ZipFileReaderUserdata* f, size_t n)
{
	// based on liolib.cpp read_chars
	size_t rlen; /* how much to read */
	size_t nr; /* number of chars actually read */
	luaL_Buffer b;
	luaL_buffinit(L, &b);
	rlen = LUAL_BUFFERSIZE; /* try to read that much each time */
	do {
		char *p = luaL_prepbuffer(&b);
		if (rlen > n)
			rlen = n; /* cannot read more than asked */
		nr = f->archive->ReadFile(f->file, p, rlen);
		luaL_addsize(&b, nr);
		n -= nr; /* still have to read `n' chars */
	} while (n > 0 && nr == rlen); /* until end of count or eof */
	luaL_pushresult(&b); /* close buffer */
	return (n == 0 || lua_objlen(L, -1) > 0);
}


/**
 * Similar to Lua's built-in (I/O library) read function.
 *
 * Except that "*number" and *line" aren't supported; only "*all" and <num> are
 * supported. The special case read(0) (test for end of file) is handled.
 *
 * Note that reading is only possible after a chunk has been opened using
 * openchunk().
 */
int LuaZipFileReader::meta_read(lua_State* L)
{
	ZipFileReaderUserdata* f = toreader(L);

	if (!f->archive) {
		luaL_error(L, "zip not open");
	}
	if (!f->file) {
		luaL_error(L, "file in zip not open");
	}

	// based on liolib.cpp g_read
	int first = 2;
	int nargs = lua_gettop(L) - 1;
	int success;
	int n;
	if (nargs == 0) { /* no arguments? */
		return luaL_error(L, "read() without arguments is not supported");
	} else { /* ensure stack space for all results and for auxlib's buffer */
		luaL_checkstack(L, nargs + LUA_MINSTACK, "too many arguments");
		success = 1;
		for (n = first; nargs-- && success; n++) {
			if (lua_type(L, n) == LUA_TNUMBER) {
				size_t l = (size_t) lua_tointeger(L, n);
				success = (l == 0) ? test_eof(L, f) : read_chars(L, f, l);
			} else {
				const char *p = lua_tostring(L, n);
				luaL_argcheck(L, p && p[0] == '*', n, "invalid option");
				switch (p[1]) {
				case 'n': /* number */
					return luaL_argerror(L, n, "*n is not supported");
				case 'l': /* line */
					return luaL_argerror(L, n, "*l is not supported");
				case 'a': /* file */
					read_chars(L, f, ~((size_t) 0)); /* read MAX_SIZE_T chars */
					success = 1; /* always success */
					break;
				default:
					return luaL_argerror(L, n, "invalid format");
				}
			}
		}
	}
	if (!success) {
		lua_pop(L, 1); /* remove last result */
		lua_pushnil(L); /* push nil instead */
	}
	return n - first;
}
