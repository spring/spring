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
#include "Sim/SideParser.h"
#include "ExternalAI/Interface/aidefines.h"
#include "ExternalAI/Interface/SSAILibrary.h"

#include "LuaParserAPI.h"
#include "Syncer.h"
#include "SyncServer.h"
#include "unitsyncLogOutput.h"

#include <string>
#include <string.h>
#include <vector>
#include <set>
#include <algorithm>
#include <cstdio>
#include <cstdarg>

using std::string;


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
BOOL DllMain(HINSTANCE hInst,
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


const char* GetSpringVersion()
{
	return VERSION_STRING;
}

void Message(const char* p_szMessage)
{
	MessageBox(NULL, p_szMessage, "Message from DLL", MB_OK);
}

void UnInit()
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

int Init(bool isServer, int id)
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

int ProcessUnits(void)
{
	logOutput.Print("syncer: process units\n");
	return syncer->ProcessUnits();
}

int ProcessUnitsNoChecksum(void)
{
	logOutput.Print("syncer: process units\n");
	return syncer->ProcessUnits(false);
}

const char* GetCurrentList()
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

void AddClient(int id, const char *unitList)
{
	logOutput.Print("syncer: add client\n");
	((CSyncServer *)syncer)->AddClient(id, unitList);
}

void RemoveClient(int id)
{
	logOutput.Print("syncer: remove client\n");
	((CSyncServer *)syncer)->RemoveClient(id);
}

const char* GetClientDiff(int id)
{
	logOutput.Print("syncer: get client diff\n");
	string tmp = ((CSyncServer *)syncer)->GetClientDiff(id);
	return GetStr(tmp);
}

void InstallClientDiff(const char *diff)
{
	logOutput.Print("syncer: install client diff\n");
	syncer->InstallClientDiff(diff);
}

int GetUnitCount()
{
	logOutput.Print("syncer: get unit count\n");
	return syncer->GetUnitCount();
}

const char* GetUnitName(int unit)
{
	logOutput.Print("syncer: get unit %d name\n", unit);
	string tmp = syncer->GetUnitName(unit);
	return GetStr(tmp);
}

const char* GetFullUnitName(int unit)
{
	logOutput.Print("syncer: get full unit %d name\n", unit);
	string tmp = syncer->GetFullUnitName(unit);
	return GetStr(tmp);
}

int IsUnitDisabled(int unit)
{
	logOutput.Print("syncer: is unit %d disabled\n", unit);
	if (syncer->IsUnitDisabled(unit))
		return 1;
	else
		return 0;
}

int IsUnitDisabledByClient(int unit, int clientId)
{
	logOutput.Print("syncer: is unit %d disabled by client %d\n", unit, clientId);
	if (syncer->IsUnitDisabledByClient(unit, clientId))
		return 1;
	else
		return 0;
}

//////////////////////////
//////////////////////////

void AddArchive(const char* name)
{
	ASSERT(archiveScanner && vfsHandler, "Call InitArchiveScanner before AddArchive.");
	ASSERT(name && *name, "Don't pass a NULL pointer or an empty string to AddArchive.");
	vfsHandler->AddArchive(name, false);
}

void AddAllArchives(const char* root)
{
	ASSERT(archiveScanner && vfsHandler, "Call InitArchiveScanner before AddAllArchives.");
	ASSERT(root && *root, "Don't pass a NULL pointer or an empty string to AddAllArchives.");
	vector<string> ars = archiveScanner->GetArchives(root);
//	Message(root);
	for (vector<string>::iterator i = ars.begin(); i != ars.end(); ++i) {
		logOutput.Print("adding archive: %s\n", i->c_str());
		vfsHandler->AddArchive(*i, false);
	}
}

unsigned int GetArchiveChecksum(const char* arname)
{
	ASSERT(archiveScanner && vfsHandler, "Call InitArchiveScanner before GetArchiveChecksum.");
	ASSERT(arname && *arname, "Don't pass a NULL pointer or an empty string to GetArchiveChecksum.");
	logOutput.Print("archive checksum: %s\n", arname);
	return archiveScanner->GetArchiveChecksum(arname);
}

const char* GetArchivePath(const char* arname)
{
	ASSERT(archiveScanner && vfsHandler, "Call InitArchiveScanner before GetArchivePath.");
	ASSERT(arname && *arname, "Don't pass a NULL pointer or an empty string to GetArchivePath.");
	logOutput.Print("archive path: %s\n", arname);
	return GetStr(archiveScanner->GetArchivePath(arname));
}

// Updated on every call to GetMapCount
static vector<string> mapNames;

int GetMapCount()
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

const char* GetMapName(int index)
{
	ASSERT(archiveScanner && vfsHandler, "Call InitArchiveScanner before GetMapName.");
	ASSERT((unsigned)index < mapNames.size(), "Array index out of bounds. Call GetMapCount before GetMapName.");
	return GetStr(mapNames[index]);
}


int GetMapInfoEx(const char* name, MapInfo* outInfo, int version)
{
	ASSERT(archiveScanner && vfsHandler, "Call InitArchiveScanner before GetMapInfo.");
	ASSERT(name && *name && outInfo, "Don't pass a NULL pointer or an empty string to GetMapInfo.");

	const string mapName = name;
	ScopedMapLoader mapLoader(mapName);

	string err("");

	MapParser mapParser(mapName);
	if (!mapParser.IsValid()) {
		err = mapParser.GetErrorLog();
	}
	const	LuaTable mapTable = mapParser.GetRoot();

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


int GetMapInfo(const char* name, MapInfo* outInfo)
{
	return GetMapInfoEx(name, outInfo, 0);
}

static vector<string> mapArchives;

int GetMapArchiveCount(const char* mapName)
{
	ASSERT(archiveScanner && vfsHandler, "Call InitArchiveScanner before GetMapArchiveCount.");
	mapArchives = archiveScanner->GetArchivesForMap(mapName);
	return mapArchives.size();
}

const char* GetMapArchiveName(int index)
{
	ASSERT(archiveScanner && vfsHandler, "Call InitArchiveScanner before GetMapArchiveName.");
	ASSERT((unsigned)index < mapArchives.size(), "Array index out of bounds. Call GetMapArchiveCount before GetMapArchiveName.");
	return GetStr(mapArchives[index]);
}

unsigned int GetMapChecksum(int index)
{
	ASSERT(archiveScanner && vfsHandler, "Call InitArchiveScanner before GetMapChecksum.");
	ASSERT((unsigned)index < mapNames.size(), "Array index out of bounds. Call GetMapCount before GetMapChecksum.");
	return archiveScanner->GetMapChecksum(mapNames[index]);
}

unsigned int GetMapChecksumFromName(const char* mapName)
{
	ASSERT(archiveScanner && vfsHandler, "Call InitArchiveScanner before GetMapChecksumFromName.");
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

void* GetMinimap(const char* filename, int miplevel)
{
	ASSERT(archiveScanner && vfsHandler, "Call InitArchiveScanner before GetMinimap.");
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


int GetPrimaryModCount()
{
	ASSERT(archiveScanner && vfsHandler, "Call InitArchiveScanner before GetPrimaryModCount.");
	modData = archiveScanner->GetPrimaryMods();
	return modData.size();
}


const char* GetPrimaryModName(int index)
{
	ASSERT(archiveScanner && vfsHandler, "Call InitArchiveScanner before GetPrimaryModName.");
	ASSERT((unsigned)index < modData.size(), "Array index out of bounds. Call GetPrimaryModCount before GetPrimaryModName.");
	string x = modData[index].name;
	return GetStr(x);
}


const char* GetPrimaryModShortName(int index)
{
	ASSERT(archiveScanner && vfsHandler, "Call InitArchiveScanner before GetPrimaryModShortName.");
	ASSERT((unsigned)index < modData.size(), "Array index out of bounds. Call GetPrimaryModCount before GetPrimaryModShortName.");
	string x = modData[index].shortName;
	return GetStr(x);
}


const char* GetPrimaryModVersion(int index)
{
	ASSERT(archiveScanner && vfsHandler, "Call InitArchiveScanner before GetPrimaryModVersion.");
	ASSERT((unsigned)index < modData.size(), "Array index out of bounds. Call GetPrimaryModCount before GetPrimaryModMutator.");
	string x = modData[index].version;
	return GetStr(x);
}


const char* GetPrimaryModMutator(int index)
{
	ASSERT(archiveScanner && vfsHandler, "Call InitArchiveScanner before GetPrimaryModMutator.");
	ASSERT((unsigned)index < modData.size(), "Array index out of bounds. Call GetPrimaryModCount before GetPrimaryModMutator.");
	string x = modData[index].mutator;
	return GetStr(x);
}


const char* GetPrimaryModGame(int index)
{
	ASSERT(archiveScanner && vfsHandler, "Call InitArchiveScanner before GetPrimaryModName.");
	ASSERT((unsigned)index < modData.size(), "Array index out of bounds. Call GetPrimaryModCount before GetPrimaryModName.");
	string x = modData[index].game;
	return GetStr(x);
}


const char* GetPrimaryModShortGame(int index)
{
	ASSERT(archiveScanner && vfsHandler, "Call InitArchiveScanner before GetPrimaryModShortGame.");
	ASSERT((unsigned)index < modData.size(), "Array index out of bounds. Call GetPrimaryModCount before GetPrimaryModShortGame.");
	string x = modData[index].shortGame;
	return GetStr(x);
}


const char* GetPrimaryModDescription(int index)
{
	ASSERT(archiveScanner && vfsHandler, "Call InitArchiveScanner before GetPrimaryModDescription.");
	ASSERT((unsigned)index < modData.size(), "Array index out of bounds. Call GetPrimaryModCount before GetPrimaryModDescription.");
	string x = modData[index].description;
	return GetStr(x);
}


const char* GetPrimaryModArchive(int index)
{
	ASSERT(archiveScanner && vfsHandler, "Call InitArchiveScanner before GetPrimaryModArchive.");
	ASSERT((unsigned)index < modData.size(), "Array index out of bounds. Call GetPrimaryModCount before GetPrimaryModArchive.");
	return GetStr(modData[index].dependencies[0]);
}


vector<string> primaryArchives;

int GetPrimaryModArchiveCount(int index)
{
	ASSERT(archiveScanner && vfsHandler, "Call InitArchiveScanner before GetPrimaryModArchiveCount.");
	ASSERT((unsigned)index < modData.size(), "Array index out of bounds. Call GetPrimaryModCount before GetPrimaryModArchiveCount.");
	primaryArchives = archiveScanner->GetArchives(modData[index].dependencies[0]);
	return primaryArchives.size();
}

const char* GetPrimaryModArchiveList(int archiveNr)
{
	ASSERT(archiveScanner && vfsHandler, "Call InitArchiveScanner before GetPrimaryModArchiveList.");
	ASSERT((unsigned)archiveNr < primaryArchives.size(), "Array index out of bounds. Call GetPrimaryModArchiveCount before GetPrimaryModArchiveList.");
	logOutput.Print("primary mod archive list: %s\n", primaryArchives[archiveNr].c_str());
	return GetStr(primaryArchives[archiveNr]);
}

int GetPrimaryModIndex(const char* name)
{
	ASSERT(archiveScanner && vfsHandler, "Call InitArchiveScanner before GetPrimaryModIndex.");
	string n(name);
	for (unsigned i = 0; i < modData.size(); ++i) {
		if (modData[i].name == n)
			return i;
	}
	// if it returns -1, make sure you call GetPrimaryModCount before GetPrimaryModIndex.
	return -1;
}

unsigned int GetPrimaryModChecksum(int index)
{
	ASSERT(archiveScanner && vfsHandler, "Call InitArchiveScanner before GetPrimaryModChecksum.");
	ASSERT((unsigned)index < modData.size(), "Array index out of bounds. Call GetPrimaryModCount before GetPrimaryModChecksum.");
	return archiveScanner->GetModChecksum(GetPrimaryModArchive(index));
}

unsigned int GetPrimaryModChecksumFromName(const char* name)
{
	ASSERT(archiveScanner && vfsHandler, "Call InitArchiveScanner before GetPrimaryModChecksumFromName.");
	return archiveScanner->GetModChecksum(archiveScanner->ModNameToModArchive(name));
}


//////////////////////////
//////////////////////////

int GetSideCount()
{
	if (!sideParser.Load()) {
		logOutput.Print("failed: %s\n", sideParser.GetErrorLog().c_str());
		return 0;
	}
	return sideParser.GetCount();
}


const char* GetSideName(int side)
{
	ASSERT((unsigned int)side < sideParser.GetCount(),
	       "Array index out of bounds. Call GetSideCount before GetSideName.");
	// the full case name  (not the lowered version)
	return GetStr(sideParser.GetCaseName(side));
}


const char* GetSideStartUnit(int side)
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


int GetLuaAICount()
{
	GetLuaAIOptions();
	return luaAIOptions.size();
}


const char* GetLuaAIName(int aiIndex)
{
	if ((aiIndex < 0) || (aiIndex >= (int)luaAIOptions.size())) {
		return NULL;
	}
	return GetStr(luaAIOptions[aiIndex].name);
}


const char* GetLuaAIDesc(int aiIndex)
{
	if ((aiIndex < 0) || (aiIndex >= (int)luaAIOptions.size())) {
		return NULL;
	}
	return GetStr(luaAIOptions[aiIndex].desc);
}


//////////////////////////
//////////////////////////

//static const char* badKeyChars = " =;\r\n\t";


/*
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
*/


static vector<Option> options;

/*

static bool ParseOption(const LuaTable& root, int index, Option& opt)
{
	const LuaTable& optTbl = root.SubTable(index);
	if (!optTbl.IsValid()) {
		return false;
	}

	// common options properties
	string opt_key = optTbl.GetString("key", "");
	if (opt_key.empty() ||
	    (opt_key.find_first_of(badKeyChars) != string::npos)) {
		return false;
	}
	opt_key = StringToLower(opt_key);
	if (optionsSet.find(opt_key) != optionsSet.end()) {
		return false;
	}
	opt.key = opt_key.c_str();
	string opt_name = optTbl.GetString("name", opt_key);
	if (opt_name.empty()) {
		return false;
	}
	opt.name = opt_name.c_str();
	string opt_desc = optTbl.GetString("desc", opt_name);
	opt.desc = opt_desc.c_str();
	

	string opt_type = optTbl.GetString("type", "");
	opt_type = StringToLower(opt_type);
	opt.type = opt_type.c_str();

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
		opt.stringDef    = optTbl.GetString("def", "").c_str();
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
			string item_key = listTbl.GetString(i, "");
			if (!item_key.empty() &&
			    (item_key.find_first_of(badKeyChars) == string::npos)) {
				item.key = item_key.c_str();
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
			    (item_key.find_first_of(badKeyChars) != string::npos)) {
				return false;
			}
			item_key = StringToLower(item_key);
			item.key = item_key.c_str();
			string item_name = itemTbl.GetString("name", item_key);
			if (item_name.empty()) {
				return false;
			}
			item.name = item_name.c_str();
			string item_desc = itemTbl.GetString("desc", item_name);
			item.desc = item_desc.c_str();
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

		opt.listDef = optTbl.GetString("def", opt.list[0].name).c_str();
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

	const string configName = MapParser::GetMapConfigName(mapName);

	if (!mapName.empty() && !configName.empty()) {
		luaParser.GetTable("Map");
		luaParser.AddString("fileName", mapName);
		luaParser.AddString("fullName", "maps/" + mapName);
		luaParser.AddString("configFile", configName);
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
}

*/

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


vector<InfoItem> infos;
set<string> infosSet;
/*
static bool ParseInfo(const LuaTable& root, int index, InfoItem& info)
{
	const LuaTable& infoTbl = root.SubTable(index);
	if (!infoTbl.IsValid()) {
		return false;
	}

	// info properties
	info.key = infoTbl.GetString("key", "");
	if (info.key.empty() ||
	    (info.key.find_first_of(badKeyChars) != string::npos)) {
		return false;
	}
	string keyLower = StringToLower(info.key);
	if (infosSet.find(keyLower) != infosSet.end()) {
		return false;
	}
	
	info.value = infoTbl.GetString("value", "");
	if (info.value.empty()) {
		return false;
	}
	
	info.desc = infoTbl.GetString("desc", "");

	infosSet.insert(keyLower);

	return true;
}

static void ParseInfos(const string& fileName,
                         const string& fileModes,
                         const string& accessModes)
{
	infos.clear();
	
	LuaParser luaParser(fileName, fileModes, accessModes);
		
	if (!luaParser.Execute()) {
		printf("ParseInfos(%s) ERROR: %s\n",
		       fileName.c_str(), luaParser.GetErrorLog().c_str());
		return;
	}

	const LuaTable root = luaParser.GetRoot();
	if (!root.IsValid()) {
		return;
	}

	infosSet.clear();
	for (int index = 1; root.KeyExists(index); index++) {
		InfoItem info;
		if (ParseInfo(root, index, info)) {
			infos.push_back(info);
		}
	}
	infosSet.clear();
}
*/


std::vector<Option> ParseOptions(
		const std::string& fileName,
		const std::string& fileModes,
		const std::string& accessModes,
		const std::string& mapName = "")
{
	std::vector<Option> options;
	
	static const unsigned int MAX_OPTIONS = 128;
	Option tmpOptions[MAX_OPTIONS];
	unsigned int num = ParseOptions(fileName.c_str(), fileModes.c_str(),
			accessModes.c_str(), mapName.c_str(), tmpOptions, MAX_OPTIONS);
	for (unsigned int i=0; i < num; ++i) {
		options.push_back(tmpOptions[i]);
    }
	
	return options;
}

int GetMapOptionCount(const char* name)
{
	ASSERT(archiveScanner && vfsHandler,
	       "Call InitArchiveScanner before GetMapOptionCount.");
	ASSERT(name && *name,
				 "Don't pass a NULL pointer or an empty string to GetMapOptionCount.");

	ScopedMapLoader mapLoader(name);

	options = ParseOptions("MapOptions.lua", SPRING_VFS_MAP, SPRING_VFS_MAP, name);

	return (int)options.size();
}


int GetModOptionCount()
{
	options = ParseOptions("ModOptions.lua", SPRING_VFS_MOD, SPRING_VFS_MOD);
	return (int)options.size();
}


// Updated on every call to GetSkirmishAICount
static vector<string> skirmishAIDataDirs;

int GetSkirmishAICount() {
	
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

struct SSAISpecifyer GetSkirmishAISpecifyer(int index) {
	
	SSAISpecifyer spec = {NULL, NULL};
	
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
	std::vector<InfoItem> infos;
	
	static const unsigned int MAX_INFOS = 128;
	InfoItem tmpInfos[MAX_INFOS];
	unsigned int num = ParseInfos(fileName.c_str(), fileModes.c_str(), accessModes.c_str(), tmpInfos, MAX_INFOS);
	for (unsigned int i=0; i < num; ++i) {
		infos.push_back(tmpInfos[i]);
    }
	
	return infos;
}

int GetSkirmishAIInfoCount(int index) {
	
	ASSERT(archiveScanner && vfsHandler, "Call InitArchiveScanner before GetSkirmishAIInfoCount.");
	infos = ParseInfos(skirmishAIDataDirs[index] + "/AIInfo.lua", SPRING_VFS_RAW, SPRING_VFS_RAW);
	return (int)infos.size();
}
const char* GetInfoKey(int index) {
	
	ASSERT(!infos.empty(), "Call GetSkirmishAIInfoCount before GetInfoKey.");
	return infos.at(index).key;
}
const char* GetInfoValue(int index) {
	
	ASSERT(!infos.empty(), "Call GetSkirmishAIInfoCount before GetInfoValue.");
	return infos.at(index).value;
}
const char* GetInfoDescription(int index) {
	
	ASSERT(!infos.empty(), "Call GetSkirmishAIInfoCount before GetInfoDescription.");
	return infos.at(index).desc;
}
//struct LevelOfSupport GetSkirmishAILevelOfSupport(int index, const char* engineVersionString, int engineVersionNumber, const char* aiInterfaceShortName, const char* aiInterfaceVersion) {}



int GetSkirmishAIOptionCount(int index) {
//int GetSkirmishAIOptionCount(const char* shortName, const char* version = NULL) {
	
	ASSERT(!skirmishAIDataDirs.empty(), "Call GetSkirmishAICount before GetSkirmishAIOptionCount.");
	//SSAISpecifyer spec = {shortName, version};
	options = ParseOptions(skirmishAIDataDirs[index] + "/AIOptions.lua", SPRING_VFS_RAW, SPRING_VFS_RAW);
	return (int)options.size();
}


const char* GetOptionKey(int optIndex)
{
	if (InvalidOptionIndex(optIndex)) {
		return NULL;
	}
	return GetStr(options[optIndex].key);
}


const char* GetOptionName(int optIndex)
{
	if (InvalidOptionIndex(optIndex)) {
		return NULL;
	}
	return GetStr(options[optIndex].name);
}


const char* GetOptionDesc(int optIndex)
{
	if (InvalidOptionIndex(optIndex)) {
		return NULL;
	}
	return GetStr(options[optIndex].desc);
}


int GetOptionType(int optIndex)
{
	if (InvalidOptionIndex(optIndex)) {
		return 0;
	}
	return options[optIndex].typeCode;
}


int GetOptionBoolDef(int optIndex)
{
	if (WrongOptionType(optIndex, opt_bool)) {
		return 0;
	}
	return options[optIndex].boolDef ? 1 : 0;
}


float GetOptionNumberDef(int optIndex)
{
	if (WrongOptionType(optIndex, opt_number)) {
		return 0.0f;
	}
	return options[optIndex].numberDef;
}


float GetOptionNumberMin(int optIndex)
{
	if (WrongOptionType(optIndex, opt_number)) {
		return -1.0e30f; // FIXME ?
	}
	return options[optIndex].numberMin;
}


float GetOptionNumberMax(int optIndex)
{
	if (WrongOptionType(optIndex, opt_number)) {
		return +1.0e30f; // FIXME ?
	}
	return options[optIndex].numberMax;
}


float GetOptionNumberStep(int optIndex)
{
	if (WrongOptionType(optIndex, opt_number)) {
		return 0.0f;
	}
	return options[optIndex].numberStep;
}


const char* GetOptionStringDef(int optIndex)
{
	if (WrongOptionType(optIndex, opt_string)) {
		return NULL;
	}
	return GetStr(options[optIndex].stringDef);
}


int GetOptionStringMaxLen(int optIndex)
{
	if (WrongOptionType(optIndex, opt_string)) {
		return 0;
	}
	return options[optIndex].stringMaxLen;
}


int GetOptionListCount(int optIndex)
{
	if (WrongOptionType(optIndex, opt_list)) {
		return 0;
	}
	return options[optIndex].numListItems;
}


const char* GetOptionListDef(int optIndex)
{
	if (WrongOptionType(optIndex, opt_list)) {
		return 0;
	}
	return GetStr(options[optIndex].listDef);
}


const char* GetOptionListItemKey(int optIndex, int itemIndex)
{
	if (WrongOptionType(optIndex, opt_list)) {
		return NULL;
	}
	//const vector<OptionListItem>& list = options[optIndex].list;
	if ((itemIndex < 0) || (itemIndex >= options[optIndex].numListItems)) {
		return NULL;
	}
	return GetStr(options[optIndex].list[itemIndex].key);
}


const char* GetOptionListItemName(int optIndex, int itemIndex)
{
	if (WrongOptionType(optIndex, opt_list)) {
		return NULL;
	}
	//const vector<OptionListItem>& list = options[optIndex].list;
	if ((itemIndex < 0) || (itemIndex >= options[optIndex].numListItems)) {
		return NULL;
	}
	return GetStr(options[optIndex].list[itemIndex].name);
}


const char* GetOptionListItemDesc(int optIndex, int itemIndex)
{
	if (WrongOptionType(optIndex, opt_list)) {
		return NULL;
	}
	//const vector<OptionListItem>& list = options[optIndex].list;
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


int GetModValidMapCount()
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


const char* GetModValidMap(int index)
{
	if ((index < 0) || (index >= (int)modValidMaps.size())) {
		return NULL;
	}
	return GetStr(modValidMaps[index]);
}


//////////////////////////
//////////////////////////


static map<int, CFileHandler*> openFiles;
static int nextFile = 0;
static vector<string> curFindFiles;

int OpenFileVFS(const char* name)
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

void CloseFileVFS(int handle)
{
	ASSERT(openFiles.find(handle) != openFiles.end(), "Unregistered handle. Pass the handle returned by OpenFileVFS to CloseFileVFS.");
	logOutput.Print("closefilevfs: %d\n", handle);
	delete openFiles[handle];
	openFiles.erase(handle);
}

void ReadFileVFS(int handle, void* buf, int length)
{
	ASSERT(openFiles.find(handle) != openFiles.end(), "Unregistered handle. Pass the handle returned by OpenFileVFS to ReadFileVFS.");
	ASSERT(buf, "Don't pass a NULL pointer to ReadFileVFS.");
	logOutput.Print("readfilevfs: %d\n", handle);
	CFileHandler* fh = openFiles[handle];
	fh->Read(buf, length);
}

int FileSizeVFS(int handle)
{
	ASSERT(openFiles.find(handle) != openFiles.end(), "Unregistered handle. Pass the handle returned by OpenFileVFS to FileSizeVFS.");
	logOutput.Print("filesizevfs: %d\n", handle);
	CFileHandler* fh = openFiles[handle];
	return fh->FileSize();
}


int InitFindVFS(const char* pattern)
{
	string path = filesystem.GetDirectory(pattern);
	string patt = filesystem.GetFilename(pattern);
	logOutput.Print("initfindvfs: %s\n", pattern);
	curFindFiles = CFileHandler::FindFiles(path, patt);
	return 0;
}

int InitDirListVFS(const char* path, const char* pattern, const char* modes)
{
	if (path    == NULL) { path = "";              }
	if (modes   == NULL) { modes = SPRING_VFS_ALL; }
	if (pattern == NULL) { pattern = "*";          }
	logOutput.Print("InitDirListVFS: '%s' '%s' '%s'\n", path, pattern, modes);
	curFindFiles = CFileHandler::DirList(path, pattern, modes);
	return 0;
}

int InitSubDirsVFS(const char* path, const char* pattern, const char* modes)
{
	if (path    == NULL) { path = "";              }
	if (modes   == NULL) { modes = SPRING_VFS_ALL; }
	if (pattern == NULL) { pattern = "*";          }
	logOutput.Print("InitSubDirsVFS: '%s' '%s' '%s'\n", path, pattern, modes);
	curFindFiles = CFileHandler::SubDirs(path, pattern, modes);
	return 0;
}

int FindFilesVFS(int handle, char* nameBuf, int size)
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

int OpenArchive(const char* name)
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

int OpenArchiveType(const char* name, const char* type)
{
	ASSERT(name && *name && type && *type,
	       "Don't pass a NULL pointer or an empty string to OpenArchiveType.");
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

void CloseArchive(int archive)
{
	ASSERT(openArchives.find(archive) != openArchives.end(), "Unregistered archive. Pass the handle returned by OpenArchive to CloseArchive.");
	delete openArchives[archive];
	openArchives.erase(archive);
}

int FindFilesArchive(int archive, int cur, char* nameBuf, int* size)
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

int OpenArchiveFile(int archive, const char* name)
{
	ASSERT(openArchives.find(archive) != openArchives.end(), "Unregistered archive. Pass the handle returned by OpenArchive to OpenArchiveFile.");
	ASSERT(name && *name, "Don't pass a NULL pointer or an empty string to OpenArchiveFile.");
	CArchiveBase* a = openArchives[archive];
	return a->OpenFile(name);
}

int ReadArchiveFile(int archive, int handle, void* buffer, int numBytes)
{
	ASSERT(openArchives.find(archive) != openArchives.end(), "Unregistered archive. Pass the handle returned by OpenArchive to ReadArchiveFile.");
	ASSERT(buffer, "Don't pass a NULL pointer to ReadArchiveFile.");
	CArchiveBase* a = openArchives[archive];
	return a->ReadFile(handle, buffer, numBytes);
}

void CloseArchiveFile(int archive, int handle)
{
	ASSERT(openArchives.find(archive) != openArchives.end(), "Unregistered archive. Pass the handle returned by OpenArchive to CloseArchiveFile.");
	CArchiveBase* a = openArchives[archive];
	return a->CloseFile(handle);
}

int SizeArchiveFile(int archive, int handle)
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

const char* GetSpringConfigString( const char* name, const char* defValue )
{
	string res = configHandler.GetString( name, defValue );
	return GetStr(res);
}

int GetSpringConfigInt( const char* name, const int defValue )
{
	return configHandler.GetInt( name, defValue );
}

float GetSpringConfigFloat( const char* name, const float defValue)
{
	return configHandler.GetFloat( name, defValue );
}

void SetSpringConfigString(const char* name, const char* value)
{
	configHandler.SetString( name, value );
}

void SetSpringConfigInt(const char* name, const int value)
{
	configHandler.SetInt( name, value );
}

void SetSpringConfigFloat(const char* name, const float value)
{
	configHandler.SetFloat( name, value );
}

