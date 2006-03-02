#include "stdafx.h"
#include "unitsync.h"

#include "FileSystem/VFSHandler.h"
#include "FileSystem/ArchiveScanner.h"
#include "FileSystem/FileHandler.h"
#include "TdfParser.h"
#include "Sim/Map/mapfile.h"
#include "FileSystem/ArchiveFactory.h"

#include "Syncer.h"
#include "SyncServer.h"

#include <string>
#include <vector>
#include <algorithm>

//This means that the DLL can only support one instance. Don't think this should be a problem.
static CSyncer *syncer = NULL;
static CArchiveScanner* scanner = NULL;

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

DLL_EXPORT int __stdcall Init(bool isServer, int id)
{
	if (hpiHandler)
		delete hpiHandler;
	hpiHandler = new CVFSHandler();

	if (isServer) {
		syncer = new CSyncServer(id);
	}
	else {
		syncer = new CSyncer(id);
	}

	return 1;
}

DLL_EXPORT void __stdcall UnInit()
{
	if ( hpiHandler )
	{
		delete hpiHandler;
		hpiHandler = 0;
	}

	if ( syncer )
	{
		delete syncer;
		syncer = 0;
	}

	if (scanner) {
		delete scanner;
		scanner = NULL;
	}
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
	if (!scanner) 
		scanner = new CArchiveScanner();
	else {
		delete scanner;
		scanner = new CArchiveScanner();
	}

	scanner->ReadCacheData();
	scanner->Scan("./maps", true);
	scanner->Scan("./base", true);
	scanner->Scan("./mods", true);
	scanner->WriteCacheData();

	if (!hpiHandler)
		hpiHandler = new CVFSHandler();

	return 1;
}

DLL_EXPORT void __stdcall AddArchive(const char* name)
{
	hpiHandler->AddArchive(name, false);
}

DLL_EXPORT void __stdcall AddAllArchives(const char* root)
{
	vector<string> ars = scanner->GetArchives(root);
//	Message(root);
	for (vector<string>::iterator i = ars.begin(); i != ars.end(); ++i) {
		hpiHandler->AddArchive(*i, false);
	}
}

DLL_EXPORT unsigned int __stdcall GetArchiveChecksum(const char* arname)
{
	return scanner->GetArchiveChecksum(arname);
}

// Updated on every call to getmapcount
static vector<string> mapNames;

DLL_EXPORT int __stdcall GetMapCount()
{
	vector<string> files = CFileHandler::FindFiles("maps/*.smf");
	vector<string> ars = scanner->GetMaps();

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
	return GetStr(mapNames[index]);
}

DLL_EXPORT int __stdcall GetMapInfo(const char* name, MapInfo* outInfo)
{
	// Determine if the map is found in an archive
	string mapName = name;
	CVFSHandler *oh = hpiHandler;

	CFileHandler* f = new CFileHandler("maps/" + mapName);
	if (!f->FileExists()) {
		vector<string> ars = scanner->GetArchivesForMap(mapName);

		hpiHandler = new CVFSHandler(false);
		hpiHandler->AddArchive(ars[0], false);
	}
	delete f;

	TdfParser parser;
	string smd = mapName.replace(mapName.find_last_of('.'), 4, ".smd");

	string err("");
	try {
		parser.LoadFile("maps/" + smd);
	} catch (const TdfParser::parse_error& e) {
		err = e.what();
	}

	// Retrieve the map header as well
	string origName = name;
	CFileHandler* in = new CFileHandler("maps/" + origName);
	MapHeader mh;
	mh.mapx = -1;
	if (in->FileExists()) {
		in->Read(&mh, sizeof(mh));
	}
	delete in;

	if (hpiHandler != oh) {
		delete hpiHandler;
		hpiHandler = oh;
	}

	// If the map didn't parse, say so now
	if (err.length() > 0) {

		if (err.length() > 254)
			err = err.substr(0, 254);
		strcpy(outInfo->description, err.c_str());

		// Fill in stuff so tasclient won't crash
		outInfo->posCount = 0;
		outInfo->width = mh.mapx * SQUARE_SIZE;
		outInfo->height = mh.mapy * SQUARE_SIZE;
		return 1;
	}

	// Make sure we found stuff in both the smd and the header
	if ( !parser.SectionExist("MAP") ) {
		return 0;
	}
	if (mh.mapx < 0)
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

	values = parser.GetAllValues("MAP\\ATMOSPHERE");

	outInfo->minWind = atoi(values["minwind"].c_str());
	outInfo->maxWind = atoi(values["maxwind"].c_str());

	// New stuff
	outInfo->width = mh.mapx * SQUARE_SIZE;
	outInfo->height = mh.mapy * SQUARE_SIZE;

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

#define RM	0x0000F800
#define GM  0x000007E0
#define BM  0x0000001F

#define RED_RGB565(x) ((x&RM)>>11)
#define GREEN_RGB565(x) ((x&GM)>>5)
#define BLUE_RGB565(x) (x&BM)
#define PACKRGB(r, g, b) (((r<<11)&RM) | ((g << 5)&GM) | (b&BM) )

// Used to return the image
char* imgbuf[1024*1024*2];

DLL_EXPORT void* __stdcall GetMinimap(const char* filename, int miplevel)
{
	// Determine if the map is found in an archive
	string mapName = filename;
	CVFSHandler *oh = hpiHandler;

	CFileHandler* f = new CFileHandler("maps/" + mapName);
	if (!f->FileExists()) {
		vector<string> ars = scanner->GetArchivesForMap(mapName);

		hpiHandler = new CVFSHandler(false);
		hpiHandler->AddArchive(ars[0], false);
	}
	delete f;		

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
		in.Seek(mh.minimapPtr);
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

	// Restore VFS
	if (hpiHandler != oh) {
		delete hpiHandler;
		hpiHandler = oh;
	}

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

//////////////////////////
//////////////////////////

vector<CArchiveScanner::ModData> modData;

DLL_EXPORT int __stdcall GetPrimaryModCount()
{
	modData = scanner->GetPrimaryMods();
	return modData.size();
}

DLL_EXPORT const char* __stdcall GetPrimaryModName(int index)
{
	string x = modData[index].name;
	return GetStr(x);
}

DLL_EXPORT const char* __stdcall GetPrimaryModArchive(int index)
{
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
	primaryArchives = scanner->GetArchives(modData[index].dependencies[0]);
	return primaryArchives.size();
}

DLL_EXPORT const char* __stdcall GetPrimaryModArchiveList(int arnr)
{
	return GetStr(primaryArchives[arnr]);
}

DLL_EXPORT int __stdcall GetPrimaryModIndex(const char* name)
{
	string n(name);
	for (int i = 0; i < modData.size(); ++i) {
		if (modData[i].name == n)
			return i;
	}
	return -1;
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
	} catch (const TdfParser::parse_error& e) {
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
	CFileHandler* fh = openFiles[handle];
	delete fh;
}

DLL_EXPORT void __stdcall ReadFileVFS(int handle, void* buf, int length)
{
	CFileHandler* fh = openFiles[handle];
	fh->Read(buf, length);
}

DLL_EXPORT int __stdcall FileSizeVFS(int handle)
{
	CFileHandler* fh = openFiles[handle];
	return fh->FileSize();
}


// Does not currently support more than one call at a time (a new call to initfind destroys data from previous ones)
// pass the returned handle to findfiles to get the results
DLL_EXPORT int __stdcall InitFindVFS(const char* pattern)
{
	curFindFiles = CFileHandler::FindFiles(pattern);
	return 0;
}

// On first call, pass handle from initfind. pass the return value of this function on subsequent calls
// until 0 is returned. size should be set to max namebuffer size on call
DLL_EXPORT int __stdcall FindFilesVFS(int handle, char* nameBuf, int size)
{
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
	CArchiveBase* a = openArchives[archive];
	openArchives.erase(archive);
}

DLL_EXPORT int __stdcall FindFilesArchive(int archive, int cur, char* nameBuf, int* size)
{
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
	CArchiveBase* a = openArchives[archive];
	return a->OpenFile(name);
}

DLL_EXPORT int __stdcall ReadArchiveFile(int archive, int handle, void* buffer, int numBytes)
{
	CArchiveBase* a = openArchives[archive];
	return a->ReadFile(handle, buffer, numBytes);
}

DLL_EXPORT void __stdcall CloseArchiveFile(int archive, int handle)
{
	CArchiveBase* a = openArchives[archive];
	return a->CloseFile(handle);
}

DLL_EXPORT int __stdcall SizeArchiveFile(int archive, int handle)
{
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
