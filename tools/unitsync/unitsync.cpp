/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "unitsync.h"
#include "unitsync_api.h"

#include <algorithm>
#include <cstring>
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
#include "ExternalAI/LuaAIImplHandler.h"
#include "System/Config/ConfigHandler.h"
#include "System/FileSystem/Archives/IArchive.h"
#include "System/FileSystem/ArchiveLoader.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/DataDirsAccess.h"
#include "System/FileSystem/DataDirLocater.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/FileSystemInitializer.h"
#include "System/Log/ILog.h"
#include "System/Log/Level.h"
#include "System/Log/DefaultFilter.h"
#include "System/Misc/SpringTime.h"
#include "System/Platform/Misc.h" //!!
#include "System/Threading/ThreadPool.h"
#include "System/Exceptions.h"
#include "System/Info.h"
#include "System/LogOutput.h"
#include "System/Option.h"
#include "System/SafeCStrings.h"
#include "System/SafeUtil.h"
#include "System/StringUtil.h"
#include "System/ExportDefines.h"

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

// avoid including GlobalConstants.h
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

// function argument checking

static bool CheckInit(bool throwException = true)
{
	if (archiveScanner == nullptr || vfsHandler == nullptr) {
		if (throwException)
			throw std::logic_error("UnitSync not initialized. Call Init first.");

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

struct GameDataUnitDef {
	std::string name;
	std::string fullName;
};

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


static std::vector<InfoItem> infoItems;
static std::set<std::string> infoSet;
static std::vector<GameDataUnitDef> unitDefs;
static std::map<int, InternalMapInfo> mapInfos;
static std::map<int, IArchive*> openArchives;
static std::map<int, CFileHandler*> openFiles;
static std::vector<std::string> curFindFiles;

// Updated on every call to GetMapCount
static std::vector<std::string> mapNames;
static std::vector<std::string> mapArchives;

static std::vector<Option> options;
static std::set<std::string> optionsSet;

static CLuaAIImplHandler::InfoItemVector luaAIInfos;
// Updated on every call to GetSkirmishAICount
static std::vector<std::string> skirmishAIDataDirs;

static std::vector<std::string> modValidMaps;

static std::string lastError;

static int nextArchive = 0;
static int nextFile = 0;

static bool autoUnLoadmap = true;



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



//////////////////////////
//////////////////////////

// error handling
static void _SetLastError(const std::string& err)
{
	LOG_L(L_ERROR, "%s", err.c_str());
	lastError = err;
}

#define SetLastError(str) \
	_SetLastError(std::string(__func__) + ": " + (str))

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
		if (lastError.empty())
			return nullptr;

		const std::string err = std::move(lastError);
		return GetStr(err);
	}
	UNITSYNC_CATCH_BLOCKS;

	// Oops, can't even return errors anymore...
	// Returning anything but NULL might cause infinite loop in lobby client...
	//return __func__ ": fatal error: an exception was thrown in GetNextError";
	return nullptr;
}




//////////////////////////
//////////////////////////

static std::string GetMapFile(const std::string& mapName)
{
	const std::string mapFile = archiveScanner->MapNameToMapFile(mapName);

	// translation finished fine
	if (mapFile != mapName)
		return mapFile;

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
		ScopedMapLoader(const std::string& mapName, const std::string& mapFile): oldHandler(vfsHandler)
		{
			if (!autoUnLoadmap)
				return;
			CFileHandler f(mapFile);
			if (f.FileExists())
				return;

			CVFSHandler::SetGlobalInstance(new CVFSHandler());
			vfsHandler->AddArchiveWithDeps(mapName, false);
		}

		~ScopedMapLoader()
		{
			if (!autoUnLoadmap)
				return;
			if (vfsHandler != oldHandler) {
				CVFSHandler::FreeGlobalInstance();
				CVFSHandler::SetGlobalInstance(oldHandler);
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
		configHandler->NotifyOnChange(this, {"UnitsyncAutoUnLoadMaps"});
	}

	~UnitsyncConfigObserver() {
		configHandler->RemoveObserver(this);
	}

	void ConfigNotify(const std::string& key, const std::string& value) {
		autoUnLoadmap = configHandler->GetBool("UnitsyncAutoUnLoadMaps");
	}
};



static void internal_deleteMapInfos();
static UnitsyncConfigObserver* unitsyncConfigObserver = nullptr;

static void _Cleanup()
{
	spring::SafeDelete(unitsyncConfigObserver);
	internal_deleteMapInfos();

	lpClose();
	LOG("deinitialized");
}


static void CheckForImportantFilesInVFS()
{
	const std::array<std::string, 4> filesToCheck = {{
		"base/springcontent.sdz",
		"base/maphelper.sdz",
		"base/spring/bitmaps.sdz",
		"base/cursors.sdz",
	}};

	for (const std::string& file: filesToCheck) {
		if (!CFileHandler::FileExists(file, SPRING_VFS_RAW))
			throw content_error("Required base file '" + file + "' does not exist.");
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
		log_filter_section_setMinLevel(LOG_LEVEL_INFO, LOG_SECTION_UNITSYNC);
#endif

		// reinitialize filesystem to detect new files
		if (CheckInit(false))
			FileSystemInitializer::Cleanup();

		dataDirLocater.UpdateIsolationModeByEnvVar();

		const std::string& configFile = (configHandler != nullptr)? configHandler->GetConfigFile(): "";
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
		LOG("[UnitSync::%s] initialized %s (call %d)", __func__, springFull.c_str(), numCalls);
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
	return nullptr;
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
		if (index > datadirs.size())
			return nullptr;

		return GetStr(datadirs[index]);
	}
	UNITSYNC_CATCH_BLOCKS;
	return nullptr;
}

EXPORT(int) ProcessUnits()
{
	try {
		CheckInit();
		LOG_L(L_DEBUG, "[%s] loaded=%d", __func__, unitDefs.empty());
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
		LOG_L(L_DEBUG, "[%s]", __func__);
		count = unitDefs.size();
	}
	UNITSYNC_CATCH_BLOCKS;

	return count;
}


EXPORT(const char*) GetUnitName(int unitDefID)
{
	try {
		CheckInit();
		LOG_L(L_DEBUG, "[%s(UnitDefID=%d)]", __func__, unitDefID);
		return GetStr(unitDefs[unitDefID].name);
	}
	UNITSYNC_CATCH_BLOCKS;
	return nullptr;
}


EXPORT(const char*) GetFullUnitName(int unitDefID)
{
	try {
		CheckInit();
		LOG_L(L_DEBUG, "[%s(UnitDefID=%d)]", __func__, unitDefID);
		return GetStr(unitDefs[unitDefID].fullName);
	}
	UNITSYNC_CATCH_BLOCKS;
	return nullptr;
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

		CVFSHandler::FreeGlobalInstance();
		CVFSHandler::SetGlobalInstance(new CVFSHandler());
	}
	UNITSYNC_CATCH_BLOCKS;
}

EXPORT(unsigned int) GetArchiveChecksum(const char* archiveName)
{
	try {
		CheckInit();
		CheckNullOrEmpty(archiveName);

		LOG_L(L_DEBUG, "archive checksum: %s", archiveName);
		return archiveScanner->GetArchiveSingleChecksum(archiveName);
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
	return nullptr;
}



static bool internal_GetMapInfo(const char* mapName, InternalMapInfo* outInfo)
{
	CheckInit();
	CheckNullOrEmpty(mapName);
	CheckNull(outInfo);

	LOG_L(L_DEBUG, "get map info: %s", mapName);

	const std::string mapFile = GetMapFile(mapName);

	ScopedMapLoader mapLoader(mapName, mapFile);

	std::string err;

	MapParser mapParser(mapFile);
	if (!mapParser.IsValid())
		err = mapParser.GetErrorLog();

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
	return nullptr;
}


EXPORT(const char*) GetMapFileName(int index)
{
	try {
		CheckInit();
		CheckBounds(index, mapNames.size());

		return GetStr(archiveScanner->MapNameToMapFile(mapNames[index]));
	}
	UNITSYNC_CATCH_BLOCKS;
	return nullptr;
}



static InternalMapInfo* internal_getMapInfo(int index)
{
	if (index >= mapNames.size()) {
		SetLastError("invalid map index");
	} else {
		if (mapInfos.find(index) != mapInfos.end())
			return &(mapInfos[index]);

		try {
			InternalMapInfo imi;
			if (internal_GetMapInfo(mapNames[index].c_str(), &imi)) {
				mapInfos[index] = imi;
				return &(mapInfos[index]);
			}
		}
		UNITSYNC_CATCH_BLOCKS;
	}

	return nullptr;
}

static void internal_deleteMapInfos()
{
	while (!mapInfos.empty()) {
		mapInfos.erase(mapInfos.begin());
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

		// override the header's minHeight value
		if (smfTable.KeyExists("minHeight"))
			return (smfTable.GetFloat("minHeight", 0.0f));

		return (header.minHeight);
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
	return nullptr;
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
	return nullptr;

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

	const int numblocks = buffer.size() / 8;
	for (int i = 0; i < numblocks; i++) {
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

		unsigned short* ret = nullptr;
		const std::string extension = FileSystem::GetExtension(mapFile);
		if (extension == "smf") {
			ret = GetMinimapSMF(mapFile, mipLevel);
		} else if (extension == "sm3") {
			ret = GetMinimapSM3(mapFile, mipLevel);
		}

		return ret;
	}
	UNITSYNC_CATCH_BLOCKS;
	return nullptr;
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

		const std::vector<InfoItem>& modInfoItems = modData[modIndex].GetInfoItems();

		infoItems.clear();
		infoItems.insert(infoItems.end(), modInfoItems.begin(), modInfoItems.end());

		return (int)infoItems.size();
	}
	UNITSYNC_CATCH_BLOCKS;

	infoItems.clear();

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
	return nullptr;
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
	return nullptr;
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
	return nullptr;
}

EXPORT(const char*) GetSideStartUnit(int side)
{
	try {
		CheckInit();
		CheckBounds(side, sideParser.GetCount());

		return GetStr(sideParser.GetStartUnit(side));
	}
	UNITSYNC_CATCH_BLOCKS;
	return nullptr;
}





//////////////////////////
//////////////////////////

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

static void GetLuaAIInfo()
{
	luaAIInfos = luaAIImplHandler.LoadInfoItems();
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

EXPORT(int) GetSkirmishAICount() {

	int count = -1;

	try {
		CheckInit();

		skirmishAIDataDirs.clear();

		// filter out dirs not containing an AIInfo.lua file
		for (const std::string& dataDir: dataDirsAccess.FindDirsInDirectSubDirs(SKIRMISH_AI_DATA_DIR)) {
			const std::vector<std::string>& infoFiles = CFileHandler::FindFiles(dataDir, "AIInfo.lua");

			if (!infoFiles.empty())
				skirmishAIDataDirs.push_back(dataDir);
		}

		std::sort(skirmishAIDataDirs.begin(), skirmishAIDataDirs.end());

		count = skirmishAIDataDirs.size() + GetNumberOfLuaAIs();
	}
	UNITSYNC_CATCH_BLOCKS;

	return count;
}


static void ParseInfo(const std::string& fileName, const std::string& fileModes, const std::string& accessModes)
{
	info_parseInfo(infoItems, fileName, fileModes, accessModes, &infoSet);
}

static void CheckSkirmishAIIndex(int aiIndex)
{
	CheckInit();
	CheckBounds(aiIndex, skirmishAIDataDirs.size() + luaAIInfos.size());
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

		infoItems.clear();

		if (IsLuaAIIndex(aiIndex)) {
			const auto& iInfo = luaAIInfos[ToPureLuaAIIndex(aiIndex)];
			infoItems.insert(infoItems.end(), iInfo.begin(), iInfo.end());
		} else {
			infoSet.clear();
			ParseInfo(skirmishAIDataDirs[aiIndex] + "/AIInfo.lua", SPRING_VFS_RAW, SPRING_VFS_RAW);
			infoSet.clear();
		}

		return (int)infoItems.size();
	}
	UNITSYNC_CATCH_BLOCKS;

	infoItems.clear();

	return -1;
}

EXPORT(int) GetMapInfoCount(int index) {
	try{
		infoItems.clear();
		CheckBounds(index, mapNames.size());
		const InternalMapInfo* mapInfo = internal_getMapInfo(index);
		if (mapInfo == nullptr)
			return -1;

		infoItems.emplace_back("description", "", mapInfo->description);
		infoItems.emplace_back("author", "", mapInfo->author);
		infoItems.emplace_back("tidalStrength", "", mapInfo->tidalStrength);
		infoItems.emplace_back("gravity", "", mapInfo->gravity);
		infoItems.emplace_back("maxMetal", "", mapInfo->maxMetal);
		infoItems.emplace_back("extractorRadius", "", mapInfo->extractorRadius);
		infoItems.emplace_back("minWind", "", mapInfo->minWind);
		infoItems.emplace_back("maxWind", "", mapInfo->maxWind);
		infoItems.emplace_back("width", "", mapInfo->width);
		infoItems.emplace_back("height", "", mapInfo->height);
		infoItems.emplace_back("resource", "", "Metal");
		for (int i = 0; i < mapInfo->xPos.size() && i < mapInfo->zPos.size(); i++) {
			infoItems.emplace_back("xPos", "", mapInfo->xPos[i]);
			infoItems.emplace_back("zPos", "", mapInfo->zPos[i]);
		}
		return (int)infoItems.size();
	}
	UNITSYNC_CATCH_BLOCKS;

	infoItems.clear();

	return -1;
}

static const InfoItem* GetInfoItem(int infoIndex)
{
	CheckInit();
	CheckBounds(infoIndex, infoItems.size());
	return &(infoItems[infoIndex]);
}

static void CheckInfoValueType(const InfoItem* infoItem, InfoValueType requiredValueType)
{
	if (infoItem->valueType != requiredValueType) {
		throw std::invalid_argument(
				std::string("Tried to fetch info-value of type ")
				+ info_convertTypeToString(infoItem->valueType)
				+ " as " + info_convertTypeToString(requiredValueType) + ".");
	}
}

EXPORT(const char*) GetInfoKey(int infoIndex) {

	const char* key = nullptr;

	try {
		key = GetStr(GetInfoItem(infoIndex)->key);
	}
	UNITSYNC_CATCH_BLOCKS;

	return key;
}
EXPORT(const char*) GetInfoType(int infoIndex) {

	const char* type = nullptr;

	try {
		type = info_convertTypeToString(GetInfoItem(infoIndex)->valueType);
	}
	UNITSYNC_CATCH_BLOCKS;

	return type;
}

EXPORT(const char*) GetInfoValueString(int infoIndex) {

	const char* value = nullptr;

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

	const char* desc = nullptr;

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
	return nullptr;
}

EXPORT(const char*) GetOptionScope(int optIndex)
{
	try {
		CheckOptionIndex(optIndex);
		return GetStr(options[optIndex].scope);
	}
	UNITSYNC_CATCH_BLOCKS;
	return nullptr;
}

EXPORT(const char*) GetOptionName(int optIndex)
{
	try {
		CheckOptionIndex(optIndex);
		return GetStr(options[optIndex].name);
	}
	UNITSYNC_CATCH_BLOCKS;
	return nullptr;
}

EXPORT(const char*) GetOptionSection(int optIndex)
{
	try {
		CheckOptionIndex(optIndex);
		return GetStr(options[optIndex].section);
	}
	UNITSYNC_CATCH_BLOCKS;
	return nullptr;
}

EXPORT(const char*) GetOptionDesc(int optIndex)
{
	try {
		CheckOptionIndex(optIndex);
		return GetStr(options[optIndex].desc);
	}
	UNITSYNC_CATCH_BLOCKS;
	return nullptr;
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
	return nullptr;
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
	return nullptr;
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
	return nullptr;
}

EXPORT(const char*) GetOptionListItemName(int optIndex, int itemIndex)
{
	try {
		CheckOptionType(optIndex, opt_list);
		const std::vector<OptionListItem>& list = options[optIndex].list;
		CheckBounds(itemIndex, list.size());
		return GetStr(list[itemIndex].name);
	}
	UNITSYNC_CATCH_BLOCKS;
	return nullptr;
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
	return nullptr;
}



//////////////////////////
//////////////////////////

static int LuaGetMapList(lua_State* L)
{
	lua_createtable(L, GetMapCount(), 0);
	const int mapCount = GetMapCount();
	for (int i = 0; i < mapCount; i++) {
		lua_pushnumber(L, i + 1);
		lua_pushstring(L, GetMapName(i));
		lua_rawset(L, -3);
	}
	return 1;
}


static void LuaPushNamedString(lua_State* L, const std::string& key, const std::string& value)
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
		LOG_L(L_ERROR, "LuaGetMapInfo: internal_GetMapInfo(\"%s\") failed", mapName.c_str());
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
	lua_createtable(L, mi.xPos.size(), 0);
	for (size_t p = 0; p < mi.xPos.size(); p++) {
		lua_pushnumber(L, p + 1);
		lua_createtable(L, 2, 0);
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

		if (!luaParser.Execute())
			throw content_error("luaParser.Execute() failed: " + luaParser.GetErrorLog());

		const LuaTable root = luaParser.GetRoot();
		if (!root.IsValid())
			throw content_error("root table invalid");

		for (int index = 1; root.KeyExists(index); index++) {
			const std::string map = root.GetString(index, "");
			if (!map.empty())
				modValidMaps.push_back(map);
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
	return nullptr;
}


//////////////////////////
//////////////////////////

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

		openFiles[++nextFile] = fh;

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
		return openFiles[file]->Read(buf, numBytes);
	}
	UNITSYNC_CATCH_BLOCKS;
	return -1;
}

EXPORT(int) FileSizeVFS(int file)
{
	try {
		CheckFileHandle(file);

		LOG_L(L_DEBUG, "FileSizeVFS: %d", file);
		return openFiles[file]->FileSize();
	}
	UNITSYNC_CATCH_BLOCKS;
	return -1;
}

EXPORT(int) InitFindVFS(const char* pattern)
{
	try {
		CheckInit();
		CheckNullOrEmpty(pattern);

		const std::string path = FileSystem::GetDirectory(pattern);
		const std::string patt = FileSystem::GetFilename(pattern);
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

		if (path    == nullptr) { path = "";              }
		if (modes   == nullptr) { modes = SPRING_VFS_ALL; }
		if (pattern == nullptr) { pattern = "*";          }

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

		if (path    == nullptr) { path = "";              }
		if (modes   == nullptr) { modes = SPRING_VFS_ALL; }
		if (pattern == nullptr) { pattern = "*";          }

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
		if ((unsigned)file >= curFindFiles.size())
			return 0;

		STRCPY_T(nameBuf, size, curFindFiles[file].c_str());
		return file + 1;
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


//////////////////////////
//////////////////////////

static void CheckArchiveHandle(int archive)
{
	CheckInit();

	if (openArchives.find(archive) != openArchives.end())
		return;

	throw content_error("Unregistered archive handle. Pass an archive handle returned by OpenArchive.");
}


EXPORT(int) OpenArchive(const char* name)
{
	try {
		CheckInit();
		CheckNullOrEmpty(name);

		IArchive* a = archiveLoader.OpenArchive(name);

		if (a == nullptr)
			throw content_error("Archive '" + std::string(name) + "' could not be opened");

		openArchives[++nextArchive] = a;
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

		if (file < arch->NumFiles()) {
			const int nameBufSize = *size;
			std::string fileName;
			int fileSize;
			arch->FileInfo(file, fileName, fileSize);
			*size = fileSize;

			if (nameBufSize > fileName.length()) {
				STRCPY(nameBuf, fileName.c_str());
				return ++file;
			}

			SetLastError("name-buffer is too small");
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

		if ((fileID = arch->FindFile(name)) == arch->NumFiles())
			fileID = -2;

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

		memcpy(buffer, &buf[0], std::min(buf.size(), (size_t)numBytes));
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
const char* GetStr(const std::string& str)
{
	if (str.length() + 1 > STRBUF_SIZE) {
		sprintf(strBuf, "Increase STRBUF_SIZE (needs %u bytes)", (unsigned) str.length() + 1);
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
	return nullptr;
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


EXPORT(const char*) GetSysInfoHash() {
	static std::array<char, 16384> infoHashBuf;
	const std::string& sysInfoHash = Platform::GetSysInfoHash();

	memset(infoHashBuf.data(), 0, sizeof(infoHashBuf));
	memcpy(infoHashBuf.data(), sysInfoHash.data(), std::min(sysInfoHash.size(), infoHashBuf.size()));
	return (infoHashBuf.data());
}

EXPORT(const char*) GetMacAddrHash() {
	static std::array<char, 16384> macAddrBuf;
	const std::string& macAddrHash = Platform::GetMacAddrHash();

	memset(macAddrBuf.data(), 0, sizeof(macAddrBuf));
	memcpy(macAddrBuf.data(), macAddrHash.data(), std::min(macAddrHash.size(), macAddrBuf.size()));
	return (macAddrBuf.data());
}

