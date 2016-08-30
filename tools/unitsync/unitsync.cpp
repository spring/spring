/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "unitsync.h"
#include "unitsync_api.h"

#include <algorithm>
#include <string>
#include <vector>
#include <set>

// shared with spring:
#include "lib/lua/include/LuaInclude.h"
#include "Game/GameVersion.h"
#include "Lua/LuaParser.h"
#include "Map/MapParser.h"
#include "Map/ReadMap.h"
#include "Map/SMF/SMFMapFile.h"
#include "Sim/Misc/SideParser.h"
#include "ExternalAI/Interface/aidefines.h"
#include "ExternalAI/Interface/SSkirmishAILibrary.h"
#include "ExternalAI/LuaAIImplHandler.h"
#include "System/FileSystem/Archives/IArchive.h"
#include "System/FileSystem/ArchiveLoader.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/DataDirsAccess.h"
#include "System/FileSystem/DataDirLocater.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/FileSystemInitializer.h"
#include "System/Config/ConfigHandler.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"
#include "System/Log/Level.h"
#include "System/Log/DefaultFilter.h"
#include "System/LogOutput.h"
#include "System/Misc/SpringTime.h"
#include "System/Util.h"
#include "System/exportdefines.h"
#include "System/Info.h"
#include "System/Option.h"
#include "System/SafeCStrings.h"
#include "System/ThreadPool.h"

#ifdef WIN32
#include <windows.h>
#endif

//////////////////////////
//////////////////////////

#define LOG_SECTION_UNITSYNC "unitsync"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_UNITSYNC)

// use the specific section for all LOG*() calls in this source file
#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_UNITSYNC

// for we do not have to include global-stuff (Sim/Misc/GlobalConstants.h)
#define SQUARE_SIZE 8


#ifdef WIN32
BOOL CALLING_CONV DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved)
{
	return TRUE;
}
#endif


CONFIG(bool, UnitsyncAutoUnLoadMaps).defaultValue(true).description("Automaticly load and unload the required map for some unitsync functions.");
CONFIG(bool, UnitsyncAutoUnLoadMapsIsSupported).defaultValue(true).readOnly(true).description("Check for support of UnitsyncAutoUnLoadMaps");

//////////////////////////
//////////////////////////

// Helper class for popping up a MessageBox only once

class CMessageOnce
{
public:
	CMessageOnce(const std::string& message)
		: alreadyDone(false)
		, message(message)
	{
		assert(!message.empty());
	}

	void print() {
		if (!alreadyDone) {
			alreadyDone = true;
			LOG_L(L_WARNING, "%s", message.c_str());
		}
	}

private:
	bool alreadyDone;
	const std::string message;
};


//////////////////////////
//////////////////////////

// function argument checking

static bool CheckInit(bool throwException = true)
{
	if (archiveScanner == NULL || vfsHandler == NULL) {
		if (throwException) {
			throw std::logic_error("UnitSync not initialized. Call Init first.");
		}

		return false;
	}

	return true;
}

static void _CheckNull(void* condition, const char* name)
{
	if (!condition)
		throw std::invalid_argument("Argument " + std::string(name) + " may not be null.");
}

static void _CheckNullOrEmpty(const char* condition, const char* name)
{
	if (!condition || *condition == 0)
		throw std::invalid_argument("Argument " + std::string(name) + " may not be null or empty.");
}

static void _CheckBounds(int index, int size, const char* name)
{
	if (index < 0 || index >= size)
		throw std::out_of_range("Argument " + std::string(name) + " out of bounds. Index: " +
		                         IntToString(index) + " Array size: " + IntToString(size));
}

static void _CheckPositive(int value, const char* name)
{
	if (value <= 0)
		throw std::out_of_range("Argument " + std::string(name) + " must be positive.");
}

#define CheckNull(arg)         _CheckNull((arg), #arg)
#define CheckNullOrEmpty(arg)  _CheckNullOrEmpty((arg), #arg)
#define CheckBounds(arg, size) _CheckBounds((arg), (size), #arg)
#define CheckPositive(arg)     _CheckPositive((arg), #arg);

static std::vector<InfoItem> info;
static std::set<std::string> infoSet;


struct GameDataUnitDef {
	std::string name;
	std::string fullName;
};

static std::vector<GameDataUnitDef> unitDefs;

void LoadGameDataUnitDefs() {
	unitDefs.clear();

	LuaParser luaParser("gamedata/defs.lua", SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);

	if (!luaParser.Execute()) {
		throw content_error("luaParser.Execute() failed: " + luaParser.GetErrorLog());
	}

	LuaTable rootTable = luaParser.GetRoot().SubTable("UnitDefs");

	if (!rootTable.IsValid()) {
		throw content_error("root unitdef table invalid");
	}

	std::vector<std::string> unitDefNames;
	rootTable.GetKeys(unitDefNames);

	for (unsigned int i = 0; i < unitDefNames.size(); ++i) {
		const std::string& udName = unitDefNames[i];
		const LuaTable udTable = rootTable.SubTable(udName);
		const GameDataUnitDef ud = {udName, udTable.GetString("name", udName)};

		unitDefs.push_back(ud);
	}
}

static bool autoUnLoadmap = true;

//////////////////////////
//////////////////////////

// error handling

static std::string lastError;

static void _SetLastError(const std::string& err)
{
	LOG_L(L_ERROR, "%s", err.c_str());
	lastError = err;
}

#define SetLastError(str) \
	_SetLastError(std::string(__FUNCTION__) + ": " + (str))

#define UNITSYNC_CATCH_BLOCKS \
	catch (const user_error& ex) { \
		SetLastError(ex.what()); \
	} \
	catch (const std::exception& ex) { \
		SetLastError(ex.what()); \
	} \
	catch (...) { \
		SetLastError("an unknown exception was thrown"); \
	}

EXPORT(const char*) GetNextError()
{
	try {
		// queue is only 1 element long now for simplicity :-)

		if (lastError.empty()) return NULL;

		std::string err = lastError;
		lastError.clear();
		return GetStr(err);
	}
	UNITSYNC_CATCH_BLOCKS;

	// Oops, can't even return errors anymore...
	// Returning anything but NULL might cause infinite loop in lobby client...
	//return __FUNCTION__ ": fatal error: an exception was thrown in GetNextError";
	return NULL;
}




//////////////////////////
//////////////////////////

static std::string GetMapFile(const std::string& mapName)
{
	const std::string mapFile = archiveScanner->MapNameToMapFile(mapName);

	if (mapFile != mapName) {
		//! translation finished fine
		return mapFile;
	}

	throw std::invalid_argument("Could not find a map named \"" + mapName + "\"");
	return "";
}


class ScopedMapLoader {
	public:
		/**
		 * @brief Helper class for loading a map archive temporarily
		 * @param mapName the name of the to be loaded map
		 * @param mapFile checks if this file already exists in the current VFS,
		 *   if so skip reloading
		 */
		ScopedMapLoader(const std::string& mapName, const std::string& mapFile)
			: oldHandler(vfsHandler)
		{
			if (!autoUnLoadmap)
				return;
			CFileHandler f(mapFile);
			if (f.FileExists()) {
				return;
			}

			vfsHandler = new CVFSHandler();
			vfsHandler->AddArchiveWithDeps(mapName, false);
		}

		~ScopedMapLoader()
		{
			if (!autoUnLoadmap)
				return;
			if (vfsHandler != oldHandler) {
				delete vfsHandler;
				vfsHandler = oldHandler;
			}
		}

	private:
		CVFSHandler* oldHandler;
};



EXPORT(const char*) GetSpringVersion()
{
	return GetStr(SpringVersion::GetSync());
}


EXPORT(const char*) GetSpringVersionPatchset()
{
	return GetStr(SpringVersion::GetPatchSet());
}


EXPORT(bool) IsSpringReleaseVersion()
{
	return SpringVersion::IsRelease();
}

class UnitsyncConfigObserver
{
public:
	UnitsyncConfigObserver() {
		configHandler->NotifyOnChange(this);
	}

	~UnitsyncConfigObserver() {
		configHandler->RemoveObserver(this);
	}

	void ConfigNotify(const std::string& key, const std::string& value) {
		if (key == "UnitsyncAutoUnLoadMaps" ) {
			autoUnLoadmap = configHandler->GetBool("UnitsyncAutoUnLoadMaps");
		}
	}
};



static void internal_deleteMapInfos();
static UnitsyncConfigObserver* unitsyncConfigObserver = nullptr;

static void _Cleanup()
{
	SafeDelete(unitsyncConfigObserver);
	internal_deleteMapInfos();

	lpClose();
	LOG("deinitialized");
}


static void CheckForImportantFilesInVFS()
{
	std::vector<std::string> filesToCheck;
	filesToCheck.push_back("base/springcontent.sdz");
	filesToCheck.push_back("base/maphelper.sdz");
	filesToCheck.push_back("base/spring/bitmaps.sdz");
	filesToCheck.push_back("base/cursors.sdz");

	for (std::vector<std::string>::const_iterator it = filesToCheck.begin(); it != filesToCheck.end(); ++it) {
		if (!CFileHandler::FileExists(*it, SPRING_VFS_RAW)) {
			throw content_error("Required base file '" + *it + "' does not exist.");
		}
	}
}


EXPORT(int) Init(bool isServer, int id)
{
	static int numCalls = 0;
	int ret = 0;

	try {
		if (numCalls == 0) {
			// only ever do this once
			spring_clock::PushTickRate(false);
			spring_time::setstarttime(spring_time::gettime(true));
		}

		// Cleanup data from previous Init() calls
		_Cleanup();
		CLogOutput::LogSystemInfo();

#ifndef DEBUG
		log_filter_section_setMinLevel(LOG_SECTION_UNITSYNC, LOG_LEVEL_INFO);
#endif

		if (CheckInit(false)) {
			// reinitialize filesystem to detect new files
			FileSystemInitializer::Cleanup();
		}

		dataDirLocater.UpdateIsolationModeByEnvVar();

		const std::string& configFile = (configHandler != NULL)? configHandler->GetConfigFile(): "";
		const std::string& springFull = SpringVersion::GetFull();

		ThreadPool::SetThreadCount(ThreadPool::GetMaxThreads());
		FileSystemInitializer::PreInitializeConfigHandler(configFile);
		FileSystemInitializer::InitializeLogOutput("unitsync.log");
		FileSystemInitializer::Initialize();
		// check if VFS is okay (throws if not)
		CheckForImportantFilesInVFS();
		ThreadPool::SetThreadCount(0);
		configHandler->Set("UnitsyncAutoUnLoadMaps", true); //reset on each load (backwards compatibility)
		unitsyncConfigObserver = new UnitsyncConfigObserver();
		ret = 1;
		LOG("[UnitSync::%s] initialized %s (call %d)", __FUNCTION__, springFull.c_str(), numCalls);
	}

	UNITSYNC_CATCH_BLOCKS;

	numCalls++;
	return ret;
}

EXPORT(void) UnInit()
{
	try {
		_Cleanup();
		FileSystemInitializer::Cleanup();
		ConfigHandler::Deallocate();
		DataDirLocater::FreeInstance();
	}
	UNITSYNC_CATCH_BLOCKS;
}


EXPORT(const char*) GetWritableDataDirectory()
{
	try {
		CheckInit();
		return GetStr(dataDirLocater.GetWriteDirPath());
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}

EXPORT(int) GetDataDirectoryCount()
{
	int count = -1;

	try {
		CheckInit();
		count = (int) dataDirLocater.GetDataDirs().size();
	}
	UNITSYNC_CATCH_BLOCKS;

	return count;
}

EXPORT(const char*) GetDataDirectory(int index)
{
	try {
		CheckInit();
		const std::vector<std::string> datadirs = dataDirLocater.GetDataDirPaths();
		if (index > datadirs.size()) {
			return NULL;
		}
		return GetStr(datadirs[index]);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}

EXPORT(int) ProcessUnits()
{
	try {
		CheckInit();
		LOG_L(L_DEBUG, "[%s] loaded=%d", __FUNCTION__, unitDefs.empty());
		LoadGameDataUnitDefs();
	}
	UNITSYNC_CATCH_BLOCKS;

	return 0;
}

EXPORT(int) GetUnitCount()
{
	int count = -1;

	try {
		CheckInit();
		LOG_L(L_DEBUG, "[%s]", __FUNCTION__);
		count = unitDefs.size();
	}
	UNITSYNC_CATCH_BLOCKS;

	return count;
}


EXPORT(const char*) GetUnitName(int unitDefID)
{
	try {
		CheckInit();
		LOG_L(L_DEBUG, "[%s(UnitDefID=%d)]", __FUNCTION__, unitDefID);
		return GetStr(unitDefs[unitDefID].name);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


EXPORT(const char*) GetFullUnitName(int unitDefID)
{
	try {
		CheckInit();
		LOG_L(L_DEBUG, "[%s(UnitDefID=%d)]", __FUNCTION__, unitDefID);
		return GetStr(unitDefs[unitDefID].fullName);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}

//////////////////////////
//////////////////////////

EXPORT(void) AddArchive(const char* archiveName)
{
	try {
		CheckInit();
		CheckNullOrEmpty(archiveName);

		LOG_L(L_DEBUG, "adding archive: %s", archiveName);
		vfsHandler->AddArchive(archiveScanner->NameFromArchive(archiveName), false);
	}
	UNITSYNC_CATCH_BLOCKS;
}


EXPORT(void) AddAllArchives(const char* rootArchiveName)
{
	try {
		CheckInit();
		CheckNullOrEmpty(rootArchiveName);
		vfsHandler->AddArchiveWithDeps(rootArchiveName, false);
	}
	UNITSYNC_CATCH_BLOCKS;
}

EXPORT(void) RemoveAllArchives()
{
	try {
		CheckInit();

		LOG_L(L_DEBUG, "removing all archives");
		SafeDelete(vfsHandler);
		vfsHandler = new CVFSHandler();
	}
	UNITSYNC_CATCH_BLOCKS;
}

EXPORT(unsigned int) GetArchiveChecksum(const char* archiveName)
{
	try {
		CheckInit();
		CheckNullOrEmpty(archiveName);

		LOG_L(L_DEBUG, "archive checksum: %s", archiveName);
		return archiveScanner->GetSingleArchiveChecksum(archiveName);
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}

EXPORT(const char*) GetArchivePath(const char* archiveName)
{
	try {
		CheckInit();
		CheckNullOrEmpty(archiveName);

		LOG_L(L_DEBUG, "archive path: %s", archiveName);
		return GetStr(archiveScanner->GetArchivePath(archiveName));
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


static void safe_strzcpy(char* dst, std::string src, size_t max)
{
	STRCPY_T(dst, max, src.c_str());
}


/**
 * @brief map related meta-data
 */
struct InternalMapInfo
{
	std::string description;  ///< Description (max 255 chars)
	std::string author;       ///< Creator of the map (max 200 chars)
	int tidalStrength;        ///< Tidal strength
	int gravity;              ///< Gravity
	float maxMetal;           ///< Metal scale factor
	int extractorRadius;      ///< Extractor radius (of metal extractors)
	int minWind;              ///< Minimum wind speed
	int maxWind;              ///< Maximum wind speed
	int width;                ///< Width of the map
	int height;               ///< Height of the map
	std::vector<float> xPos;  ///< Start positions X coordinates defined by the map
	std::vector<float> zPos;  ///< Start positions Z coordinates defined by the map
};

static bool internal_GetMapInfo(const char* mapName, InternalMapInfo* outInfo)
{
	CheckInit();
	CheckNullOrEmpty(mapName);
	CheckNull(outInfo);

	LOG_L(L_DEBUG, "get map info: %s", mapName);

	const std::string mapFile = GetMapFile(mapName);

	ScopedMapLoader mapLoader(mapName, mapFile);

	std::string err("");

	MapParser mapParser(mapFile);
	if (!mapParser.IsValid()) {
		err = mapParser.GetErrorLog();
	}
	const LuaTable mapTable = mapParser.GetRoot();

	// Retrieve the map header as well
	if (err.empty()) {
		const std::string extension = FileSystem::GetExtension(mapFile);
		if (extension == "smf") {
			try {
				const CSMFMapFile file(mapFile);
				const SMFHeader& mh = file.GetHeader();

				outInfo->width  = mh.mapx * SQUARE_SIZE;
				outInfo->height = mh.mapy * SQUARE_SIZE;
			}
			catch (content_error&) {
				outInfo->width  = -1;
			}
		} else {
			const int w = mapTable.GetInt("gameAreaW", 0);
			const int h = mapTable.GetInt("gameAreaW", 1);

			outInfo->width  = w * SQUARE_SIZE;
			outInfo->height = h * SQUARE_SIZE;
		}

		// Make sure we found stuff in both the smd and the header
		if (outInfo->width <= 0) {
			err = "Bad map width";
		} else if (outInfo->height <= 0) {
			err = "Bad map height";
		}
	}

	// If the map did not parse, say so now
	if (!err.empty()) {
		SetLastError(err);
		outInfo->description = err;
		return false;
	}

	outInfo->description = mapTable.GetString("description", "");

	outInfo->tidalStrength   = mapTable.GetInt("tidalstrength", 0);
	outInfo->gravity         = mapTable.GetInt("gravity", 0);
	outInfo->extractorRadius = mapTable.GetInt("extractorradius", 0);
	outInfo->maxMetal        = mapTable.GetFloat("maxmetal", 0.0f);

	outInfo->author = mapTable.GetString("author", "");

	const LuaTable atmoTable = mapTable.SubTable("atmosphere");
	outInfo->minWind = atmoTable.GetInt("minWind", 0);
	outInfo->maxWind = atmoTable.GetInt("maxWind", 0);

	// Find as many start positions as there are defined by the map
	for (size_t curTeam = 0; true; ++curTeam) {
		float3 pos(-1.0f, -1.0f, -1.0f); // defaults
		if (!mapParser.GetStartPos(curTeam, pos)) {
			break; // position could not be parsed
		}
		outInfo->xPos.push_back(pos.x);
		outInfo->zPos.push_back(pos.z);
		LOG_L(L_DEBUG, "startpos: %.0f, %.0f", pos.x, pos.z);
	}

	return true;
}


// Updated on every call to GetMapCount
static std::vector<std::string> mapNames;

EXPORT(int) GetMapCount()
{
	int count = -1;

	try {
		CheckInit();

		const std::vector<std::string> scannedNames = archiveScanner->GetMaps();

		mapNames.clear();
		mapNames.insert(mapNames.begin(), scannedNames.begin(), scannedNames.end());

		sort(mapNames.begin(), mapNames.end());

		count = mapNames.size();
	}
	UNITSYNC_CATCH_BLOCKS;

	return count;
}

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


EXPORT(const char*) GetMapFileName(int index)
{
	try {
		CheckInit();
		CheckBounds(index, mapNames.size());

		return GetStr(archiveScanner->MapNameToMapFile(mapNames[index]));
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


static std::map<int, InternalMapInfo> mapInfos;

static InternalMapInfo* internal_getMapInfo(int index) {

	if (index >= mapNames.size()) {
		SetLastError("invalid map index");
	} else {
		if (mapInfos.find(index) == mapInfos.end()) {
			try {
				InternalMapInfo imi;
				if (internal_GetMapInfo(mapNames[index].c_str(), &imi)) {
					mapInfos[index] = imi;
					return &(mapInfos[index]);
				}
			}
			UNITSYNC_CATCH_BLOCKS;
		} else {
			return &(mapInfos[index]);
		}
	}

	return NULL;
}

static void internal_deleteMapInfos() {

	while (!mapInfos.empty()) {
		std::map<int, InternalMapInfo>::iterator mi = mapInfos.begin();
		mapInfos.erase(mi);
	}
}


EXPORT(float) GetMapMinHeight(const char* mapName) {
	try {
		CheckInit();
		const std::string mapFile = GetMapFile(mapName);
		ScopedMapLoader loader(mapName, mapFile);
		CSMFMapFile file(mapFile);
		MapParser parser(mapFile);

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

EXPORT(float) GetMapMaxHeight(const char* mapName) {
	try {
		CheckInit();
		const std::string mapFile = GetMapFile(mapName);
		ScopedMapLoader loader(mapName, mapFile);
		CSMFMapFile file(mapFile);
		MapParser parser(mapFile);

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



static std::vector<std::string> mapArchives;

EXPORT(int) GetMapArchiveCount(const char* mapName)
{
	int count = -1;

	try {
		CheckInit();
		CheckNullOrEmpty(mapName);

		mapArchives = archiveScanner->GetAllArchivesUsedBy(mapName);
		count = mapArchives.size();
	}
	UNITSYNC_CATCH_BLOCKS;

	return count;
}

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


EXPORT(unsigned int) GetMapChecksum(int index)
{
	try {
		CheckInit();
		CheckBounds(index, mapNames.size());

		return archiveScanner->GetArchiveCompleteChecksum(mapNames[index]);
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


EXPORT(unsigned int) GetMapChecksumFromName(const char* mapName)
{
	try {
		CheckInit();

		return archiveScanner->GetArchiveCompleteChecksum(mapName);
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


#define RM  0x0000F800
#define GM  0x000007E0
#define BM  0x0000001F

#define RED_RGB565(x) ((x&RM)>>11)
#define GREEN_RGB565(x) ((x&GM)>>5)
#define BLUE_RGB565(x) (x&BM)
#define PACKRGB(r, g, b) (((r<<11)&RM) | ((g << 5)&GM) | (b&BM) )

// Used to return the image
static unsigned short imgbuf[1024*1024];

static unsigned short* GetMinimapSM3(std::string mapFileName, int mipLevel)
{
	throw content_error("SM3 maps are no longer supported as of Spring 95.0");
	return NULL;

	/*
	MapParser mapParser(mapFileName);
	const std::string minimapFile = mapParser.GetRoot().GetString("minimap", "");

	if (minimapFile.empty()) {
		memset(imgbuf,0,sizeof(imgbuf));
		return imgbuf;
	}

	CBitmap bm;
	if (!bm.Load(minimapFile)) {
		memset(imgbuf,0,sizeof(imgbuf));
		return imgbuf;
	}

	if (1024 >> mipLevel != bm.xsize || 1024 >> mipLevel != bm.ysize)
		bm = bm.CreateRescaled(1024 >> mipLevel, 1024 >> mipLevel);

	unsigned short* dst = (unsigned short*)imgbuf;
	unsigned char* src = bm.mem;
	for (int y=0; y < bm.ysize; y++) {
		for (int x=0; x < bm.xsize; x++) {
			*dst = 0;

			*dst |= ((src[0]>>3) << 11) & RM;
			*dst |= ((src[1]>>2) << 5) & GM;
			*dst |= (src[2]>>3) & BM;

			dst ++;
			src += 4;
		}
	}

	return imgbuf;
	*/
}

static unsigned short* GetMinimapSMF(std::string mapFileName, int mipLevel)
{
	CSMFMapFile in(mapFileName);
	std::vector<uint8_t> buffer;
	const int mipsize = in.ReadMinimap(buffer, mipLevel);

	// Do stuff
	unsigned short* colors = (unsigned short*)((void*)imgbuf);

	unsigned char* temp = &buffer[0];

	const int numblocks = buffer.size()/8;
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

	return colors;
}

EXPORT(unsigned short*) GetMinimap(const char* mapName, int mipLevel)
{
	try {
		CheckInit();
		CheckNullOrEmpty(mapName);

		if (mipLevel < 0 || mipLevel > 8)
			throw std::out_of_range("Miplevel must be between 0 and 8 (inclusive) in GetMinimap.");

		const std::string mapFile = GetMapFile(mapName);
		ScopedMapLoader mapLoader(mapName, mapFile);

		unsigned short* ret = NULL;
		const std::string extension = FileSystem::GetExtension(mapFile);
		if (extension == "smf") {
			ret = GetMinimapSMF(mapFile, mipLevel);
		} else if (extension == "sm3") {
			ret = GetMinimapSM3(mapFile, mipLevel);
		}

		return ret;
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


EXPORT(int) GetInfoMapSize(const char* mapName, const char* name, int* width, int* height)
{
	try {
		CheckInit();
		CheckNullOrEmpty(mapName);
		CheckNullOrEmpty(name);
		CheckNull(width);
		CheckNull(height);

		const std::string mapFile = GetMapFile(mapName);
		ScopedMapLoader mapLoader(mapName, mapFile);
		CSMFMapFile file(mapFile);
		MapBitmapInfo bmInfo;

		file.GetInfoMapSize(name, &bmInfo);

		*width = bmInfo.width;
		*height = bmInfo.height;

		return bmInfo.width * bmInfo.height;
	}
	UNITSYNC_CATCH_BLOCKS;

	if (width)  *width  = 0;
	if (height) *height = 0;

	return -1;
}


EXPORT(int) GetInfoMap(const char* mapName, const char* name, unsigned char* data, int typeHint)
{
	int ret = -1;

	try {
		CheckInit();
		CheckNullOrEmpty(mapName);
		CheckNullOrEmpty(name);
		CheckNull(data);

		const std::string mapFile = GetMapFile(mapName);
		ScopedMapLoader mapLoader(mapName, mapFile);
		CSMFMapFile file(mapFile);

		const std::string n = name;
		int actualType = (n == "height" ? bm_grayscale_16 : bm_grayscale_8);

		if (actualType == typeHint) {
			ret = file.ReadInfoMap(n, data);
		} else if (actualType == bm_grayscale_16 && typeHint == bm_grayscale_8) {
			// convert from 16 bits per pixel to 8 bits per pixel
			MapBitmapInfo bmInfo;
			file.GetInfoMapSize(name, &bmInfo);

			const int size = bmInfo.width * bmInfo.height;
			if (size > 0) {
				unsigned short* temp = new unsigned short[size];
				if (file.ReadInfoMap(n, temp)) {
					const unsigned short* inp = temp;
					const unsigned short* inp_end = temp + size;
					unsigned char* outp = data;
					for (; inp < inp_end; ++inp, ++outp) {
						*outp = *inp >> 8;
					}
					ret = 1;
				}
				delete[] temp;
			}
		} else if (actualType == bm_grayscale_8 && typeHint == bm_grayscale_16) {
			throw content_error("converting from 8 bits per pixel to 16 bits per pixel is unsupported");
		}
	}
	UNITSYNC_CATCH_BLOCKS;

	return ret;
}


//////////////////////////
//////////////////////////

std::vector<CArchiveScanner::ArchiveData> modData;

EXPORT(int) GetPrimaryModCount()
{
	int count = -1;

	try {
		CheckInit();

		modData = archiveScanner->GetPrimaryMods();
		count = modData.size();
	}
	UNITSYNC_CATCH_BLOCKS;

	return count;
}

EXPORT(int) GetPrimaryModInfoCount(int modIndex) {

	try {
		CheckInit();
		CheckBounds(modIndex, modData.size());

		info.clear();

		std::vector<InfoItem> modInfoItems = modData[modIndex].GetInfoItems();
		info.insert(info.end(), modInfoItems.begin(), modInfoItems.end());

		return (int)info.size();
	}
	UNITSYNC_CATCH_BLOCKS;

	info.clear();

	return -1;
}

EXPORT(const char*) GetPrimaryModArchive(int index)
{
	try {
		CheckInit();
		CheckBounds(index, modData.size());

		const std::string& x = modData[index].GetDependencies()[0];
		return GetStr(x);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


std::vector<std::string> primaryArchives;

EXPORT(int) GetPrimaryModArchiveCount(int index)
{
	int count = -1;

	try {
		CheckInit();
		CheckBounds(index, modData.size());

		primaryArchives = archiveScanner->GetAllArchivesUsedBy(modData[index].GetDependencies()[0]);
		count = primaryArchives.size();
	}
	UNITSYNC_CATCH_BLOCKS;

	return count;
}

EXPORT(const char*) GetPrimaryModArchiveList(int archiveNr)
{
	try {
		CheckInit();
		CheckBounds(archiveNr, primaryArchives.size());

		LOG_L(L_DEBUG, "primary mod archive list: %s",
				primaryArchives[archiveNr].c_str());
		return GetStr(primaryArchives[archiveNr]);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}

EXPORT(int) GetPrimaryModIndex(const char* name)
{
	try {
		CheckInit();

		const std::string searchedName(name);
		for (unsigned i = 0; i < modData.size(); ++i) {
			if (modData[i].GetNameVersioned() == searchedName)
				return i;
		}
	}
	UNITSYNC_CATCH_BLOCKS;

	return -1;
}

EXPORT(unsigned int) GetPrimaryModChecksum(int index)
{
	try {
		CheckInit();
		CheckBounds(index, modData.size());

		return archiveScanner->GetArchiveCompleteChecksum(GetPrimaryModArchive(index));
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}

EXPORT(unsigned int) GetPrimaryModChecksumFromName(const char* name)
{
	try {
		CheckInit();

		return archiveScanner->GetArchiveCompleteChecksum(archiveScanner->ArchiveFromName(name));
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


//////////////////////////
//////////////////////////

EXPORT(int) GetSideCount()
{
	int count = -1;

	try {
		CheckInit();

		if (!sideParser.Load()) {
			throw content_error("failed: " + sideParser.GetErrorLog());
		}
		count = sideParser.GetCount();
	}
	UNITSYNC_CATCH_BLOCKS;

	return count;
}

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

static void ParseOptions(const std::string& fileName, const std::string& fileModes, const std::string& accessModes)
{
	option_parseOptions(options, fileName, fileModes, accessModes, &optionsSet);
}


static void ParseMapOptions(const std::string& mapName)
{
	option_parseMapOptions(options, "MapOptions.lua", mapName, SPRING_VFS_MAP,
			SPRING_VFS_MAP, &optionsSet);
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


EXPORT(int) GetMapOptionCount(const char* name)
{
	try {
		CheckInit();
		CheckNullOrEmpty(name);

		const std::string mapFile = GetMapFile(name);
		ScopedMapLoader mapLoader(name, mapFile);

		options.clear();
		optionsSet.clear();

		ParseMapOptions(name);

		optionsSet.clear();

		return options.size();
	}
	UNITSYNC_CATCH_BLOCKS;

	options.clear();
	optionsSet.clear();

	return -1;
}


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

	return -1;
}

EXPORT(int) GetCustomOptionCount(const char* fileName)
{
	try {
		CheckInit();

		options.clear();
		optionsSet.clear();

		try {
			ParseOptions(fileName, SPRING_VFS_ZIP, SPRING_VFS_ZIP);
		}
		UNITSYNC_CATCH_BLOCKS;

		optionsSet.clear();

		return options.size();
	}
	UNITSYNC_CATCH_BLOCKS;

	// Failed to load custom options file
	options.clear();
	optionsSet.clear();

	return -1;
}

//////////////////////////
//////////////////////////

static std::vector< std::vector<InfoItem> > luaAIInfos;

static void GetLuaAIInfo()
{
	luaAIInfos = luaAIImplHandler.LoadInfos();
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
static std::vector<std::string> skirmishAIDataDirs;

EXPORT(int) GetSkirmishAICount() {

	int count = -1;

	try {
		CheckInit();

		skirmishAIDataDirs.clear();

		std::vector<std::string> dataDirs_tmp =
				dataDirsAccess.FindDirsInDirectSubDirs(SKIRMISH_AI_DATA_DIR);

		// filter out dirs not containing an AIInfo.lua file
		std::vector<std::string>::const_iterator i;
		for (i = dataDirs_tmp.begin(); i != dataDirs_tmp.end(); ++i) {
			const std::string& possibleDataDir = *i;
			std::vector<std::string> infoFile = CFileHandler::FindFiles(
					possibleDataDir, "AIInfo.lua");
			if (!infoFile.empty()) {
				skirmishAIDataDirs.push_back(possibleDataDir);
			}
		}

		sort(skirmishAIDataDirs.begin(), skirmishAIDataDirs.end());

		int luaAIs = GetNumberOfLuaAIs();

		//LOG_L(L_DEBUG, "GetSkirmishAICount: luaAIs: %i / skirmishAIs: %u",
		//		luaAIs, skirmishAIDataDirs.size());
		count = skirmishAIDataDirs.size() + luaAIs;
	}
	UNITSYNC_CATCH_BLOCKS;

	return count;
}


static void ParseInfo(const std::string& fileName,
                      const std::string& fileModes,
                      const std::string& accessModes)
{
	info_parseInfo(info, fileName, fileModes, accessModes, &infoSet);
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

EXPORT(int) GetSkirmishAIInfoCount(int aiIndex) {

	try {
		CheckSkirmishAIIndex(aiIndex);

		info.clear();

		if (IsLuaAIIndex(aiIndex)) {
			const std::vector<InfoItem>& iInfo = luaAIInfos[ToPureLuaAIIndex(aiIndex)];
			info.insert(info.end(), iInfo.begin(), iInfo.end());
		} else {
			infoSet.clear();
			ParseInfo(skirmishAIDataDirs[aiIndex] + "/AIInfo.lua",
					SPRING_VFS_RAW, SPRING_VFS_RAW);
			infoSet.clear();
		}

		return (int)info.size();
	}
	UNITSYNC_CATCH_BLOCKS;

	info.clear();

	return -1;
}

EXPORT(int) GetMapInfoCount(int index) {
	try{
		info.clear();
		CheckBounds(index, mapNames.size());
		const InternalMapInfo* mapInfo = internal_getMapInfo(index);
		if (mapInfo == nullptr) {
			return -1;
		}
		info.push_back(InfoItem("description", "", mapInfo->description));
		info.push_back(InfoItem("author", "", mapInfo->author));
		info.push_back(InfoItem("tidalStrength", "", mapInfo->tidalStrength));
		info.push_back(InfoItem("gravity", "", mapInfo->gravity));
		info.push_back(InfoItem("maxMetal", "", mapInfo->maxMetal));
		info.push_back(InfoItem("extractorRadius", "", mapInfo->extractorRadius));
		info.push_back(InfoItem("minWind", "", mapInfo->minWind));
		info.push_back(InfoItem("maxWind", "", mapInfo->maxWind));
		info.push_back(InfoItem("width", "", mapInfo->width));
		info.push_back(InfoItem("height", "", mapInfo->height));
		info.push_back(InfoItem("resource", "", "Metal"));
		for(int i = 0; i < mapInfo->xPos.size() && i < mapInfo->zPos.size(); i++) {
			info.push_back(InfoItem("xPos", "", mapInfo->xPos[i]));
			info.push_back(InfoItem("zPos", "", mapInfo->zPos[i]));
		}
		return (int)info.size();
	}
	UNITSYNC_CATCH_BLOCKS;

	info.clear();

	return -1;
}

static const InfoItem* GetInfoItem(int infoIndex) {

	CheckInit();
	CheckBounds(infoIndex, info.size());
	return &(info[infoIndex]);
}

static void CheckInfoValueType(const InfoItem* infoItem, InfoValueType requiredValueType) {

	if (infoItem->valueType != requiredValueType) {
		throw std::invalid_argument(
				std::string("Tried to fetch info-value of type ")
				+ info_convertTypeToString(infoItem->valueType)
				+ " as " + info_convertTypeToString(requiredValueType) + ".");
	}
}

EXPORT(const char*) GetInfoKey(int infoIndex) {

	const char* key = NULL;

	try {
		key = GetStr(GetInfoItem(infoIndex)->key);
	}
	UNITSYNC_CATCH_BLOCKS;

	return key;
}
EXPORT(const char*) GetInfoType(int infoIndex) {

	const char* type = NULL;

	try {
		type = info_convertTypeToString(GetInfoItem(infoIndex)->valueType);
	}
	UNITSYNC_CATCH_BLOCKS;

	return type;
}

EXPORT(const char*) GetInfoValueString(int infoIndex) {

	const char* value = NULL;

	try {
		const InfoItem* infoItem = GetInfoItem(infoIndex);
		CheckInfoValueType(infoItem, INFO_VALUE_TYPE_STRING);
		value = GetStr(infoItem->valueTypeString);
	}
	UNITSYNC_CATCH_BLOCKS;

	return value;
}
EXPORT(int) GetInfoValueInteger(int infoIndex) {

	int value = -1;

	try {
		const InfoItem* infoItem = GetInfoItem(infoIndex);
		CheckInfoValueType(infoItem, INFO_VALUE_TYPE_INTEGER);
		value = infoItem->value.typeInteger;
	}
	UNITSYNC_CATCH_BLOCKS;

	return value;
}
EXPORT(float) GetInfoValueFloat(int infoIndex) {

	float value = -1.0f;

	try {
		const InfoItem* infoItem = GetInfoItem(infoIndex);
		CheckInfoValueType(infoItem, INFO_VALUE_TYPE_FLOAT);
		value = infoItem->value.typeFloat;
	}
	UNITSYNC_CATCH_BLOCKS;

	return value;
}
EXPORT(bool) GetInfoValueBool(int infoIndex) {

	bool value = false;

	try {
		const InfoItem* infoItem = GetInfoItem(infoIndex);
		CheckInfoValueType(infoItem, INFO_VALUE_TYPE_BOOL);
		value = infoItem->value.typeBool;
	}
	UNITSYNC_CATCH_BLOCKS;

	return value;
}
EXPORT(const char*) GetInfoDescription(int infoIndex) {

	const char* desc = NULL;

	try {
		desc = GetStr(GetInfoItem(infoIndex)->desc);
	}
	UNITSYNC_CATCH_BLOCKS;

	return desc;
}

EXPORT(int) GetSkirmishAIOptionCount(int aiIndex) {

	try {
		CheckSkirmishAIIndex(aiIndex);

		options.clear();
		optionsSet.clear();

		if (IsLuaAIIndex(aiIndex)) {
			// lua AIs do not have options
			return 0;
		} else {
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

	return -1;
}


// Common Options Parameters

EXPORT(const char*) GetOptionKey(int optIndex)
{
	try {
		CheckOptionIndex(optIndex);
		return GetStr(options[optIndex].key);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}

EXPORT(const char*) GetOptionScope(int optIndex)
{
	try {
		CheckOptionIndex(optIndex);
		return GetStr(options[optIndex].scope);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}

EXPORT(const char*) GetOptionName(int optIndex)
{
	try {
		CheckOptionIndex(optIndex);
		return GetStr(options[optIndex].name);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}

EXPORT(const char*) GetOptionSection(int optIndex)
{
	try {
		CheckOptionIndex(optIndex);
		return GetStr(options[optIndex].section);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}

EXPORT(const char*) GetOptionDesc(int optIndex)
{
	try {
		CheckOptionIndex(optIndex);
		return GetStr(options[optIndex].desc);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}

EXPORT(int) GetOptionType(int optIndex)
{
	int type = -1;

	try {
		CheckOptionIndex(optIndex);
		type = options[optIndex].typeCode;
	}
	UNITSYNC_CATCH_BLOCKS;

	return type;
}


// Bool Options

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

EXPORT(float) GetOptionNumberDef(int optIndex)
{
	float numDef = -1.0f;

	try {
		CheckOptionType(optIndex, opt_number);
		numDef = options[optIndex].numberDef;
	}
	UNITSYNC_CATCH_BLOCKS;

	return numDef;
}

EXPORT(float) GetOptionNumberMin(int optIndex)
{
	float numMin = -1.0e30f; // FIXME error return should be -1.0f, or use FLOAT_MIN ?

	try {
		CheckOptionType(optIndex, opt_number);
		numMin = options[optIndex].numberMin;
	}
	UNITSYNC_CATCH_BLOCKS;

	return numMin;
}

EXPORT(float) GetOptionNumberMax(int optIndex)
{
	float numMax = +1.0e30f; // FIXME error return should be -1.0f, or use FLOAT_MAX ?

	try {
		CheckOptionType(optIndex, opt_number);
		numMax = options[optIndex].numberMax;
	}
	UNITSYNC_CATCH_BLOCKS;

	return numMax;
}

EXPORT(float) GetOptionNumberStep(int optIndex)
{
	float numStep = -1.0f;

	try {
		CheckOptionType(optIndex, opt_number);
		numStep = options[optIndex].numberStep;
	}
	UNITSYNC_CATCH_BLOCKS;

	return numStep;
}


// String Options

EXPORT(const char*) GetOptionStringDef(int optIndex)
{
	try {
		CheckOptionType(optIndex, opt_string);
		return GetStr(options[optIndex].stringDef);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}

EXPORT(int) GetOptionStringMaxLen(int optIndex)
{
	int count = -1;

	try {
		CheckOptionType(optIndex, opt_string);
		count = options[optIndex].stringMaxLen;
	}
	UNITSYNC_CATCH_BLOCKS;

	return count;
}


// List Options

EXPORT(int) GetOptionListCount(int optIndex)
{
	int count = -1;

	try {
		CheckOptionType(optIndex, opt_list);
		count = options[optIndex].list.size();
	}
	UNITSYNC_CATCH_BLOCKS;

	return count;
}

EXPORT(const char*) GetOptionListDef(int optIndex)
{
	try {
		CheckOptionType(optIndex, opt_list);
		return GetStr(options[optIndex].listDef);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}

EXPORT(const char*) GetOptionListItemKey(int optIndex, int itemIndex)
{
	try {
		CheckOptionType(optIndex, opt_list);
		const std::vector<OptionListItem>& list = options[optIndex].list;
		CheckBounds(itemIndex, list.size());
		return GetStr(list[itemIndex].key);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}

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

EXPORT(const char*) GetOptionListItemDesc(int optIndex, int itemIndex)
{
	try {
		CheckOptionType(optIndex, opt_list);
		const std::vector<OptionListItem>& list = options[optIndex].list;
		CheckBounds(itemIndex, list.size());
		return GetStr(list[itemIndex].desc);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


//////////////////////////
//////////////////////////

static std::vector<std::string> modValidMaps;


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
                              const std::string& key, const std::string& value)
{
	lua_pushstring(L, key.c_str());
	lua_pushstring(L, value.c_str());
	lua_rawset(L, -3);
}


static void LuaPushNamedNumber(lua_State* L, const std::string& key, float value)
{
	lua_pushstring(L, key.c_str());
	lua_pushnumber(L, value);
	lua_rawset(L, -3);
}


static int LuaGetMapInfo(lua_State* L)
{
	const std::string mapName = luaL_checkstring(L, 1);

	InternalMapInfo mi;
	if (!internal_GetMapInfo(mapName.c_str(), &mi)) {
		LOG_L(L_ERROR, "LuaGetMapInfo: internal_GetMapInfo(\"%s\") failed",
				mapName.c_str());
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
	for (size_t p = 0; p < mi.xPos.size(); p++) {
		lua_pushnumber(L, p + 1);
		lua_newtable(L);
		LuaPushNamedNumber(L, "x", mi.xPos[p]);
		LuaPushNamedNumber(L, "z", mi.zPos[p]);
		lua_rawset(L, -3);
	}
	lua_rawset(L, -3);

	return 1;
}


EXPORT(int) GetModValidMapCount()
{
	int count = -1;

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
			const std::string map = root.GetString(index, "");
			if (!map.empty()) {
				modValidMaps.push_back(map);
			}
		}

		count = modValidMaps.size();
	}
	UNITSYNC_CATCH_BLOCKS;

	return count;
}

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

static std::map<int, CFileHandler*> openFiles;
static int nextFile = 0;
static std::vector<std::string> curFindFiles;

static void CheckFileHandle(int file)
{
	CheckInit();

	if (openFiles.find(file) == openFiles.end())
		throw content_error("Unregistered file handle. Pass a file handle returned by OpenFileVFS.");
}


EXPORT(int) OpenFileVFS(const char* name)
{
	try {
		CheckInit();
		CheckNullOrEmpty(name);

		LOG_L(L_DEBUG, "OpenFileVFS: %s", name);

		CFileHandler* fh = new CFileHandler(name);
		if (!fh->FileExists()) {
			delete fh;
			throw content_error("File '" + std::string(name) + "' does not exist");
		}

		nextFile++;
		openFiles[nextFile] = fh;

		return nextFile;
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}

EXPORT(void) CloseFileVFS(int file)
{
	try {
		CheckFileHandle(file);

		LOG_L(L_DEBUG, "CloseFileVFS: %d", file);
		delete openFiles[file];
		openFiles.erase(file);
	}
	UNITSYNC_CATCH_BLOCKS;
}

EXPORT(int) ReadFileVFS(int file, unsigned char* buf, int numBytes)
{
	try {
		CheckFileHandle(file);
		CheckNull(buf);
		CheckPositive(numBytes);

		LOG_L(L_DEBUG, "ReadFileVFS: %d", file);
		CFileHandler* fh = openFiles[file];
		return fh->Read(buf, numBytes);
	}
	UNITSYNC_CATCH_BLOCKS;
	return -1;
}

EXPORT(int) FileSizeVFS(int file)
{
	try {
		CheckFileHandle(file);

		LOG_L(L_DEBUG, "FileSizeVFS: %d", file);
		CFileHandler* fh = openFiles[file];
		return fh->FileSize();
	}
	UNITSYNC_CATCH_BLOCKS;
	return -1;
}

EXPORT(int) InitFindVFS(const char* pattern)
{
	try {
		CheckInit();
		CheckNullOrEmpty(pattern);

		std::string path = FileSystem::GetDirectory(pattern);
		std::string patt = FileSystem::GetFilename(pattern);
		LOG_L(L_DEBUG, "InitFindVFS: %s", pattern);
		curFindFiles = CFileHandler::FindFiles(path, patt);
		return 0;
	}
	UNITSYNC_CATCH_BLOCKS;
	return -1;
}

EXPORT(int) InitDirListVFS(const char* path, const char* pattern, const char* modes)
{
	try {
		CheckInit();

		if (path    == NULL) { path = "";              }
		if (modes   == NULL) { modes = SPRING_VFS_ALL; }
		if (pattern == NULL) { pattern = "*";          }
		LOG_L(L_DEBUG, "InitDirListVFS: '%s' '%s' '%s'", path, pattern, modes);
		curFindFiles = CFileHandler::DirList(path, pattern, modes);
		return 0;
	}
	UNITSYNC_CATCH_BLOCKS;
	return -1;
}

EXPORT(int) InitSubDirsVFS(const char* path, const char* pattern, const char* modes)
{
	try {
		CheckInit();
		if (path    == NULL) { path = "";              }
		if (modes   == NULL) { modes = SPRING_VFS_ALL; }
		if (pattern == NULL) { pattern = "*";          }
		LOG_L(L_DEBUG, "InitSubDirsVFS: '%s' '%s' '%s'", path, pattern, modes);
		curFindFiles = CFileHandler::SubDirs(path, pattern, modes);
		return 0;
	}
	UNITSYNC_CATCH_BLOCKS;
	return -1;
}

EXPORT(int) FindFilesVFS(int file, char* nameBuf, int size)
{
	try {
		CheckInit();
		CheckNull(nameBuf);
		CheckPositive(size);

		LOG_L(L_DEBUG, "FindFilesVFS: %d", file);
		if ((unsigned)file >= curFindFiles.size()) {
			return 0;
		}
		safe_strzcpy(nameBuf, curFindFiles[file], size);
		return file + 1;
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


//////////////////////////
//////////////////////////

static std::map<int, IArchive*> openArchives;
static int nextArchive = 0;


static void CheckArchiveHandle(int archive)
{
	CheckInit();

	if (openArchives.find(archive) == openArchives.end()) {
		throw content_error("Unregistered archive handle. Pass an archive handle returned by OpenArchive.");
	}
}


EXPORT(int) OpenArchive(const char* name)
{
	try {
		CheckInit();
		CheckNullOrEmpty(name);

		IArchive* a = archiveLoader.OpenArchive(name);

		if (!a) {
			throw content_error("Archive '" + std::string(name) + "' could not be opened");
		}

		nextArchive++;
		openArchives[nextArchive] = a;
		return nextArchive;
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}

EXPORT(void) CloseArchive(int archive)
{
	try {
		CheckArchiveHandle(archive);

		delete openArchives[archive];
		openArchives.erase(archive);
	}
	UNITSYNC_CATCH_BLOCKS;
}

EXPORT(int) FindFilesArchive(int archive, int file, char* nameBuf, int* size)
{
	try {
		CheckArchiveHandle(archive);
		CheckNull(nameBuf);
		CheckNull(size);

		IArchive* arch = openArchives[archive];

		LOG_L(L_DEBUG, "FindFilesArchive: %d", archive);

		if (file < arch->NumFiles())
		{
			const int nameBufSize = *size;
			std::string fileName;
			int fileSize;
			arch->FileInfo(file, fileName, fileSize);
			*size = fileSize;
			if (nameBufSize > fileName.length())
			{
				STRCPY(nameBuf, fileName.c_str());
				return ++file;
			}
			else
			{
				SetLastError("name-buffer is too small");
			}
		}
		return 0;
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}

EXPORT(int) OpenArchiveFile(int archive, const char* name)
{
	int fileID = -1;

	try {
		CheckArchiveHandle(archive);
		CheckNullOrEmpty(name);

		IArchive* arch = openArchives[archive];
		fileID = arch->FindFile(name);
		if (fileID == arch->NumFiles()) {
			fileID = -2;
		}
	}
	UNITSYNC_CATCH_BLOCKS;

	return fileID;
}

EXPORT(int) ReadArchiveFile(int archive, int file, unsigned char* buffer, int numBytes)
{
	try {
		CheckArchiveHandle(archive);
		CheckNull(buffer);
		CheckPositive(numBytes);

		IArchive* a = openArchives[archive];
		std::vector<uint8_t> buf;
		if (!a->GetFile(file, buf))
			return -1;
		std::memcpy(buffer, &buf[0], std::min(buf.size(), (size_t)numBytes));
		return std::min(buf.size(), (size_t)numBytes);
	}
	UNITSYNC_CATCH_BLOCKS;
	return -1;
}

EXPORT(void) CloseArchiveFile(int archive, int file)
{
	try {
		// nuting
	}
	UNITSYNC_CATCH_BLOCKS;
}

EXPORT(int) SizeArchiveFile(int archive, int file)
{
	try {
		CheckArchiveHandle(archive);

		IArchive* a = openArchives[archive];
		std::string name;
		int s;
		a->FileInfo(file, name, s);
		return s;
	}
	UNITSYNC_CATCH_BLOCKS;
	return -1;
}


//////////////////////////
//////////////////////////

char strBuf[STRBUF_SIZE];

/// defined in unitsync.h. Just returning str.c_str() does not work
const char* GetStr(std::string str)
{
	if (str.length() + 1 > STRBUF_SIZE) {
		sprintf(strBuf, "Increase STRBUF_SIZE (needs " _STPF_ " bytes)", str.length() + 1);
	} else {
		STRCPY(strBuf, str.c_str());
	}

	return strBuf;
}


//////////////////////////
//////////////////////////

EXPORT(void) SetSpringConfigFile(const char* fileNameAsAbsolutePath)
{
	dataDirLocater.UpdateIsolationModeByEnvVar();
	FileSystemInitializer::PreInitializeConfigHandler(fileNameAsAbsolutePath);
}

static void CheckConfigHandler()
{
	if (!configHandler)
		throw std::logic_error("Unitsync config handler not initialized, check config source.");
}


EXPORT(const char*) GetSpringConfigFile()
{
	try {
		CheckConfigHandler();
		return GetStr(configHandler->GetConfigFile());
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


EXPORT(const char*) GetSpringConfigString(const char* name, const char* defValue)
{
	try {
		CheckConfigHandler();
		std::string res = configHandler->IsSet(name) ? configHandler->GetString(name) : defValue;
		return GetStr(res);
	}
	UNITSYNC_CATCH_BLOCKS;
	return defValue;
}

EXPORT(int) GetSpringConfigInt(const char* name, const int defValue)
{
	try {
		CheckConfigHandler();
		return configHandler->IsSet(name) ? configHandler->GetInt(name) : defValue;
	}
	UNITSYNC_CATCH_BLOCKS;
	return defValue;
}

EXPORT(float) GetSpringConfigFloat(const char* name, const float defValue)
{
	try {
		CheckConfigHandler();
		return configHandler->IsSet(name) ? configHandler->GetFloat(name) : defValue;
	}
	UNITSYNC_CATCH_BLOCKS;
	return defValue;
}

EXPORT(void) SetSpringConfigString(const char* name, const char* value)
{
	try {
		CheckConfigHandler();
		configHandler->SetString( name, value );
	}
	UNITSYNC_CATCH_BLOCKS;
}

EXPORT(void) SetSpringConfigInt(const char* name, const int value)
{
	try {
		CheckConfigHandler();
		configHandler->Set(name, value);
	}
	UNITSYNC_CATCH_BLOCKS;
}

EXPORT(void) SetSpringConfigFloat(const char* name, const float value)
{
	try {
		CheckConfigHandler();
		configHandler->Set(name, value);
	}
	UNITSYNC_CATCH_BLOCKS;
}

EXPORT(void) DeleteSpringConfigKey(const char* name)
{
	try {
		CheckConfigHandler();
		configHandler->Delete(name);
	}
	UNITSYNC_CATCH_BLOCKS;
}

#ifdef ENABLE_DEPRECATED_FUNCTIONS
/*
**********************DEPRECATED SECTION
*/

#define DEPRECATED \
	static CMessageOnce msg( \
			"The deprecated unitsync function " \
			+ std::string(__FUNCTION__) + " was called." \
			" Please update your lobby client"); \
	msg.print(); \
	SetLastError("deprecated unitsync function called: " \
			+ std::string(__FUNCTION__))



static bool _GetMapInfoEx(const char* mapName, MapInfo* outInfo, int version)
{
	CheckInit();
	CheckNullOrEmpty(mapName);
	CheckNull(outInfo);

	bool fetchOk;

	InternalMapInfo internalMapInfo;
	fetchOk = internal_GetMapInfo(mapName, &internalMapInfo);

	if (fetchOk) {
		safe_strzcpy(outInfo->description, internalMapInfo.description, 255);
		outInfo->tidalStrength   = internalMapInfo.tidalStrength;
		outInfo->gravity         = internalMapInfo.gravity;
		outInfo->maxMetal        = internalMapInfo.maxMetal;
		outInfo->extractorRadius = internalMapInfo.extractorRadius;
		outInfo->minWind         = internalMapInfo.minWind;
		outInfo->maxWind         = internalMapInfo.maxWind;

		outInfo->width           = internalMapInfo.width;
		outInfo->height          = internalMapInfo.height;
		outInfo->posCount        = internalMapInfo.xPos.size();
		if (outInfo->posCount > 16) {
			// legacy interface does not support more then 16
			outInfo->posCount = 16;
		}
		for (size_t curTeam = 0; curTeam < outInfo->posCount; ++curTeam) {
			outInfo->positions[curTeam].x = internalMapInfo.xPos[curTeam];
			outInfo->positions[curTeam].z = internalMapInfo.zPos[curTeam];
		}

		if (version >= 1) {
			safe_strzcpy(outInfo->author, internalMapInfo.author, 200);
		}
	} else {
		// contains the error message
		safe_strzcpy(outInfo->description, internalMapInfo.description, 255);

		// Fill in stuff so TASClient does not crash
		outInfo->posCount = 0;
		if (version >= 1) {
			outInfo->author[0] = '\0';
		}
		return false;
	}

	return fetchOk;
}

EXPORT(int) ProcessUnitsNoChecksum()
{
	DEPRECATED;
	return ProcessUnits();
}

EXPORT(int) GetMapInfoEx(const char* mapName, MapInfo* outInfo, int version)
{
	DEPRECATED;
	int ret = 0;

	try {
		const bool fetchOk = _GetMapInfoEx(mapName, outInfo, version);
		ret = fetchOk ? 1 : 0;
	}
	UNITSYNC_CATCH_BLOCKS;

	return ret;
}

EXPORT(int) GetMapInfo(const char* mapName, MapInfo* outInfo)
{
	DEPRECATED;
	int ret = 0;

	try {
		const bool fetchOk = _GetMapInfoEx(mapName, outInfo, 0);
		ret = fetchOk ? 1 : 0;
	}
	UNITSYNC_CATCH_BLOCKS;

	return ret;
}

EXPORT(const char*) GetMapDescription(int index) {
	DEPRECATED;
	const InternalMapInfo* mapInfo = internal_getMapInfo(index);
	if (mapInfo) {
		return mapInfo->description.c_str();
	}

	return NULL;
}

EXPORT(const char*) GetMapAuthor(int index) {
	DEPRECATED;
	const InternalMapInfo* mapInfo = internal_getMapInfo(index);
	if (mapInfo) {
		return mapInfo->author.c_str();
	}

	return NULL;
}

EXPORT(int) GetMapWidth(int index) {
	DEPRECATED;
	const InternalMapInfo* mapInfo = internal_getMapInfo(index);
	if (mapInfo) {
		return mapInfo->width;
	}

	return -1;
}

EXPORT(int) GetMapHeight(int index) {
	DEPRECATED;
	const InternalMapInfo* mapInfo = internal_getMapInfo(index);
	if (mapInfo) {
		return mapInfo->height;
	}

	return -1;
}

EXPORT(int) GetMapTidalStrength(int index) {
	DEPRECATED;
	const InternalMapInfo* mapInfo = internal_getMapInfo(index);
	if (mapInfo) {
		return mapInfo->tidalStrength;
	}

	return -1;
}

EXPORT(int) GetMapWindMin(int index) {
	DEPRECATED;
	const InternalMapInfo* mapInfo = internal_getMapInfo(index);
	if (mapInfo) {
		return mapInfo->minWind;
	}

	return -1;
}

EXPORT(int) GetMapWindMax(int index) {
	DEPRECATED;
	const InternalMapInfo* mapInfo = internal_getMapInfo(index);
	if (mapInfo) {
		return mapInfo->maxWind;
	}

	return -1;
}

EXPORT(int) GetMapGravity(int index) {
	DEPRECATED;
	const InternalMapInfo* mapInfo = internal_getMapInfo(index);
	if (mapInfo) {
		return mapInfo->gravity;
	}

	return -1;
}

EXPORT(int) GetMapResourceCount(int index) {
	DEPRECATED;
	return 1;
}

EXPORT(const char*) GetMapResourceName(int index, int resourceIndex) {
	DEPRECATED;
	if (resourceIndex == 0) {
		return "Metal";
	} else {
		SetLastError("No valid map resource index");
	}

	return NULL;
}

EXPORT(float) GetMapResourceMax(int index, int resourceIndex) {
	DEPRECATED;
	if (resourceIndex == 0) {
		const InternalMapInfo* mapInfo = internal_getMapInfo(index);
		if (mapInfo) {
			return mapInfo->maxMetal;
		}
	} else {
		SetLastError("No valid map resource index");
	}

	return 0.0f;
}

EXPORT(int) GetMapResourceExtractorRadius(int index, int resourceIndex) {
	DEPRECATED;
	if (resourceIndex == 0) {
		const InternalMapInfo* mapInfo = internal_getMapInfo(index);
		if (mapInfo) {
			return mapInfo->extractorRadius;
		}
	} else {
		SetLastError("No valid map resource index");
	}

	return -1;
}


EXPORT(int) GetMapPosCount(int index) {
	DEPRECATED;
	int count = -1;

	const InternalMapInfo* mapInfo = internal_getMapInfo(index);
	if (mapInfo) {
		count = mapInfo->xPos.size();
	}

	return count;
}

EXPORT(float) GetMapPosX(int index, int posIndex) {
	DEPRECATED;
	const InternalMapInfo* mapInfo = internal_getMapInfo(index);
	if (mapInfo) {
		return mapInfo->xPos[posIndex];
	}

	return -1.0f;
}

EXPORT(float) GetMapPosZ(int index, int posIndex) {
	DEPRECATED;
	const InternalMapInfo* mapInfo = internal_getMapInfo(index);
	if (mapInfo) {
		return mapInfo->zPos[posIndex];
	}

	return -1.0f;
}

EXPORT(const char*) GetInfoValue(int infoIndex) {
	DEPRECATED;

	const char* value = NULL;

	try {
		const InfoItem* infoItem = GetInfoItem(infoIndex);
		value = GetStr(infoItem->GetValueAsString());
	}
	UNITSYNC_CATCH_BLOCKS;

	return value;
}

EXPORT(const char*) GetPrimaryModName(int index)
{
	DEPRECATED;
	try {
		CheckInit();
		CheckBounds(index, modData.size());

		const std::string& x = modData[index].GetNameVersioned();
		return GetStr(x);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}

EXPORT(const char*) GetPrimaryModShortName(int index)
{
	DEPRECATED;
	try {
		CheckInit();
		CheckBounds(index, modData.size());

		const std::string& x = modData[index].GetShortName();
		return GetStr(x);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}

EXPORT(const char*) GetPrimaryModVersion(int index)
{
	DEPRECATED;
	try {
		CheckInit();
		CheckBounds(index, modData.size());

		const std::string& x = modData[index].GetVersion();
		return GetStr(x);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}

EXPORT(const char*) GetPrimaryModMutator(int index)
{
	DEPRECATED;
	try {
		CheckInit();
		CheckBounds(index, modData.size());

		const std::string& x = modData[index].GetMutator();
		return GetStr(x);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}

EXPORT(const char*) GetPrimaryModGame(int index)
{
	DEPRECATED;
	try {
		CheckInit();
		CheckBounds(index, modData.size());

		const std::string& x = modData[index].GetGame();
		return GetStr(x);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}

EXPORT(const char*) GetPrimaryModShortGame(int index)
{
	DEPRECATED;
	try {
		CheckInit();
		CheckBounds(index, modData.size());

		const std::string& x = modData[index].GetShortGame();
		return GetStr(x);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}

EXPORT(const char*) GetPrimaryModDescription(int index)
{
	DEPRECATED;
	try {
		CheckInit();
		CheckBounds(index, modData.size());

		const std::string& x = modData[index].GetDescription();
		return GetStr(x);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}

EXPORT(int) OpenArchiveType(const char* name, const char* type)
{
	DEPRECATED;
	try {
		CheckInit();
		CheckNullOrEmpty(name);
		CheckNullOrEmpty(type);

		IArchive* a = archiveLoader.OpenArchive(name, type);

		if (!a) {
			throw content_error("Archive '" + std::string(name) + "' could not be opened");
		}

		nextArchive++;
		openArchives[nextArchive] = a;
		return nextArchive;
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}

// when removing this function, remove "std::string style" in Option.h, too
EXPORT(const char*) GetOptionStyle(int optIndex)
{
	DEPRECATED;
	try {
		CheckOptionIndex(optIndex);
		return GetStr(options[optIndex].style);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


#endif //
/*
**********************DEPRECATED SECTION END
*/

