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
#include "FileSystem/FileSystemHandler.h"
#include "Rendering/Textures/Bitmap.h"
#include "Sim/Misc/SideParser.h"
#include "ExternalAI/Interface/aidefines.h"
#include "ExternalAI/Interface/SSkirmishAILibrary.h"
#include "ExternalAI/LuaAIImplHandler.h"
#include "System/Exceptions.h"
#include "System/LogOutput.h"
#include "System/Util.h"
#include "System/exportdefines.h"
#include "System/Info.h"
#include "System/Option.h"

#ifdef WIN32
#include <windows.h>
#endif

// unitsync only:
#include "LuaParserAPI.h"
#include "Syncer.h"

using std::string;

//////////////////////////
//////////////////////////

static CLogSubsystem LOG_UNITSYNC("unitsync", true);

//This means that the DLL can only support one instance. Don't think this should be a problem.
static CSyncer* syncer;

static bool logOutputInitialised = false;
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

static std::string GetMapFile(const std::string& mapname)
{
	std::string mapFile = archiveScanner->MapNameToMapFile(mapname);

	if (mapFile != mapname) {
		//! translation finished fine
		return mapFile;
	}

	/*CFileHandler f(mapFile);
	if (f.FileExists()) {
		return mapFile;
	}

	CFileHandler f = CFileHandler(map);
	if (f.FileExists()) {
		return map;
	}

	f = CFileHandler("maps/" + map);
	if (f.FileExists()) {
		return "maps/" + map;
	}*/

	throw std::invalid_argument("Couldn't find a map named \"" + mapname + "\"");
	return "";
}


class ScopedMapLoader {
	public:
		/**
		 * @brief Helper class for loading a map archive temporarily
		 * @param mapName the name of the to be loaded map
		 * @param mapFile checks if this file already exists in the current VFS, if so skip reloading
		 */
		ScopedMapLoader(const string& mapName, const string& mapFile) : oldHandler(vfsHandler)
		{
			CFileHandler f(mapFile);
			if (f.FileExists()) {
				return;
			}

			vfsHandler = new CVFSHandler();
			vfsHandler->AddArchiveWithDeps(mapName, false);
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


EXPORT(const char*) GetSpringVersion()
{
	return GetStr(SpringVersion::Get());
}


static void internal_deleteMapInfos();

static void _UnInit()
{
	internal_deleteMapInfos();

	lpClose();

	FileSystemHandler::Cleanup();

	if ( syncer )
	{
		SafeDelete(syncer);
		logOutput.Print(LOG_UNITSYNC, "deinitialized");
	}
}

EXPORT(int) Init(bool isServer, int id, bool enable_logging)
{
	try {
		if (!logOutputInitialised)
			logOutput.SetFileName("unitsync.log");
		if (!configHandler)
			ConfigHandler::Instantiate(); // use the default config file
		FileSystemHandler::Initialize(false);

		if (!logOutputInitialised)
		{
			logOutput.Initialize();
			logOutputInitialised = true;
		}
		LOG_UNITSYNC.enabled = enable_logging;
		logOutput.Print(LOG_UNITSYNC, "loaded, %s\n", SpringVersion::GetFull().c_str());

		_UnInit();

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

EXPORT(void) UnInit()
{
	try {
		_UnInit();
		ConfigHandler::Deallocate();
	}
	UNITSYNC_CATCH_BLOCKS;
}


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


EXPORT(int) ProcessUnits()
{
	try {
		logOutput.Print(LOG_UNITSYNC, "syncer: process units\n");
		return syncer->ProcessUnits();
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


EXPORT(int) ProcessUnitsNoChecksum()
{
	return ProcessUnits();
}


EXPORT(int) GetUnitCount()
{
	try {
		logOutput.Print(LOG_UNITSYNC, "syncer: get unit count\n");
		return syncer->GetUnitCount();
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


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


EXPORT(void) AddAllArchives(const char* root)
{
	try {
		CheckInit();
		CheckNullOrEmpty(root);
		vfsHandler->AddArchiveWithDeps(root, false);
	}
	UNITSYNC_CATCH_BLOCKS;
}

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

EXPORT(unsigned int) GetArchiveChecksum(const char* arname)
{
	try {
		CheckInit();
		CheckNullOrEmpty(arname);

		logOutput.Print(LOG_UNITSYNC, "archive checksum: %s\n", arname);
		return archiveScanner->GetSingleArchiveChecksum(arname);
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}

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


static void safe_strzcpy(char* dst, std::string src, size_t max)
{
	if (src.length() > max-1) {
		src = src.substr(0, max-1);
	}
	strcpy(dst, src.c_str());
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

static bool internal_GetMapInfo(const char* mapname, InternalMapInfo* outInfo)
{
	CheckInit();
	CheckNullOrEmpty(mapname);
	CheckNull(outInfo);

	logOutput.Print(LOG_UNITSYNC, "get map info: %s", mapname);

	const std::string mapFile = GetMapFile(mapname);

	ScopedMapLoader mapLoader(mapname, mapFile);

	std::string err("");

	MapParser mapParser(mapFile);
	if (!mapParser.IsValid()) {
		err = mapParser.GetErrorLog();
	}
	const LuaTable mapTable = mapParser.GetRoot();

	// Retrieve the map header as well
	if (err.empty()) {
		const std::string extension = filesystem.GetExtension(mapFile);
		if (extension == "smf") {
			try {
				CSmfMapFile file(mapFile);
				const SMFHeader& mh = file.GetHeader();

				outInfo->width  = mh.mapx * SQUARE_SIZE;
				outInfo->height = mh.mapy * SQUARE_SIZE;
			}
			catch (content_error&) {
				outInfo->width  = -1;
			}
		}
		else {
			const int w = mapTable.GetInt("gameAreaW", 0);
			const int h = mapTable.GetInt("gameAreaW", 1);

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
		logOutput.Print(LOG_UNITSYNC, "  startpos: %.0f, %.0f", pos.x, pos.z);
	}

	return true;
}

/** @deprecated */
static bool _GetMapInfoEx(const char* mapname, MapInfo* outInfo, int version)
{
	CheckInit();
	CheckNullOrEmpty(mapname);
	CheckNull(outInfo);

	bool fetchOk;

	InternalMapInfo internalMapInfo;
	fetchOk = internal_GetMapInfo(mapname, &internalMapInfo);

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

EXPORT(int) GetMapInfoEx(const char* mapname, MapInfo* outInfo, int version)
{
	int ret = 0;

	try {
		const bool fetchOk = _GetMapInfoEx(mapname, outInfo, version);
		ret = fetchOk ? 1 : 0;
	}
	UNITSYNC_CATCH_BLOCKS;

	return ret;
}


EXPORT(int) GetMapInfo(const char* mapname, MapInfo* outInfo)
{
	int ret = 0;

	try {
		const bool fetchOk = _GetMapInfoEx(mapname, outInfo, 0);
		ret = fetchOk ? 1 : 0;
	}
	UNITSYNC_CATCH_BLOCKS;

	return ret;
}


// Updated on every call to GetMapCount
static vector<string> mapNames;

EXPORT(int) GetMapCount()
{
	try {
		CheckInit();

		mapNames.clear();

		vector<string> ars = archiveScanner->GetMaps();
		for (vector<string>::iterator i = ars.begin(); i != ars.end(); ++i)
			mapNames.push_back(*i);

		sort(mapNames.begin(), mapNames.end());

		return mapNames.size();
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
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


/**
 * @brief Get the filename (+ VFS-path) of a map
 * @return NULL on error; the filename of the map (e.g. "maps/SmallDivide.smf") on success
 */
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

EXPORT(const char*) GetMapDescription(int index) {

	const InternalMapInfo* mapInfo = internal_getMapInfo(index);
	if (mapInfo) {
		return mapInfo->description.c_str();
	}

	return NULL;
}

EXPORT(const char*) GetMapAuthor(int index) {

	const InternalMapInfo* mapInfo = internal_getMapInfo(index);
	if (mapInfo) {
		return mapInfo->author.c_str();
	}

	return NULL;
}

EXPORT(int) GetMapWidth(int index) {

	const InternalMapInfo* mapInfo = internal_getMapInfo(index);
	if (mapInfo) {
		return mapInfo->width;
	}

	return -1;
}

EXPORT(int) GetMapHeight(int index) {

	const InternalMapInfo* mapInfo = internal_getMapInfo(index);
	if (mapInfo) {
		return mapInfo->height;
	}

	return -1;
}

EXPORT(int) GetMapTidalStrength(int index) {

	const InternalMapInfo* mapInfo = internal_getMapInfo(index);
	if (mapInfo) {
		return mapInfo->tidalStrength;
	}

	return -1;
}

EXPORT(int) GetMapWindMin(int index) {

	const InternalMapInfo* mapInfo = internal_getMapInfo(index);
	if (mapInfo) {
		return mapInfo->minWind;
	}

	return -1;
}

EXPORT(int) GetMapWindMax(int index) {

	const InternalMapInfo* mapInfo = internal_getMapInfo(index);
	if (mapInfo) {
		return mapInfo->maxWind;
	}

	return -1;
}

EXPORT(int) GetMapGravity(int index) {

	const InternalMapInfo* mapInfo = internal_getMapInfo(index);
	if (mapInfo) {
		return mapInfo->gravity;
	}

	return -1;
}

EXPORT(int) GetMapResourceCount(int index) {
	return 1;
}

EXPORT(const char*) GetMapResourceName(int index, int resourceIndex) {

	if (resourceIndex == 0) {
		return "Metal";
	} else {
		SetLastError("No valid map resource index");
	}

	return NULL;
}

EXPORT(float) GetMapResourceMax(int index, int resourceIndex) {

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

	const InternalMapInfo* mapInfo = internal_getMapInfo(index);
	if (mapInfo) {
		return mapInfo->xPos.size();
	}

	return -1;
}

//FIXME: rename to GetMapStartPosX ?
EXPORT(float) GetMapPosX(int index, int posIndex) {

	const InternalMapInfo* mapInfo = internal_getMapInfo(index);
	if (mapInfo) {
		return mapInfo->xPos[posIndex];
	}

	return -1.0f;
}

EXPORT(float) GetMapPosZ(int index, int posIndex) {

	const InternalMapInfo* mapInfo = internal_getMapInfo(index);
	if (mapInfo) {
		return mapInfo->zPos[posIndex];
	}

	return -1.0f;
}


EXPORT(float) GetMapMinHeight(const char* mapname) {
	try {
		const std::string mapFile = GetMapFile(mapname);
		ScopedMapLoader loader(mapname, mapFile);
		CSmfMapFile file(mapFile);
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

EXPORT(float) GetMapMaxHeight(const char* mapname) {
	try {
		const std::string mapFile = GetMapFile(mapname);
		ScopedMapLoader loader(mapname, mapFile);
		CSmfMapFile file(mapFile);
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



static vector<string> mapArchives;

EXPORT(int) GetMapArchiveCount(const char* mapName)
{
	try {
		CheckInit();
		CheckNullOrEmpty(mapName);

		mapArchives = archiveScanner->GetArchives(mapName);
		return mapArchives.size();
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
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

static unsigned short* GetMinimapSM3(string mapFileName, int miplevel)
{
	MapParser mapParser(mapFileName);
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
		bm = bm.CreateRescaled(1024 >> miplevel, 1024 >> miplevel);

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
}

static unsigned short* GetMinimapSMF(string mapFileName, int miplevel)
{
	CSmfMapFile in(mapFileName);
	std::vector<uint8_t> buffer;
	const int mipsize = in.ReadMinimap(buffer, miplevel);

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

EXPORT(unsigned short*) GetMinimap(const char* mapname, int miplevel)
{
	try {
		CheckInit();
		CheckNullOrEmpty(mapname);

		if (miplevel < 0 || miplevel > 8)
			throw std::out_of_range("Miplevel must be between 0 and 8 (inclusive) in GetMinimap.");

		const std::string mapFile = GetMapFile(mapname);
		ScopedMapLoader mapLoader(mapname, mapFile);

		unsigned short* ret = NULL;
		const string extension = filesystem.GetExtension(mapFile);
		if (extension == "smf") {
			ret = GetMinimapSMF(mapFile, miplevel);
		} else if (extension == "sm3") {
			ret = GetMinimapSM3(mapFile, miplevel);
		}

		return ret;
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


EXPORT(int) GetInfoMapSize(const char* mapname, const char* name, int* width, int* height)
{
	try {
		CheckInit();
		CheckNullOrEmpty(mapname);
		CheckNullOrEmpty(name);
		CheckNull(width);
		CheckNull(height);

		const std::string mapFile = GetMapFile(mapname);
		ScopedMapLoader mapLoader(mapname, mapFile);
		CSmfMapFile file(mapFile);

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


EXPORT(int) GetInfoMap(const char* mapname, const char* name, unsigned char* data, int typeHint)
{
	try {
		CheckInit();
		CheckNullOrEmpty(mapname);
		CheckNullOrEmpty(name);
		CheckNull(data);

		const std::string mapFile = GetMapFile(mapname);
		ScopedMapLoader mapLoader(mapname, mapFile);
		CSmfMapFile file(mapFile);

		const string n = name;
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
			unsigned char* outp = data;
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

vector<CArchiveScanner::ArchiveData> modData;

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

static void ParseOptions(const string& fileName,
                         const string& fileModes,
                         const string& accessModes)
{
	parseOptions(options, fileName, fileModes, accessModes, &optionsSet, &LOG_UNITSYNC);
}


static void ParseMapOptions(const string& fileName,
                         const string& mapName,
                         const string& fileModes,
                         const string& accessModes)
{
	parseMapOptions(options, fileName, mapName, fileModes, accessModes,
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


EXPORT(int) GetMapOptionCount(const char* name)
{
	try {
		CheckInit();
		CheckNullOrEmpty(name);

		const std::string mapFile = GetMapFile(name);
		ScopedMapLoader mapLoader(name, mapFile);

		options.clear();
		optionsSet.clear();

		ParseMapOptions("MapOptions.lua", name, SPRING_VFS_MAP, SPRING_VFS_MAP);

		optionsSet.clear();

		return options.size();
	}
	UNITSYNC_CATCH_BLOCKS;

	options.clear();
	optionsSet.clear();

	return 0;
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

	return 0;
}

EXPORT(int) GetCustomOptionCount(const char* filename)
{
	try {
		CheckInit();

		options.clear();
		optionsSet.clear();

		try {
			ParseOptions(filename, SPRING_VFS_ZIP, SPRING_VFS_ZIP);
		}
		UNITSYNC_CATCH_BLOCKS;

		optionsSet.clear();

		return options.size();
	}
	UNITSYNC_CATCH_BLOCKS;

	// Failed to load custom options file
	options.clear();
	optionsSet.clear();

	return 0;
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
			if (!infoFile.empty()) {
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

EXPORT(const char*) GetOptionStyle(int optIndex)
{
	try {
		CheckOptionIndex(optIndex);
		return GetStr(options[optIndex].style);
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
	try {
		CheckOptionIndex(optIndex);
		return options[optIndex].typeCode;
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
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
	try {
		CheckOptionType(optIndex, opt_number);
		return options[optIndex].numberDef;
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0.0f;
}

EXPORT(float) GetOptionNumberMin(int optIndex)
{
	try {
		CheckOptionType(optIndex, opt_number);
		return options[optIndex].numberMin;
	}
	UNITSYNC_CATCH_BLOCKS;
	return -1.0e30f; // FIXME ?
}

EXPORT(float) GetOptionNumberMax(int optIndex)
{
	try {
		CheckOptionType(optIndex, opt_number);
		return options[optIndex].numberMax;
	}
	UNITSYNC_CATCH_BLOCKS;
	return +1.0e30f; // FIXME ?
}

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
	try {
		CheckOptionType(optIndex, opt_string);
		return options[optIndex].stringMaxLen;
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


// List Options

EXPORT(int) GetOptionListCount(int optIndex)
{
	try {
		CheckOptionType(optIndex, opt_list);
		return options[optIndex].list.size();
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
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
		const vector<OptionListItem>& list = options[optIndex].list;
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
	const std::string mapName = luaL_checkstring(L, 1);

	InternalMapInfo mi;
	if (!internal_GetMapInfo(mapName.c_str(), &mi)) {
		logOutput.Print(LOG_UNITSYNC, "LuaGetMapInfo: internal_GetMapInfo(\"%s\") failed", mapName.c_str());
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

EXPORT(int) ReadFileVFS(int handle, unsigned char* buf, int length)
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

		if (cur < a->NumFiles())
		{
			string name;
			int s;
			a->FileInfo(cur, name, s);
			strcpy(nameBuf, name.c_str()); // FIXME: oops, buffer overflow
			*size = s;
			return ++cur;
		}
		return 0;
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}

EXPORT(int) OpenArchiveFile(int archive, const char* name)
{
	try {
		CheckArchiveHandle(archive);
		CheckNullOrEmpty(name);

		CArchiveBase* a = openArchives[archive];
		return a->FindFile(name);
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}

EXPORT(int) ReadArchiveFile(int archive, int handle, unsigned char* buffer, int numBytes)
{
	try {
		CheckArchiveHandle(archive);
		CheckNull(buffer);
		CheckPositive(numBytes);

		CArchiveBase* a = openArchives[archive];
		std::vector<uint8_t> buf;
		if (!a->GetFile(handle, buf))
			return -1;
		std::memcpy(buffer, &buf[0], std::min(buf.size(), (size_t)numBytes));
		return std::min(buf.size(), (size_t)numBytes);
	}
	UNITSYNC_CATCH_BLOCKS;
	return -1;
}

EXPORT(void) CloseArchiveFile(int archive, int handle)
{
	try {
		// nuting
	}
	UNITSYNC_CATCH_BLOCKS;
}

EXPORT(int) SizeArchiveFile(int archive, int handle)
{
	try {
		CheckArchiveHandle(archive);

		CArchiveBase* a = openArchives[archive];
		string name;
		int s;
		a->FileInfo(handle, name, s);
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
	//static char strBuf[STRBUF_SIZE];

	if (str.length() + 1 > STRBUF_SIZE) {
		sprintf(strBuf, "Increase STRBUF_SIZE (needs "_STPF_" bytes)", str.length() + 1);
	} else {
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

EXPORT(int) GetSpringConfigInt(const char* name, const int defValue)
{
	try {
		CheckConfigHandler();
		return configHandler->Get(name, defValue);
	}
	UNITSYNC_CATCH_BLOCKS;
	return defValue;
}

EXPORT(float) GetSpringConfigFloat(const char* name, const float defValue)
{
	try {
		CheckConfigHandler();
		return configHandler->Get(name, defValue);
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
