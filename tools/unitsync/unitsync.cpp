#include "StdAfx.h"
#include "unitsync.h"

#include <string>
#include <string.h>
#include <vector>
#include <set>
#include <algorithm>
#include <cstdio>
#include <cstdarg>

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
#include "Sim/SideParser.h"
#include "ExternalAI/Interface/aidefines.h"
#include "ExternalAI/Interface/SSAILibrary.h"
#include "System/Util.h"
#include "System/Exceptions.h"

#include "LuaParserAPI.h"
#include "Syncer.h"
#include "SyncServer.h"
#include "unitsyncLogOutput.h"


using std::string;


#define ASSERT(condition, message) \
	do { \
		if (!(condition)) { \
			char buf[256]; \
			sprintf(buf, "%s:%d: %s: %s", __FILE__, __LINE__, __FUNCTION__, message); \
			MessageBox(0, buf, "Unitsync assertion failed", MB_OK); \
		} \
		assert(condition); /* just crash after the error in release mode */ \
	} while(0)

#define CHECK_INIT() \
	ASSERT(archiveScanner && vfsHandler, "Call Init first.")

#define CHECK_NULL(condition) \
	ASSERT((condition) != NULL, #condition " may not be null.")

#define CHECK_NULL_OR_EMPTY(condition) \
	ASSERT((condition) != NULL && *(condition) != 0, #condition " may not be null or empty.")

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
BOOL CALLING_CONV DllMain(HINSTANCE hInst,
                       DWORD dwReason,
                       LPVOID lpReserved) {
	logOutput.Print("----\nunitsync loaded\n");
	return TRUE;
}
#endif


// Helper class for loading a map archive temporarily

class ScopedMapLoader {
	public:
		ScopedMapLoader(const string& mapName) : oldHandler(vfsHandler) {
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

		~ScopedMapLoader() {
			if (vfsHandler != oldHandler) {
				delete vfsHandler;
				vfsHandler = oldHandler;
			}
		}

	private:
		CVFSHandler* oldHandler;
};


Export(const char*) GetSpringVersion()
{
	return VERSION_STRING;
}

Export(void) Message(const char* p_szMessage)
{
	MessageBox(NULL, p_szMessage, "Message from DLL", MB_OK);
}

Export(void) UnInit()
{
	lpClose();

	FileSystemHandler::Cleanup();

	if ( syncer )
	{
		delete syncer;
		syncer = 0;
		logOutput.Print("unitsync deinitialized\n----\n");
	}

	ConfigHandler::Deallocate();
}

Export(int) Init(bool isServer, int id)
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
	}
	catch (const std::exception& e) {
		Message(e.what());
		return 0;
	}

	return 1;
}

Export(int) ProcessUnits(void)
{
	logOutput.Print("syncer: process units\n");
	return syncer->ProcessUnits();
}

Export(int) ProcessUnitsNoChecksum(void)
{
	logOutput.Print("syncer: process units\n");
	return syncer->ProcessUnits(false);
}

Export(const char*) GetCurrentList()
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

Export(void) AddClient(int id, const char *unitList)
{
	logOutput.Print("syncer: add client\n");
	((CSyncServer *)syncer)->AddClient(id, unitList);
}

Export(void) RemoveClient(int id)
{
	logOutput.Print("syncer: remove client\n");
	((CSyncServer *)syncer)->RemoveClient(id);
}

Export(const char*) GetClientDiff(int id)
{
	logOutput.Print("syncer: get client diff\n");
	string tmp = ((CSyncServer *)syncer)->GetClientDiff(id);
	return GetStr(tmp);
}

Export(void) InstallClientDiff(const char *diff)
{
	logOutput.Print("syncer: install client diff\n");
	syncer->InstallClientDiff(diff);
}

Export(int) GetUnitCount()
{
	logOutput.Print("syncer: get unit count\n");
	return syncer->GetUnitCount();
}

Export(const char*) GetUnitName(int unit)
{
	logOutput.Print("syncer: get unit %d name\n", unit);
	string tmp = syncer->GetUnitName(unit);
	return GetStr(tmp);
}

Export(const char*) GetFullUnitName(int unit)
{
	logOutput.Print("syncer: get full unit %d name\n", unit);
	string tmp = syncer->GetFullUnitName(unit);
	return GetStr(tmp);
}

Export(int) IsUnitDisabled(int unit)
{
	logOutput.Print("syncer: is unit %d disabled\n", unit);
	if (syncer->IsUnitDisabled(unit))
		return 1;
	else
		return 0;
}

Export(int) IsUnitDisabledByClient(int unit, int clientId)
{
	logOutput.Print("syncer: is unit %d disabled by client %d\n", unit, clientId);
	if (syncer->IsUnitDisabledByClient(unit, clientId))
		return 1;
	else
		return 0;
}

//////////////////////////
//////////////////////////

Export(void) AddArchive(const char* name)
{
	CHECK_INIT();
	CHECK_NULL_OR_EMPTY(name);

	vfsHandler->AddArchive(name, false);
}

Export(void) AddAllArchives(const char* root)
{
	CHECK_INIT();
	CHECK_NULL_OR_EMPTY(root);

	vector<string> ars = archiveScanner->GetArchives(root);
//	Message(root);
	for (vector<string>::iterator i = ars.begin(); i != ars.end(); ++i) {
		logOutput.Print("adding archive: %s\n", i->c_str());
		vfsHandler->AddArchive(*i, false);
	}
}

Export(unsigned int) GetArchiveChecksum(const char* arname)
{
	CHECK_INIT();
	CHECK_NULL_OR_EMPTY(arname);

	logOutput.Print("archive checksum: %s\n", arname);
	return archiveScanner->GetArchiveChecksum(arname);
}

Export(const char*) GetArchivePath(const char* arname)
{
	CHECK_INIT();
	CHECK_NULL_OR_EMPTY(arname);

	logOutput.Print("archive path: %s\n", arname);
	return GetStr(archiveScanner->GetArchivePath(arname));
}

// Updated on every call to GetMapCount
static vector<string> mapNames;

Export(int) GetMapCount()
{
	ASSERT(archiveScanner && vfsHandler, "Call InitArchiveScanner before GetMapCount.");
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

Export(const char*) GetMapName(int index)
{
	ASSERT(archiveScanner && vfsHandler, "Call InitArchiveScanner before GetMapName.");
	ASSERT((unsigned)index < mapNames.size(), "Array index out of bounds. Call GetMapCount before GetMapName.");

	return GetStr(mapNames[index]);
}


Export(int) GetMapInfoEx(const char* name, MapInfo* outInfo, int version)
{
	CHECK_INIT();
	CHECK_NULL_OR_EMPTY(name);
	CHECK_NULL(outInfo);

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


Export(int) GetMapInfo(const char* name, MapInfo* outInfo)
{
	CHECK_INIT();
	CHECK_NULL_OR_EMPTY(name);
	CHECK_NULL(outInfo);

	return GetMapInfoEx(name, outInfo, 0);
}

static vector<string> mapArchives;

Export(int) GetMapArchiveCount(const char* mapName)
{
	CHECK_INIT();
	CHECK_NULL_OR_EMPTY(mapName);

	mapArchives = archiveScanner->GetArchivesForMap(mapName);
	return mapArchives.size();
}

Export(const char*) GetMapArchiveName(int index)
{
	CHECK_INIT();
	ASSERT((unsigned)index < mapArchives.size(), "Array index out of bounds. Call GetMapArchiveCount before GetMapArchiveName.");

	return GetStr(mapArchives[index]);
}

Export(unsigned int) GetMapChecksum(int index)
{
	CHECK_INIT();
	ASSERT((unsigned)index < mapNames.size(), "Array index out of bounds. Call GetMapCount before GetMapChecksum.");

	return archiveScanner->GetMapChecksum(mapNames[index]);
}

Export(unsigned int) GetMapChecksumFromName(const char* mapName)
{
	CHECK_INIT();

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
	unsigned char* buffer = (unsigned char*)malloc(size);

	if (in.FileExists()) {
		SMFHeader mh;
		in.Read(&mh, sizeof(mh));
		in.Seek(mh.minimapPtr + offset);
		in.Read(buffer, size);
	}
	else {
		free(buffer);
		return NULL;
	}

	// Do stuff

	//void* ret = malloc(mipsize*mipsize*2);
	void* ret = (void*)imgbuf;
	unsigned short* colors = (unsigned short*)ret;

	unsigned char* temp = buffer;

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

Export(void*) GetMinimap(const char* filename, int miplevel)
{
	CHECK_INIT();
	CHECK_NULL_OR_EMPTY(filename);
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


/**
 * @brief Retrieves dimensions of infomap for a map.
 * @param filename The name of the map, including extension.
 * @param name     Of which infomap to retrieve the dimensions.
 * @param width    This is set to the width of the infomap, or 0 on error.
 * @param height   This is set to the height of the infomap, or 0 on error.
 * @return 1 when the infomap was found with a nonzero size, 0 on error.
 * @see GetInfoMap
 */
Export(int) GetInfoMapSize(const char* filename, const char* name, int* width, int* height)
{
	CHECK_INIT();
	CHECK_NULL_OR_EMPTY(filename);
	CHECK_NULL_OR_EMPTY(name);
	CHECK_NULL(width);
	CHECK_NULL(height);

	try {
		ScopedMapLoader mapLoader(filename);
		CSmfMapFile file(filename);
		MapBitmapInfo bmInfo = file.GetInfoMapSize(name);

		*width = bmInfo.width;
		*height = bmInfo.height;

		return bmInfo.width > 0;
	}
	catch (content_error&) {
	}

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
Export(int) GetInfoMap(const char* filename, const char* name, void* data, int typeHint)
{
	CHECK_INIT();
	CHECK_NULL_OR_EMPTY(filename);
	CHECK_NULL_OR_EMPTY(name);
	CHECK_NULL(data);

	string n = name;

	try {
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
			// converting from 8 bits per pixel to 16 bits per pixel is unsupported
			return 0;
		}
	}
	catch (content_error&) {
	}

	return 0;
}


//////////////////////////
//////////////////////////

vector<CArchiveScanner::ModData> modData;


Export(int) GetPrimaryModCount()
{
	CHECK_INIT();

	modData = archiveScanner->GetPrimaryMods();
	return modData.size();
}


Export(const char*) GetPrimaryModName(int index)
{
	CHECK_INIT();
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
Export(const char*) GetPrimaryModShortName(int index)
{
	CHECK_INIT();
	ASSERT((unsigned)index < modData.size(), "Array index out of bounds. Call GetPrimaryModCount before GetPrimaryModShortName.");

	string x = modData[index].shortName;
	return GetStr(x);
}


Export(const char*) GetPrimaryModVersion(int index)
{
	CHECK_INIT();
	ASSERT((unsigned)index < modData.size(), "Array index out of bounds. Call GetPrimaryModCount before GetPrimaryModMutator.");

	string x = modData[index].version;
	return GetStr(x);
}


Export(const char*) GetPrimaryModMutator(int index)
{
	CHECK_INIT();
	ASSERT((unsigned)index < modData.size(), "Array index out of bounds. Call GetPrimaryModCount before GetPrimaryModMutator.");

	string x = modData[index].mutator;
	return GetStr(x);
}


Export(const char*) GetPrimaryModGame(int index)
{
	CHECK_INIT();
	ASSERT((unsigned)index < modData.size(), "Array index out of bounds. Call GetPrimaryModCount before GetPrimaryModName.");

	string x = modData[index].game;
	return GetStr(x);
}


Export(const char*) GetPrimaryModShortGame(int index)
{
	CHECK_INIT();
	ASSERT((unsigned)index < modData.size(), "Array index out of bounds. Call GetPrimaryModCount before GetPrimaryModShortGame.");

	string x = modData[index].shortGame;
	return GetStr(x);
}


Export(const char*) GetPrimaryModDescription(int index)
{
	CHECK_INIT();
	ASSERT((unsigned)index < modData.size(), "Array index out of bounds. Call GetPrimaryModCount before GetPrimaryModDescription.");

	string x = modData[index].description;
	return GetStr(x);
}


Export(const char*) GetPrimaryModArchive(int index)
{
	CHECK_INIT();
	ASSERT((unsigned)index < modData.size(), "Array index out of bounds. Call GetPrimaryModCount before GetPrimaryModArchive.");

	return GetStr(modData[index].dependencies[0]);
}


vector<string> primaryArchives;

Export(int) GetPrimaryModArchiveCount(int index)
{
	CHECK_INIT();
	ASSERT((unsigned)index < modData.size(), "Array index out of bounds. Call GetPrimaryModCount before GetPrimaryModArchiveCount.");

	primaryArchives = archiveScanner->GetArchives(modData[index].dependencies[0]);
	return primaryArchives.size();
}

Export(const char*) GetPrimaryModArchiveList(int archiveNr)
{
	CHECK_INIT();
	ASSERT((unsigned)archiveNr < primaryArchives.size(), "Array index out of bounds. Call GetPrimaryModArchiveCount before GetPrimaryModArchiveList.");

	logOutput.Print("primary mod archive list: %s\n", primaryArchives[archiveNr].c_str());
	return GetStr(primaryArchives[archiveNr]);
}

Export(int) GetPrimaryModIndex(const char* name)
{
	CHECK_INIT();

	string n(name);
	for (unsigned i = 0; i < modData.size(); ++i) {
		if (modData[i].name == n)
			return i;
	}
	// if it returns -1, make sure you call GetPrimaryModCount before GetPrimaryModIndex.
	return -1;
}

Export(unsigned int) GetPrimaryModChecksum(int index)
{
	CHECK_INIT();
	ASSERT((unsigned)index < modData.size(), "Array index out of bounds. Call GetPrimaryModCount before GetPrimaryModChecksum.");

	return archiveScanner->GetModChecksum(GetPrimaryModArchive(index));
}

Export(unsigned int) GetPrimaryModChecksumFromName(const char* name)
{
	CHECK_INIT();

	return archiveScanner->GetModChecksum(archiveScanner->ModNameToModArchive(name));
}


//////////////////////////
//////////////////////////

Export(int) GetSideCount()
{
	if (!sideParser.Load()) {
		logOutput.Print("failed: %s\n", sideParser.GetErrorLog().c_str());
		return 0;
	}
	return sideParser.GetCount();
}


Export(const char*) GetSideName(int side)
{
	ASSERT((unsigned int)side < sideParser.GetCount(),
	       "Array index out of bounds. Call GetSideCount before GetSideName.");
	// the full case name  (not the lowered version)
	return GetStr(sideParser.GetCaseName(side));
}


Export(const char*) GetSideStartUnit(int side)
{
	ASSERT((unsigned int)side < sideParser.GetCount(),
	       "Array index out of bounds. Call GetSideCount before GetSideStartUnit.");
	return GetStr(sideParser.GetStartUnit(side));
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


Export(int) GetLuaAICount()
{
	GetLuaAIOptions();
	return luaAIOptions.size();
}


Export(const char*) GetLuaAIName(int aiIndex)
{
	if ((aiIndex < 0) || (aiIndex >= (int)luaAIOptions.size())) {
		return NULL;
	}
	return GetStr(luaAIOptions[aiIndex].name);
}


Export(const char*) GetLuaAIDesc(int aiIndex)
{
	if ((aiIndex < 0) || (aiIndex >= (int)luaAIOptions.size())) {
		return NULL;
	}
	return GetStr(luaAIOptions[aiIndex].desc);
}


//////////////////////////
//////////////////////////


static vector<Option> options;
static set<string> optionsSet;


static bool ParseOption(const LuaTable& root, int index, Option& opt)
{
	const LuaTable& optTbl = root.SubTable(index);
	if (!optTbl.IsValid()) {
		return false;
	}

	// common options properties
	std::string opt_key = optTbl.GetString("key", "");
	if (opt_key.empty() ||
	    (opt_key.find_first_of(Option_badKeyChars) != std::string::npos)) {
		return false;
	}
	opt_key = StringToLower(opt_key);
	if (optionsSet.find(opt_key) != optionsSet.end()) {
		return false;
	}
	opt.key = mallocCopyString(opt_key.c_str());
	
	std::string opt_name = optTbl.GetString("name", opt_key);
	if (opt_name.empty()) {
		return false;
	}
	opt.name = mallocCopyString(opt_name.c_str());
	
	std::string opt_desc = optTbl.GetString("desc", opt_name);
	opt.desc = mallocCopyString(opt_desc.c_str());
	
	std::string opt_section = optTbl.GetString("section", "");
	opt.section = mallocCopyString(opt_section.c_str());
	
	std::string opt_style = optTbl.GetString("style", "");
	opt.style = mallocCopyString(opt_style.c_str());

	std::string opt_type = optTbl.GetString("type", "");
	opt_type = StringToLower(opt_type);
	opt.type = mallocCopyString(opt_type.c_str());

	// option type specific properties
	if (opt_type == "bool") {
		opt.typeCode = opt_bool;
		opt.boolDef = optTbl.GetBool("def", false);
	}
	else if (opt_type == "number") {
		opt.typeCode = opt_number;
		opt.numberDef  = optTbl.GetFloat("def",  0.0f);
		opt.numberMin  = optTbl.GetFloat("min",  -1.0e30f);
		opt.numberMax  = optTbl.GetFloat("max",  +1.0e30f);
		opt.numberStep = optTbl.GetFloat("step", 0.0f);
	}
	else if (opt_type == "string") {
		opt.typeCode = opt_string;
		opt.stringDef    = mallocCopyString(optTbl.GetString("def", "").c_str());
		opt.stringMaxLen = optTbl.GetInt("maxlen", 0);
	}
	else if (opt_type == "list") {
		opt.typeCode = opt_list;

		const LuaTable& listTbl = optTbl.SubTable("items");
		if (!listTbl.IsValid()) {
			return false;
		}

		vector<OptionListItem> opt_list;
		for (int i = 1; listTbl.KeyExists(i); i++) {
			OptionListItem item;

			// string format
			std::string item_key = listTbl.GetString(i, "");
			if (!item_key.empty() &&
			    (item_key.find_first_of(Option_badKeyChars) == string::npos)) {
				item.key = mallocCopyString(item_key.c_str());
				item.name = item.key;
				item.desc = item.name;
				opt_list.push_back(item);
				continue;
			}

			// table format  (name & desc)
			const LuaTable& itemTbl = listTbl.SubTable(i);
			if (!itemTbl.IsValid()) {
				break;
			}
			item_key = itemTbl.GetString("key", "");
			if (item_key.empty() ||
			    (item_key.find_first_of(Option_badKeyChars) != string::npos)) {
				return false;
			}
			item_key = StringToLower(item_key);
			//item.key = item_key.c_str();
			item.key = mallocCopyString(item_key.c_str());
			std::string item_name = itemTbl.GetString("name", item_key);
			if (item_name.empty()) {
				return false;
			}
			//item.name = item_name.c_str();
			item.name = mallocCopyString(item_name.c_str());
			std::string item_desc = itemTbl.GetString("desc", item_name);
			//item.desc = item_desc.c_str();
			item.desc = mallocCopyString(item_desc.c_str());
			opt_list.push_back(item);
		}

		if (opt_list.size() <= 0) {
			return false; // no empty lists
		}
		
		opt.numListItems = opt_list.size();
		opt.list = (OptionListItem*) calloc(sizeof(OptionListItem), opt.numListItems);
        for (int i=0; i < opt.numListItems; ++i) {
            opt.list[i] = opt_list.at(i);
        }

		//opt.listDef = optTbl.GetString("def", opt.list[0].name).c_str();
		opt.listDef = mallocCopyString(optTbl.GetString("def", opt.list[0].name).c_str());
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
		logOutput.Print("ParseOptions(%s) ERROR: %s\n",
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


Export(int) GetMapOptionCount(const char* name)
{
	CHECK_INIT();
	CHECK_NULL_OR_EMPTY(name);

	ScopedMapLoader mapLoader(name);

	options.clear();
	optionsSet.clear();

	ParseOptions("MapOptions.lua", SPRING_VFS_MAP, SPRING_VFS_MAP, name);

	optionsSet.clear();

	return (int)options.size();
}


Export(int) GetModOptionCount()
{
	options.clear();
	optionsSet.clear();

	// EngineOptions must be read first, so accidentally "overloading" engine
	// options with mod options with identical names is not possible.
	ParseOptions("EngineOptions.lua", SPRING_VFS_MOD_BASE, SPRING_VFS_MOD_BASE);
	ParseOptions("ModOptions.lua", SPRING_VFS_MOD, SPRING_VFS_MOD);

	optionsSet.clear();

	return (int)options.size();
}


vector<InfoItem> infos;
set<string> infosSet;

// Updated on every call to GetSkirmishAICount
static vector<string> skirmishAIDataDirs;

Export(int) GetSkirmishAICount() {
	
	ASSERT(archiveScanner && vfsHandler, "Call InitArchiveScanner before GetSkirmishAICount.");
	
	skirmishAIDataDirs = CFileHandler::SubDirs(SKIRMISH_AI_DATA_DIR, "*", SPRING_VFS_RAW);
	
	// filter out dirs not containing an AIInfo.lua file 
	for (vector<string>::iterator i = skirmishAIDataDirs.begin(); i != skirmishAIDataDirs.end(); ++i) {
		const string& possibleDataDir = *i;
		vector<string> infoFile = CFileHandler::FindFiles(possibleDataDir, "AIInfo.lua");
		if (infoFile.size() == 0) {
			skirmishAIDataDirs.erase(i);
		}
	}
	
	sort(skirmishAIDataDirs.begin(), skirmishAIDataDirs.end());

	return skirmishAIDataDirs.size();
}

Export(struct SSAISpecifier) GetSkirmishAISpecifier(int index) {
	
	SSAISpecifier spec = {NULL, NULL};
	
	int num = GetSkirmishAIInfoCount(index);
	
	for (int i=0; i < num; ++i) {
		string info_key = string(GetInfoKey(i));
		if (info_key == SKIRMISH_AI_PROPERTY_SHORT_NAME) {
			spec.shortName = GetInfoValue(i);
		} else if (info_key == SKIRMISH_AI_PROPERTY_VERSION) {
			spec.version = GetInfoValue(i);
		}
	}
	
	return spec;
}

std::vector<InfoItem> ParseInfos(
		const std::string& fileName,
		const std::string& fileModes,
		const std::string& accessModes)
{
	std::vector<InfoItem> info;
	
	static const unsigned int MAX_INFOS = 128;
	InfoItem tmpInfo[MAX_INFOS];
	unsigned int num = ParseInfo(fileName.c_str(), fileModes.c_str(),
			accessModes.c_str(), tmpInfo, MAX_INFOS);
	for (unsigned int i=0; i < num; ++i) {
		info.push_back(tmpInfo[i]);
    }
	
	return info;
}

Export(int) GetSkirmishAIInfoCount(int index) {
	
	ASSERT(archiveScanner && vfsHandler, "Call InitArchiveScanner before GetSkirmishAIInfoCount.");
	infos = ParseInfos(skirmishAIDataDirs[index] + "/AIInfo.lua", SPRING_VFS_RAW, SPRING_VFS_RAW);
	return (int)infos.size();
}
Export(const char*) GetInfoKey(int index) {
	
	ASSERT(!infos.empty(), "Call GetSkirmishAIInfoCount before GetInfoKey.");
	return infos.at(index).key;
}
Export(const char*) GetInfoValue(int index) {
	
	ASSERT(!infos.empty(), "Call GetSkirmishAIInfoCount before GetInfoValue.");
	return infos.at(index).value;
}
Export(const char*) GetInfoDescription(int index) {
	
	ASSERT(!infos.empty(), "Call GetSkirmishAIInfoCount before GetInfoDescription.");
	return infos.at(index).desc;
}


Export(int) GetSkirmishAIOptionCount(int index) {

	options.clear();
	optionsSet.clear();

	ParseOptions(skirmishAIDataDirs[index] + "/AIOptions.lua", SPRING_VFS_RAW, SPRING_VFS_RAW);

	optionsSet.clear();

	return (int)options.size();
}


Export(const char*) GetOptionKey(int optIndex)
{
	if (InvalidOptionIndex(optIndex)) {
		return NULL;
	}
	return GetStr(options[optIndex].key);
}


Export(const char*) GetOptionName(int optIndex)
{
	if (InvalidOptionIndex(optIndex)) {
		return NULL;
	}
	return GetStr(options[optIndex].name);
}

Export(const char*) GetOptionSection(int optIndex)
{
	if (InvalidOptionIndex(optIndex)) {
		return NULL;
	}
	return GetStr(options[optIndex].section);
}

Export(const char*) GetOptionStyle(int optIndex)
{
	if (InvalidOptionIndex(optIndex)) {
		return NULL;
	}
	return GetStr(options[optIndex].style);
}

Export(const char*) GetOptionDesc(int optIndex)
{
	if (InvalidOptionIndex(optIndex)) {
		return NULL;
	}
	return GetStr(options[optIndex].desc);
}


Export(int) GetOptionType(int optIndex)
{
	if (InvalidOptionIndex(optIndex)) {
		return 0;
	}
	return options[optIndex].typeCode;
}


Export(int) GetOptionBoolDef(int optIndex)
{
	if (WrongOptionType(optIndex, opt_bool)) {
		return 0;
	}
	return options[optIndex].boolDef ? 1 : 0;
}


// Number Options

Export(float) GetOptionNumberDef(int optIndex)
{
	if (WrongOptionType(optIndex, opt_number)) {
		return 0.0f;
	}
	return options[optIndex].numberDef;
}


Export(float) GetOptionNumberMin(int optIndex)
{
	if (WrongOptionType(optIndex, opt_number)) {
		return -1.0e30f; // FIXME ?
	}
	return options[optIndex].numberMin;
}


Export(float) GetOptionNumberMax(int optIndex)
{
	if (WrongOptionType(optIndex, opt_number)) {
		return +1.0e30f; // FIXME ?
	}
	return options[optIndex].numberMax;
}


Export(float) GetOptionNumberStep(int optIndex)
{
	if (WrongOptionType(optIndex, opt_number)) {
		return 0.0f;
	}
	return options[optIndex].numberStep;
}


// String Options

Export(const char*) GetOptionStringDef(int optIndex)
{
	if (WrongOptionType(optIndex, opt_string)) {
		return NULL;
	}
	return GetStr(options[optIndex].stringDef);
}


Export(int) GetOptionStringMaxLen(int optIndex)
{
	if (WrongOptionType(optIndex, opt_string)) {
		return 0;
	}
	return options[optIndex].stringMaxLen;
}


// List Options

Export(int) GetOptionListCount(int optIndex)
{
	if (WrongOptionType(optIndex, opt_list)) {
		return 0;
	}
	return options[optIndex].numListItems;
}


Export(const char*) GetOptionListDef(int optIndex)
{
	if (WrongOptionType(optIndex, opt_list)) {
		return 0;
	}
	return GetStr(options[optIndex].listDef);
}


Export(const char*) GetOptionListItemKey(int optIndex, int itemIndex)
{
	if (WrongOptionType(optIndex, opt_list)) {
		return NULL;
	}
	if ((itemIndex < 0) || (itemIndex >= options[optIndex].numListItems)) {
		return NULL;
	}
	return GetStr(options[optIndex].list[itemIndex].key);
}


Export(const char*) GetOptionListItemName(int optIndex, int itemIndex)
{
	if (WrongOptionType(optIndex, opt_list)) {
		return NULL;
	}
	if ((itemIndex < 0) || (itemIndex >= options[optIndex].numListItems)) {
		return NULL;
	}
	return GetStr(options[optIndex].list[itemIndex].name);
}


Export(const char*) GetOptionListItemDesc(int optIndex, int itemIndex)
{
	if (WrongOptionType(optIndex, opt_list)) {
		return NULL;
	}
	if ((itemIndex < 0) || (itemIndex >= options[optIndex].numListItems)) {
		return NULL;
	}
	return GetStr(options[optIndex].list[itemIndex].desc);
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


Export(int) GetModValidMapCount()
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


Export(const char*) GetModValidMap(int index)
{
	if ((index < 0) || (index >= (int)modValidMaps.size())) {
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

Export(int) OpenFileVFS(const char* name)
{
	CHECK_NULL_OR_EMPTY(name);

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

Export(void) CloseFileVFS(int handle)
{
	ASSERT(openFiles.find(handle) != openFiles.end(), "Unregistered handle. Pass the handle returned by OpenFileVFS to CloseFileVFS.");

	logOutput.Print("closefilevfs: %d\n", handle);
	delete openFiles[handle];
	openFiles.erase(handle);
}

Export(void) ReadFileVFS(int handle, void* buf, int length)
{
	ASSERT(openFiles.find(handle) != openFiles.end(), "Unregistered handle. Pass the handle returned by OpenFileVFS to ReadFileVFS.");
	CHECK_NULL(buf);

	logOutput.Print("readfilevfs: %d\n", handle);
	CFileHandler* fh = openFiles[handle];
	fh->Read(buf, length);
}

Export(int) FileSizeVFS(int handle)
{
	ASSERT(openFiles.find(handle) != openFiles.end(), "Unregistered handle. Pass the handle returned by OpenFileVFS to FileSizeVFS.");

	logOutput.Print("filesizevfs: %d\n", handle);
	CFileHandler* fh = openFiles[handle];
	return fh->FileSize();
}


Export(int) InitFindVFS(const char* pattern)
{
	string path = filesystem.GetDirectory(pattern);
	string patt = filesystem.GetFilename(pattern);
	logOutput.Print("initfindvfs: %s\n", pattern);
	curFindFiles = CFileHandler::FindFiles(path, patt);
	return 0;
}

Export(int) InitDirListVFS(const char* path, const char* pattern, const char* modes)
{
	if (path    == NULL) { path = "";              }
	if (modes   == NULL) { modes = SPRING_VFS_ALL; }
	if (pattern == NULL) { pattern = "*";          }
	logOutput.Print("InitDirListVFS: '%s' '%s' '%s'\n", path, pattern, modes);
	curFindFiles = CFileHandler::DirList(path, pattern, modes);
	return 0;
}

Export(int) InitSubDirsVFS(const char* path, const char* pattern, const char* modes)
{
	if (path    == NULL) { path = "";              }
	if (modes   == NULL) { modes = SPRING_VFS_ALL; }
	if (pattern == NULL) { pattern = "*";          }
	logOutput.Print("InitSubDirsVFS: '%s' '%s' '%s'\n", path, pattern, modes);
	curFindFiles = CFileHandler::SubDirs(path, pattern, modes);
	return 0;
}

Export(int) FindFilesVFS(int handle, char* nameBuf, int size)
{
	CHECK_NULL(nameBuf);
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

Export(int) OpenArchive(const char* name)
{
	CHECK_NULL_OR_EMPTY(name);

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

Export(int) OpenArchiveType(const char* name, const char* type)
{
	CHECK_NULL_OR_EMPTY(name);
	CHECK_NULL_OR_EMPTY(type);

	CArchiveBase* a = CArchiveFactory::OpenArchive(name, type);
	if (a) {
		nextArchive++;
		openArchives[nextArchive] = a;
		return nextArchive;
	}
	else {
		return 0;
	}
}

Export(void) CloseArchive(int archive)
{
	ASSERT(openArchives.find(archive) != openArchives.end(), "Unregistered archive. Pass the handle returned by OpenArchive to CloseArchive.");

	delete openArchives[archive];
	openArchives.erase(archive);
}

Export(int) FindFilesArchive(int archive, int cur, char* nameBuf, int* size)
{
	ASSERT(openArchives.find(archive) != openArchives.end(), "Unregistered archive. Pass the handle returned by OpenArchive to FindFilesArchive.");
	CHECK_NULL(nameBuf);
	CHECK_NULL(size);

	CArchiveBase* a = openArchives[archive];

	logOutput.Print("findfilesarchive: %d\n", archive);

	string name;
	int s;

	int ret = a->FindFiles(cur, &name, &s);
	strcpy(nameBuf, name.c_str());
	*size = s;

	return ret;
}

Export(int) OpenArchiveFile(int archive, const char* name)
{
	ASSERT(openArchives.find(archive) != openArchives.end(), "Unregistered archive. Pass the handle returned by OpenArchive to OpenArchiveFile.");
	CHECK_NULL_OR_EMPTY(name);

	CArchiveBase* a = openArchives[archive];
	return a->OpenFile(name);
}

Export(int) ReadArchiveFile(int archive, int handle, void* buffer, int numBytes)
{
	ASSERT(openArchives.find(archive) != openArchives.end(), "Unregistered archive. Pass the handle returned by OpenArchive to ReadArchiveFile.");
	CHECK_NULL(buffer);

	CArchiveBase* a = openArchives[archive];
	return a->ReadFile(handle, buffer, numBytes);
}

Export(void) CloseArchiveFile(int archive, int handle)
{
	ASSERT(openArchives.find(archive) != openArchives.end(), "Unregistered archive. Pass the handle returned by OpenArchive to CloseArchiveFile.");

	CArchiveBase* a = openArchives[archive];
	return a->CloseFile(handle);
}

Export(int) SizeArchiveFile(int archive, int handle)
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

Export(const char*) GetSpringConfigString( const char* name, const char* defValue )
{
	string res = configHandler.GetString( name, defValue );
	return GetStr(res);
}

Export(int) GetSpringConfigInt( const char* name, const int defValue )
{
	return configHandler.GetInt( name, defValue );
}

Export(float) GetSpringConfigFloat( const char* name, const float defValue)
{
	return configHandler.GetFloat( name, defValue );
}

Export(void) SetSpringConfigString(const char* name, const char* value)
{
	configHandler.SetString( name, value );
}

Export(void) SetSpringConfigInt(const char* name, const int value)
{
	configHandler.SetInt( name, value );
}

Export(void) SetSpringConfigFloat(const char* name, const float value)
{
	configHandler.SetFloat( name, value );
}

