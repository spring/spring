#include "StdAfx.h"
#include "unitsync.h"
#include "unitsync_api.h"

#include <algorithm>
#include <string>
#include <vector>
#include <set>

// shared with spring:
#include "LuaInclude.h"
#include "FileSystem/ArchiveFactory.h"
#include "FileSystem/ArchiveScanner.h"
#include "FileSystem/FileHandler.h"
#include "FileSystem/VFSHandler.h"
#include "Game/GameVersion.h"
#include "Lua/LuaParser.h"
#include "Map/MapParser.h"
#include "Map/SMF/SmfMapFile.h"
#include "ConfigHandler.h"
#include "FileSystem/FileSystem.h"
#include "Rendering/Textures/Bitmap.h"
#include "Sim/Misc/SideParser.h"
#include "ExternalAI/Interface/aidefines.h"
#include "ExternalAI/Interface/SSkirmishAILibrary.h"
#include "System/Exceptions.h"
#include "System/LogOutput.h"
#include "System/Util.h"
#include "System/exportdefines.h"
#include "System/Info.h"
#include "System/Option.h"


// unitsync only:
#include "LuaParserAPI.h"
#include "Syncer.h"

using std::string;

//////////////////////////
//////////////////////////

static CLogSubsystem LOG_UNITSYNC("unitsync", true);

//This means that the DLL can only support one instance. Don't think this should be a problem.
static CSyncer* syncer;

static bool logOutputInitialised=false;
// I'd rather not include globalstuff
#define SQUARE_SIZE 8


#ifdef WIN32
BOOL CALLING_CONV DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved)
{
	return TRUE;
}
#endif


//////////////////////////
//////////////////////////

// function argument checking

static void CheckInit()
{
	if (!archiveScanner || !vfsHandler)
		throw std::logic_error("Unitsync not initialized. Call Init first.");
}

static void _CheckNull(void* condition, const char* name)
{
	if (!condition)
		throw std::invalid_argument("Argument " + string(name) + " may not be null.");
}

static void _CheckNullOrEmpty(const char* condition, const char* name)
{
	if (!condition || *condition == 0)
		throw std::invalid_argument("Argument " + string(name) + " may not be null or empty.");
}

static void _CheckBounds(int index, int size, const char* name)
{
	if (index < 0 || index >= size)
		throw std::out_of_range("Argument " + string(name) + " out of bounds. Index: " +
		                         IntToString(index) + " Array size: " + IntToString(size));
}

static void _CheckPositive(int value, const char* name)
{
	if (value <= 0)
		throw std::out_of_range("Argument " + string(name) + " must be positive.");
}

#define CheckNull(arg)         _CheckNull((arg), #arg)
#define CheckNullOrEmpty(arg)  _CheckNullOrEmpty((arg), #arg)
#define CheckBounds(arg, size) _CheckBounds((arg), (size), #arg)
#define CheckPositive(arg)     _CheckPositive((arg), #arg);

//////////////////////////
//////////////////////////

// error handling

static string lastError;

static void _SetLastError(string err)
{
	logOutput.Prints(LOG_UNITSYNC, "error: " + err);
	lastError = err;
}

#define SetLastError(str) \
	_SetLastError(string(__FUNCTION__) + ": " + (str))

#define UNITSYNC_CATCH_BLOCKS \
	catch (const std::exception& e) { \
		SetLastError(e.what()); \
	} \
	catch (...) { \
		SetLastError("an unknown exception was thrown"); \
	}

//////////////////////////
//////////////////////////

// Helper class for loading a map archive temporarily

class ScopedMapLoader {
	public:
		ScopedMapLoader(const string& mapName) : oldHandler(vfsHandler)
		{
			CFileHandler f("maps/" + mapName);
			if (f.FileExists()) {
				return;
			}

			vfsHandler = new CVFSHandler();

			const vector<string> ars = archiveScanner->GetArchivesForMap(mapName);
			vector<string>::const_iterator it;
			for (it = ars.begin(); it != ars.end(); ++it) {
				vfsHandler->AddArchive(*it, false);
			}
		}

		~ScopedMapLoader()
		{
			if (vfsHandler != oldHandler) {
				delete vfsHandler;
				vfsHandler = oldHandler;
			}
		}

	private:
		CVFSHandler* oldHandler;
};

//////////////////////////
//////////////////////////

/** @addtogroup unitsync_api
	@{ */

/**
 * @brief Retrieve next error in queue of errors and removes this error from queue
 * @return An error message, or NULL if there are no more errors in the queue
 *
 * Use this method to get a (short) description of errors that occurred in any
 * other unitsync methods. Call this in a loop until it returns NULL to get all
 * errors.
 *
 * The error messages may be varying in detail etc.; nothing is guaranteed about
 * them, not even whether they have terminating newline or not.
 *
 * Example:
 *		@code
 *		const char* err;
 *		while ((err = GetNextError()) != NULL)
 *			printf("unitsync error: %s\n", err);
 *		@endcode
 */
EXPORT(const char*) GetNextError()
{
	try {
		// queue is only 1 element long now for simplicity :-)

		if (lastError.empty()) return NULL;

		string err = lastError;
		lastError.clear();
		return GetStr(err);
	}
	UNITSYNC_CATCH_BLOCKS;

	// Oops, can't even return errors anymore...
	// Returning anything but NULL might cause infinite loop in lobby client...
	//return __FUNCTION__ ": fatal error: an exception was thrown in GetNextError";
	return NULL;
}


/**
 * @brief Retrieve the version of Spring this unitsync was compiled with
 * @return The Spring/unitsync version string
 *
 * Returns a const char* string specifying the version of spring used to build this library with.
 * It was added to aid in lobby creation, where checks for updates to spring occur.
 */
EXPORT(const char*) GetSpringVersion()
{
	return GetStr(SpringVersion::Get());
}


static void _UnInit()
{
	lpClose();

	FileSystemHandler::Cleanup();

	if ( syncer )
	{
		SafeDelete(syncer);
		logOutput.Print(LOG_UNITSYNC, "deinitialized");
	}
}


/**
 * @brief Uninitialize the unitsync library
 *
 * also resets the config handler
 */
EXPORT(void) UnInit()
{
	try {
		_UnInit();
		ConfigHandler::Deallocate();
	}
	UNITSYNC_CATCH_BLOCKS;
}


/**
 * @brief Initialize the unitsync library
 * @return Zero on error; non-zero on success
 *
 * Call this function before calling any other function in unitsync.
 * In case unitsync was already initialized, it is uninitialized and then
 * reinitialized.
 *
 * Calling this function is currently the only way to clear the VFS of the
 * files which are mapped into it.  In other words, after using AddArchive() or
 * AddAllArchives() you have to call Init when you want to remove the archives
 * from the VFS and start with a clean state.
 *
 * The config handler won't be reset, it will however be initialised if it wasn't before (with SetSpringConfigFile())
 */
EXPORT(int) Init(bool isServer, int id)
{
	try {
		if (!logOutputInitialised)
		{
			logOutput.SetFilename("unitsync.log");
			logOutput.Initialize();
			logOutputInitialised = true;
		}
		logOutput.Print(LOG_UNITSYNC, "loaded, %s\n", SpringVersion::GetFull().c_str());

		_UnInit();

		if (!configHandler)
			ConfigHandler::Instantiate("");
		FileSystemHandler::Initialize(false);

		std::vector<string> filesToCheck;
		filesToCheck.push_back("base/springcontent.sdz");
		filesToCheck.push_back("base/maphelper.sdz");
		filesToCheck.push_back("base/spring/bitmaps.sdz");
		filesToCheck.push_back("base/cursors.sdz");

		for (std::vector<string>::const_iterator it = filesToCheck.begin(); it != filesToCheck.end(); ++it) {
			CFileHandler f(*it, SPRING_VFS_RAW);
			if (!f.FileExists()) {
				throw content_error("Required base file '" + *it + "' does not exist.");
			}
		}

		syncer = new CSyncer();
		logOutput.Print(LOG_UNITSYNC, "initialized, %s\n", SpringVersion::GetFull().c_str());
		logOutput.Print(LOG_UNITSYNC, "%s\n", isServer ? "hosting" : "joining");
		return 1;
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


/**
 * @brief Get the main data directory that's used by unitsync and Spring
 * @return NULL on error; the data directory path on success
 *
 * This is the data directory which is used to write logs, screenshots, demos, etc.
 */
EXPORT(const char*) GetWritableDataDirectory()
{
	try {
		CheckInit();
		return GetStr(FileSystemHandler::GetInstance().GetWriteDir());
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}

// TODO (when needed): GetDataDirectoryCount(), GetDataDirectory(int index)


/**
 * @brief Process another unit and return how many are left to process
 * @return The number of unprocessed units to be handled
 *
 * Call this function repeatedly until it returns 0 before calling any other
 * function related to units.
 *
 * Because of risk for infinite loops, this function can not return any error code.
 * It is advised to poll GetNextError() after calling this function.
 *
 * Before any units are available, you'll first need to map a mod's archives
 * into the VFS using AddArchive() or AddAllArchives().
 */
EXPORT(int) ProcessUnits()
{
	try {
		logOutput.Print(LOG_UNITSYNC, "syncer: process units\n");
		return syncer->ProcessUnits();
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


/**
 * @brief Identical to ProcessUnits(), neither generates checksum anymore
 * @see ProcessUnits
 */
EXPORT(int) ProcessUnitsNoChecksum()
{
	return ProcessUnits();
}


/**
 * @brief Get the number of units
 * @return Zero on error; the number of units available on success
 *
 * Will return the number of units. Remember to call ProcessUnits() beforehand
 * until it returns 0.  As ProcessUnits() is called the number of processed
 * units goes up, and so will the value returned by this function.
 *
 * Example:
 *		@code
 *		while (ProcessUnits() != 0) {}
 *		int unit_number = GetUnitCount();
 *		@endcode
 */
EXPORT(int) GetUnitCount()
{
	try {
		logOutput.Print(LOG_UNITSYNC, "syncer: get unit count\n");
		return syncer->GetUnitCount();
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


/**
 * @brief Get the units internal mod name
 * @param unit The units id number
 * @return The units internal modname or NULL on error
 *
 * This function returns the units internal mod name. For example it would
 * return 'armck' and not 'Arm Construction kbot'.
 */
EXPORT(const char*) GetUnitName(int unit)
{
	try {
		logOutput.Print(LOG_UNITSYNC, "syncer: get unit %d name\n", unit);
		string tmp = syncer->GetUnitName(unit);
		return GetStr(tmp);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


/**
 * @brief Get the units human readable name
 * @param unit The units id number
 * @return The units human readable name or NULL on error
 *
 * This function returns the units human name. For example it would return
 * 'Arm Construction kbot' and not 'armck'.
 */
EXPORT(const char*) GetFullUnitName(int unit)
{
	try {
		logOutput.Print(LOG_UNITSYNC, "syncer: get full unit %d name\n", unit);
		string tmp = syncer->GetFullUnitName(unit);
		return GetStr(tmp);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}

//////////////////////////
//////////////////////////

/**
 * @brief Adds an archive to the VFS (Virtual File System)
 *
 * After this, the contents of the archive are available to other unitsync functions,
 * for example: ProcessUnits(), OpenFileVFS(), ReadFileVFS(), FileSizeVFS(), etc.
 */
EXPORT(void) AddArchive(const char* name)
{
	try {
		CheckInit();
		CheckNullOrEmpty(name);

		logOutput.Print(LOG_UNITSYNC, "adding archive: %s\n", name);
		vfsHandler->AddArchive(name, false);
	}
	UNITSYNC_CATCH_BLOCKS;
}


/**
 * @brief Adds an achive and all it's dependencies to the VFS
 * @see AddArchive
 */
EXPORT(void) AddAllArchives(const char* root)
{
	try {
		CheckInit();
		CheckNullOrEmpty(root);

		vector<string> ars = archiveScanner->GetArchives(root);
		for (vector<string>::iterator i = ars.begin(); i != ars.end(); ++i) {
			logOutput.Print(LOG_UNITSYNC, "adding archive: %s\n", i->c_str());
			vfsHandler->AddArchive(*i, false);
		}
	}
	UNITSYNC_CATCH_BLOCKS;
}

/**
 * @brief Removes all archives from the VFS (Virtual File System)
 *
 * After this, the contents of the archives are not available to other unitsync functions anymore,
 * for example: ProcessUnits(), OpenFileVFS(), ReadFileVFS(), FileSizeVFS(), etc.
 *
 * In a lobby client, this may be used instead of Init() when switching mod archive.
 */
EXPORT(void) RemoveAllArchives()
{
	try {
		CheckInit();

		logOutput.Print(LOG_UNITSYNC, "removing all archives\n");
		SafeDelete(vfsHandler);
		SafeDelete(syncer);
		vfsHandler = new CVFSHandler();
		syncer = new CSyncer();
	}
	UNITSYNC_CATCH_BLOCKS;
}

/**
 * @brief Get checksum of an archive
 * @return Zero on error; the checksum on success
 *
 * This checksum depends only on the contents from the archive itself, and not
 * on the contents from dependencies of this archive (if any).
 */
EXPORT(unsigned int) GetArchiveChecksum(const char* arname)
{
	try {
		CheckInit();
		CheckNullOrEmpty(arname);

		logOutput.Print(LOG_UNITSYNC, "archive checksum: %s\n", arname);
		return archiveScanner->GetArchiveChecksum(arname);
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


/**
 * @brief Gets the real path to the archive
 * @return NULL on error; a path to the archive on success
 */
EXPORT(const char*) GetArchivePath(const char* arname)
{
	try {
		CheckInit();
		CheckNullOrEmpty(arname);

		logOutput.Print(LOG_UNITSYNC, "archive path: %s\n", arname);
		return GetStr(archiveScanner->GetArchivePath(arname));
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


// Updated on every call to GetMapCount
static vector<string> mapNames;


/**
 * @brief Get the number of maps available
 * @return Zero on error; the number of maps available on success
 *
 * Call this before any of the map functions which take a map index as parameter.
 * This function actually performs a relatively costly enumeration of all maps,
 * so you should resist from calling it repeatedly in a loop.  Rather use:
 *		@code
 *		int map_count = GetMapCount();
 *		for (int index = 0; index < map_count; ++index) {
 *			printf("map name: %s\n", GetMapName(index));
 *		}
 *		@endcode
 * Then:
 *		@code
 *		for (int index = 0; index < GetMapCount(); ++index) { ... }
 *		@endcode
 */
EXPORT(int) GetMapCount()
{
	try {
		CheckInit();

		//vector<string> files = CFileHandler::FindFiles("{maps/*.smf,maps/*.sm3}");
		vector<string> files = CFileHandler::FindFiles("maps/", "{*.smf,*.sm3}");
		vector<string> ars = archiveScanner->GetMaps();

		mapNames.clear();
		for (vector<string>::iterator i = files.begin(); i != files.end(); ++i) {
			string mn = *i;
			mn = mn.substr(mn.find_last_of('/') + 1);
			mapNames.push_back(mn);
		}
		for (vector<string>::iterator i = ars.begin(); i != ars.end(); ++i)
			mapNames.push_back(*i);
		sort(mapNames.begin(), mapNames.end());

		return mapNames.size();
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


/**
 * @brief Get the name of a map
 * @return NULL on error; the name of the map (e.g. "SmallDivide.smf") on success
 */
EXPORT(const char*) GetMapName(int index)
{
	try {
		CheckInit();
		CheckBounds(index, mapNames.size());

		return GetStr(mapNames[index]);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


static void safe_strzcpy(char* dst, std::string src, size_t max)
{
	if (src.length() > max-1) {
		src = src.substr(0, max-1);
	}
	strcpy(dst, src.c_str());
}


static int _GetMapInfoEx(const char* name, MapInfo* outInfo, int version)
{
	CheckInit();
	CheckNullOrEmpty(name);
	CheckNull(outInfo);

	logOutput.Print(LOG_UNITSYNC, "get map info: %s", name);

	const string mapName = name;
	ScopedMapLoader mapLoader(mapName);

	string err("");

	MapParser mapParser(mapName);
	if (!mapParser.IsValid()) {
		err = mapParser.GetErrorLog();
	}
	const LuaTable mapTable = mapParser.GetRoot();

	// Retrieve the map header as well
	if (err.empty()) {
		const string extension = mapName.substr(mapName.length() - 3);
		if (extension == "smf") {
			try {
				CSmfMapFile file(name);
				const SMFHeader& mh = file.GetHeader();

				outInfo->width  = mh.mapx * SQUARE_SIZE;
				outInfo->height = mh.mapy * SQUARE_SIZE;
			}
			catch (content_error&) {
				outInfo->width  = -1;
			}
		}
		else {
			int w = mapTable.GetInt("gameAreaW", 0);
			int h = mapTable.GetInt("gameAreaW", 1);

			outInfo->width  = w * SQUARE_SIZE;
			outInfo->height = h * SQUARE_SIZE;
		}

		// Make sure we found stuff in both the smd and the header
		if (outInfo->width <= 0) {
			err = "Bad map width";
		}
		else if (outInfo->height <= 0) {
			err = "Bad map height";
		}
	}

	// If the map didn't parse, say so now
	if (!err.empty()) {
		SetLastError(err);
		safe_strzcpy(outInfo->description, err, 255);

		// Fill in stuff so tasclient won't crash
		outInfo->posCount = 0;
		if (version >= 1) {
			outInfo->author[0] = 0;
		}
		return 0;
	}

	const string desc = mapTable.GetString("description", "");
	safe_strzcpy(outInfo->description, desc, 255);

	outInfo->tidalStrength   = mapTable.GetInt("tidalstrength", 0);
	outInfo->gravity         = mapTable.GetInt("gravity", 0);
	outInfo->extractorRadius = mapTable.GetInt("extractorradius", 0);
	outInfo->maxMetal        = mapTable.GetFloat("maxmetal", 0.0f);

	if (version >= 1) {
		const string author = mapTable.GetString("author", "");
		safe_strzcpy(outInfo->author, author, 200);
	}

	const LuaTable atmoTable = mapTable.SubTable("atmosphere");
	outInfo->minWind = atmoTable.GetInt("minWind", 0);
	outInfo->maxWind = atmoTable.GetInt("maxWind", 0);

	// Find the start positions
	int curTeam;
	for (curTeam = 0; curTeam < 16; ++curTeam) {
		float3 pos(-1.0f, -1.0f, -1.0f); // defaults
		if (!mapParser.GetStartPos(curTeam, pos)) {
			break; // position could not be parsed
		}
		outInfo->positions[curTeam].x = pos.x;
		outInfo->positions[curTeam].z = pos.z;
		logOutput.Print(LOG_UNITSYNC, "  startpos: %.0f, %.0f", pos.x, pos.z);
	}

	outInfo->posCount = curTeam;

	return 1;
}


/**
 * @brief Retrieve map info
 * @param name name of the map, e.g. "SmallDivide.smf"
 * @param outInfo pointer to structure which is filled with map info
 * @param version this determines which fields of the MapInfo structure are filled
 * @return Zero on error; non-zero on success
 *
 * If version >= 1, then the author field is filled.
 *
 * Important: the description and author fields must point to a valid, and sufficiently long buffer
 * to store their contents.  Description is max 255 chars, and author is max 200 chars. (including
 * terminating zero byte).
 *
 * If an error occurs (return value 0), the description is set to an error message.
 * However, using GetNextError() is the recommended way to get the error message.
 *
 * Example:
 *		@code
 *		char description[255];
 *		char author[200];
 *		MapInfo mi;
 *		mi.description = description;
 *		mi.author = author;
 *		if (GetMapInfoEx("somemap.smf", &mi, 1)) {
 *			//now mi is contains map data
 *		} else {
 *			//handle the error
 *		}
 *		@endcode
 */
EXPORT(int) GetMapInfoEx(const char* name, MapInfo* outInfo, int version)
{
	try {
		return _GetMapInfoEx(name, outInfo, version);
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


/**
 * @brief Retrieve map info, equivalent to GetMapInfoEx(name, outInfo, 0)
 * @param name name of the map, e.g. "SmallDivide.smf"
 * @param outInfo pointer to structure which is filled with map info
 * @return Zero on error; non-zero on success
 * @see GetMapInfoEx
 */
EXPORT(int) GetMapInfo(const char* name, MapInfo* outInfo)
{
	try {
		return _GetMapInfoEx(name, outInfo, 0);
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}



/**
 * @brief return the map's minimum height
 * @param name name of the map, e.g. "SmallDivide.smf"
 *
 * Together with maxHeight, this determines the
 * range of the map's height values in-game. The
 * conversion formula for any raw 16-bit height
 * datum <h> is
 *
 *    minHeight + (h * (maxHeight - minHeight) / 65536.0f)
 */
EXPORT(float) GetMapMinHeight(const char* name) {
	try {
		ScopedMapLoader loader(name);
		CSmfMapFile file(name);
		MapParser parser(name);

		const SMFHeader& header = file.GetHeader();
		const LuaTable rootTable = parser.GetRoot();
		const LuaTable smfTable = rootTable.SubTable("smf");

		if (smfTable.KeyExists("minHeight")) {
			// override the header's minHeight value
			return (smfTable.GetFloat("minHeight", 0.0f));
		} else {
			return (header.minHeight);
		}
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0.0f;
}

/**
 * @brief return the map's maximum height
 * @param name name of the map, e.g. "SmallDivide.smf"
 *
 * Together with minHeight, this determines the
 * range of the map's height values in-game. See
 * GetMapMinHeight() for the conversion formula.
 */
EXPORT(float) GetMapMaxHeight(const char* name) {
	try {
		ScopedMapLoader loader(name);
		CSmfMapFile file(name);
		MapParser parser(name);

		const SMFHeader& header = file.GetHeader();
		const LuaTable rootTable = parser.GetRoot();
		const LuaTable smfTable = rootTable.SubTable("smf");

		if (smfTable.KeyExists("maxHeight")) {
			// override the header's maxHeight value
			return (smfTable.GetFloat("maxHeight", 0.0f));
		} else {
			return (header.maxHeight);
		}
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0.0f;
}




static vector<string> mapArchives;


/**
 * @brief Retrieves the number of archives a map requires
 * @param mapName name of the map, e.g. "SmallDivide.smf"
 * @return Zero on error; the number of archives on success
 *
 * Must be called before GetMapArchiveName()
 */
EXPORT(int) GetMapArchiveCount(const char* mapName)
{
	try {
		CheckInit();
		CheckNullOrEmpty(mapName);

		mapArchives = archiveScanner->GetArchivesForMap(mapName);
		return mapArchives.size();
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


/**
 * @brief Retrieves an archive a map requires
 * @param index the index of the archive
 * @return NULL on error; the name of the archive on success
 */
EXPORT(const char*) GetMapArchiveName(int index)
{
	try {
		CheckInit();
		CheckBounds(index, mapArchives.size());

		return GetStr(mapArchives[index]);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


/**
 * @brief Get map checksum given a map index
 * @param index the index of the map
 * @return Zero on error; the checksum on success
 *
 * This checksum depends on Spring internals, and as such should not be expected
 * to remain stable between releases.
 *
 * (It is ment to check sync between clients in lobby, for example.)
 */
EXPORT(unsigned int) GetMapChecksum(int index)
{
	try {
		CheckInit();
		CheckBounds(index, mapNames.size());

		return archiveScanner->GetMapChecksum(mapNames[index]);
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


/**
 * @brief Get map checksum given a map name
 * @param mapName name of the map, e.g. "SmallDivide.smf"
 * @return Zero on error; the checksum on success
 * @see GetMapChecksum
 */
EXPORT(unsigned int) GetMapChecksumFromName(const char* mapName)
{
	try {
		CheckInit();

		return archiveScanner->GetMapChecksum(mapName);
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


#define RM	0x0000F800
#define GM  0x000007E0
#define BM  0x0000001F

#define RED_RGB565(x) ((x&RM)>>11)
#define GREEN_RGB565(x) ((x&GM)>>5)
#define BLUE_RGB565(x) (x&BM)
#define PACKRGB(r, g, b) (((r<<11)&RM) | ((g << 5)&GM) | (b&BM) )

// Used to return the image
static char* imgbuf[1024*1024*2];

static void* GetMinimapSM3(string mapName, int miplevel)
{
	MapParser mapParser(mapName);
	const string minimapFile = mapParser.GetRoot().GetString("minimap", "");

	if (minimapFile.empty()) {
		memset(imgbuf,0,sizeof(imgbuf));
		return imgbuf;
	}

	CBitmap bm;
	if (!bm.Load(minimapFile)) {
		memset(imgbuf,0,sizeof(imgbuf));
		return imgbuf;
	}

	if (1024 >> miplevel != bm.xsize || 1024 >> miplevel != bm.ysize)
		bm = bm.CreateRescaled (1024 >> miplevel, 1024 >> miplevel);

	unsigned short *dst = (unsigned short*)imgbuf;
	unsigned char *src = bm.mem;
	for (int y=0;y<bm.ysize;y++)
		for (int x=0;x<bm.xsize;x++)
		{
			*dst = 0;

			*dst |= ((src[0]>>3) << 11) & RM;
			*dst |= ((src[1]>>2) << 5) & GM;
			*dst |= (src[2]>>3) & BM;

			dst ++;
			src += 4;
		}

	return imgbuf;
}

static void* GetMinimapSMF(string mapName, int miplevel)
{
	// Calculate stuff

	int mipsize = 1024;
	int offset = 0;

	for ( int i = 0; i < miplevel; i++ ) {
		int size = ((mipsize+3)/4)*((mipsize+3)/4)*8;
		offset += size;
		mipsize >>= 1;
	}

	int size = ((mipsize+3)/4)*((mipsize+3)/4)*8;
	int numblocks = size/8;

	// Read the map data
	CFileHandler in("maps/" + mapName);

	if (!in.FileExists()) {
		throw content_error("File '" + mapName + "' does not exist");
	}

	unsigned char* buffer = (unsigned char*)malloc(size);

	SMFHeader mh;
	in.Read(&mh, sizeof(mh));
	in.Seek(mh.minimapPtr + offset);
	in.Read(buffer, size);

	// Do stuff

	void* ret = (void*)imgbuf;
	unsigned short* colors = (unsigned short*)ret;

	unsigned char* temp = buffer;

	for ( int i = 0; i < numblocks; i++ ) {
		unsigned short color0 = (*(unsigned short*)&temp[0]);
		unsigned short color1 = (*(unsigned short*)&temp[2]);
		unsigned int bits = (*(unsigned int*)&temp[4]);

		for ( int a = 0; a < 4; a++ ) {
			for ( int b = 0; b < 4; b++ ) {
				int x = 4*(i % ((mipsize+3)/4))+b;
				int y = 4*(i / ((mipsize+3)/4))+a;
				unsigned char code = bits & 0x3;
				bits >>= 2;

				if ( color0 > color1 ) {
					if ( code == 0 ) {
						colors[y*mipsize+x] = color0;
					}
					else if ( code == 1 ) {
						colors[y*mipsize+x] = color1;
					}
					else if ( code == 2 ) {
						colors[y*mipsize+x] = PACKRGB((2*RED_RGB565(color0)+RED_RGB565(color1))/3, (2*GREEN_RGB565(color0)+GREEN_RGB565(color1))/3, (2*BLUE_RGB565(color0)+BLUE_RGB565(color1))/3);
					}
					else {
						colors[y*mipsize+x] = PACKRGB((2*RED_RGB565(color1)+RED_RGB565(color0))/3, (2*GREEN_RGB565(color1)+GREEN_RGB565(color0))/3, (2*BLUE_RGB565(color1)+BLUE_RGB565(color0))/3);
					}
				}
				else {
					if ( code == 0 ) {
						colors[y*mipsize+x] = color0;
					}
					else if ( code == 1 ) {
						colors[y*mipsize+x] = color1;
					}
					else if ( code == 2 ) {
						colors[y*mipsize+x] = PACKRGB((RED_RGB565(color0)+RED_RGB565(color1))/2, (GREEN_RGB565(color0)+GREEN_RGB565(color1))/2, (BLUE_RGB565(color0)+BLUE_RGB565(color1))/2);
					}
					else {
						colors[y*mipsize+x] = 0;
					}
				}
			}
		}
		temp += 8;
	}
	free(buffer);
	return (void*)ret;
}

/**
 * @brief Retrieves a minimap image for a map.
 * @param filename The name of the map, including extension.
 * @param miplevel Which miplevel of the minimap to extract from the file.
 * Set miplevel to 0 to get the largest, 1024x1024 minimap. Each increment
 * divides the width and height by 2. The maximum miplevel is 8, resulting in a
 * 4x4 image.
 * @return A pointer to a static memory area containing the minimap as a 16 bit
 * packed RGB-565 (MSB to LSB: 5 bits red, 6 bits green, 5 bits blue) linear
 * bitmap on success; NULL on error.
 *
 * An example usage would be GetMinimap("SmallDivide.smf", 2).
 * This would return a 16 bit packed RGB-565 256x256 (= 1024/2^2) bitmap.
 */
EXPORT(void*) GetMinimap(const char* filename, int miplevel)
{
	try {
		CheckInit();
		CheckNullOrEmpty(filename);

		if (miplevel < 0 || miplevel > 8)
			throw std::out_of_range("Miplevel must be between 0 and 8 (inclusive) in GetMinimap.");

		const string mapName = filename;
		ScopedMapLoader mapLoader(mapName);

		const string extension = mapName.substr(mapName.length() - 3);

		void* ret = NULL;

		if (extension == "smf") {
			ret = GetMinimapSMF(mapName, miplevel);
		} else if (extension == "sm3") {
			ret = GetMinimapSM3(mapName, miplevel);
		}

		return ret;
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


/**
 * @brief Retrieves dimensions of infomap for a map.
 * @param filename The name of the map, including extension.
 * @param name     Of which infomap to retrieve the dimensions.
 * @param width    This is set to the width of the infomap, or 0 on error.
 * @param height   This is set to the height of the infomap, or 0 on error.
 * @return Non-zero when the infomap was found with a non-zero size; zero on error.
 * @see GetInfoMap
 */
EXPORT(int) GetInfoMapSize(const char* filename, const char* name, int* width, int* height)
{
	try {
		CheckInit();
		CheckNullOrEmpty(filename);
		CheckNullOrEmpty(name);
		CheckNull(width);
		CheckNull(height);

		ScopedMapLoader mapLoader(filename);
		CSmfMapFile file(filename);
		MapBitmapInfo bmInfo = file.GetInfoMapSize(name);

		*width = bmInfo.width;
		*height = bmInfo.height;

		return bmInfo.width > 0;
	}
	UNITSYNC_CATCH_BLOCKS;

	if (width)  *width  = 0;
	if (height) *height = 0;

	return 0;
}


/**
 * @brief Retrieves infomap data of a map.
 * @param filename The name of the map, including extension.
 * @param name     Which infomap to extract from the file.
 * @param data     Pointer to memory location with enough room to hold the infomap data.
 * @param typeHint One of bm_grayscale_8 (or 1) and bm_grayscale_16 (or 2).
 * @return Non-zero if the infomap was succesfully extracted (and optionally
 * converted), or zero on error (map wasn't found, infomap wasn't found, or
 * typeHint could not be honoured.)
 *
 * This function extracts an infomap from a map. This can currently be one of:
 * "height", "metal", "grass", "type". The heightmap is natively in 16 bits per
 * pixel, the others are in 8 bits pixel. Using typeHint one can give a hint to
 * this function to convert from one format to another. Currently only the
 * conversion from 16 bpp to 8 bpp is implemented.
 */
EXPORT(int) GetInfoMap(const char* filename, const char* name, void* data, int typeHint)
{
	try {
		CheckInit();
		CheckNullOrEmpty(filename);
		CheckNullOrEmpty(name);
		CheckNull(data);

		string n = name;
		ScopedMapLoader mapLoader(filename);
		CSmfMapFile file(filename);
		int actualType = (n == "height" ? bm_grayscale_16 : bm_grayscale_8);

		if (actualType == typeHint) {
			return file.ReadInfoMap(n, data);
		}
		else if (actualType == bm_grayscale_16 && typeHint == bm_grayscale_8) {
			// convert from 16 bits per pixel to 8 bits per pixel
			MapBitmapInfo bmInfo = file.GetInfoMapSize(name);
			const int size = bmInfo.width * bmInfo.height;
			if (size <= 0) return 0;

			unsigned short* temp = new unsigned short[size];
			if (!file.ReadInfoMap(n, temp)) {
				delete[] temp;
				return 0;
			}

			const unsigned short* inp = temp;
			const unsigned short* inp_end = temp + size;
			unsigned char* outp = (unsigned char*) data;
			for (; inp < inp_end; ++inp, ++outp) {
				*outp = *inp >> 8;
			}
			delete[] temp;
			return 1;
		}
		else if (actualType == bm_grayscale_8 && typeHint == bm_grayscale_16) {
			throw content_error("converting from 8 bits per pixel to 16 bits per pixel is unsupported");
		}
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


//////////////////////////
//////////////////////////

vector<CArchiveScanner::ModData> modData;


/**
 * @brief Retrieves the number of mods available
 * @return int Zero on error; The number of mods available on success
 * @see GetMapCount
 */
EXPORT(int) GetPrimaryModCount()
{
	try {
		CheckInit();

		modData = archiveScanner->GetPrimaryMods();
		return modData.size();
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


/**
 * @brief Retrieves the name of this mod
 * @param index The mods index/id
 * @return NULL on error; The mods name on success
 *
 * Returns the name of the mod usually found in modinfo.tdf.
 * Be sure you've made a call to GetPrimaryModCount() prior to using this.
 */
EXPORT(const char*) GetPrimaryModName(int index)
{
	try {
		CheckInit();
		CheckBounds(index, modData.size());

		string x = modData[index].name;
		return GetStr(x);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


/**
 * @brief Retrieves the shortened name of this mod
 * @param index The mods index/id
 * @return NULL on error; The mods abbrieviated name on success
 *
 * Returns the shortened name of the mod usually found in modinfo.tdf.
 * Be sure you've made a call GetPrimaryModCount() prior to using this.
 */
EXPORT(const char*) GetPrimaryModShortName(int index)
{
	try {
		CheckInit();
		CheckBounds(index, modData.size());

		string x = modData[index].shortName;
		return GetStr(x);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


/**
 * @brief Retrieves the version string of this mod
 * @param index The mods index/id
 * @return NULL on error; The mods version string on success
 *
 * Returns value of the mutator tag for the specified mod usually found in modinfo.tdf.
 * Be sure you've made a call to GetPrimaryModCount() prior to using this.
 */
EXPORT(const char*) GetPrimaryModVersion(int index)
{
	try {
		CheckInit();
		CheckBounds(index, modData.size());

		string x = modData[index].version;
		return GetStr(x);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


/**
 * @brief Retrieves the mutator name of this mod
 * @param index The mods index/id
 * @return NULL on error; The mods mutator name on success
 *
 * Returns value of the mutator tag for the specified mod usually found in modinfo.tdf.
 * Be sure you've made a call to GetPrimaryModCount() prior to using this.
 */
EXPORT(const char*) GetPrimaryModMutator(int index)
{
	try {
		CheckInit();
		CheckBounds(index, modData.size());

		string x = modData[index].mutator;
		return GetStr(x);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


/**
 * @brief Retrieves the game name of this mod
 * @param index The mods index/id
 * @return NULL on error; The mods game name on success
 *
 * Returns the name of the game this mod belongs to usually found in modinfo.tdf.
 * Be sure you've made a call to GetPrimaryModCount() prior to using this.
 */
EXPORT(const char*) GetPrimaryModGame(int index)
{
	try {
		CheckInit();
		CheckBounds(index, modData.size());

		string x = modData[index].game;
		return GetStr(x);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


/**
 * @brief Retrieves the short game name of this mod
 * @param index The mods index/id
 * @return NULL on error; The mods abbrieviated game name on success
 *
 * Returns the abbrieviated name of the game this mod belongs to usually found in modinfo.tdf.
 * Be sure you've made a call to GetPrimaryModCount() prior to using this.
 */
EXPORT(const char*) GetPrimaryModShortGame(int index)
{
	try {
		CheckInit();
		CheckBounds(index, modData.size());

		string x = modData[index].shortGame;
		return GetStr(x);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


/**
 * @brief Retrieves the description of this mod
 * @param index The mods index/id
 * @return NULL on error; The mods description on success
 *
 * Returns a description for the specified mod usually found in modinfo.tdf.
 * Be sure you've made a call to GetPrimaryModCount() prior to using this.
 */
EXPORT(const char*) GetPrimaryModDescription(int index)
{
	try {
		CheckInit();
		CheckBounds(index, modData.size());

		string x = modData[index].description;
		return GetStr(x);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


/**
 * @brief Retrieves the mod's first/primary archive
 * @param index The mods index/id
 * @return NULL on error; The mods primary archive on success
 *
 * Returns the name of the primary archive of the mod.
 * Be sure you've made a call to GetPrimaryModCount() prior to using this.
 */
EXPORT(const char*) GetPrimaryModArchive(int index)
{
	try {
		CheckInit();
		CheckBounds(index, modData.size());

		return GetStr(modData[index].dependencies[0]);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


vector<string> primaryArchives;

/**
 * @brief Retrieves the number of archives a mod requires
 * @param index The index of the mod
 * @return Zero on error; the number of archives this mod depends on otherwise
 *
 * This is used to get the entire list of archives that a mod requires.
 * Call GetPrimaryModArchiveCount() with selected mod first to get number of
 * archives, and then use GetPrimaryModArchiveList() for 0 to count-1 to get the
 * name of each archive.  In code:
 *		@code
 *		int count = GetPrimaryModArchiveCount(mod_index);
 *		for (int arnr = 0; arnr < count; ++arnr) {
 *			printf("primary mod archive: %s\n", GetPrimaryModArchiveList(arnr));
 *		}
 *		@endcode
 */
EXPORT(int) GetPrimaryModArchiveCount(int index)
{
	try {
		CheckInit();
		CheckBounds(index, modData.size());

		primaryArchives = archiveScanner->GetArchives(modData[index].dependencies[0]);
		return primaryArchives.size();
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


/**
 * @brief Retrieves the name of the current mod's archive.
 * @param arnr The archive's index/id.
 * @return NULL on error; the name of the archive on success
 * @see GetPrimaryModArchiveCount
 */
EXPORT(const char*) GetPrimaryModArchiveList(int archiveNr)
{
	try {
		CheckInit();
		CheckBounds(archiveNr, primaryArchives.size());

		logOutput.Print(LOG_UNITSYNC, "primary mod archive list: %s\n", primaryArchives[archiveNr].c_str());
		return GetStr(primaryArchives[archiveNr]);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


/**
 * @brief The reverse of GetPrimaryModName()
 * @param name The name of the mod
 * @return -1 if the mod can not be found; the index of the mod otherwise
 */
EXPORT(int) GetPrimaryModIndex(const char* name)
{
	try {
		CheckInit();

		string n(name);
		for (unsigned i = 0; i < modData.size(); ++i) {
			if (modData[i].name == n)
				return i;
		}
	}
	UNITSYNC_CATCH_BLOCKS;

	// if it returns -1, make sure you call GetPrimaryModCount before GetPrimaryModIndex.
	return -1;
}


/**
 * @brief Get checksum of mod
 * @param index The mods index/id
 * @return Zero on error; the checksum on success.
 * @see GetMapChecksum
 */
EXPORT(unsigned int) GetPrimaryModChecksum(int index)
{
	try {
		CheckInit();
		CheckBounds(index, modData.size());

		return archiveScanner->GetModChecksum(GetPrimaryModArchive(index));
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


/**
 * @brief Get checksum of mod given the mod's name
 * @param name The name of the mod
 * @return Zero on error; the checksum on success.
 * @see GetMapChecksum
 */
EXPORT(unsigned int) GetPrimaryModChecksumFromName(const char* name)
{
	try {
		CheckInit();

		return archiveScanner->GetModChecksum(archiveScanner->ModNameToModArchive(name));
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


//////////////////////////
//////////////////////////

/**
 * @brief Retrieve the number of available sides
 * @return Zero on error; the number of sides on success
 *
 * This function parses the mod's side data, and returns the number of sides
 * available. Be sure to map the mod into the VFS using AddArchive() or
 * AddAllArchives() prior to using this function.
 */
EXPORT(int) GetSideCount()
{
	try {
		CheckInit();

		if (!sideParser.Load()) {
			throw content_error("failed: " + sideParser.GetErrorLog());
		}
		return sideParser.GetCount();
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


/**
 * @brief Retrieve a side's name
 * @return NULL on error; the side's name on success
 *
 * Be sure you've made a call to GetSideCount() prior to using this.
 */
EXPORT(const char*) GetSideName(int side)
{
	try {
		CheckInit();
		CheckBounds(side, sideParser.GetCount());

		// the full case name  (not the lowered version)
		return GetStr(sideParser.GetCaseName(side));
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


/**
 * @brief Retrieve a side's default starting unit
 * @return NULL on error; the side's starting unit name on success
 *
 * Be sure you've made a call to GetSideCount() prior to using this.
 */
EXPORT(const char*) GetSideStartUnit(int side)
{
	try {
		CheckInit();
		CheckBounds(side, sideParser.GetCount());

		return GetStr(sideParser.GetStartUnit(side));
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


//////////////////////////
//////////////////////////


static std::vector<Option> options;
static std::set<std::string> optionsSet;

static void ParseOptions(const string& fileName,
                         const string& fileModes,
                         const string& accessModes,
                         const string& mapName = "")
{
	parseOptions(options, fileName, fileModes, accessModes, mapName,
			&optionsSet, &LOG_UNITSYNC);
}


static void CheckOptionIndex(int optIndex)
{
	CheckInit();
	CheckBounds(optIndex, options.size());
}

static void CheckOptionType(int optIndex, int type)
{
	CheckOptionIndex(optIndex);

	if (options[optIndex].typeCode != type)
		throw std::invalid_argument("wrong option type");
}


/**
 * @brief Retrieve the number of map options available
 * @param name the name of the map
 * @return Zero on error; the number of map options available on success
 */
EXPORT(int) GetMapOptionCount(const char* name)
{
	try {
		CheckInit();
		CheckNullOrEmpty(name);

		ScopedMapLoader mapLoader(name);

		options.clear();
		optionsSet.clear();

		ParseOptions("MapOptions.lua", SPRING_VFS_MAP, SPRING_VFS_MAP, name);

		optionsSet.clear();

		return options.size();
	}
	UNITSYNC_CATCH_BLOCKS;

	options.clear();
	optionsSet.clear();

	return 0;
}


/**
 * @brief Retrieve the number of mod options available
 * @return Zero on error; the number of mod options available on success
 *
 * Be sure to map the mod into the VFS using AddArchive() or AddAllArchives()
 * prior to using this function.
 */
EXPORT(int) GetModOptionCount()
{
	try {
		CheckInit();

		options.clear();
		optionsSet.clear();

		// EngineOptions must be read first, so accidentally "overloading" engine
		// options with mod options with identical names is not possible.
		// Both files are now optional
		try {
			ParseOptions("EngineOptions.lua", SPRING_VFS_MOD_BASE, SPRING_VFS_MOD_BASE);
		}
		UNITSYNC_CATCH_BLOCKS;
		try {
			ParseOptions("ModOptions.lua", SPRING_VFS_MOD, SPRING_VFS_MOD);
		}
		UNITSYNC_CATCH_BLOCKS;

		optionsSet.clear();

		return options.size();
	}
	UNITSYNC_CATCH_BLOCKS;

	// Failed to load engineoptions
	options.clear();
	optionsSet.clear();

	return 0;
}



//////////////////////////
//////////////////////////

static std::vector< std::vector<InfoItem> > luaAIInfos;

static void GetLuaAIInfo()
{
	luaAIInfos.clear();

	LuaParser luaParser("LuaAI.lua", SPRING_VFS_MOD_BASE, SPRING_VFS_MOD_BASE);
	if (!luaParser.Execute()) {
		// It is not an error if the mod does not come with Lua AIs.
		return;
	}

	const LuaTable root = luaParser.GetRoot();
	if (!root.IsValid()) {
		throw content_error("root table invalid");
	}

	for (int i = 1; root.KeyExists(i); i++) {
		std::string shortName;
		std::string description;
		// Lua AIs can be specified in two different formats:
		shortName = root.GetString(i, "");
		if (!shortName.empty()) {
			// ... string format (name)

			description = "(please see mod description, forum or homepage)";
		} else {
			// ... table format  (name & desc)

			const LuaTable& optTbl = root.SubTable(i);
			if (!optTbl.IsValid()) {
				continue;
			}
			shortName = optTbl.GetString("name", "");
			if (shortName.empty()) {
				continue;
			}

			description = optTbl.GetString("desc", shortName);
		}

		struct InfoItem ii;
		std::vector<InfoItem> aiInfo;

		ii.key = SKIRMISH_AI_PROPERTY_SHORT_NAME;
		ii.value = shortName;
		ii.desc = "the short name of this Lua Skirmish AI";
		aiInfo.push_back(ii);

		ii.key = SKIRMISH_AI_PROPERTY_VERSION;
		ii.value = "<not versioned>";
		ii.desc = "Lua Skirmish AIs do not have a version, "
				"because they are fully defined by the mods version already.";
		aiInfo.push_back(ii);

		ii.key = SKIRMISH_AI_PROPERTY_NAME;
		ii.value = shortName + " (Mod specific AI)";
		ii.desc = "the human readable name of this Lua Skirmish AI";
		aiInfo.push_back(ii);

		ii.key = SKIRMISH_AI_PROPERTY_DESCRIPTION;
		ii.value = description;
		ii.desc = "a short description of this Lua Skirmish AI";
		aiInfo.push_back(ii);

		luaAIInfos.push_back(aiInfo);
	}
}


/**
 * @brief Retrieve the number of LUA AIs available
 * @return Zero on error; the number of LUA AIs otherwise
 *
 * Usually LUA AIs are shipped inside a mod, so be sure to map the mod into
 * the VFS using AddArchive() or AddAllArchives() prior to using this function.
 */
static int GetNumberOfLuaAIs()
{
	try {
		CheckInit();

		GetLuaAIInfo();
		return luaAIInfos.size();
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


//////////////////////////
//////////////////////////



// Updated on every call to GetSkirmishAICount
static vector<std::string> skirmishAIDataDirs;

EXPORT(int) GetSkirmishAICount() {

	try {
		CheckInit();

		skirmishAIDataDirs.clear();

		std::vector<std::string> dataDirs_tmp =
				filesystem.FindDirsInDirectSubDirs(SKIRMISH_AI_DATA_DIR);

		// filter out dirs not containing an AIInfo.lua file
		std::vector<std::string>::const_iterator i;
		for (i = dataDirs_tmp.begin(); i != dataDirs_tmp.end(); ++i) {
			const std::string& possibleDataDir = *i;
			vector<std::string> infoFile = CFileHandler::FindFiles(
					possibleDataDir, "AIInfo.lua");
			if (infoFile.size() > 0) {
				skirmishAIDataDirs.push_back(possibleDataDir);
			}
		}

		sort(skirmishAIDataDirs.begin(), skirmishAIDataDirs.end());

		int luaAIs = GetNumberOfLuaAIs();

//logOutput.Print(LOG_UNITSYNC, "GetSkirmishAICount: luaAIs: %i / skirmishAIs: %u", luaAIs, skirmishAIDataDirs.size());
		return skirmishAIDataDirs.size() + luaAIs;
	}
	UNITSYNC_CATCH_BLOCKS;

	return 0;
}


static std::vector<InfoItem> info;
static std::set<std::string> infoSet;

static void ParseInfo(const std::string& fileName,
                      const std::string& fileModes,
                      const std::string& accessModes)
{
	parseInfo(info, fileName, fileModes, accessModes, &infoSet, &LOG_UNITSYNC);
}

static void CheckSkirmishAIIndex(int aiIndex)
{
	CheckInit();
	int numSkirmishAIs = skirmishAIDataDirs.size() + luaAIInfos.size();
	CheckBounds(aiIndex, numSkirmishAIs);
}

static bool IsLuaAIIndex(int aiIndex) {
	return (((unsigned int) aiIndex) >= skirmishAIDataDirs.size());
}

static int ToPureLuaAIIndex(int aiIndex) {
	return (aiIndex - skirmishAIDataDirs.size());
}

static int loadedLuaAIIndex = -1;

EXPORT(int) GetSkirmishAIInfoCount(int aiIndex) {

	try {
		CheckSkirmishAIIndex(aiIndex);

		if (IsLuaAIIndex(aiIndex)) {
			loadedLuaAIIndex = aiIndex;
			return luaAIInfos[ToPureLuaAIIndex(loadedLuaAIIndex)].size();
		} else {
			loadedLuaAIIndex = -1;

			info.clear();
			infoSet.clear();

			ParseInfo(skirmishAIDataDirs[aiIndex] + "/AIInfo.lua",
					SPRING_VFS_RAW, SPRING_VFS_RAW);

			infoSet.clear();

			return (int)info.size();
		}
	}
	UNITSYNC_CATCH_BLOCKS;

	info.clear();

	return 0;
}

static void CheckSkirmishAIInfoIndex(int infoIndex)
{
	CheckInit();
	if (loadedLuaAIIndex >= 0) {
		CheckBounds(infoIndex, luaAIInfos[ToPureLuaAIIndex(loadedLuaAIIndex)].size());
	} else {
		CheckBounds(infoIndex, info.size());
	}
}

EXPORT(const char*) GetInfoKey(int infoIndex) {

	try {
		CheckSkirmishAIInfoIndex(infoIndex);
		if (loadedLuaAIIndex >= 0) {
			return GetStr(luaAIInfos[ToPureLuaAIIndex(loadedLuaAIIndex)][infoIndex].key);
		} else {
			return GetStr(info[infoIndex].key);
		}
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}
EXPORT(const char*) GetInfoValue(int infoIndex) {

	try {
		CheckSkirmishAIInfoIndex(infoIndex);
		if (loadedLuaAIIndex >= 0) {
			return GetStr(luaAIInfos[ToPureLuaAIIndex(loadedLuaAIIndex)][infoIndex].value);
		} else {
			return GetStr(info[infoIndex].value);
		}
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}
EXPORT(const char*) GetInfoDescription(int infoIndex) {

	try {
		CheckSkirmishAIInfoIndex(infoIndex);
		if (loadedLuaAIIndex >= 0) {
			return GetStr(luaAIInfos[ToPureLuaAIIndex(loadedLuaAIIndex)][infoIndex].desc);
		} else {
			return GetStr(info[infoIndex].desc);
		}
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}

EXPORT(int) GetSkirmishAIOptionCount(int aiIndex) {

	try {
		CheckSkirmishAIIndex(aiIndex);

		if (IsLuaAIIndex(aiIndex)) {
			// lua AIs do not have options
			return 0;
		} else {
			options.clear();
			optionsSet.clear();

			ParseOptions(skirmishAIDataDirs[aiIndex] + "/AIOptions.lua",
					SPRING_VFS_RAW, SPRING_VFS_RAW);

			optionsSet.clear();

			GetLuaAIInfo();

			return options.size();
		}
	}
	UNITSYNC_CATCH_BLOCKS;

	options.clear();
	optionsSet.clear();

	return 0;
}


// Common Options Parameters

/**
 * @brief Retrieve an option's key
 * @param optIndex option index/id
 * @return NULL on error; the option's key on success
 *
 * The key of an option is the name it should be given in the start script's
 * MODOPTIONS or MAPOPTIONS section.
 * Be sure you've made a call to either GetMapOptionCount()
 * or GetModOptionCount() prior to using this.
 */
EXPORT(const char*) GetOptionKey(int optIndex)
{
	try {
		CheckOptionIndex(optIndex);
		return GetStr(options[optIndex].key);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


/**
 * @brief Retrieve an option's scope
 * @param optIndex option index/id
 * @return NULL on error; the option's scope on success
 *
 * Will be either "global" (default), "player", "team" or "allyteam"
 */
EXPORT(const char*) GetOptionScope(int optIndex)
{
	try {
		CheckOptionIndex(optIndex);
		return GetStr(options[optIndex].scope);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}

/**
 * @brief Retrieve an option's name
 * @param optIndex option index/id
 * @return NULL on error; the option's user visible name on success
 *
 * Be sure you've made a call to either GetMapOptionCount()
 * or GetModOptionCount() prior to using this.
 */
EXPORT(const char*) GetOptionName(int optIndex)
{
	try {
		CheckOptionIndex(optIndex);
		return GetStr(options[optIndex].name);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


/**
 * @brief Retrieve an option's section
 * @param optIndex option index/id
 * @return NULL on error; the option's section name on success
 *
 * Be sure you've made a call to either GetMapOptionCount()
 * or GetModOptionCount() prior to using this.
 */
EXPORT(const char*) GetOptionSection(int optIndex)
{
	try {
		CheckOptionIndex(optIndex);
		return GetStr(options[optIndex].section);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


/**
 * @brief Retrieve an option's style
 * @param optIndex option index/id
 * @return NULL on error; the option's style on success
 *
 * The format of an option style string is currently undecided.
 *
 * Be sure you've made a call to either GetMapOptionCount()
 * or GetModOptionCount() prior to using this.
 */
EXPORT(const char*) GetOptionStyle(int optIndex)
{
	try {
		CheckOptionIndex(optIndex);
		return GetStr(options[optIndex].style);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


/**
 * @brief Retrieve an option's description
 * @param optIndex option index/id
 * @return NULL on error; the option's description on success
 *
 * Be sure you've made a call to either GetMapOptionCount()
 * or GetModOptionCount() prior to using this.
 */
EXPORT(const char*) GetOptionDesc(int optIndex)
{
	try {
		CheckOptionIndex(optIndex);
		return GetStr(options[optIndex].desc);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


/**
 * @brief Retrieve an option's type
 * @param optIndex option index/id
 * @return opt_error on error; the option's type on success
 *
 * Be sure you've made a call to either GetMapOptionCount()
 * or GetModOptionCount() prior to using this.
 */
EXPORT(int) GetOptionType(int optIndex)
{
	try {
		CheckOptionIndex(optIndex);
		return options[optIndex].typeCode;
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


// Bool Options

/**
 * @brief Retrieve an opt_bool option's default value
 * @param optIndex option index/id
 * @return Zero on error; the option's default value (0 or 1) on success
 *
 * Be sure you've made a call to either GetMapOptionCount()
 * or GetModOptionCount() prior to using this.
 */
EXPORT(int) GetOptionBoolDef(int optIndex)
{
	try {
		CheckOptionType(optIndex, opt_bool);
		return options[optIndex].boolDef ? 1 : 0;
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


// Number Options

/**
 * @brief Retrieve an opt_number option's default value
 * @param optIndex option index/id
 * @return Zero on error; the option's default value on success
 *
 * Be sure you've made a call to either GetMapOptionCount()
 * or GetModOptionCount() prior to using this.
 */
EXPORT(float) GetOptionNumberDef(int optIndex)
{
	try {
		CheckOptionType(optIndex, opt_number);
		return options[optIndex].numberDef;
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0.0f;
}


/**
 * @brief Retrieve an opt_number option's minimum value
 * @param optIndex option index/id
 * @return -1.0e30 on error; the option's minimum value on success
 *
 * Be sure you've made a call to either GetMapOptionCount()
 * or GetModOptionCount() prior to using this.
 */
EXPORT(float) GetOptionNumberMin(int optIndex)
{
	try {
		CheckOptionType(optIndex, opt_number);
		return options[optIndex].numberMin;
	}
	UNITSYNC_CATCH_BLOCKS;
	return -1.0e30f; // FIXME ?
}


/**
 * @brief Retrieve an opt_number option's maximum value
 * @param optIndex option index/id
 * @return +1.0e30 on error; the option's maximum value on success
 *
 * Be sure you've made a call to either GetMapOptionCount()
 * or GetModOptionCount() prior to using this.
 */
EXPORT(float) GetOptionNumberMax(int optIndex)
{
	try {
		CheckOptionType(optIndex, opt_number);
		return options[optIndex].numberMax;
	}
	UNITSYNC_CATCH_BLOCKS;
	return +1.0e30f; // FIXME ?
}


/**
 * @brief Retrieve an opt_number option's step value
 * @param optIndex option index/id
 * @return Zero on error; the option's step value on success
 *
 * Be sure you've made a call to either GetMapOptionCount()
 * or GetModOptionCount() prior to using this.
 */
EXPORT(float) GetOptionNumberStep(int optIndex)
{
	try {
		CheckOptionType(optIndex, opt_number);
		return options[optIndex].numberStep;
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0.0f;
}


// String Options

/**
 * @brief Retrieve an opt_string option's default value
 * @param optIndex option index/id
 * @return NULL on error; the option's default value on success
 *
 * Be sure you've made a call to either GetMapOptionCount()
 * or GetModOptionCount() prior to using this.
 */
EXPORT(const char*) GetOptionStringDef(int optIndex)
{
	try {
		CheckOptionType(optIndex, opt_string);
		return GetStr(options[optIndex].stringDef);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


/**
 * @brief Retrieve an opt_string option's maximum length
 * @param optIndex option index/id
 * @return Zero on error; the option's maximum length on success
 *
 * Be sure you've made a call to either GetMapOptionCount()
 * or GetModOptionCount() prior to using this.
 */
EXPORT(int) GetOptionStringMaxLen(int optIndex)
{
	try {
		CheckOptionType(optIndex, opt_string);
		return options[optIndex].stringMaxLen;
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


// List Options

/**
 * @brief Retrieve an opt_list option's number of available items
 * @param optIndex option index/id
 * @return Zero on error; the option's number of available items on success
 *
 * Be sure you've made a call to either GetMapOptionCount()
 * or GetModOptionCount() prior to using this.
 */
EXPORT(int) GetOptionListCount(int optIndex)
{
	try {
		CheckOptionType(optIndex, opt_list);
		return options[optIndex].list.size();
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


/**
 * @brief Retrieve an opt_list option's default value
 * @param optIndex option index/id
 * @return NULL on error; the option's default value (list item key) on success
 *
 * Be sure you've made a call to either GetMapOptionCount()
 * or GetModOptionCount() prior to using this.
 */
EXPORT(const char*) GetOptionListDef(int optIndex)
{
	try {
		CheckOptionType(optIndex, opt_list);
		return GetStr(options[optIndex].listDef);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


/**
 * @brief Retrieve an opt_list option item's key
 * @param optIndex option index/id
 * @param itemIndex list item index/id
 * @return NULL on error; the option item's key (list item key) on success
 *
 * Be sure you've made a call to either GetMapOptionCount()
 * or GetModOptionCount() prior to using this.
 */
EXPORT(const char*) GetOptionListItemKey(int optIndex, int itemIndex)
{
	try {
		CheckOptionType(optIndex, opt_list);
		const vector<OptionListItem>& list = options[optIndex].list;
		CheckBounds(itemIndex, list.size());
		return GetStr(list[itemIndex].key);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


/**
 * @brief Retrieve an opt_list option item's name
 * @param optIndex option index/id
 * @param itemIndex list item index/id
 * @return NULL on error; the option item's name on success
 *
 * Be sure you've made a call to either GetMapOptionCount()
 * or GetModOptionCount() prior to using this.
 */
EXPORT(const char*) GetOptionListItemName(int optIndex, int itemIndex)
{
	try {
		CheckOptionType(optIndex, opt_list);
		const vector<OptionListItem>& list = options[optIndex].list;
		CheckBounds(itemIndex, list.size());
		return GetStr(list[itemIndex].name);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


/**
 * @brief Retrieve an opt_list option item's description
 * @param optIndex option index/id
 * @param itemIndex list item index/id
 * @return NULL on error; the option item's description on success
 *
 * Be sure you've made a call to either GetMapOptionCount()
 * or GetModOptionCount() prior to using this.
 */
EXPORT(const char*) GetOptionListItemDesc(int optIndex, int itemIndex)
{
	try {
		CheckOptionType(optIndex, opt_list);
		const vector<OptionListItem>& list = options[optIndex].list;
		CheckBounds(itemIndex, list.size());
		return GetStr(list[itemIndex].desc);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


//////////////////////////
//////////////////////////

static vector<string> modValidMaps;


static int LuaGetMapList(lua_State* L)
{
	lua_newtable(L);
	const int mapCount = GetMapCount();
	for (int i = 0; i < mapCount; i++) {
		lua_pushnumber(L, i + 1);
		lua_pushstring(L, GetMapName(i));
		lua_rawset(L, -3);
	}
	return 1;
}


static void LuaPushNamedString(lua_State* L,
                              const string& key, const string& value)
{
	lua_pushstring(L, key.c_str());
	lua_pushstring(L, value.c_str());
	lua_rawset(L, -3);
}


static void LuaPushNamedNumber(lua_State* L, const string& key, float value)
{
	lua_pushstring(L, key.c_str());
	lua_pushnumber(L, value);
	lua_rawset(L, -3);
}


static int LuaGetMapInfo(lua_State* L)
{
	const string mapName = luaL_checkstring(L, 1);

	MapInfo mi;
	char auth[256];
	char desc[256];
	mi.author = auth;
 	mi.author[0] = 0;
	mi.description = desc;
	mi.description[0] = 0;

	if (!GetMapInfoEx(mapName.c_str(), &mi, 1)) {
		logOutput.Print(LOG_UNITSYNC, "LuaGetMapInfo: _GetMapInfoEx(\"%s\") failed", mapName.c_str());
		return 0;
	}

	lua_newtable(L);

	LuaPushNamedString(L, "author", mi.author);
	LuaPushNamedString(L, "desc",   mi.description);

	LuaPushNamedNumber(L, "tidal",   mi.tidalStrength);
	LuaPushNamedNumber(L, "gravity", mi.gravity);
	LuaPushNamedNumber(L, "metal",   mi.maxMetal);
	LuaPushNamedNumber(L, "windMin", mi.minWind);
	LuaPushNamedNumber(L, "windMax", mi.maxWind);
	LuaPushNamedNumber(L, "mapX",    mi.width);
	LuaPushNamedNumber(L, "mapY",    mi.height);
	LuaPushNamedNumber(L, "extractorRadius", mi.extractorRadius);

	lua_pushstring(L, "startPos");
	lua_newtable(L);
	for (int i = 0; i < mi.posCount; i++) {
		lua_pushnumber(L, i + 1);
		lua_newtable(L);
		LuaPushNamedNumber(L, "x", mi.positions[i].x);
		LuaPushNamedNumber(L, "z", mi.positions[i].z);
		lua_rawset(L, -3);
	}
	lua_rawset(L, -3);

	return 1;
}


/**
 * @brief Retrieve the number of valid maps for the current mod
 * @return 0 on error; the number of valid maps on success
 *
 * A return value of 0 means that any map can be selected.
 * Be sure to map the mod into the VFS using AddArchive() or AddAllArchives()
 * prior to using this function.
 */
EXPORT(int) GetModValidMapCount()
{
	try {
		CheckInit();

		modValidMaps.clear();

		LuaParser luaParser("ValidMaps.lua", SPRING_VFS_MOD, SPRING_VFS_MOD);
		luaParser.GetTable("Spring");
		luaParser.AddFunc("GetMapList", LuaGetMapList);
		luaParser.AddFunc("GetMapInfo", LuaGetMapInfo);
		luaParser.EndTable();
		if (!luaParser.Execute()) {
			throw content_error("luaParser.Execute() failed: " + luaParser.GetErrorLog());
		}

		const LuaTable root = luaParser.GetRoot();
		if (!root.IsValid()) {
			throw content_error("root table invalid");
		}

		for (int index = 1; root.KeyExists(index); index++) {
			const string map = root.GetString(index, "");
			if (!map.empty()) {
				modValidMaps.push_back(map);
			}
		}

		return modValidMaps.size();
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


/**
 * @brief Retrieve the name of a map valid for the current mod
 * @return NULL on error; the name of the map on success
 *
 * Map names should be complete  (including the .smf or .sm3 extension.)
 * Be sure you've made a call to GetModValidMapCount() prior to using this.
 */
EXPORT(const char*) GetModValidMap(int index)
{
	try {
		CheckInit();
		CheckBounds(index, modValidMaps.size());
		return GetStr(modValidMaps[index]);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


//////////////////////////
//////////////////////////

static map<int, CFileHandler*> openFiles;
static int nextFile = 0;
static vector<string> curFindFiles;

static void CheckFileHandle(int handle)
{
	CheckInit();

	if (openFiles.find(handle) == openFiles.end())
		throw content_error("Unregistered handle. Pass a handle returned by OpenFileVFS.");
}


/**
 * @brief Open a file from the VFS
 * @param name the name of the file
 * @return Zero on error; a non-zero file handle on success.
 *
 * The returned file handle is needed for subsequent calls to CloseFileVFS(),
 * ReadFileVFS() and FileSizeVFS().
 *
 * Map the wanted archives into the VFS with AddArchive() or AddAllArchives()
 * before using this function.
 */
EXPORT(int) OpenFileVFS(const char* name)
{
	try {
		CheckInit();
		CheckNullOrEmpty(name);

		logOutput.Print(LOG_UNITSYNC, "openfilevfs: %s\n", name);

		CFileHandler* fh = new CFileHandler(name);
		if (!fh->FileExists()) {
			delete fh;
			throw content_error("File '" + string(name) + "' does not exist");
		}

		nextFile++;
		openFiles[nextFile] = fh;

		return nextFile;
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


/**
 * @brief Close a file in the VFS
 * @param handle the file handle as returned by OpenFileVFS()
 */
EXPORT(void) CloseFileVFS(int handle)
{
	try {
		CheckFileHandle(handle);

		logOutput.Print(LOG_UNITSYNC, "closefilevfs: %d\n", handle);
		delete openFiles[handle];
		openFiles.erase(handle);
	}
	UNITSYNC_CATCH_BLOCKS;
}

/**
 * @brief Read some data from a file in the VFS
 * @param handle the file handle as returned by OpenFileVFS()
 * @param buf output buffer, must be at least length bytes
 * @param length how many bytes to read from the file
 * @return -1 on error; the number of bytes read on success
 * (if this is less than length you reached the end of the file.)
 */
EXPORT(int) ReadFileVFS(int handle, void* buf, int length)
{
	try {
		CheckFileHandle(handle);
		CheckNull(buf);
		CheckPositive(length);

		logOutput.Print(LOG_UNITSYNC, "readfilevfs: %d\n", handle);
		CFileHandler* fh = openFiles[handle];
		return fh->Read(buf, length);
	}
	UNITSYNC_CATCH_BLOCKS;
	return -1;
}


/**
 * @brief Retrieve size of a file in the VFS
 * @param handle the file handle as returned by OpenFileVFS()
 * @return -1 on error; the size of the file on success
 */
EXPORT(int) FileSizeVFS(int handle)
{
	try {
		CheckFileHandle(handle);

		logOutput.Print(LOG_UNITSYNC, "filesizevfs: %d\n", handle);
		CFileHandler* fh = openFiles[handle];
		return fh->FileSize();
	}
	UNITSYNC_CATCH_BLOCKS;
	return -1;
}

/**
 * Does not currently support more than one call at a time.
 * (a new call to initfind destroys data from previous ones)
 * pass the returned handle to findfiles to get the results
 */
EXPORT(int) InitFindVFS(const char* pattern)
{
	try {
		CheckInit();
		CheckNullOrEmpty(pattern);

		string path = filesystem.GetDirectory(pattern);
		string patt = filesystem.GetFilename(pattern);
		logOutput.Print(LOG_UNITSYNC, "initfindvfs: %s\n", pattern);
		curFindFiles = CFileHandler::FindFiles(path, patt);
		return 0;
	}
	UNITSYNC_CATCH_BLOCKS;
	return -1;
}

/**
 * Does not currently support more than one call at a time.
 * (a new call to initfind destroys data from previous ones)
 * pass the returned handle to findfiles to get the results
 */
EXPORT(int) InitDirListVFS(const char* path, const char* pattern, const char* modes)
{
	try {
		CheckInit();

		if (path    == NULL) { path = "";              }
		if (modes   == NULL) { modes = SPRING_VFS_ALL; }
		if (pattern == NULL) { pattern = "*";          }
		logOutput.Print(LOG_UNITSYNC, "InitDirListVFS: '%s' '%s' '%s'\n", path, pattern, modes);
		curFindFiles = CFileHandler::DirList(path, pattern, modes);
		return 0;
	}
	UNITSYNC_CATCH_BLOCKS;
	return -1;
}

/**
 * Does not currently support more than one call at a time.
 * (a new call to initfind destroys data from previous ones)
 * pass the returned handle to findfiles to get the results
 */
EXPORT(int) InitSubDirsVFS(const char* path, const char* pattern, const char* modes)
{
	try {
		CheckInit();
		if (path    == NULL) { path = "";              }
		if (modes   == NULL) { modes = SPRING_VFS_ALL; }
		if (pattern == NULL) { pattern = "*";          }
		logOutput.Print(LOG_UNITSYNC, "InitSubDirsVFS: '%s' '%s' '%s'\n", path, pattern, modes);
		curFindFiles = CFileHandler::SubDirs(path, pattern, modes);
		return 0;
	}
	UNITSYNC_CATCH_BLOCKS;
	return -1;
}

// On first call, pass handle from initfind. pass the return value of this function on subsequent calls
// until 0 is returned. size should be set to max namebuffer size on call
EXPORT(int) FindFilesVFS(int handle, char* nameBuf, int size)
{
	try {
		CheckInit();
		CheckNull(nameBuf);
		CheckPositive(size);

		logOutput.Print(LOG_UNITSYNC, "findfilesvfs: %d\n", handle);
		if ((unsigned)handle >= curFindFiles.size())
			return 0;
		safe_strzcpy(nameBuf, curFindFiles[handle], size);
		return handle + 1;
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


//////////////////////////
//////////////////////////

static map<int, CArchiveBase*> openArchives;
static int nextArchive = 0;


static void CheckArchiveHandle(int handle)
{
	CheckInit();

	if (openArchives.find(handle) == openArchives.end())
		throw content_error("Unregistered handle. Pass a handle returned by OpenArchive.");
}


/**
 * @brief Open an archive
 * @param name the name of the archive (*.sdz, *.sd7, ...)
 * @return Zero on error; a non-zero archive handle on success.
 * @sa OpenArchiveType
 */
EXPORT(int) OpenArchive(const char* name)
{
	try {
		CheckInit();
		CheckNullOrEmpty(name);

		CArchiveBase* a = CArchiveFactory::OpenArchive(name);

		if (!a) {
			throw content_error("Archive '" + string(name) + "' could not be opened");
		}

		nextArchive++;
		openArchives[nextArchive] = a;
		return nextArchive;
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


/**
 * @brief Open an archive
 * @param name the name of the archive (*.sd7, *.sdz, *.sdd, *.ccx, *.hpi, *.ufo, *.gp3, *.gp4, *.swx)
 * @param type the type of the archive (sd7, 7z, sdz, zip, sdd, dir, ccx, hpi, ufo, gp3, gp4, swx)
 * @return Zero on error; a non-zero archive handle on success.
 * @sa OpenArchive
 *
 * The list of supported types and recognized extensions may change at any time.
 * (But this list will always be the same as the file types recognized by the engine.)
 *
 * This function is pointless, because OpenArchive() does the same and automatically
 * detects the file type based on it's extension.  Who added it anyway?
 */
EXPORT(int) OpenArchiveType(const char* name, const char* type)
{
	try {
		CheckInit();
		CheckNullOrEmpty(name);
		CheckNullOrEmpty(type);

		CArchiveBase* a = CArchiveFactory::OpenArchive(name, type);

		if (!a) {
			throw content_error("Archive '" + string(name) + "' could not be opened");
		}

		nextArchive++;
		openArchives[nextArchive] = a;
		return nextArchive;
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


/**
 * @brief Close an archive in the VFS
 * @param archive the archive handle as returned by OpenArchive()
 */
EXPORT(void) CloseArchive(int archive)
{
	try {
		CheckArchiveHandle(archive);

		delete openArchives[archive];
		openArchives.erase(archive);
	}
	UNITSYNC_CATCH_BLOCKS;
}


EXPORT(int) FindFilesArchive(int archive, int cur, char* nameBuf, int* size)
{
	try {
		CheckArchiveHandle(archive);
		CheckNull(nameBuf);
		CheckNull(size);

		CArchiveBase* a = openArchives[archive];

		logOutput.Print(LOG_UNITSYNC, "findfilesarchive: %d\n", archive);

		string name;
		int s;

		int ret = a->FindFiles(cur, &name, &s);
		strcpy(nameBuf, name.c_str()); // FIXME: oops, buffer overflow
		*size = s;
		return ret;
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


/**
 * @brief Open an archive member
 * @param archive the archive handle as returned by OpenArchive()
 * @param name the name of the file
 * @return Zero on error; a non-zero file handle on success.
 *
 * The returned file handle is needed for subsequent calls to ReadArchiveFile(),
 * CloseArchiveFile() and SizeArchiveFile().
 */
EXPORT(int) OpenArchiveFile(int archive, const char* name)
{
	try {
		CheckArchiveHandle(archive);
		CheckNullOrEmpty(name);

		CArchiveBase* a = openArchives[archive];
		return a->OpenFile(name);
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


/**
 * @brief Read some data from an archive member
 * @param archive the archive handle as returned by OpenArchive()
 * @param handle the file handle as returned by OpenArchiveFile()
 * @param buffer output buffer, must be at least numBytes bytes
 * @param numBytes how many bytes to read from the file
 * @return -1 on error; the number of bytes read on success
 * (if this is less than numBytes you reached the end of the file.)
 */
EXPORT(int) ReadArchiveFile(int archive, int handle, void* buffer, int numBytes)
{
	try {
		CheckArchiveHandle(archive);
		CheckNull(buffer);
		CheckPositive(numBytes);

		CArchiveBase* a = openArchives[archive];
		return a->ReadFile(handle, buffer, numBytes);
	}
	UNITSYNC_CATCH_BLOCKS;
	return -1;
}


/**
 * @brief Close an archive member
 * @param archive the archive handle as returned by OpenArchive()
 * @param handle the file handle as returned by OpenArchiveFile()
 */
EXPORT(void) CloseArchiveFile(int archive, int handle)
{
	try {
		CheckArchiveHandle(archive);

		CArchiveBase* a = openArchives[archive];
		a->CloseFile(handle);
	}
	UNITSYNC_CATCH_BLOCKS;
}


/**
 * @brief Retrieve size of an archive member
 * @param archive the archive handle as returned by OpenArchive()
 * @param handle the file handle as returned by OpenArchiveFile()
 * @return -1 on error; the size of the file on success
 */
EXPORT(int) SizeArchiveFile(int archive, int handle)
{
	try {
		CheckArchiveHandle(archive);

		CArchiveBase* a = openArchives[archive];
		return a->FileSize(handle);
	}
	UNITSYNC_CATCH_BLOCKS;
	return -1;
}

//////////////////////////
//////////////////////////

char strBuf[STRBUF_SIZE];

//Just returning str.c_str() does not work
const char *GetStr(string str)
{
	//static char strBuf[STRBUF_SIZE];

	if (str.length() + 1 > STRBUF_SIZE) {
		sprintf(strBuf, "Increase STRBUF_SIZE (needs %d bytes)", str.length() + 1);
	}
	else {
		strcpy(strBuf, str.c_str());
	}

	return strBuf;
}

void PrintLoadMsg(const char* text)
{
}


//////////////////////////
//////////////////////////

EXPORT(void) SetSpringConfigFile(const char* filenameAsAbsolutePath)
{
	ConfigHandler::Instantiate(filenameAsAbsolutePath);
}

EXPORT(const char*) GetSpringConfigFile()
{
	return GetStr(configHandler->GetConfigFile());
}

static void CheckConfigHandler()
{
	if (!configHandler)
		throw std::logic_error("Unitsync config handler not initialized, check config source.");
}


/**
 * @brief get string from Spring configuration
 * @param name name of key to get
 * @param defvalue default string value to use if key is not found, may not be NULL
 * @return string value
 */
EXPORT(const char*) GetSpringConfigString(const char* name, const char* defValue)
{
	try {
		CheckConfigHandler();
		string res = configHandler->GetString(name, defValue);
		return GetStr(res);
	}
	UNITSYNC_CATCH_BLOCKS;
	return defValue;
}

/**
 * @brief get integer from Spring configuration
 * @param name name of key to get
 * @param defvalue default integer value to use if key is not found
 * @return integer value
 */
EXPORT(int) GetSpringConfigInt(const char* name, const int defValue)
{
	try {
		CheckConfigHandler();
		return configHandler->Get(name, defValue);
	}
	UNITSYNC_CATCH_BLOCKS;
	return defValue;
}

/**
 * @brief get float from Spring configuration
 * @param name name of key to get
 * @param defvalue default float value to use if key is not found
 * @return float value
 */
EXPORT(float) GetSpringConfigFloat(const char* name, const float defValue)
{
	try {
		CheckConfigHandler();
		return configHandler->Get(name, defValue);
	}
	UNITSYNC_CATCH_BLOCKS;
	return defValue;
}

/**
 * @brief set string in Spring configuration
 * @param name name of key to set
 * @param value string value to set
 */
EXPORT(void) SetSpringConfigString(const char* name, const char* value)
{
	try {
		CheckConfigHandler();
		configHandler->SetString( name, value );
	}
	UNITSYNC_CATCH_BLOCKS;
}

/**
 * @brief set integer in Spring configuration
 * @param name name of key to set
 * @param value integer value to set
 */
EXPORT(void) SetSpringConfigInt(const char* name, const int value)
{
	try {
		CheckConfigHandler();
		configHandler->Set(name, value);
	}
	UNITSYNC_CATCH_BLOCKS;
}

/**
 * @brief set float in Spring configuration
 * @param name name of key to set
 * @param value float value to set
 */
EXPORT(void) SetSpringConfigFloat(const char* name, const float value)
{
	try {
		CheckConfigHandler();
		configHandler->Set(name, value);
	}
	UNITSYNC_CATCH_BLOCKS;
}

/** @} */

//////////////////////////
//////////////////////////

// Helper class for popping up a MessageBox only once

class CMessageOnce
{
	private:
		bool alreadyDone;

	public:
		CMessageOnce() : alreadyDone(false) {}
		void operator() (const string& msg)
		{
			if (alreadyDone) return;
			alreadyDone = true;
			logOutput.Print(LOG_UNITSYNC, "Message from DLL: %s\n", msg.c_str());
#ifdef WIN32
			MessageBox(NULL, msg.c_str(), "Message from DLL", MB_OK);
#endif
		}
};

#define DEPRECATED \
	static CMessageOnce msg; \
	msg(string(__FUNCTION__) + ": deprecated unitsync function called, please update your lobby client"); \
	SetLastError("deprecated unitsync function called")
