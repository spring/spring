#include "StdAfx.h"
#include "unitsync.h"

#include "FileSystem/ArchiveFactory.h"
#include "FileSystem/ArchiveScanner.h"
#include "FileSystem/FileHandler.h"
#include "FileSystem/VFSHandler.h"
#include "Map/SMF/mapfile.h"
#include "Platform/ConfigHandler.h"
#include "Platform/FileSystem.h"
#include "Rendering/Textures/Bitmap.h"
#include "TdfParser.h"

#include "Syncer.h"
#include "SyncServer.h"

#include <string>
#include <vector>
#include <algorithm>

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

// And the following makes the hpihandler happy
class CInfoConsole {
public:
	void AddLine(const char *, ...);
	void AddLine(const string& text); // HACKS FTW!   :/
};

// I'd rather not include globalstuff
#define SQUARE_SIZE 8

void CInfoConsole::AddLine(const char *, ...)
{
}
void CInfoConsole::AddLine (const string& text)
{
}

void ErrorMessageBox(const char *msg, const char *capt, unsigned int) {
	MessageBox(0,msg,capt,MB_OK);
}

class CInfoConsole *info;

#ifdef WIN32
BOOL __stdcall DllMain(HINSTANCE hInst,
                       DWORD dwReason,
                       LPVOID lpReserved) {
   return TRUE;
}
#endif

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
	}

	ConfigHandler::Deallocate();
}

DLL_EXPORT int __stdcall Init(bool isServer, int id)
{
	UnInit();

	try {
		// first call to GetInstance() initializes the VFS
		FileSystemHandler::Initialize(false);

		if (isServer) {
			syncer = new CSyncServer(id);
		}
		else {
			syncer = new CSyncer(id);
		}
	} catch (const std::exception& e) {
		Message(e.what());
		return 0;
	}

	return 1;
}

DLL_EXPORT int __stdcall ProcessUnits(void)
{
	return syncer->ProcessUnits();
}

DLL_EXPORT int __stdcall ProcessUnitsNoChecksum(void)
{
	return syncer->ProcessUnits(false);
}

DLL_EXPORT const char * __stdcall GetCurrentList()
{
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

/*void AddClient(int id, char *unitList)
void RemoveClient(int id)
char *GetClientDiff(int id)
void InstallClientDiff(char *diff) */

DLL_EXPORT void __stdcall AddClient(int id, const char *unitList)
{
	((CSyncServer *)syncer)->AddClient(id, unitList);
}

DLL_EXPORT void __stdcall RemoveClient(int id)
{
	((CSyncServer *)syncer)->RemoveClient(id);
}

DLL_EXPORT const char * __stdcall GetClientDiff(int id)
{
	string tmp = ((CSyncServer *)syncer)->GetClientDiff(id);
	return GetStr(tmp);
}

DLL_EXPORT void __stdcall InstallClientDiff(const char *diff)
{
	syncer->InstallClientDiff(diff);
}

/*int GetUnitCount()
char *GetUnitName(int unit)
int IsUnitDisabled(int unit)
int IsUnitDisabledByClient(int unit, int clientId)*/

DLL_EXPORT int __stdcall GetUnitCount()
{
	return syncer->GetUnitCount();
}

DLL_EXPORT const char * __stdcall GetUnitName(int unit)
{
	string tmp = syncer->GetUnitName(unit);
	return GetStr(tmp);
}

DLL_EXPORT const char * __stdcall GetFullUnitName(int unit)
{
	string tmp = syncer->GetFullUnitName(unit);
	return GetStr(tmp);
}

DLL_EXPORT int __stdcall IsUnitDisabled(int unit)
{
	if (syncer->IsUnitDisabled(unit))
		return 1;
	else
		return 0;
}

DLL_EXPORT int __stdcall IsUnitDisabledByClient(int unit, int clientId)
{
	if (syncer->IsUnitDisabledByClient(unit, clientId))
		return 1;
	else
		return 0;
}

//////////////////////////
//////////////////////////

DLL_EXPORT int __stdcall InitArchiveScanner()
{
	// The functionality of this function is now provided by the Init() function
	// once no lobby uses it anymore it can be removed.
	// (there wasn't really a point in having them separate anyway, at least I don't see it)
	return 1;
}

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
		hpiHandler->AddArchive(*i, false);
	}
}

DLL_EXPORT unsigned int __stdcall GetArchiveChecksum(const char* arname)
{
	ASSERT(archiveScanner && hpiHandler, "Call InitArchiveScanner before GetArchiveChecksum.");
	ASSERT(arname && *arname, "Don't pass a NULL pointer or an empty string to GetArchiveChecksum.");
	return archiveScanner->GetArchiveChecksum(arname);
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

	// Determine if the map is found in an archive
	string mapName = name;
	CVFSHandler *oh = hpiHandler;

	{
		CFileHandler f("maps/" + mapName);
		if (!f.FileExists()) {
			vector<string> ars = archiveScanner->GetArchivesForMap(mapName);

			hpiHandler = new CVFSHandler(false);
			hpiHandler->AddArchive(ars[0], false);
		}
	}

	TdfParser parser;
	string extension = mapName.substr(mapName.length()-3), maptdf;
	if (extension == "smf")
		maptdf = string("maps/")+mapName.substr(0,mapName.find_last_of('.'))+".smd";
	else if(extension == "sm3")
		maptdf = string("maps/")+mapName;

	string err("");
	try {
		parser.LoadFile(maptdf);
	} catch (const std::exception& e) {
		err = e.what();
	}

	// Retrieve the map header as well
	if (extension == "smf") 
	{
		MapHeader mh;
		string origName = name;
		mh.mapx = -1;

		{
			CFileHandler in("maps/" + origName);
			if (in.FileExists())
				in.Read(&mh, sizeof(mh));
		}
		outInfo->width = mh.mapx * SQUARE_SIZE;
		outInfo->height = mh.mapy * SQUARE_SIZE;

		if (hpiHandler != oh) {
			delete hpiHandler;
			hpiHandler = oh;
		}
	}
	else
	{
		int w = atoi(parser.SGetValueDef(string(), "map\\gameAreaW").c_str());
		int h = atoi(parser.SGetValueDef(string(), "map\\gameAreaH").c_str());
			
		outInfo->width = w * SQUARE_SIZE;
		outInfo->height = h * SQUARE_SIZE;
	}

	// If the map didn't parse, say so now
	if (err.length() > 0) {

		if (err.length() > 254)
			err = err.substr(0, 254);
		strcpy(outInfo->description, err.c_str());

		// Fill in stuff so tasclient won't crash
		outInfo->posCount = 0;
		return 1;
	}

	// Make sure we found stuff in both the smd and the header
	if ( !parser.SectionExist("MAP") ) {
		return 0;
	}
	if (outInfo->width < 0)
		return 0;

	map<string, string> values = parser.GetAllValues("MAP");

	string tmpdesc = values["description"];
	if (tmpdesc.length() > 254)
		tmpdesc = tmpdesc.substr(0, 254);
	strcpy(outInfo->description, tmpdesc.c_str());
	outInfo->tidalStrength = atoi(values["tidalstrength"].c_str());
	outInfo->gravity = atoi(values["gravity"].c_str());
	outInfo->maxMetal = atof(values["maxmetal"].c_str());
	outInfo->extractorRadius = atoi(values["extractorradius"].c_str());

	if(version >= 1)
		strncpy(outInfo->author, values["author"].c_str(), 200);

	values = parser.GetAllValues("MAP\\ATMOSPHERE");

	outInfo->minWind = atoi(values["minwind"].c_str());
	outInfo->maxWind = atoi(values["maxwind"].c_str());

	// Find the start positions
	int curPos;
	for (curPos = 0; curPos < 16; ++curPos) {
		char buf[50];
		sprintf(buf, "MAP\\TEAM%d", curPos);
		string section(buf);

		if (!parser.SectionExist(section))
			break;

		parser.GetDef(outInfo->positions[curPos].x, "-1", section + "\\StartPosX");
		parser.GetDef(outInfo->positions[curPos].z, "-1", section + "\\StartPosZ");
	}

	outInfo->posCount = curPos;

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
	CFileHandler f("maps/" + mapNames[index]);
	if (!f.FileExists())
		return archiveScanner->GetChecksumForMap(mapNames[index]);
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
char* imgbuf[1024*1024*2];

static void* GetMinimapSM3(string mapName, int miplevel)
{
	string minimapFile;
	try {
		TdfParser tdf("Maps/" + mapName);
		minimapFile = tdf.SGetValueDef(string(), "map\\minimap");
	} catch (TdfParser::parse_error&) {
		memset(imgbuf,0,sizeof(imgbuf));
		return imgbuf;
	}

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
	
		MapHeader mh;
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
				bits >>= 2;
				unsigned char code = bits & 0x3;
				
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

DLL_EXPORT void* __stdcall GetMinimap(const char* filename, int miplevel)
{
	ASSERT(archiveScanner && hpiHandler, "Call InitArchiveScanner before GetMinimap.");
	ASSERT(filename && *filename, "Don't pass a NULL pointer or an empty string to GetMinimap.");
	ASSERT(miplevel >= 0 && miplevel <= 10, "Miplevel must be between 0 and 10 (inclusive) in GetMinimap.");

	// Determine if the map is found in an archive
	string mapName = filename;
	string extension = mapName.substr(mapName.length()-3);
	CVFSHandler *oh = hpiHandler; // original handler, so hpiHandler can be restored later

	{
		CFileHandler f("maps/" + mapName);
		if (!f.FileExists()) {
			vector<string> ars = archiveScanner->GetArchivesForMap(mapName);

			if (ars.empty())
				return NULL;

			hpiHandler = new CVFSHandler(false);
			hpiHandler->AddArchive(ars[0], false);
		}
	}

	void *ret = 0;
	if (extension == "smf") 
		ret = GetMinimapSMF(mapName, miplevel);
	else if (extension == "sm3")
		ret = GetMinimapSM3(mapName, miplevel);

	// Restore VFS
	if (hpiHandler != oh) {
		delete hpiHandler;
		hpiHandler = oh;
	}

	return ret;
}

//////////////////////////
//////////////////////////

vector<CArchiveScanner::ModData> modData;

DLL_EXPORT int __stdcall GetPrimaryModCount()
{
	ASSERT(archiveScanner && hpiHandler, "Call InitArchiveScanner before GetPrimaryModCount.");
	modData = archiveScanner->GetPrimaryMods();
	return modData.size();
}

DLL_EXPORT const char* __stdcall GetPrimaryModName(int index)
{
	ASSERT(archiveScanner && hpiHandler, "Call InitArchiveScanner before GetPrimaryModName.");
	ASSERT((unsigned)index < modData.size(), "Array index out of bounds. Call GetPrimaryModCount before GetPrimaryModName.");
	string x = modData[index].name;
	return GetStr(x);
}

DLL_EXPORT const char* __stdcall GetPrimaryModArchive(int index)
{
	ASSERT(archiveScanner && hpiHandler, "Call InitArchiveScanner before GetPrimaryModArchive.");
	ASSERT((unsigned)index < modData.size(), "Array index out of bounds. Call GetPrimaryModCount before GetPrimaryModArchive.");
	return GetStr(modData[index].dependencies[0]);
}

/*
 * These two funtions are used to get the entire list of archives that a mod
 * requires. Call ..Count with selected mod first to get number of archives,
 * and then use ..List for 0 to count-1 to get the name of each archive.
 */
vector<string> primaryArchives;

DLL_EXPORT int __stdcall GetPrimaryModArchiveCount(int index)
{
	ASSERT(archiveScanner && hpiHandler, "Call InitArchiveScanner before GetPrimaryModArchiveCount.");
	ASSERT((unsigned)index < modData.size(), "Array index out of bounds. Call GetPrimaryModCount before GetPrimaryModArchiveCount.");
	primaryArchives = archiveScanner->GetArchives(modData[index].dependencies[0]);
	return primaryArchives.size();
}

DLL_EXPORT const char* __stdcall GetPrimaryModArchiveList(int arnr)
{
	ASSERT(archiveScanner && hpiHandler, "Call InitArchiveScanner before GetPrimaryModArchiveList.");
	ASSERT((unsigned)arnr < primaryArchives.size(), "Array index out of bounds. Call GetPrimaryModArchiveCount before GetPrimaryModArchiveList.");
	return GetStr(primaryArchives[arnr]);
}

DLL_EXPORT int __stdcall GetPrimaryModIndex(const char* name)
{
	ASSERT(archiveScanner && hpiHandler, "Call InitArchiveScanner before GetPrimaryModIndex.");
	string n(name);
	for (int i = 0; i < modData.size(); ++i) {
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
	return archiveScanner->GetChecksum(GetPrimaryModArchive(index));
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

	TdfParser p;
	try {
		p.LoadFile("gamedata/sidedata.tdf");
	} catch (const std::exception&) {
		return 0;
	}

	for(int b=0;;++b){					//loop over all sides
		char sideText[50];
		sprintf(sideText,"side%i",b);
		if(p.SectionExist(sideText)){
			SideData sd;
			sd.name = p.SGetValueDef("arm",string(sideText)+"\\name");
			sideData.push_back(sd);
		} else break;
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

// Map the wanted archives into the VFS with AddArchive before using these functions

static map<int, CFileHandler*> openFiles;
static int nextFile = 0;
static vector<string> curFindFiles;

DLL_EXPORT int __stdcall OpenFileVFS(const char* name)
{
	ASSERT(name && *name, "Don't pass a NULL pointer or an empty string to OpenFileVFS.");
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
	delete openFiles[handle];
	openFiles.erase(handle);
}

DLL_EXPORT void __stdcall ReadFileVFS(int handle, void* buf, int length)
{
	ASSERT(openFiles.find(handle) != openFiles.end(), "Unregistered handle. Pass the handle returned by OpenFileVFS to ReadFileVFS.");
	ASSERT(buf, "Don't pass a NULL pointer to ReadFileVFS.");
	CFileHandler* fh = openFiles[handle];
	fh->Read(buf, length);
}

DLL_EXPORT int __stdcall FileSizeVFS(int handle)
{
	ASSERT(openFiles.find(handle) != openFiles.end(), "Unregistered handle. Pass the handle returned by OpenFileVFS to FileSizeVFS.");
	CFileHandler* fh = openFiles[handle];
	return fh->FileSize();
}


// Does not currently support more than one call at a time (a new call to initfind destroys data from previous ones)
// pass the returned handle to findfiles to get the results
DLL_EXPORT int __stdcall InitFindVFS(const char* pattern)
{
	std::string path = filesystem.GetDirectory(pattern);
	std::string patt = filesystem.GetFilename(pattern);
	curFindFiles = CFileHandler::FindFiles(path, patt);
	return 0;
}

// On first call, pass handle from initfind. pass the return value of this function on subsequent calls
// until 0 is returned. size should be set to max namebuffer size on call
DLL_EXPORT int __stdcall FindFilesVFS(int handle, char* nameBuf, int size)
{
	ASSERT(nameBuf, "Don't pass a NULL pointer to FindFilesVFS.");
	ASSERT(size > 0, "Negative or zero buffer length doesn't make sense.");
	if (handle >= curFindFiles.size())
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
