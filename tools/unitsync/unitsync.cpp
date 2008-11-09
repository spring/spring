#include "StdAfx.h"
#include "unitsync.h"

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
#include "Platform/ConfigHandler.h"
#include "Platform/FileSystem.h"
#include "Rendering/Textures/Bitmap.h"
#include "Sim/Misc/SideParser.h"
#include "System/Exceptions.h"
#include "System/LogOutput.h"
#include "System/Util.h"

// unitsync only:
#include "LuaParserAPI.h"
#include "Syncer.h"

using std::string;

//////////////////////////
//////////////////////////

static CLogSubsystem LOG_UNITSYNC("unitsync", true);

//This means that the DLL can only support one instance. Don't think this should be a problem.
static CSyncer* syncer;

// I'd rather not include globalstuff
#define SQUARE_SIZE 8


#ifdef WIN32
BOOL __stdcall DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved)
{
	return TRUE;
}
#endif


namespace
{
	struct COneTimeInit
	{
		COneTimeInit()
		{
			logOutput.SetFilename("unitsync.log");
			logOutput.Print(LOG_UNITSYNC, "loaded, %s\n", SpringVersion::GetFull().c_str());
		}
	};
}
static COneTimeInit global_initializer;

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
	logOutput.Print(LOG_UNITSYNC, "error: " + err);
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

/**
 * @brief returns next error in queue of errors and removes this error from queue
 *
 * Use this method to get a (short) description of errors that occurred in any
 * other unitsync methods. Call this in a loop until it returns NULL to get all
 * errors.
 *
 * The error messages may be varying in detail etc.; nothing is guaranteed about
 * them, not even whether they have terminating newline or not.
 *
 * Example:
 *		const char* err;
 *		while ((err = GetNextError()) != NULL)
 *			printf("unitsync error: %s\n", err);
 */
DLL_EXPORT const char* __stdcall GetNextError()
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
 * @brief returns the version fo spring this was compiled with
 *
 * Returns a const char* string specifying the version of spring used to build this library with.
 * It was added to aid in lobby creation, where checks for updates to spring occur.
 */
DLL_EXPORT const char* __stdcall GetSpringVersion()
{
	return SpringVersion::Get().c_str();
}


/**
 * @brief Creates a messagebox with said message
 * @param p_szMessage const char* string holding the message
 *
 * Creates a messagebox with the title "Message from DLL", an OK button, and the specified message
 */
DLL_EXPORT void __stdcall Message(const char* p_szMessage)
{
	try {
		logOutput.Print(LOG_UNITSYNC, "Message from DLL: %s\n", p_szMessage);
#ifdef WIN32
		MessageBox(NULL, p_szMessage, "Message from DLL", MB_OK);
#else
		// this may cause message to be printed on console twice, if StdoutDebug is on
		fprintf(stderr, "unitsync: Message from DLL: %s\n", p_szMessage);
#endif
	}
	UNITSYNC_CATCH_BLOCKS;
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

	ConfigHandler::Deallocate();
}


/**
 * @brief uninitialize the unitsync library
 */
DLL_EXPORT void __stdcall UnInit()
{
	try {
		_UnInit();
	}
	UNITSYNC_CATCH_BLOCKS;
}


/**
 * @brief initialize the unitsync library
 *
 * Call this function before calling any other function in unitsync.
 * In case unitsync was already initialized, it is uninitialized and then
 * reinitialized.
 *
 * @return zero on failure, non-zero on success
 */
DLL_EXPORT int __stdcall Init(bool isServer, int id)
{
	try {
		_UnInit();

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
 * @brief get the main, writable, data directory that's used by unitsync and Spring
 */
DLL_EXPORT const char* __stdcall GetWritableDataDirectory()
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
 * @brief process another unit and return how many are left to process
 * @return int The number of unprocessed units to be handled
 *
 * Call this function repeatedly until it returns 0 before calling any other function related to units.
 *
 * Because of risk for infinite loops, this function can not return any error code.
 * It is advised to poll GetNextError() after calling this function.
 */
DLL_EXPORT int __stdcall ProcessUnits()
{
	try {
		logOutput.Print(LOG_UNITSYNC, "syncer: process units\n");
		return syncer->ProcessUnits();
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


/**
 * @brief identical to ProcessUnits, neither generates checksum anymore
 */
DLL_EXPORT int __stdcall ProcessUnitsNoChecksum()
{
	return ProcessUnits();
}


/**
 * @brief returns the number of units
 * @return int number of units processed and available, 0 on error
 *
 * Will return the number of units. Remember to call processUnits() beforehand until it returns 0.
 * As ProcessUnits is called the number of processed units goes up, and so will the value returned
 * by this function.
 *
 * Example:
 *		while (ProcessUnits()) {}
 *		int unit_number = GetUnitCount();
 */
DLL_EXPORT int __stdcall GetUnitCount()
{
	try {
		logOutput.Print(LOG_UNITSYNC, "syncer: get unit count\n");
		return syncer->GetUnitCount();
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


/**
 * @brief returns the units internal mod name
 * @param int the units id number
 * @return const char* The units internal modname or NULL on error.
 *
 * This function returns the units internal mod name. For example it would return armck and not
 * Arm Construction kbot.
 */
DLL_EXPORT const char * __stdcall GetUnitName(int unit)
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
 * @brief returns The units human readable name
 * @param int The units id number
 * @return const char* The Units human readable name or NULL on error.
 *
 * This function returns the units human name. For example it would return Arm Construction kbot
 * and not armck.
 */
DLL_EXPORT const char * __stdcall GetFullUnitName(int unit)
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
 * @brief adds an archive to the VFS (Virtual File System)
 *
 * After this, the contents of the archive are available to other unitsync functions,
 * for example: OpenFileVFS, ReadFileVFS, FileSizeVFS, etc.
 */
DLL_EXPORT void __stdcall AddArchive(const char* name)
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
 * @brief adds an achive and all it's dependencies to the VFS
 *
 * After this, the contents of the archive are available to other unitsync functions,
 * for example: OpenFileVFS, ReadFileVFS, FileSizeVFS, etc.
 */
DLL_EXPORT void __stdcall AddAllArchives(const char* root)
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
 * @brief gets checksum of an archive
 *
 * This checksum depends only on the contents from the archive itself, and not
 * on the contents from dependencies of this archive (if any).
 */
DLL_EXPORT unsigned int __stdcall GetArchiveChecksum(const char* arname)
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
 * @brief gets the real path to the archive
 */
DLL_EXPORT const char* __stdcall GetArchivePath(const char* arname)
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


// Updated on every call to getmapcount
static vector<string> mapNames;


/**
 * @brief gets the number of maps available
 *
 * Call this before any of the map functions which take a map index as parameter.
 */
DLL_EXPORT int __stdcall GetMapCount()
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
 * @brief get the name of a map, e.g. "SmallDivide.smf"
 */
DLL_EXPORT const char* __stdcall GetMapName(int index)
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
 * @brief get map info
 * @param name name of the map, e.g. "SmallDivide.smf"
 * @param outInfo pointer to structure which is filled with map info
 * @param version this determines which fields of the MapInfo structure are filled
 * @return zero on failure, non-zero on success
 *
 * If version >= 1, then the author field is filled.
 */
DLL_EXPORT int __stdcall GetMapInfoEx(const char* name, MapInfo* outInfo, int version)
{
	try {
		return _GetMapInfoEx(name, outInfo, version);
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


/**
 * @brief get map info, equivalent to GetMapInfoEx(name, outInfo, 0)
 * @param name name of the map, e.g. "SmallDivide.smf"
 * @param outInfo pointer to structure which is filled with map info
 * @return zero on failure, non-zero on success
 */
DLL_EXPORT int __stdcall GetMapInfo(const char* name, MapInfo* outInfo)
{
	try {
		return _GetMapInfoEx(name, outInfo, 0);
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


static vector<string> mapArchives;

DLL_EXPORT int __stdcall GetMapArchiveCount(const char* mapName)
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

DLL_EXPORT const char* __stdcall GetMapArchiveName(int index)
{
	try {
		CheckInit();
		CheckBounds(index, mapArchives.size());

		return GetStr(mapArchives[index]);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}

DLL_EXPORT unsigned int __stdcall GetMapChecksum(int index)
{
	try {
		CheckInit();
		CheckBounds(index, mapNames.size());

		return archiveScanner->GetMapChecksum(mapNames[index]);
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}

DLL_EXPORT unsigned int __stdcall GetMapChecksumFromName(const char* mapName)
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
 * bitmap.
 *
 * An example usage would be GetMinimap("SmallDivide.smf", 2).
 * This would return a 16 bit packed RGB-565 256x256 (= 1024/2^2) bitmap.
 *
 * Be sure you've made a call to Init prior to using this.
 */
DLL_EXPORT void* __stdcall GetMinimap(const char* filename, int miplevel)
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
 * @return 1 when the infomap was found with a nonzero size, 0 on error.
 * @see GetInfoMap
 */
DLL_EXPORT int __stdcall GetInfoMapSize(const char* filename, const char* name, int* width, int* height)
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
 * @return 1 if the infomap was succesfully extracted (and optionally converted),
 * or 0 on error (map wasn't found, infomap wasn't found, or typeHint could not
 * be honoured.)
 *
 * This function extracts an infomap from a map. This can currently be one of:
 * "height", "metal", "grass", "type". The heightmap is natively in 16 bits per
 * pixel, the others are in 8 bits pixel. Using typeHint one can give a hint to
 * this function to convert from one format to another. Currently only the
 * conversion from 16 bpp to 8 bpp is implemented.
 */
DLL_EXPORT int __stdcall GetInfoMap(const char* filename, const char* name, void* data, int typeHint)
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
 * @brief Retrieves the name of this mod
 * @return int The number of mods
 *
 * Returns the name of the mod usually found in modinfo.tdf.
 * Be sure you've made calls to Init and GetPrimaryModCount prior to using this.
 */
DLL_EXPORT int __stdcall GetPrimaryModCount()
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
 * @param index in The mods index/id
 * @return const char* The mods name
 *
 * Returns the name of the mod usually found in modinfo.tdf.
 * Be sure you've made calls to Init and GetPrimaryModCount prior to using this.
 */
DLL_EXPORT const char* __stdcall GetPrimaryModName(int index)
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
 * @param index in The mods index/id
 * @return const char* The mods abbrieviated name
 *
 * Returns the shortened name of the mod usually found in modinfo.tdf.
 * Be sure you've made calls to Init and GetPrimaryModCount prior to using this.
 */
DLL_EXPORT const char* __stdcall GetPrimaryModShortName(int index)
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
 * @param index in The mods index/id
 * @return const char* The mods version string
 *
 * Returns value of the mutator tag for the specified mod usually found in modinfo.tdf.
 * Be sure you've made calls to Init and GetPrimaryModCount prior to using this.
 */
DLL_EXPORT const char* __stdcall GetPrimaryModVersion(int index)
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
 * @param index in The mods index/id
 * @return const char* The mods mutator name
 *
 * Returns value of the mutator tag for the specified mod usually found in modinfo.tdf.
 * Be sure you've made calls to Init and GetPrimaryModCount prior to using this.
 */
DLL_EXPORT const char* __stdcall GetPrimaryModMutator(int index)
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
 * @param index in The mods index/id
 * @return const char* The mods game
 *
 * Returns the name of the game this mod belongs to usually found in modinfo.tdf.
 * Be sure you've made calls to Init and GetPrimaryModCount prior to using this.
 */
DLL_EXPORT const char* __stdcall GetPrimaryModGame(int index)
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
 * @param index in The mods index/id
 * @return const char* The mods abbrieviated game name
 *
 * Returns the abbrieviated name of the game this mod belongs to usually found in modinfo.tdf.
 * Be sure you've made calls to Init and GetPrimaryModCount prior to using this.
 */
DLL_EXPORT const char* __stdcall GetPrimaryModShortGame(int index)
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
 * @param index in The mods index/id
 * @return const char* The mods description
 *
 * Returns a description for the specified mod usually found in modinfo.tdf.
 * Be sure you've made calls to Init and GetPrimaryModCount prior to using this.
 */
DLL_EXPORT const char* __stdcall GetPrimaryModDescription(int index)
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


DLL_EXPORT const char* __stdcall GetPrimaryModArchive(int index)
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
 * @param index int The index of the mod
 * @return the number of archives this mod depends on.
 *
 * This is used to get the entire list of archives that a mod requires.
 * Call GetPrimaryModArchiveCount() with selected mod first to get number of
 * archives, and then use GetPrimaryModArchiveList() for 0 to count-1 to get the
 * name of each archive.
 */
DLL_EXPORT int __stdcall GetPrimaryModArchiveCount(int index)
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
 * @param arnr The mod's archive index/id.
 * @return the name of the archive
 * @see GetPrimaryModArchiveCount()
 */
DLL_EXPORT const char* __stdcall GetPrimaryModArchiveList(int arnr)
{
	try {
		CheckInit();
		CheckBounds(arnr, primaryArchives.size());

		logOutput.Print(LOG_UNITSYNC, "primary mod archive list: %s\n", primaryArchives[arnr].c_str());
		return GetStr(primaryArchives[arnr]);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}

DLL_EXPORT int __stdcall GetPrimaryModIndex(const char* name)
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

DLL_EXPORT unsigned int __stdcall GetPrimaryModChecksum(int index)
{
	try {
		CheckInit();
		CheckBounds(index, modData.size());

		return archiveScanner->GetModChecksum(GetPrimaryModArchive(index));
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}

DLL_EXPORT unsigned int __stdcall GetPrimaryModChecksumFromName(const char* name)
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

DLL_EXPORT int __stdcall GetSideCount()
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


DLL_EXPORT const char* __stdcall GetSideName(int side)
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


DLL_EXPORT const char* __stdcall GetSideStartUnit(int side)
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

struct LuaAIData {
	string name;
	string desc;
};


vector<LuaAIData> luaAIOptions;


static void GetLuaAIOptions()
{
	luaAIOptions.clear();

	LuaParser luaParser("LuaAI.lua", SPRING_VFS_MOD_BASE, SPRING_VFS_MOD_BASE);
	if (!luaParser.Execute()) {
		throw content_error("luaParser.Execute() failed: " + luaParser.GetErrorLog());
	}

	const LuaTable root = luaParser.GetRoot();
	if (!root.IsValid()) {
		throw content_error("root table invalid");
	}

	for (int i = 1; root.KeyExists(i); i++) {
		LuaAIData aiData;

		// string format
		aiData.name = root.GetString(i, "");
		if (!aiData.name.empty()) {
			aiData.desc = aiData.name;
			luaAIOptions.push_back(aiData);
			continue;
		}

		// table format  (name & desc)
		const LuaTable& optTbl = root.SubTable(i);
		if (!optTbl.IsValid()) {
			continue;
		}
		aiData.name = optTbl.GetString("name", "");
		if (aiData.name.empty()) {
			continue;
		}
		aiData.desc = optTbl.GetString("desc", aiData.name);
		luaAIOptions.push_back(aiData);
	}
}


DLL_EXPORT int __stdcall GetLuaAICount()
{
	try {
		CheckInit();

		GetLuaAIOptions();
		return luaAIOptions.size();
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


DLL_EXPORT const char* __stdcall GetLuaAIName(int aiIndex)
{
	try {
		CheckInit();
		CheckBounds(aiIndex, luaAIOptions.size());

		return GetStr(luaAIOptions[aiIndex].name);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


DLL_EXPORT const char* __stdcall GetLuaAIDesc(int aiIndex)
{
	try {
		CheckInit();
		CheckBounds(aiIndex, luaAIOptions.size());

		return GetStr(luaAIOptions[aiIndex].desc);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


//////////////////////////
//////////////////////////

static const char* badKeyChars = " =;\r\n\t";


struct ListItem {
	string key;
	string name;
	string desc;
};


struct Option {
	Option() : typeCode(opt_error) {}

	string key;
	string name;
	string desc;
	string section;
	string style;

	string type; // "bool", "number", "string", "list", "section"

	OptionType typeCode;

	bool   boolDef;

	float  numberDef;
	float  numberMin;
	float  numberMax;
	float  numberStep; // aligned to numberDef

	string stringDef;
	int    stringMaxLen;

	string listDef;
	vector<ListItem> list;
};


static vector<Option> options;
static set<string> optionsSet;


static bool ParseOption(const LuaTable& root, int index, Option& opt)
{
	const LuaTable& optTbl = root.SubTable(index);
	if (!optTbl.IsValid()) {
		logOutput.Print(LOG_UNITSYNC, "ParseOption: subtable %d invalid", index);
		return false;
	}

	// common options properties
	opt.key = optTbl.GetString("key", "");
	if (opt.key.empty() || (opt.key.find_first_of(badKeyChars) != string::npos)) {
		logOutput.Print(LOG_UNITSYNC, "ParseOption: empty key or key contains bad characters");
		return false;
	}
	opt.key = StringToLower(opt.key);
	if (optionsSet.find(opt.key) != optionsSet.end()) {
		logOutput.Print(LOG_UNITSYNC, "ParseOption: key %s exists already", opt.key.c_str());
		return false;
	}
	opt.name = optTbl.GetString("name", opt.key);
	if (opt.name.empty()) {
		logOutput.Print(LOG_UNITSYNC, "ParseOption: %s: empty name", opt.key.c_str());
		return false;
	}
	opt.desc = optTbl.GetString("desc", opt.name);

	opt.section = optTbl.GetString("section", "");
	opt.style = optTbl.GetString("style", "");

	opt.type = optTbl.GetString("type", "");
	opt.type = StringToLower(opt.type);

	// option type specific properties
	if (opt.type == "bool") {
		opt.typeCode = opt_bool;
		opt.boolDef = optTbl.GetBool("def", false);
	}
	else if (opt.type == "number") {
		opt.typeCode = opt_number;
		opt.numberDef  = optTbl.GetFloat("def",  0.0f);
		opt.numberMin  = optTbl.GetFloat("min",  -1.0e30f);
		opt.numberMax  = optTbl.GetFloat("max",  +1.0e30f);
		opt.numberStep = optTbl.GetFloat("step", 0.0f);
	}
	else if (opt.type == "string") {
		opt.typeCode = opt_string;
		opt.stringDef    = optTbl.GetString("def", "");
		opt.stringMaxLen = optTbl.GetInt("maxlen", 0);
	}
	else if (opt.type == "list") {
		opt.typeCode = opt_list;

		const LuaTable& listTbl = optTbl.SubTable("items");
		if (!listTbl.IsValid()) {
			logOutput.Print(LOG_UNITSYNC, "ParseOption: %s: subtable items invalid", opt.key.c_str());
			return false;
		}

		for (int i = 1; listTbl.KeyExists(i); i++) {
			ListItem item;

			// string format
			item.key = listTbl.GetString(i, "");
			if (!item.key.empty() &&
			    (item.key.find_first_of(badKeyChars) == string::npos)) {
				item.name = item.key;
				item.desc = item.name;
				opt.list.push_back(item);
				continue;
			}

			// table format  (name & desc)
			const LuaTable& itemTbl = listTbl.SubTable(i);
			if (!itemTbl.IsValid()) {
				logOutput.Print(LOG_UNITSYNC, "ParseOption: %s: subtable %d of subtable items invalid", opt.key.c_str(), i);
				break;
			}
			item.key = itemTbl.GetString("key", "");
			if (item.key.empty() || (item.key.find_first_of(badKeyChars) != string::npos)) {
				logOutput.Print(LOG_UNITSYNC, "ParseOption: %s: empty key or key contains bad characters", opt.key.c_str());
				return false;
			}
			item.key = StringToLower(item.key);
			item.name = itemTbl.GetString("name", item.key);
			if (item.name.empty()) {
				logOutput.Print(LOG_UNITSYNC, "ParseOption: %s: empty name", opt.key.c_str());
				return false;
			}
			item.desc = itemTbl.GetString("desc", item.name);
			opt.list.push_back(item);
		}

		if (opt.list.size() <= 0) {
			logOutput.Print(LOG_UNITSYNC, "ParseOption: %s: empty list", opt.key.c_str());
			return false; // no empty lists
		}

		opt.listDef = optTbl.GetString("def", opt.list[0].name);
	}
	else if (opt.type == "section") {
		opt.typeCode = opt_section;
	}
	else {
		logOutput.Print(LOG_UNITSYNC, "ParseOption: %s: unknown type %s", opt.key.c_str(), opt.type.c_str());
		return false; // unknown type
	}

	optionsSet.insert(opt.key);

	return true;
}


static void ParseOptions(const string& fileName,
                         const string& fileModes,
                         const string& accessModes,
                         const string& mapName = "")
{
	LuaParser luaParser(fileName, fileModes, accessModes);

	const string configName = MapParser::GetMapConfigName(mapName);

	if (!mapName.empty() && !configName.empty()) {
		luaParser.GetTable("Map");
		luaParser.AddString("fileName", mapName);
		luaParser.AddString("fullName", "maps/" + mapName);
		luaParser.AddString("configFile", configName);
		luaParser.EndTable();
	}

	if (!luaParser.Execute()) {
		throw content_error("luaParser.Execute() failed: " + luaParser.GetErrorLog());
	}

	const LuaTable root = luaParser.GetRoot();
	if (!root.IsValid()) {
		throw content_error("root table invalid");
	}

	for (int index = 1; root.KeyExists(index); index++) {
		Option opt;
		if (ParseOption(root, index, opt)) {
			options.push_back(opt);
		}
	}
};


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


DLL_EXPORT int __stdcall GetMapOptionCount(const char* name)
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


DLL_EXPORT int __stdcall GetModOptionCount()
{
	try {
		CheckInit();

		options.clear();
		optionsSet.clear();

		// EngineOptions must be read first, so accidentally "overloading" engine
		// options with mod options with identical names is not possible.
		ParseOptions("EngineOptions.lua", SPRING_VFS_MOD_BASE, SPRING_VFS_MOD_BASE);
		ParseOptions("ModOptions.lua", SPRING_VFS_MOD, SPRING_VFS_MOD);

		optionsSet.clear();

		return options.size();
	}
	UNITSYNC_CATCH_BLOCKS;

	options.clear();
	optionsSet.clear();

	return 0;
}


// Common Parameters

DLL_EXPORT const char* __stdcall GetOptionKey(int optIndex)
{
	try {
		CheckOptionIndex(optIndex);
		return GetStr(options[optIndex].key);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


DLL_EXPORT const char* __stdcall GetOptionName(int optIndex)
{
	try {
		CheckOptionIndex(optIndex);
		return GetStr(options[optIndex].name);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}

DLL_EXPORT const char* __stdcall GetOptionSection(int optIndex)
{
	try {
		CheckOptionIndex(optIndex);
		return GetStr(options[optIndex].section);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}

DLL_EXPORT const char* __stdcall GetOptionStyle(int optIndex)
{
	try {
		CheckOptionIndex(optIndex);
		return GetStr(options[optIndex].style);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}

DLL_EXPORT const char* __stdcall GetOptionDesc(int optIndex)
{
	try {
		CheckOptionIndex(optIndex);
		return GetStr(options[optIndex].desc);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


DLL_EXPORT int __stdcall GetOptionType(int optIndex)
{
	try {
		CheckOptionIndex(optIndex);
		return options[optIndex].typeCode;
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


// Bool Options

DLL_EXPORT int __stdcall GetOptionBoolDef(int optIndex)
{
	try {
		CheckOptionType(optIndex, opt_bool);
		return options[optIndex].boolDef ? 1 : 0;
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


// Number Options

DLL_EXPORT float __stdcall GetOptionNumberDef(int optIndex)
{
	try {
		CheckOptionType(optIndex, opt_number);
		return options[optIndex].numberDef;
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0.0f;
}


DLL_EXPORT float __stdcall GetOptionNumberMin(int optIndex)
{
	try {
		CheckOptionType(optIndex, opt_number);
		return options[optIndex].numberMin;
	}
	UNITSYNC_CATCH_BLOCKS;
	return -1.0e30f; // FIXME ?
}


DLL_EXPORT float __stdcall GetOptionNumberMax(int optIndex)
{
	try {
		CheckOptionType(optIndex, opt_number);
		return options[optIndex].numberMax;
	}
	UNITSYNC_CATCH_BLOCKS;
	return +1.0e30f; // FIXME ?
}


DLL_EXPORT float __stdcall GetOptionNumberStep(int optIndex)
{
	try {
		CheckOptionType(optIndex, opt_number);
		return options[optIndex].numberStep;
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0.0f;
}


// String Options

DLL_EXPORT const char* __stdcall GetOptionStringDef(int optIndex)
{
	try {
		CheckOptionType(optIndex, opt_string);
		return GetStr(options[optIndex].stringDef);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


DLL_EXPORT int __stdcall GetOptionStringMaxLen(int optIndex)
{
	try {
		CheckOptionType(optIndex, opt_string);
		return options[optIndex].stringMaxLen;
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


// List Options

DLL_EXPORT int __stdcall GetOptionListCount(int optIndex)
{
	try {
		CheckOptionType(optIndex, opt_list);
		return options[optIndex].list.size();
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}


DLL_EXPORT const char* __stdcall GetOptionListDef(int optIndex)
{
	try {
		CheckOptionType(optIndex, opt_list);
		return GetStr(options[optIndex].listDef);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


DLL_EXPORT const char* __stdcall GetOptionListItemKey(int optIndex, int itemIndex)
{
	try {
		CheckOptionType(optIndex, opt_list);
		const vector<ListItem>& list = options[optIndex].list;
		CheckBounds(itemIndex, list.size());
		return GetStr(list[itemIndex].key);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


DLL_EXPORT const char* __stdcall GetOptionListItemName(int optIndex, int itemIndex)
{
	try {
		CheckOptionType(optIndex, opt_list);
		const vector<ListItem>& list = options[optIndex].list;
		CheckBounds(itemIndex, list.size());
		return GetStr(list[itemIndex].name);
	}
	UNITSYNC_CATCH_BLOCKS;
	return NULL;
}


DLL_EXPORT const char* __stdcall GetOptionListItemDesc(int optIndex, int itemIndex)
{
	try {
		CheckOptionType(optIndex, opt_list);
		const vector<ListItem>& list = options[optIndex].list;
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


// A return value of 0 means that any map can be selected
// Map names should be complete  (including the .smf or .sm3 extension)
DLL_EXPORT int __stdcall GetModValidMapCount()
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


DLL_EXPORT const char* __stdcall GetModValidMap(int index)
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

// Map the wanted archives into the VFS with AddArchive before using these functions

static map<int, CFileHandler*> openFiles;
static int nextFile = 0;
static vector<string> curFindFiles;

static void CheckFileHandle(int handle)
{
	CheckInit();

	if (openFiles.find(handle) == openFiles.end())
		throw content_error("Unregistered handle. Pass a handle returned by OpenFileVFS.");
}

DLL_EXPORT int __stdcall OpenFileVFS(const char* name)
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

DLL_EXPORT void __stdcall CloseFileVFS(int handle)
{
	try {
		CheckFileHandle(handle);

		logOutput.Print(LOG_UNITSYNC, "closefilevfs: %d\n", handle);
		delete openFiles[handle];
		openFiles.erase(handle);
	}
	UNITSYNC_CATCH_BLOCKS;
}

DLL_EXPORT void __stdcall ReadFileVFS(int handle, void* buf, int length)
{
	try {
		CheckFileHandle(handle);
		CheckNull(buf);
		CheckPositive(length);

		logOutput.Print(LOG_UNITSYNC, "readfilevfs: %d\n", handle);
		CFileHandler* fh = openFiles[handle];
		fh->Read(buf, length);
	}
	UNITSYNC_CATCH_BLOCKS;
}

DLL_EXPORT int __stdcall FileSizeVFS(int handle)
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


// Does not currently support more than one call at a time (a new call to initfind destroys data from previous ones)
// pass the returned handle to findfiles to get the results
DLL_EXPORT int __stdcall InitFindVFS(const char* pattern)
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

// Does not currently support more than one call at a time (a new call to initfind destroys data from previous ones)
// pass the returned handle to findfiles to get the results
DLL_EXPORT int __stdcall InitDirListVFS(const char* path, const char* pattern, const char* modes)
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

// Does not currently support more than one call at a time (a new call to initfind destroys data from previous ones)
// pass the returned handle to findfiles to get the results
DLL_EXPORT int __stdcall InitSubDirsVFS(const char* path, const char* pattern, const char* modes)
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
DLL_EXPORT int __stdcall FindFilesVFS(int handle, char* nameBuf, int size)
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

// returns 0 on error, a handle otherwise
DLL_EXPORT int __stdcall OpenArchive(const char* name)
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

// returns 0 on error, a handle otherwise
DLL_EXPORT int __stdcall OpenArchiveType(const char* name, const char* type)
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

DLL_EXPORT void __stdcall CloseArchive(int archive)
{
	try {
		CheckArchiveHandle(archive);

		delete openArchives[archive];
		openArchives.erase(archive);
	}
	UNITSYNC_CATCH_BLOCKS;
}

DLL_EXPORT int __stdcall FindFilesArchive(int archive, int cur, char* nameBuf, int* size)
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

DLL_EXPORT int __stdcall OpenArchiveFile(int archive, const char* name)
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

DLL_EXPORT int __stdcall ReadArchiveFile(int archive, int handle, void* buffer, int numBytes)
{
	try {
		CheckArchiveHandle(archive);
		CheckNull(buffer);
		CheckPositive(numBytes);

		CArchiveBase* a = openArchives[archive];
		return a->ReadFile(handle, buffer, numBytes);
	}
	UNITSYNC_CATCH_BLOCKS;
	return 0;
}

DLL_EXPORT void __stdcall CloseArchiveFile(int archive, int handle)
{
	try {
		CheckArchiveHandle(archive);

		CArchiveBase* a = openArchives[archive];
		a->CloseFile(handle);
	}
	UNITSYNC_CATCH_BLOCKS;
}

DLL_EXPORT int __stdcall SizeArchiveFile(int archive, int handle)
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
		sprintf(strBuf, "Increase STRBUF_SIZE (needs %ld bytes)", str.length() + 1);
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

/**
 * @brief get string from Spring configuration
 * @param name name of key to get
 * @param defvalue default string value to use if key is not found, may not be NULL
 * @return string value
 */
DLL_EXPORT const char* __stdcall GetSpringConfigString( const char* name, const char* defvalue )
{
	try {
		string res = configHandler.GetString( name, defvalue );
		return GetStr(res);
	}
	UNITSYNC_CATCH_BLOCKS;
	return defvalue;
}

/**
 * @brief get integer from Spring configuration
 * @param name name of key to get
 * @param defvalue default integer value to use if key is not found
 * @return integer value
 */
DLL_EXPORT int __stdcall GetSpringConfigInt( const char* name, const int defvalue )
{
	try {
		return configHandler.GetInt( name, defvalue );
	}
	UNITSYNC_CATCH_BLOCKS;
	return defvalue;
}

/**
 * @brief get float from Spring configuration
 * @param name name of key to get
 * @param defvalue default float value to use if key is not found
 * @return float value
 */
DLL_EXPORT float __stdcall GetSpringConfigFloat( const char* name, const float defvalue )
{
	try {
		return configHandler.GetFloat( name, defvalue );
	}
	UNITSYNC_CATCH_BLOCKS;
	return defvalue;
}

/**
 * @brief set string in Spring configuration
 * @param name name of key to set
 * @param value string value to set
 */
DLL_EXPORT void __stdcall SetSpringConfigString(const char* name, const char* value)
{
	try {
		configHandler.SetString( name, value );
	}
	UNITSYNC_CATCH_BLOCKS;
}

/**
 * @brief set integer in Spring configuration
 * @param name name of key to set
 * @param value integer value to set
 */
DLL_EXPORT void __stdcall SetSpringConfigInt(const char* name, const int value)
{
	try {
		configHandler.SetInt( name, value );
	}
	UNITSYNC_CATCH_BLOCKS;
}

/**
 * @brief set float in Spring configuration
 * @param name name of key to set
 * @param value float value to set
 */
DLL_EXPORT void __stdcall SetSpringConfigFloat(const char* name, const float value)
{
	try {
		configHandler.SetFloat( name, value );
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
			Message(msg.c_str());
		}
};

#define DEPRECATED \
	static CMessageOnce msg; \
	msg(string(__FUNCTION__) + ": deprecated unitsync function called, please update your lobby client"); \
	SetLastError("deprecated unitsync function called")


// deprecated 2008/10
DLL_EXPORT const char * __stdcall GetCurrentList()
{
	DEPRECATED;
	return NULL;
}

// deprecated 2008/10
DLL_EXPORT void __stdcall AddClient(int id, const char *unitList)
{
	DEPRECATED;
}

// deprecated 2008/10
DLL_EXPORT void __stdcall RemoveClient(int id)
{
	DEPRECATED;
}

// deprecated 2008/10
DLL_EXPORT const char * __stdcall GetClientDiff(int id)
{
	DEPRECATED;
	return NULL;
}

// deprecated 2008/10
DLL_EXPORT void __stdcall InstallClientDiff(const char *diff)
{
	DEPRECATED;
}

// deprecated 2008/10
DLL_EXPORT int __stdcall IsUnitDisabled(int unit)
{
	DEPRECATED;
	return 0;
}

// deprecated 2008/10
DLL_EXPORT int __stdcall IsUnitDisabledByClient(int unit, int clientId)
{
	DEPRECATED;
	return 0;
}
