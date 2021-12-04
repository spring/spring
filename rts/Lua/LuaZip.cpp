/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * @class LuaZipFileWriter
 *
 * @brief A Lua userdatum to write zip files
 *
 * This class defines functions for a Lua userdatum to write to zip files.
 * Such a userdatum supports the following methods:
 *  - close()    : close the zipFile, after this open and write raise an error
 *  - open(name) : opens a new file within the zipFile (for writing)
 *  - write(...) : writes data to the open file within the zipFile (similar to io.write)
 */

/**
 * @class LuaZipFileReader
 *
 * @brief A Lua userdatum to read zip files
 *
 * This class defines functions for a Lua userdatum to read archive files.
 * The type is currently forced to a zip-file, while allowing any file extension.
 * Such a userdatum supports the following methods:
 *  - close()    : close the archive, after this open and read raise an error
 *  - open(name) : opens a new file within the archive (for reading)
 *  - read(...)  : reads data from the open file within the archive (similar to io.read)
 */

#include "LuaZip.h"
#include "LuaInclude.h"
#include "LuaHashString.h"
#include "System/FileSystem/Archives/IArchive.h"
#include "System/FileSystem/ArchiveLoader.h"
#include "System/FileSystem/DataDirsAccess.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileQueryFlags.h"
#include "System/FileSystem/FileSystem.h"
#include "System/SafeUtil.h"
#include "System/StringUtil.h"
#include "minizip/zip.h"
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

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

	// FIXME when this is enabled LuaGaia/LuaRules unsynced has access to it too!
	//HSTR_PUSH_CFUNC(L, "CreateZipFileWriter", open);

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


static bool FileExists(const std::string& filename)
{
	CFileHandler test(filename);
	return test.FileExists();
}


/**
 * @brief Pushes a new ZipFileWriter userdatum on the Lua stack.
 *
 * If zip != nullptr:
 *  - the userdatum is made to point to the zipFile,
 *  - the zipFile will never be closed by Lua (close()->no-op, GC->no-op)
 * Otherwise:
 *  - a new zipFile is opened (without overwrite, with directory creation)
 *  - this zipFile may be closed by Lua (close() or GC)
 */
bool LuaZipFileWriter::PushNew(lua_State* L, const std::string& filename, zipFile zip)
{
	luaL_checkstack(L, 2, __func__);

	auto udata = static_cast<ZipFileWriterUserdata*>(lua_newuserdata(L, sizeof(ZipFileWriterUserdata)));
	std::memset(udata, 0, sizeof(ZipFileWriterUserdata));
	luaL_getmetatable(L, "ZipFileWriter");
	lua_setmetatable(L, -2);

	if (zip) {
		udata->zip = zip;
		udata->dontClose = true;
	}
	else {
		std::string realname = dataDirsAccess.LocateFile(filename, FileQueryFlags::WRITE | FileQueryFlags::CREATE_DIRS);
		if (!realname.empty() && !FileExists(realname)) {
			udata->zip = zipOpen(realname.c_str(), APPEND_STATUS_CREATE);
		}
	}

	if (!udata->zip) {
		lua_pop(L, 1);
		lua_pushnil(L);
	}

	return udata->zip != nullptr;
}


int LuaZipFileWriter::open(lua_State* L)
{
	PushNew(L, std::string(luaL_checkstring(L, 1)), nullptr);
	return 1;
}


static ZipFileWriterUserdata* towriter(lua_State* L)
{
	return static_cast<ZipFileWriterUserdata*>(luaL_checkudata(L, 1, "ZipFileWriter"));
}


int LuaZipFileWriter::meta_gc(lua_State* L)
{
	ZipFileWriterUserdata* f = towriter(L);

	if (f->zip && !f->dontClose) {
		const bool success = (Z_OK == zipClose(f->zip, nullptr));
		f->zip = nullptr;
		return pushresult(L, success, "close failed");
	}

	return 0;
}


int LuaZipFileWriter::meta_open(lua_State* L)
{
	ZipFileWriterUserdata* f = towriter(L);
	std::string name = luaL_checkstring(L, 2);

	if (!f->zip) {
		luaL_error(L, "zip not open");
	}

	// zipOpenNewFileInZip closes existing open file in zip.
	f->fileOpen = (Z_OK == zipOpenNewFileInZip(f->zip, name.c_str(), nullptr, nullptr, 0, nullptr, 0, nullptr, Z_DEFLATED, Z_BEST_COMPRESSION));

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

	// FIXME when this is enabled LuaGaia/LuaRules unsynced has access to it too!
	//HSTR_PUSH_CFUNC(L, "CreateZipFileReader", open);

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
	IArchive* archive;
	std::stringstream* stream;
	bool dontClose;
};


/**
 * @brief Pushes a new ZipFileReader userdatum on the Lua stack.
 *
 * If archive != nullptr:
 *  - the userdatum is made to point to the archive,
 *  - the archive will never be closed by Lua (close()->no-op, GC->no-op)
 * Otherwise:
 *  - a new archive is opened
 *  - the type is currently forced to a zip-file, while allowing any file extension
 *  - this archive may be closed by Lua (close() or GC)
 */
bool LuaZipFileReader::PushNew(lua_State* L, const std::string& filename, IArchive* archive)
{
	luaL_checkstack(L, 2, __func__);

	auto udata = static_cast<ZipFileReaderUserdata*>(lua_newuserdata(L, sizeof(ZipFileReaderUserdata)));
	std::memset(udata, 0, sizeof(ZipFileReaderUserdata));
	luaL_getmetatable(L, "ZipFileReader");
	lua_setmetatable(L, -2);

	if (archive) {
		udata->archive = archive;
		udata->dontClose = true;
	}
	else {
		std::string realname = dataDirsAccess.LocateFile(filename);
		if (!realname.empty()) {
			udata->archive = archiveLoader.OpenArchive(realname, "sdz");
		}
	}

	if (!udata->archive) {
		lua_pop(L, 1);
		lua_pushnil(L);
	}

	return udata->archive != nullptr;
}


int LuaZipFileReader::open(lua_State* L)
{
	PushNew(L, std::string(luaL_checkstring(L, 1)), nullptr);
	return 1;
}


static ZipFileReaderUserdata* toreader(lua_State* L)
{
	return static_cast<ZipFileReaderUserdata*>(luaL_checkudata(L, 1, "ZipFileReader"));
}


int LuaZipFileReader::meta_gc(lua_State* L)
{
	ZipFileReaderUserdata* f = toreader(L);

	if (f->stream) {
		spring::SafeDelete(f->stream);
	}

	if (f->archive && !f->dontClose) {
		spring::SafeDelete(f->archive);
		lua_pushboolean(L, 1);
		return 1;
	}

	return 0;
}


int LuaZipFileReader::meta_open(lua_State* L)
{
	ZipFileReaderUserdata* f = toreader(L);
	std::string name(luaL_checkstring(L, 2));

	if (!f->archive) {
		luaL_error(L, "zip not open");
		return 0;
	}

	std::vector<std::uint8_t> buf;
	const bool result = f->archive->GetFile(name, buf);

	f->stream = new std::stringstream(std::string((char*) &*buf.begin(), buf.size()), std::ios_base::in);

	return pushresult(L, result, "open failed");
}


static int test_eof (lua_State* L, ZipFileReaderUserdata* f)
{
	// based on liolib.cpp test_eof
	int c = f->stream->peek();
	lua_pushlstring(L, nullptr, 0);
	return (c != EOF);
}


static int read_chars(lua_State* L, ZipFileReaderUserdata* f, size_t n)
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
		nr = f->stream->read(p, rlen).gcount();
		luaL_addsize(&b, nr);
		n -= nr; /* still have to read `n' chars */
	} while (n > 0 && nr == rlen); /* until end of count or eof */
	luaL_pushresult(&b); /* close buffer */
	return (n == 0 || lua_objlen(L, -1) > 0);
}


/**
 * Similar to Lua's built-in (I/O library) read function.
 *
 * Except that "*number" and *line" aren't supported; only "*all" and \<num\> are
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
		return 0;
	}
	if (!f->stream) {
		luaL_error(L, "file in zip not open");
		return 0;
	}

	// based on liolib.cpp g_read
	int first = 2;
	int nargs = lua_gettop(L) - 1;
	int success;
	int n;
	if (nargs == 0) { /* no arguments? */
		return luaL_error(L, "read() without arguments is not supported");
	}

	/* ensure stack space for all results and for auxlib's buffer */
	luaL_checkstack(L, nargs + LUA_MINSTACK, "too many arguments");
	success = 1;
	for (n = first; nargs-- && success; n++) {
		if (lua_type(L, n) == LUA_TNUMBER) {
			int l = lua_toint(L, n);
			success = (l == 0) ? test_eof(L, f) : read_chars(L, f, static_cast<size_t>(l));
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
	if (!success) {
		lua_pop(L, 1); /* remove last result */
		lua_pushnil(L); /* push nil instead */
	}
	return n - first;
}

/******************************************************************************/

// generates the info for the zipped version of the file
static zip_fileinfo* GenerateZipFileInfo(const std::string& path) {
	std::string fileModificationDate = FileSystem::GetFileModificationDate(path);

	auto zipfi = new zip_fileinfo();
	zipfi->dosDate = 0;
	// FIXME: the year 10k problem :)
	zipfi->tmz_date.tm_year = atoi(fileModificationDate.substr(0, 4).c_str()) - 1900;
	zipfi->tmz_date.tm_mon = atoi(fileModificationDate.substr(4, 2).c_str());
	zipfi->tmz_date.tm_mday = atoi(fileModificationDate.substr(6, 2).c_str());
	zipfi->tmz_date.tm_hour = atoi(fileModificationDate.substr(8, 2).c_str());
	zipfi->tmz_date.tm_min = atoi(fileModificationDate.substr(10, 2).c_str());
	zipfi->tmz_date.tm_sec = atoi(fileModificationDate.substr(12, 2).c_str());

	return zipfi;
}

void RecurseZipFolder(const std::string& folderPath, zipFile& zip, const std::string& zipFolderPath, const std::string& modes) {
	// recurse through all the subdirs
	for (const std::string& childFolderPath : CFileHandler::SubDirs(folderPath, "*", modes)) {
		const std::string childFolderName = FileSystem::GetFilename(childFolderPath.substr(0, childFolderPath.length() - 1));
		const std::string childZipFolderPath = zipFolderPath + childFolderName + "/";

		zip_fileinfo* zipfi = GenerateZipFileInfo(childFolderPath);
		// write a special file for the dir, so empty folders get added
		zipOpenNewFileInZip(zip, childZipFolderPath.c_str(), zipfi, nullptr, 0, nullptr, 0, nullptr, Z_DEFLATED, Z_BEST_COMPRESSION);
		zipWriteInFileInZip(zip, "", 0);
		zipCloseFileInZip(zip);
		delete zipfi;

        // recurse
		RecurseZipFolder(childFolderPath, zip, childZipFolderPath, modes);
	}

	// iterate through all the files and write them
	for (const std::string& filePath : CFileHandler::DirList(folderPath, "*", modes)) {
		const std::string& fileName = FileSystem::GetFilename(filePath);
		const std::string zipFilePath = zipFolderPath + fileName;

		std::string fileData;
		CFileHandler fh(filePath, modes);
		fh.LoadStringData(fileData);

		zip_fileinfo* zipfi = GenerateZipFileInfo(filePath);
		zipOpenNewFileInZip(zip, zipFilePath.c_str(), zipfi, nullptr, 0, nullptr, 0, nullptr, Z_DEFLATED, Z_BEST_COMPRESSION);
		zipWriteInFileInZip(zip, fileData.c_str(), fileData.length());
		zipCloseFileInZip(zip);
		delete zipfi;
	}
}

int LuaZipFolder::ZipFolder(lua_State* L, const std::string& folderPath, const std::string& zipFilePath, bool includeFolder, const std::string& modes)
{
	zipFile zipFolderFile = zipOpen(zipFilePath.c_str(), APPEND_STATUS_CREATE);

	const std::string normFolderPath = FileSystem::GetNormalizedPath(folderPath);
	const std::string folderName = (includeFolder)? FileSystem::GetFilename(normFolderPath) + "/": "";
	char buf[1024] = {'\0'};

	if (zipFolderFile == nullptr) {
		SNPRINTF(buf, sizeof(buf), "[%s] could not open zipfile \"%s\" for writing", __func__, zipFilePath.c_str());
		lua_pushstring(L, buf);
		return 1;
	}

	if (!dataDirsAccess.InWriteDir(normFolderPath)) {
		SNPRINTF(buf, sizeof(buf), "[%s] cannot zip \"%s\": outside writable data-directory", __func__, normFolderPath.c_str());
		lua_pushstring(L, buf);
		zipClose(zipFolderFile, nullptr);
		return 1;
	}

	RecurseZipFolder(folderPath, zipFolderFile, folderName, modes);

	zipClose(zipFolderFile, nullptr);
	return 0;
}
