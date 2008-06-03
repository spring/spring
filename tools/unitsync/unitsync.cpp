#include "StdAfx.h"
#include "unitsync.h"

#include "LuaInclude.h"
#include "FileSystem/ArchiveFactory.h"
#include "FileSystem/ArchiveScanner.h"
#include "FileSystem/FileHandler.h"
#include "FileSystem/VFSHandler.h"
#include "Game/GameVersion.h"
#include "Lua/LuaParser.h"
#include "Map/MapParser.h"
#include "Map/SMF/mapfile.h"
#include "Platform/ConfigHandler.h"
#include "Platform/FileSystem.h"
#include "Rendering/Textures/Bitmap.h"

#include "Syncer.h"
#include "SyncServer.h"
#include "unitsyncLogOutput.h"

#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <cstdio>
#include <cstdarg>

#define ASSERT(condition, message) \
	do { \
		if (!(condition)) { \
			char buf[256]; \
			sprintf(buf, "%s:%d: %s", __FILE__, __LINE__, message); \
			MessageBox(0, buf, "Unitsync assertion failed", MB_OK); \
		} \
		assert(condition); /* just crash after the error in release mode */ \
	} while(0)

//This means that the DLL can only support one instance. Don't think this should be a problem.
static CSyncer *syncer = NULL;

// I'd rather not include globalstuff
#define SQUARE_SIZE 8

// And the following makes the hpihandler happy
CLogOutput logOutput;

CLogOutput::CLogOutput()
{
	file = fopen("unitsync.log", "at");
	ASSERT(file != NULL, "couldn't open logfile\n");
	setbuf(file, NULL);
}

CLogOutput::~CLogOutput()
{
	fclose(file);
}

void CLogOutput::Print (const string& text)
{
	if (*text.rbegin() != '\n')
		fprintf(file, "%s\n", text.c_str());
	else
		fprintf(file, "%s", text.c_str());
	fflush(file);
}

void CLogOutput::Print(const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(file, fmt, ap);
	va_end(ap);
	if (fmt[strlen(fmt)-1] != '\n') {
		fputc('\n', file);
	}
}

void ErrorMessageBox(const char *msg, const char *capt, unsigned int) {
	MessageBox(0,msg,capt,MB_OK);
}


#ifdef WIN32
BOOL __stdcall DllMain(HINSTANCE hInst,
                       DWORD dwReason,
                       LPVOID lpReserved) {
	logOutput.Print("----\nunitsync loaded\n");
	return TRUE;
}
#endif


// Helper class for loading a map archive temporarily

class ScopedMapLoader {
	public:
		ScopedMapLoader(const string& mapName) : oldHandler(hpiHandler) {
			CFileHandler f("maps/" + mapName);
			if (f.FileExists()) {
				return;
			}

			hpiHandler = new CVFSHandler();

			const vector<string> ars = archiveScanner->GetArchivesForMap(mapName);
			vector<string>::const_iterator it;
			for (it = ars.begin(); it != ars.end(); ++it) {
				hpiHandler->AddArchive(*it, false);
			}
		}

		~ScopedMapLoader() {
			if (hpiHandler != oldHandler) {
				delete hpiHandler;
				hpiHandler = oldHandler;
			}
		}

	private:
		CVFSHandler* oldHandler;
};


/**
 * @brief returns the version fo spring this was compiled with
 *
 * Returns a const char* string specifying the version of spring used to build this library with.
 * It was added to aid in lobby creation, where checks for updates to spring occur.
 */
DLL_EXPORT const char* __stdcall GetSpringVersion()
{
	return VERSION_STRING;
}

/**
 * @brief Creates a messagebox with said message
 * @param p_szMessage const char* string holding the message
 *
 * Creates a messagebox with the title "Message from DLL", an OK button, and the specified message
 */
DLL_EXPORT void __stdcall Message(const char* p_szMessage)
{
	MessageBox(NULL, p_szMessage, "Message from DLL", MB_OK);
}

DLL_EXPORT void __stdcall UnInit()
{
	FileSystemHandler::Cleanup();

	if ( syncer )
	{
		delete syncer;
		syncer = 0;
		logOutput.Print("unitsync deinitialized\n----\n");
	}

	ConfigHandler::Deallocate();
}

DLL_EXPORT int __stdcall Init(bool isServer, int id)
{
	UnInit();
	logOutput.Print("unitsync initialized\n");

	try {
		// first call to GetInstance() initializes the VFS
		FileSystemHandler::Initialize(false);

		if (isServer) {
			logOutput.Print("unitsync: hosting\n");
			syncer = new CSyncServer(id);
		}
		else {
			logOutput.Print("unitsync: joining\n");
			syncer = new CSyncer(id);
		}
	} catch (const std::exception& e) {
		Message(e.what());
		return 0;
	}

	return 1;
}

/* @brief Deprecated function, DO NOT USE! Temporarily readded to make buildbot'
 * generated unitsync.dll work with TASClient.exe from Spring 0.74b3.
 * FIXME: Should be removed just after 0.75 release!
 */
DLL_EXPORT int __stdcall InitArchiveScanner(void)
{
	return 1;
}

/**
 * @brief process another unit and return how many are left to process
 * @return int The number of unprocessed units to be handled
 *
 * Call this function repeatedly untill it returns 0 before calling any other function related to units.
 */
DLL_EXPORT int __stdcall ProcessUnits(void)
{
	logOutput.Print("syncer: process units\n");
	return syncer->ProcessUnits();
}

/**
 * @brief process another unit and return how many are left to process without checksumming
 * @return int The number of unprocessed units to be handled
 *
 * Call this function repeatedly untill it returns 0 before calling any other function related to units.
 * This function performs the same operations as ProcessUnits() but it does not generate checksums.
 */
DLL_EXPORT int __stdcall ProcessUnitsNoChecksum(void)
{
	logOutput.Print("syncer: process units\n");
	return syncer->ProcessUnits(false);
}

DLL_EXPORT const char * __stdcall GetCurrentList()
{
	logOutput.Print("syncer: get current list\n");
	string tmp = syncer->GetCurrentList();
/*	int tmpLen = (int)tmp.length();

	if (tmpLen > *bufLen) {
		*bufLen = tmpLen;
		return -1;
	}

	strcpy(buffer, tmp.c_str());
	buffer[tmpLen] = 0;
	*bufLen = tmpLen;

	return tmpLen; */

	return GetStr(tmp);
}

DLL_EXPORT void __stdcall AddClient(int id, const char *unitList)
{
	logOutput.Print("syncer: add client\n");
	((CSyncServer *)syncer)->AddClient(id, unitList);
}

DLL_EXPORT void __stdcall RemoveClient(int id)
{
	logOutput.Print("syncer: remove client\n");
	((CSyncServer *)syncer)->RemoveClient(id);
}

DLL_EXPORT const char * __stdcall GetClientDiff(int id)
{
	logOutput.Print("syncer: get client diff\n");
	string tmp = ((CSyncServer *)syncer)->GetClientDiff(id);
	return GetStr(tmp);
}

DLL_EXPORT void __stdcall InstallClientDiff(const char *diff)
{
	logOutput.Print("syncer: install client diff\n");
	syncer->InstallClientDiff(diff);
}

/**
 * @brief returns the number of units
 * @return int number of units processed and available
 *
 * Will return the number of units. Remember to call processUnits() beforehand untill it returns 0
 * As ProcessUnits is called the number of processed units goes up, and so will the value returned
 * by this function.
 *
 * while(processUnits()){}
 * int unit_number = GetUnitCount();
 */
DLL_EXPORT int __stdcall GetUnitCount()
{
	logOutput.Print("syncer: get unit count\n");
	return syncer->GetUnitCount();
}

/**
 * @brief returns the units internal mod name
 * @param int the units id number
 * @return const char* The units internal modname
 *
 * This function returns the units internal mod name. For example it would return armck and not
 * Arm Construction kbot.
 */
DLL_EXPORT const char * __stdcall GetUnitName(int unit)
{
	logOutput.Print("syncer: get unit %d name\n", unit);
	string tmp = syncer->GetUnitName(unit);
	return GetStr(tmp);
}

/**
 * @brief returns The units human readable name
 * @param int The units id number
 * @return const char* The Units human readable name
 *
 * This function returns the units human name. For example it would return Arm Construction kbot
 * and not armck.
 */
DLL_EXPORT const char * __stdcall GetFullUnitName(int unit)
{
	logOutput.Print("syncer: get full unit %d name\n", unit);
	string tmp = syncer->GetFullUnitName(unit);
	return GetStr(tmp);
}

DLL_EXPORT int __stdcall IsUnitDisabled(int unit)
{
	logOutput.Print("syncer: is unit %d disabled\n", unit);
	if (syncer->IsUnitDisabled(unit))
		return 1;
	else
		return 0;
}

DLL_EXPORT int __stdcall IsUnitDisabledByClient(int unit, int clientId)
{
	logOutput.Print("syncer: is unit %d disabled by client %d\n", unit, clientId);
	if (syncer->IsUnitDisabledByClient(unit, clientId))
		return 1;
	else
		return 0;
}

//////////////////////////
//////////////////////////

DLL_EXPORT void __stdcall AddArchive(const char* name)
{
	ASSERT(archiveScanner && hpiHandler, "Call InitArchiveScanner before AddArchive.");
	ASSERT(name && *name, "Don't pass a NULL pointer or an empty string to AddArchive.");
	hpiHandler->AddArchive(name, false);
}

DLL_EXPORT void __stdcall AddAllArchives(const char* root)
{
	ASSERT(archiveScanner && hpiHandler, "Call InitArchiveScanner before AddAllArchives.");
	ASSERT(root && *root, "Don't pass a NULL pointer or an empty string to AddAllArchives.");
	vector<string> ars = archiveScanner->GetArchives(root);
//	Message(root);
	for (vector<string>::iterator i = ars.begin(); i != ars.end(); ++i) {
		logOutput.Print("adding archive: %s\n", i->c_str());
		hpiHandler->AddArchive(*i, false);
	}
	// always load springcontent.sdz
	logOutput.Print("adding archive: base/springcontent.sdz\n");
	hpiHandler->AddArchive("base/springcontent.sdz", false);
}

DLL_EXPORT unsigned int __stdcall GetArchiveChecksum(const char* arname)
{
	ASSERT(archiveScanner && hpiHandler, "Call InitArchiveScanner before GetArchiveChecksum.");
	ASSERT(arname && *arname, "Don't pass a NULL pointer or an empty string to GetArchiveChecksum.");
	logOutput.Print("archive checksum: %s\n", arname);
	return archiveScanner->GetArchiveChecksum(arname);
}

DLL_EXPORT const char* __stdcall GetArchivePath(const char* arname)
{
	ASSERT(archiveScanner && hpiHandler, "Call InitArchiveScanner before GetArchivePath.");
	ASSERT(arname && *arname, "Don't pass a NULL pointer or an empty string to GetArchivePath.");
	logOutput.Print("archive path: %s\n", arname);
	return GetStr(archiveScanner->GetArchivePath(arname));
}

// Updated on every call to getmapcount
static vector<string> mapNames;

DLL_EXPORT int __stdcall GetMapCount()
{
	ASSERT(archiveScanner && hpiHandler, "Call InitArchiveScanner before GetMapCount.");
	//vector<string> files = CFileHandler::FindFiles("{maps/*.smf,maps/*.sm3}");
	vector<string> files = CFileHandler::FindFiles("maps/", "{*.smf,*.sm3}");
	vector<string> ars = archiveScanner->GetMaps();
/*	vector<string> files2 = CFileHandler::FindFiles("maps/*.sm3");
	unsigned int nfiles=files.size();
	files.resize(files.size()+files2.size());
	copy(files2.begin(),files2.end(),files.begin()+nfiles);
*/
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

DLL_EXPORT const char* __stdcall GetMapName(int index)
{
	ASSERT(archiveScanner && hpiHandler, "Call InitArchiveScanner before GetMapName.");
	ASSERT((unsigned)index < mapNames.size(), "Array index out of bounds. Call GetMapCount before GetMapName.");
	return GetStr(mapNames[index]);
}


DLL_EXPORT int __stdcall GetMapInfoEx(const char* name, MapInfo* outInfo, int version)
{
	ASSERT(archiveScanner && hpiHandler, "Call InitArchiveScanner before GetMapInfo.");
	ASSERT(name && *name && outInfo, "Don't pass a NULL pointer or an empty string to GetMapInfo.");

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
			SMFHeader mh;
			string origName = name;
			mh.mapx = -1;
			{
				CFileHandler in("maps/" + origName);
				if (in.FileExists()) {
					in.Read(&mh, sizeof(mh));
				}
			}
			outInfo->width  = mh.mapx * SQUARE_SIZE;
			outInfo->height = mh.mapy * SQUARE_SIZE;
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
	if (err.length() > 0) {
		if (err.length() > 254) {
			err = err.substr(0, 254);
		}
		strcpy(outInfo->description, err.c_str());

		// Fill in stuff so tasclient won't crash
		outInfo->posCount = 0;
		if (version >= 1) {
			outInfo->author[0] = 0;
		}
		return 0;
	}

	const string desc = mapTable.GetString("description", "");
	strncpy(outInfo->description, desc.c_str(), 254);

	outInfo->tidalStrength   = mapTable.GetInt("tidalstrength", 0);
	outInfo->gravity         = mapTable.GetInt("gravity", 0);
	outInfo->extractorRadius = mapTable.GetInt("extractorradius", 0);
	outInfo->maxMetal        = mapTable.GetFloat("maxmetal", 0.0f);

	if (version >= 1) {
		const string author = mapTable.GetString("author", "");
		strncpy(outInfo->author, author.c_str(), 200);
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
	}

	outInfo->posCount = curTeam;

	return 1;
}


DLL_EXPORT int __stdcall GetMapInfo(const char* name, MapInfo* outInfo)
{
	return GetMapInfoEx(name, outInfo, 0);
}

static vector<string> mapArchives;

DLL_EXPORT int __stdcall GetMapArchiveCount(const char* mapName)
{
	ASSERT(archiveScanner && hpiHandler, "Call InitArchiveScanner before GetMapArchiveCount.");
	mapArchives = archiveScanner->GetArchivesForMap(mapName);
	return mapArchives.size();
}

DLL_EXPORT const char* __stdcall GetMapArchiveName(int index)
{
	ASSERT(archiveScanner && hpiHandler, "Call InitArchiveScanner before GetMapArchiveName.");
	ASSERT((unsigned)index < mapArchives.size(), "Array index out of bounds. Call GetMapArchiveCount before GetMapArchiveName.");
	return GetStr(mapArchives[index]);
}

DLL_EXPORT unsigned int __stdcall GetMapChecksum(int index)
{
	ASSERT(archiveScanner && hpiHandler, "Call InitArchiveScanner before GetMapChecksum.");
	ASSERT((unsigned)index < mapNames.size(), "Array index out of bounds. Call GetMapCount before GetMapChecksum.");
	return archiveScanner->GetMapChecksum(mapNames[index]);
}

DLL_EXPORT unsigned int __stdcall GetMapChecksumFromName(const char* mapName)
{
	ASSERT(archiveScanner && hpiHandler, "Call InitArchiveScanner before GetMapChecksumFromName.");
	return archiveScanner->GetMapChecksum(mapName);
}

#define RM	0x0000F800
#define GM  0x000007E0
#define BM  0x0000001F

#define RED_RGB565(x) ((x&RM)>>11)
#define GREEN_RGB565(x) ((x&GM)>>5)
#define BLUE_RGB565(x) (x&BM)
#define PACKRGB(r, g, b) (((r<<11)&RM) | ((g << 5)&GM) | (b&BM) )

// Used to return the image
char* imgbuf[1024*1024*2];

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

	for ( int i = 0; i < miplevel; i++ )
	{
		int size = ((mipsize+3)/4)*((mipsize+3)/4)*8;
		offset += size;
		mipsize >>= 1;
	}

	int size = ((mipsize+3)/4)*((mipsize+3)/4)*8;
	int numblocks = size/8;

	// Read the map data
	CFileHandler in("maps/" + mapName);
	char* buffer = (char*)malloc(size);
	bool done = false;
	if (in.FileExists()) {

		SMFHeader mh;
		in.Read(&mh, sizeof(mh));
		in.Seek(mh.minimapPtr + offset);
		in.Read(buffer, size);

		done = true;
	}
/*	ifstream inFile;
	inFile.open(filename, ios_base::binary);
	if ( !inFile.is_open() )
		return 0;
	SM2Header sm2;
	inFile.read(reinterpret_cast<char*>(&sm2), sizeof(sm2));
	inFile.seekg(sm2.minimapPtr+offset);

	char* buffer = (char*)malloc(size);
	inFile.read(buffer, size); */

	if (!done) {
		free(buffer);
		return 0;
	}

	// Do stuff

	//void* ret = malloc(mipsize*mipsize*2);
	void* ret = (void*)imgbuf;
	unsigned short* colors = (unsigned short*)ret;

	unsigned char* temp = (unsigned char*)buffer;

	for ( int i = 0; i < numblocks; i++ )
	{
		unsigned short color0 = (*(unsigned short*)&temp[0]);
		unsigned short color1 = (*(unsigned short*)&temp[2]);
		unsigned int bits = (*(unsigned int*)&temp[4]);

		for ( int a = 0; a < 4; a++ )
		{
			for ( int b = 0; b < 4; b++ )
			{
				int x = 4*(i % ((mipsize+3)/4))+b;
				int y = 4*(i / ((mipsize+3)/4))+a;
				unsigned char code = bits & 0x3;
				bits >>= 2;

				if ( color0 > color1 )
				{
					if ( code == 0 )
					{
						colors[y*mipsize+x] = color0;
					}
					else if ( code == 1 )
					{
						colors[y*mipsize+x] = color1;
					}
					else if ( code == 2 )
					{
						colors[y*mipsize+x] = PACKRGB((2*RED_RGB565(color0)+RED_RGB565(color1))/3, (2*GREEN_RGB565(color0)+GREEN_RGB565(color1))/3, (2*BLUE_RGB565(color0)+BLUE_RGB565(color1))/3);
					}
					else
					{
						colors[y*mipsize+x] = PACKRGB((2*RED_RGB565(color1)+RED_RGB565(color0))/3, (2*GREEN_RGB565(color1)+GREEN_RGB565(color0))/3, (2*BLUE_RGB565(color1)+BLUE_RGB565(color0))/3);
					}
				}
				else
				{
					if ( code == 0 )
					{
						colors[y*mipsize+x] = color0;
					}
					else if ( code == 1 )
					{
						colors[y*mipsize+x] = color1;
					}
					else if ( code == 2 )
					{
						colors[y*mipsize+x] = PACKRGB((RED_RGB565(color0)+RED_RGB565(color1))/2, (GREEN_RGB565(color0)+GREEN_RGB565(color1))/2, (BLUE_RGB565(color0)+BLUE_RGB565(color1))/2);
					}
					else
					{
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
 * Be sure you've made a calls to Init prior to using this.
 */
DLL_EXPORT void* __stdcall GetMinimap(const char* filename, int miplevel)
{
	ASSERT(archiveScanner && hpiHandler, "Call InitArchiveScanner before GetMinimap.");
	ASSERT(filename && *filename, "Don't pass a NULL pointer or an empty string to GetMinimap.");
	ASSERT(miplevel >= 0 && miplevel <= 8, "Miplevel must be between 0 and 8 (inclusive) in GetMinimap.");

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
	ASSERT(archiveScanner && hpiHandler, "Call InitArchiveScanner before GetPrimaryModCount.");
	modData = archiveScanner->GetPrimaryMods();
	return modData.size();
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
	ASSERT(archiveScanner && hpiHandler, "Call InitArchiveScanner before GetPrimaryModName.");
	ASSERT((unsigned)index < modData.size(), "Array index out of bounds. Call GetPrimaryModCount before GetPrimaryModName.");
	string x = modData[index].name;
	return GetStr(x);
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
	ASSERT(archiveScanner && hpiHandler, "Call InitArchiveScanner before GetPrimaryModShortName.");
	ASSERT((unsigned)index < modData.size(), "Array index out of bounds. Call GetPrimaryModCount before GetPrimaryModShortName.");
	string x = modData[index].shortName;
	return GetStr(x);
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
	ASSERT(archiveScanner && hpiHandler, "Call InitArchiveScanner before GetPrimaryModVersion.");
	ASSERT((unsigned)index < modData.size(), "Array index out of bounds. Call GetPrimaryModCount before GetPrimaryModMutator.");
	string x = modData[index].version;
	return GetStr(x);
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
	ASSERT(archiveScanner && hpiHandler, "Call InitArchiveScanner before GetPrimaryModMutator.");
	ASSERT((unsigned)index < modData.size(), "Array index out of bounds. Call GetPrimaryModCount before GetPrimaryModMutator.");
	string x = modData[index].mutator;
	return GetStr(x);
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
	ASSERT(archiveScanner && hpiHandler, "Call InitArchiveScanner before GetPrimaryModName.");
	ASSERT((unsigned)index < modData.size(), "Array index out of bounds. Call GetPrimaryModCount before GetPrimaryModName.");
	string x = modData[index].game;
	return GetStr(x);
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
	ASSERT(archiveScanner && hpiHandler, "Call InitArchiveScanner before GetPrimaryModShortGame.");
	ASSERT((unsigned)index < modData.size(), "Array index out of bounds. Call GetPrimaryModCount before GetPrimaryModShortGame.");
	string x = modData[index].shortGame;
	return GetStr(x);
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
	ASSERT(archiveScanner && hpiHandler, "Call InitArchiveScanner before GetPrimaryModDescription.");
	ASSERT((unsigned)index < modData.size(), "Array index out of bounds. Call GetPrimaryModCount before GetPrimaryModDescription.");
	string x = modData[index].description;
	return GetStr(x);
}


DLL_EXPORT const char* __stdcall GetPrimaryModArchive(int index)
{
	ASSERT(archiveScanner && hpiHandler, "Call InitArchiveScanner before GetPrimaryModArchive.");
	ASSERT((unsigned)index < modData.size(), "Array index out of bounds. Call GetPrimaryModCount before GetPrimaryModArchive.");
	return GetStr(modData[index].dependencies[0]);
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
	ASSERT(archiveScanner && hpiHandler, "Call InitArchiveScanner before GetPrimaryModArchiveCount.");
	ASSERT((unsigned)index < modData.size(), "Array index out of bounds. Call GetPrimaryModCount before GetPrimaryModArchiveCount.");
	primaryArchives = archiveScanner->GetArchives(modData[index].dependencies[0]);
	return primaryArchives.size();
}

/**
 * @brief Retrieves the name of the current mod's archive.
 * @param arnr The mod's archive index/id.
 * @return the name of the archive
 * @see GetPrimaryModArchiveCount()
 */
DLL_EXPORT const char* __stdcall GetPrimaryModArchiveList(int arnr)
{
	ASSERT(archiveScanner && hpiHandler, "Call InitArchiveScanner before GetPrimaryModArchiveList.");
	ASSERT((unsigned)arnr < primaryArchives.size(), "Array index out of bounds. Call GetPrimaryModArchiveCount before GetPrimaryModArchiveList.");
	logOutput.Print("primary mod archive list: %s\n", primaryArchives[arnr].c_str());
	return GetStr(primaryArchives[arnr]);
}

DLL_EXPORT int __stdcall GetPrimaryModIndex(const char* name)
{
	ASSERT(archiveScanner && hpiHandler, "Call InitArchiveScanner before GetPrimaryModIndex.");
	string n(name);
	for (unsigned i = 0; i < modData.size(); ++i) {
		if (modData[i].name == n)
			return i;
	}
	// if it returns -1, make sure you call GetPrimaryModCount before GetPrimaryModIndex.
	return -1;
}

DLL_EXPORT unsigned int __stdcall GetPrimaryModChecksum(int index)
{
	ASSERT(archiveScanner && hpiHandler, "Call InitArchiveScanner before GetPrimaryModChecksum.");
	ASSERT((unsigned)index < modData.size(), "Array index out of bounds. Call GetPrimaryModCount before GetPrimaryModChecksum.");
	return archiveScanner->GetModChecksum(GetPrimaryModArchive(index));
}

DLL_EXPORT unsigned int __stdcall GetPrimaryModChecksumFromName(const char* name)
{
	ASSERT(archiveScanner && hpiHandler, "Call InitArchiveScanner before GetPrimaryModChecksumFromName.");
	return archiveScanner->GetModChecksum(archiveScanner->ModNameToModArchive(name));
}


//////////////////////////
//////////////////////////

struct SideData {
	string name;
};

vector<SideData> sideData;

DLL_EXPORT int __stdcall GetSideCount()
{
	sideData.clear();

	logOutput.Print("get side count: ");

	LuaParser luaParser("gamedata/sidedata.lua",
	                    SPRING_VFS_MOD_BASE, SPRING_VFS_MOD_BASE);
	if (!luaParser.Execute()) {
		logOutput.Print("failed: %s\n", luaParser.GetErrorLog().c_str());
		return 0;
	}

	const LuaTable sideDataTbl = luaParser.GetRoot();
	if (!sideDataTbl.IsValid()) {
		logOutput.Print("failed: missing 'sideData' table\n");
		return 0;
	}

	for (int i = 1; true; i++) {
		const LuaTable sideTbl = sideDataTbl.SubTable(i);
		if (!sideTbl.IsValid()) {
			break;
		}
		SideData sd;
		sd.name = sideTbl.GetString("name", "unknown");
		sideData.push_back(sd);
	}

	return sideData.size();
}

DLL_EXPORT const char* __stdcall GetSideName(int side)
{
	ASSERT((unsigned)side < sideData.size(), "Array index out of bounds. Call GetSideCount before GetSideName.");
	return GetStr(sideData[side].name);
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
		return;
	}

	const LuaTable root = luaParser.GetRoot();
	if (!root.IsValid()) {
		return;
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

	return;
}


DLL_EXPORT int __stdcall GetLuaAICount()
{
	GetLuaAIOptions();
	return luaAIOptions.size();
}


DLL_EXPORT const char* __stdcall GetLuaAIName(int aiIndex)
{
	if ((aiIndex < 0) || (aiIndex >= luaAIOptions.size())) {
		return NULL;
	}
	return GetStr(luaAIOptions[aiIndex].name);
}


DLL_EXPORT const char* __stdcall GetLuaAIDesc(int aiIndex)
{
	if ((aiIndex < 0) || (aiIndex >= luaAIOptions.size())) {
		return NULL;
	}
	return GetStr(luaAIOptions[aiIndex].desc);
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

	string type; // "bool", "number", "string", "list"

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
		return false;
	}

	// common options properties
	opt.key = optTbl.GetString("key", "");
	if (opt.key.empty() ||
	    (opt.key.find_first_of(badKeyChars) != string::npos)) {
		return false;
	}
	opt.key = StringToLower(opt.key);
	if (optionsSet.find(opt.key) != optionsSet.end()) {
		return false;
	}
	opt.name = optTbl.GetString("name", opt.key);
	if (opt.name.empty()) {
		return false;
	}
	opt.desc = optTbl.GetString("desc", opt.name);

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
				break;
			}
			item.key = itemTbl.GetString("key", "");
			if (item.key.empty() ||
			    (item.key.find_first_of(badKeyChars) != string::npos)) {
				return false;
			}
			item.key = StringToLower(item.key);
			item.name = itemTbl.GetString("name", item.key);
			if (item.name.empty()) {
				return false;
			}
			item.desc = itemTbl.GetString("desc", item.name);
			opt.list.push_back(item);
		}

		if (opt.list.size() <= 0) {
			return false; // no empty lists
		}

		opt.listDef = optTbl.GetString("def", opt.list[0].name);
	}
	else {
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
	options.clear();
	optionsSet.clear();

	LuaParser luaParser(fileName, fileModes, accessModes);
	if (!mapName.empty()) {
		luaParser.GetTable("Map");
		luaParser.AddString("fileName", mapName);
		luaParser.AddString("fullName", "maps/" + mapName);
		luaParser.AddString("configFile", MapParser::GetMapConfigName(mapName));
		luaParser.EndTable();
	}
		
	if (!luaParser.Execute()) {
		printf("ParseOptions(%s) ERROR: %s\n",
		       fileName.c_str(), luaParser.GetErrorLog().c_str());
		return;
	}

	const LuaTable root = luaParser.GetRoot();
	if (!root.IsValid()) {
		return;
	}

	for (int index = 1; root.KeyExists(index); index++) {
		Option opt;
		if (ParseOption(root, index, opt)) {
			options.push_back(opt);
		}
	}

	optionsSet.clear();

	return;
};


static bool InvalidOptionIndex(int optIndex)
{
	if ((optIndex < 0) || (optIndex >= (int)options.size())) {
		return true;
	}
	return false;
}


static bool WrongOptionType(int optIndex, int type)
{
	if (InvalidOptionIndex(optIndex)) {
		return true;
	}
	if (options[optIndex].typeCode != type) {
		return true;
	}
	return false;
}


DLL_EXPORT int __stdcall GetMapOptionCount(const char* name)
{
	ASSERT(archiveScanner && hpiHandler,
	       "Call InitArchiveScanner before GetMapOptionCount.");
	ASSERT(name && *name,
				 "Don't pass a NULL pointer or an empty string to GetMapOptionCount.");

	ScopedMapLoader mapLoader(name);

	ParseOptions("MapOptions.lua", SPRING_VFS_MAP, SPRING_VFS_MAP, name);

	return (int)options.size();
}


DLL_EXPORT int __stdcall GetModOptionCount()
{
	ParseOptions("ModOptions.lua", SPRING_VFS_MOD, SPRING_VFS_MOD);
	return (int)options.size();
}


// Common Parameters

DLL_EXPORT const char* __stdcall GetOptionKey(int optIndex)
{
	if (InvalidOptionIndex(optIndex)) {
		return NULL;
	}
	return GetStr(options[optIndex].key);
}


DLL_EXPORT const char* __stdcall GetOptionName(int optIndex)
{
	if (InvalidOptionIndex(optIndex)) {
		return NULL;
	}
	return GetStr(options[optIndex].name);
}


DLL_EXPORT const char* __stdcall GetOptionDesc(int optIndex)
{
	if (InvalidOptionIndex(optIndex)) {
		return NULL;
	}
	return GetStr(options[optIndex].desc);
}


DLL_EXPORT int __stdcall GetOptionType(int optIndex)
{
	if (InvalidOptionIndex(optIndex)) {
		return 0;
	}
	return options[optIndex].typeCode;
}


// Bool Options

DLL_EXPORT int __stdcall GetOptionBoolDef(int optIndex)
{
	if (WrongOptionType(optIndex, opt_bool)) {
		return 0;
	}
	return options[optIndex].boolDef ? 1 : 0;
}


// Number Options

DLL_EXPORT float __stdcall GetOptionNumberDef(int optIndex)
{
	if (WrongOptionType(optIndex, opt_number)) {
		return 0.0f;
	}
	return options[optIndex].numberDef;
}


DLL_EXPORT float __stdcall GetOptionNumberMin(int optIndex)
{
	if (WrongOptionType(optIndex, opt_number)) {
		return -1.0e30f; // FIXME ?
	}
	return options[optIndex].numberMin;
}


DLL_EXPORT float __stdcall GetOptionNumberMax(int optIndex)
{
	if (WrongOptionType(optIndex, opt_number)) {
		return +1.0e30f; // FIXME ?
	}
	return options[optIndex].numberMax;
}


DLL_EXPORT float __stdcall GetOptionNumberStep(int optIndex)
{
	if (WrongOptionType(optIndex, opt_number)) {
		return 0.0f;
	}
	return options[optIndex].numberStep;
}


// String Options

DLL_EXPORT const char* __stdcall GetOptionStringDef(int optIndex)
{
	if (WrongOptionType(optIndex, opt_string)) {
		return NULL;
	}
	return GetStr(options[optIndex].stringDef);
}


DLL_EXPORT int __stdcall GetOptionStringMaxLen(int optIndex)
{
	if (WrongOptionType(optIndex, opt_string)) {
		return 0;
	}
	return options[optIndex].stringMaxLen;
}


// List Options

DLL_EXPORT int __stdcall GetOptionListCount(int optIndex)
{
	if (WrongOptionType(optIndex, opt_list)) {
		return 0;
	}
	return options[optIndex].list.size();
}


DLL_EXPORT const char* __stdcall GetOptionListDef(int optIndex)
{
	if (WrongOptionType(optIndex, opt_list)) {
		return 0;
	}
	return GetStr(options[optIndex].listDef);
}


DLL_EXPORT const char* __stdcall GetOptionListItemKey(int optIndex, int itemIndex)
{
	if (WrongOptionType(optIndex, opt_list)) {
		return NULL;
	}
	const vector<ListItem>& list = options[optIndex].list;
	if ((itemIndex < 0) || (itemIndex >= (int)list.size())) {
		return NULL;
	}
	return GetStr(list[itemIndex].key);
}


DLL_EXPORT const char* __stdcall GetOptionListItemName(int optIndex, int itemIndex)
{
	if (WrongOptionType(optIndex, opt_list)) {
		return NULL;
	}
	const vector<ListItem>& list = options[optIndex].list;
	if ((itemIndex < 0) || (itemIndex >= (int)list.size())) {
		return NULL;
	}
	return GetStr(list[itemIndex].name);
}


DLL_EXPORT const char* __stdcall GetOptionListItemDesc(int optIndex, int itemIndex)
{
	if (WrongOptionType(optIndex, opt_list)) {
		return NULL;
	}
	const vector<ListItem>& list = options[optIndex].list;
	if ((itemIndex < 0) || (itemIndex >= (int)list.size())) {
		return NULL;
	}
	return GetStr(list[itemIndex].desc);
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
	modValidMaps.clear();

	LuaParser luaParser("ValidMaps.lua", SPRING_VFS_MOD, SPRING_VFS_MOD);
	luaParser.GetTable("Spring");
	luaParser.AddFunc("GetMapList", LuaGetMapList);
	luaParser.AddFunc("GetMapInfo", LuaGetMapInfo);
	luaParser.EndTable();
	if (!luaParser.Execute()) {
		return 0;
	}

	const LuaTable root = luaParser.GetRoot();
	if (!root.IsValid()) {
		return 0;
	}

	for (int index = 1; root.KeyExists(index); index++) {
		const string map = root.GetString(index, "");
		if (!map.empty()) {
			modValidMaps.push_back(map);
		}
	}

	return (int)modValidMaps.size();
}


DLL_EXPORT const char* __stdcall GetModValidMap(int index)
{
	if ((index < 0) || (index >= modValidMaps.size())) {
		return NULL;
	}
	return GetStr(modValidMaps[index]);
}


//////////////////////////
//////////////////////////

// Map the wanted archives into the VFS with AddArchive before using these functions

static map<int, CFileHandler*> openFiles;
static int nextFile = 0;
static vector<string> curFindFiles;

DLL_EXPORT int __stdcall OpenFileVFS(const char* name)
{
	ASSERT(name && *name, "Don't pass a NULL pointer or an empty string to OpenFileVFS.");
	logOutput.Print("openfilevfs: %s\n", name);

	CFileHandler* fh = new CFileHandler(name);
	if (!fh->FileExists()) {
		delete fh;
		return 0;
	}

	nextFile++;
	openFiles[nextFile] = fh;

	return nextFile;
}

DLL_EXPORT void __stdcall CloseFileVFS(int handle)
{
	ASSERT(openFiles.find(handle) != openFiles.end(), "Unregistered handle. Pass the handle returned by OpenFileVFS to CloseFileVFS.");
	logOutput.Print("closefilevfs: %d\n", handle);
	delete openFiles[handle];
	openFiles.erase(handle);
}

DLL_EXPORT void __stdcall ReadFileVFS(int handle, void* buf, int length)
{
	ASSERT(openFiles.find(handle) != openFiles.end(), "Unregistered handle. Pass the handle returned by OpenFileVFS to ReadFileVFS.");
	ASSERT(buf, "Don't pass a NULL pointer to ReadFileVFS.");
	logOutput.Print("readfilevfs: %d\n", handle);
	CFileHandler* fh = openFiles[handle];
	fh->Read(buf, length);
}

DLL_EXPORT int __stdcall FileSizeVFS(int handle)
{
	ASSERT(openFiles.find(handle) != openFiles.end(), "Unregistered handle. Pass the handle returned by OpenFileVFS to FileSizeVFS.");
	logOutput.Print("filesizevfs: %d\n", handle);
	CFileHandler* fh = openFiles[handle];
	return fh->FileSize();
}


// Does not currently support more than one call at a time (a new call to initfind destroys data from previous ones)
// pass the returned handle to findfiles to get the results
DLL_EXPORT int __stdcall InitFindVFS(const char* pattern)
{
	std::string path = filesystem.GetDirectory(pattern);
	std::string patt = filesystem.GetFilename(pattern);
	logOutput.Print("initfindvfs: %s\n", pattern);
	curFindFiles = CFileHandler::FindFiles(path, patt);
	return 0;
}

// On first call, pass handle from initfind. pass the return value of this function on subsequent calls
// until 0 is returned. size should be set to max namebuffer size on call
DLL_EXPORT int __stdcall FindFilesVFS(int handle, char* nameBuf, int size)
{
	ASSERT(nameBuf, "Don't pass a NULL pointer to FindFilesVFS.");
	ASSERT(size > 0, "Negative or zero buffer length doesn't make sense.");
	logOutput.Print("findfilesvfs: %d\n", handle);
	if ((unsigned)handle >= curFindFiles.size())
		return 0;
	strncpy(nameBuf, curFindFiles[handle].c_str(), size);
	return handle + 1;
}


//////////////////////////
//////////////////////////

static map<int, CArchiveBase*> openArchives;
static int nextArchive = 0;

// returns 0 on error, a handle otherwise
DLL_EXPORT int __stdcall OpenArchive(const char* name)
{
	ASSERT(name && *name, "Don't pass a NULL pointer or an empty string to OpenArchive.");
	CArchiveBase* a = CArchiveFactory::OpenArchive(name);
	if (a) {
		nextArchive++;
		openArchives[nextArchive] = a;
		return nextArchive;
	}
	else {
		return 0;
	}
}

DLL_EXPORT void __stdcall CloseArchive(int archive)
{
	ASSERT(openArchives.find(archive) != openArchives.end(), "Unregistered archive. Pass the handle returned by OpenArchive to CloseArchive.");
	delete openArchives[archive];
	openArchives.erase(archive);
}

DLL_EXPORT int __stdcall FindFilesArchive(int archive, int cur, char* nameBuf, int* size)
{
	ASSERT(openArchives.find(archive) != openArchives.end(), "Unregistered archive. Pass the handle returned by OpenArchive to FindFilesArchive.");
	ASSERT(nameBuf && size, "Don't pass a NULL pointer to FindFilesArchive.");
	CArchiveBase* a = openArchives[archive];

	logOutput.Print("findfilesarchive: %d\n", archive);

	string name;
	int s;

	int ret = a->FindFiles(cur, &name, &s);
	strcpy(nameBuf, name.c_str());
	*size = s;

	return ret;
}

DLL_EXPORT int __stdcall OpenArchiveFile(int archive, const char* name)
{
	ASSERT(openArchives.find(archive) != openArchives.end(), "Unregistered archive. Pass the handle returned by OpenArchive to OpenArchiveFile.");
	ASSERT(name && *name, "Don't pass a NULL pointer or an empty string to OpenArchiveFile.");
	CArchiveBase* a = openArchives[archive];
	return a->OpenFile(name);
}

DLL_EXPORT int __stdcall ReadArchiveFile(int archive, int handle, void* buffer, int numBytes)
{
	ASSERT(openArchives.find(archive) != openArchives.end(), "Unregistered archive. Pass the handle returned by OpenArchive to ReadArchiveFile.");
	ASSERT(buffer, "Don't pass a NULL pointer to ReadArchiveFile.");
	CArchiveBase* a = openArchives[archive];
	return a->ReadFile(handle, buffer, numBytes);
}

DLL_EXPORT void __stdcall CloseArchiveFile(int archive, int handle)
{
	ASSERT(openArchives.find(archive) != openArchives.end(), "Unregistered archive. Pass the handle returned by OpenArchive to CloseArchiveFile.");
	CArchiveBase* a = openArchives[archive];
	return a->CloseFile(handle);
}

DLL_EXPORT int __stdcall SizeArchiveFile(int archive, int handle)
{
	ASSERT(openArchives.find(archive) != openArchives.end(), "Unregistered archive. Pass the handle returned by OpenArchive to SizeArchiveFile.");
	CArchiveBase* a = openArchives[archive];
	return a->FileSize(handle);
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

/**
 * @brief get string from Spring configuration
 * @param name name of key to get
 * @param defvalue default string value to use if key is not found, may not be NULL
 * @return string value
 */
DLL_EXPORT const char* __stdcall GetSpringConfigString( const char* name, const char* defvalue )
{
	std::string res = configHandler.GetString( name, defvalue );
	return GetStr(res);
}

/**
 * @brief get integer from Spring configuration
 * @param name name of key to get
 * @param defvalue default integer value to use if key is not found
 * @return integer value
 */
DLL_EXPORT int __stdcall GetSpringConfigInt( const char* name, const int defvalue )
{
	return configHandler.GetInt( name, defvalue );
}

/**
 * @brief get float from Spring configuration
 * @param name name of key to get
 * @param defvalue default float value to use if key is not found
 * @return float value
 */
DLL_EXPORT float __stdcall GetSpringConfigFloat( const char* name, const float defvalue )
{
	return configHandler.GetFloat( name, defvalue );
}

/**
 * @brief set string in Spring configuration
 * @param name name of key to set
 * @param value string value to set
 */
DLL_EXPORT void __stdcall SetSpringConfigString( const char* name, const char* value )
{
	configHandler.SetString( name, value );
}

/**
 * @brief set integer in Spring configuration
 * @param name name of key to set
 * @param value integer value to set
 */
DLL_EXPORT void __stdcall SetSpringConfigInt( const char* name, const int value )
{
	configHandler.SetInt( name, value );
}

/**
 * @brief set float in Spring configuration
 * @param name name of key to set
 * @param value float value to set
 */
DLL_EXPORT void __stdcall SetSpringConfigFloat( const char* name, const float value )
{
	configHandler.SetFloat( name, value );
}
