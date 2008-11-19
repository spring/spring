#/*START
#//##########################################################################//#

file=test.cpp

start=`grep -n '^#END' $file | grep -o '^[^:]*'`
start=`expr $start + 1`
tail -n +$start $file > test.tmp.cxx
echo Clipped $start lines

g++ -I../../../rts/System test.tmp.cxx ../../../game/unitsync.so

./a.out CastlesSDD.smf ba52.sdd

exit

#//##########################################################################//#
#END*/


/******************************************************************************/
/******************************************************************************/
//  Simple file to help test unitsync, compile with:
//
//    g++ -I../../../rts/System test.cxx ../../../game/unitsync.so
//


#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <fstream>
#include <sstream>
using namespace std;

// http://www.codeproject.com/KB/recipes/JSON_Spirit.aspx
#include "json/json_spirit.h"

#include "../unitsync/unitsync.h"

using namespace boost;
using namespace json_spirit;

/******************************************************************************/
/******************************************************************************/
//
//  Generated using:  'grep "EXPORT(" unitsync.cpp'
// Scratch that, copy pasted from javabind.cpp
//

EXPORT(const char* ) GetSpringVersion();
EXPORT(void        ) Message(const char* p_szMessage);
EXPORT(int         ) Init(bool isServer, int id);
EXPORT(void        ) UnInit();
EXPORT(int         ) ProcessUnits(void);
EXPORT(int         ) ProcessUnitsNoChecksum(void);
EXPORT(const char* ) GetCurrentList();
EXPORT(void        ) AddClient(int id, const char *unitList);
EXPORT(void        ) RemoveClient(int id);
EXPORT(const char* ) GetClientDiff(int id);
EXPORT(void        ) InstallClientDiff(const char *diff);
EXPORT(int         ) GetUnitCount();
EXPORT(const char* ) GetUnitName(int unit);
EXPORT(const char* ) GetFullUnitName(int unit);
EXPORT(int         ) IsUnitDisabled(int unit);
EXPORT(int         ) IsUnitDisabledByClient(int unit, int clientId);
EXPORT(void        ) AddArchive(const char* name);
EXPORT(void        ) AddAllArchives(const char* root);
EXPORT(unsigned int) GetArchiveChecksum(const char* arname);
EXPORT(int         ) GetMapCount();
EXPORT(const char* ) GetMapName(int index);
EXPORT(int         ) GetMapInfoEx(const char* name, MapInfo* outInfo, int version);
EXPORT(int         ) GetMapInfo(const char* name, MapInfo* outInfo);
EXPORT(void*       ) GetMinimap(const char* filename, int miplevel);
EXPORT(int         ) GetMapArchiveCount(const char* mapName);
EXPORT(const char* ) GetMapArchiveName(int index);
EXPORT(unsigned int) GetMapChecksumFromName(const char* mapName);
EXPORT(unsigned int) GetMapChecksum(int index);
EXPORT(int         ) GetPrimaryModCount();
EXPORT(const char* ) GetPrimaryModName(int index);
EXPORT(const char*) GetPrimaryModShortName(int index);
EXPORT(const char*) GetPrimaryModGame(int index);
EXPORT(const char*) GetPrimaryModShortGame(int index);
EXPORT(const char*) GetPrimaryModVersion(int index);
EXPORT(const char*) GetPrimaryModMutator(int index);
EXPORT(const char*) GetPrimaryModDescription(int index);
EXPORT(const char* ) GetPrimaryModArchive(int index);
EXPORT(int         ) GetPrimaryModArchiveCount(int index);
EXPORT(const char* ) GetPrimaryModArchiveList(int arnr);
EXPORT(int         ) GetPrimaryModIndex(const char* name);
EXPORT(unsigned int) GetPrimaryModChecksum(int index);
EXPORT(unsigned int) GetPrimaryModChecksumFromName(const char* name);
EXPORT(int         ) GetSideCount();
EXPORT(const char* ) GetSideName(int side);
EXPORT(int         ) OpenFileVFS(const char* name);
EXPORT(void        ) CloseFileVFS(int handle);
EXPORT(void        ) ReadFileVFS(int handle, void* buf, int length);
EXPORT(int         ) FileSizeVFS(int handle);
EXPORT(int         ) InitFindVFS(const char* pattern);
EXPORT(int         ) FindFilesVFS(int handle, char* nameBuf, int size);
EXPORT(int         ) OpenArchive(const char* name);
EXPORT(void        ) CloseArchive(int archive);
EXPORT(int         ) FindFilesArchive(int archive, int cur, char* nameBuf, int* size);
EXPORT(int         ) OpenArchiveFile(int archive, const char* name);
EXPORT(int         ) ReadArchiveFile(int archive, int handle, void* buffer, int numBytes);
EXPORT(void        ) CloseArchiveFile(int archive, int handle);
EXPORT(int         ) SizeArchiveFile(int archive, int handle);

// lua custom lobby settings
EXPORT(int         ) GetMapOptionCount(const char* name);
EXPORT(int         ) GetModOptionCount();

EXPORT(const char* ) GetOptionKey(int optIndex);
EXPORT(const char* ) GetOptionName(int optIndex);
EXPORT(const char* ) GetOptionDesc(int optIndex);
EXPORT(int         ) GetOptionType(int optIndex);

// Bool Options
EXPORT(int         ) GetOptionBoolDef(int optIndex);

// Number Options
EXPORT(float       ) GetOptionNumberDef(int optIndex);
EXPORT(float       ) GetOptionNumberMin(int optIndex);
EXPORT(float       ) GetOptionNumberMax(int optIndex);
EXPORT(float       ) GetOptionNumberStep(int optIndex);

// String Options
EXPORT(const char* ) GetOptionStringDef(int optIndex);
EXPORT(int         ) GetOptionStringMaxLen(int optIndex);

// List Options
EXPORT(int         ) GetOptionListCount(int optIndex);
EXPORT(const char* ) GetOptionListDef(int optIndex);
EXPORT(const char* ) GetOptionListItemKey(int optIndex, int itemIndex);
EXPORT(const char* ) GetOptionListItemName(int optIndex, int itemIndex);
EXPORT(const char* ) GetOptionListItemDesc(int optIndex, int itemIndex);

// Spring settings callback

EXPORT(void        ) SetSpringConfigFile(const char* filenameAsAbsolutePath);
EXPORT(const char* ) GetSpringConfigFile();
EXPORT(const char* ) GetSpringConfigString(const char* name, const char* defvalue);
EXPORT(int         ) GetSpringConfigInt(const char* name, const int defvalue);
EXPORT(float       ) GetSpringConfigFloat( const char* name, const float defvalue );
EXPORT(void        ) SetSpringConfigString(const char* name, const char* value);
EXPORT(void        ) SetSpringConfigInt(const char* name, const int value);
EXPORT(void        ) SetSpringConfigFloat(const char* name, const float value);

/******************************************************************************/
/******************************************************************************/

static void DisplayOptions(int optionCount);
static Array GetOptions(int optionCount);
static void CacheIndex();

static void CacheMaps();


//http://techjude.blogspot.com/2006/11/neat-tostring-for-c.html
template <class T>
string toString(T toBeConverted){
	// create an out string stream
	ostringstream buffer;
	// write the value to be converted to the output stream
	buffer << toBeConverted;
	// get the string value 
	return buffer.str();
}

/******************************************************************************/
/******************************************************************************/

int main(int argc, char** argv)
{
  /*if (argc < 3) {
    printf("usage:  %s <map> <mod>\n", argv[0]);
    exit(1);
  }
  const string map = argv[1];
  const string mod = argv[2];
  printf("MAP = %s\n", map.c_str());
  printf("MOD = %s\n", mod.c_str());
*/

	CacheIndex();

  // load the mod archives
  /*AddAllArchives(mod.c_str());

  // unit names
  while (true) {
  //const int left = ProcessUnits();
    const int left = ProcessUnitsNoChecksum();
  //printf("unitsLeft = %i\n", left);
    if (left <= 0) {
      break;
    }
  }
  printf("  UNITS\n");
  const int unitCount = GetUnitCount();
  for (int i = 0; i < unitCount; i++) {
    const string unitName     = GetUnitName(i);
    const string unitFullName = GetFullUnitName(i);
    printf("    [unit %3i]   %-16s  <%s>\n", i,
           unitName.c_str(), unitFullName.c_str());
  }

  // LuaAI options
  printf("  LuaAI\n");
  const int luaAICount = GetLuaAICount();
  for (int i = 0; i < luaAICount; i++) {
    printf("    %i: name = %s\n", i, GetLuaAIName(i));
    printf("       desc = %s\n",     GetLuaAIDesc(i));
  }

  // MapOptions
  printf("  MapOptions\n");
  const int mapOptCount = GetMapOptionCount(map.c_str());
  DisplayOptions(mapOptCount);

  // ModOptions
  printf("  ModOptions\n");
  const int modOptCount = GetModOptionCount();
  DisplayOptions(modOptCount);

  // ModValidMaps
  printf("  ModValidMaps\n");
  const int modValidMapCount = GetModValidMapCount();
  if (modValidMapCount == 0) {
    printf("    * ALL MAPS *\n");
  }
  else {
    for (int i = 0; i < modValidMapCount; i++) {
      printf("    %i: %s\n", i, GetModValidMap(i));
    }
  }
*/
  UnInit();

  return 0;
}


/******************************************************************************/
/******************************************************************************/

static void DisplayOptions(int optionCount)
{
  for (int i = 0; i < optionCount; i++) {
    printf("    Option #%i\n", i);
    printf("      key  = '%s'\n", GetOptionKey(i));
    printf("      name = '%s'\n", GetOptionName(i));
    printf("      desc = '%s'\n", GetOptionDesc(i));
    printf("      type = %i\n", GetOptionType(i));

    const int type = GetOptionType(i);

    if (type == opt_error) {
      printf("      BAD OPTION\n");
    }
    else if (type == opt_bool) {
      printf("      BOOL: def = %s\n",
             GetOptionBoolDef(i) ? "true" : "false");
    }
    else if (type == opt_string) {
      printf("      STRING: def = '%s', maxlen = %i\n",
             GetOptionStringDef(i),
             GetOptionStringMaxLen(i));
    }
    else if (type == opt_number) {
      printf("      NUMBER: def = %f, min = %f, max = %f, step = %f\n",
             GetOptionNumberDef(i),
             GetOptionNumberMin(i),
             GetOptionNumberMax(i),
             GetOptionNumberStep(i));
    }
    else if (type == opt_list) {
      printf("      LIST: def = '%s'\n",
             GetOptionListDef(i));

      const int listCount = GetOptionListCount(i);
      for (int li = 0; li < listCount; li++) {
        printf("      %3i: key  = '%s'\n", li,
                                         GetOptionListItemKey(i, li));
        printf("           name = '%s'\n", GetOptionListItemName(i, li));
        printf("           desc = '%s'\n", GetOptionListItemDesc(i, li));
      }
    }
  }
}

static Array GetOptions(int optionCount)
{
	Array a;
	for (int i = 0; i < optionCount; i++) {
		Object option;

		string skey = GetOptionKey(i);
		string sname = GetOptionName(i);
		string sdesc = GetOptionDesc(i);
		string stype = toString(GetOptionType(i));
		
		option.push_back(Pair("key",skey));
		option.push_back(Pair("name",sname));
		option.push_back(Pair("desc",sdesc));
		option.push_back(Pair("type",stype));
		
		printf("    Option #%i\n", i);
		printf("      key  = '%s'\n", skey.c_str());
		
		printf("      name = '%s'\n", sname.c_str());
		printf("      desc = '%s'\n", sdesc.c_str());
		printf("      type = %s\n", stype.c_str());

		const int type = GetOptionType(i);

		if (type == opt_error) {
			printf("      BAD OPTION\n");
		}else if (type == opt_bool) {
			printf("      BOOL: def = %s\n",
				GetOptionBoolDef(i) ? "true" : "false");
		}else if (type == opt_string) {
		  printf("      STRING: def = '%s', maxlen = %i\n",
				 GetOptionStringDef(i),
				 GetOptionStringMaxLen(i));
		}else if (type == opt_number) {
		  printf("      NUMBER: def = %f, min = %f, max = %f, step = %f\n",
				 GetOptionNumberDef(i),
				 GetOptionNumberMin(i),
				 GetOptionNumberMax(i),
				 GetOptionNumberStep(i));
		}else if (type == opt_list) {
			printf("      LIST: def = '%s'\n",
				 GetOptionListDef(i));

			const int listCount = GetOptionListCount(i);
			for (int li = 0; li < listCount; li++) {
				printf("      %3i: key  = '%s'\n", li,
										 GetOptionListItemKey(i, li));
				printf("           name = '%s'\n", GetOptionListItemName(i, li));
				printf("           desc = '%s'\n", GetOptionListItemDesc(i, li));
			}
		}
		a.push_back(option);
	}
	return a;
}

static void CacheMaps(){
	//
	Object maps;
	const int mapCount = GetMapCount();
	maps.push_back(Pair("mapcount",mapCount));

	
	Object o2;
	for (int i = 0; i < mapCount; i++) {
		Object o;
		const string mapName = GetMapName(i);
		o.push_back(Pair("mapname",mapName));

		int hash = GetMapChecksum(i);
		o.push_back(Pair("crchash",hash));

		MapInfo mi;
		mi.description = new char[100];
		strcpy(mi.description,"");
		mi.author = new char[50];
		GetMapInfo(mapName.c_str(),&mi);
		strcpy(mi.author,"");

		o.push_back(Pair("author",mi.author));

		o.push_back(Pair("description",mi.description));

		o.push_back(Pair("extractorradius",mi.extractorRadius));
		o.push_back(Pair("gravity",mi.gravity));
		o.push_back(Pair("height",mi.height));
		o.push_back(Pair("maxmetal",mi.maxMetal));
		o.push_back(Pair("maxwind",mi.maxWind));
		o.push_back(Pair("minwind",mi.minWind));
		o.push_back(Pair("tidalstrength",mi.tidalStrength));
		o.push_back(Pair("width",mi.width));

		o.push_back(Pair("positioncount",mi.posCount));

		Array positions;

		for(int k = 0; k <mi.posCount;k++){
			
			Object sp;
			
			StartPos p = mi.positions[k];

			sp.push_back(Pair("x",p.x));
			sp.push_back(Pair("z",p.z));
			
			positions.push_back(sp);
		}

		o.push_back(Pair("positions",positions));


		o2.push_back(Pair(mapName,o));
		printf("    [map %3i]   %s\n", i, mapName.c_str());
	}

	maps.push_back(Pair("mapdata",o2));
	
	ofstream o("lobby/maps.js");
	write_formatted( maps, o );
	o.close();
}

static void CacheIndex(){
	//
	Object json;

	Init(false, 0);

	string springversion = GetSpringVersion();
	json.push_back(Pair("springversion",springversion));

	printf("GetSpringVersion() = %s\n", GetSpringVersion());

	// map names
	printf("  MAPS\n");

  
	CacheMaps();

  

	Object mods;
	// mod names
	printf("  MODS\n");
	const int modCount = GetPrimaryModCount();

	json.push_back(Pair("modcount",modCount));

	UnInit();

	for (int i = 0; i < modCount; i++) {
		printf("initializing\n");

		Init(false, 0);

		printf("initialized\n");

    
		string modName      = GetPrimaryModName(i);
		string modShortName = GetPrimaryModShortName(i);
		string modVersion   = GetPrimaryModVersion(i);
		string modMutator   = GetPrimaryModMutator(i);
		string modArchive   = GetPrimaryModArchive(i);
		string modGame      = GetPrimaryModGame(i);
		string modShortGame = GetPrimaryModShortGame(i);
		
		printf("adding archives\n");
		AddAllArchives(modArchive.c_str());
		printf("archives added\n");

		Object m;

		m.push_back(Pair("name",		modName));
		m.push_back(Pair("shortname",	modShortName));
		m.push_back(Pair("version",		modVersion));
		m.push_back(Pair("mutator",		modMutator));
		m.push_back(Pair("archive",		modArchive));
		m.push_back(Pair("game",		modGame));
		m.push_back(Pair("shortgame",	modShortGame));

		m.push_back(Pair("crchash",	(int)GetPrimaryModChecksum(i)));

		while (true) {
			int left = ProcessUnitsNoChecksum();
			if (left <= 0) {
				break;
			}
		}

		printf("processed units\n");

		
		Array sides;
		int sidecount = GetSideCount();

		const int unitCount = GetUnitCount();
		
		m.push_back(Pair("unitcount",	unitCount));

		mods.push_back(Pair(modName,m));

		for(int k = 0; k < sidecount; k++){
			string sn = GetSideName(k);
			sides.push_back(sn);
		}
		
		const int modOptCount = GetModOptionCount();
		Array options = GetOptions(modOptCount);
		m.push_back(Pair("luaoptions",options));

		

		m.push_back(Pair("sides",sides));

		

		Object units;

		for (int j = 0; j < unitCount; j++) {

			
			string unitName     = GetUnitName(j);
			string unitFullName = GetFullUnitName(j);
		
			units.push_back(Pair(unitName,unitFullName));
			
			printf("    [unit %3i]   %-16s  <%s>\n", j,
				   unitName.c_str(), unitFullName.c_str());
		}

		m.push_back(Pair("units",units));

		
		{
			// keep 'o' out of scope
			string f = "lobby/";
			f += modName+".";
			f += modVersion+".js";
			ofstream o(f.c_str());
			write_formatted( m, o );
			o.close();
		}


		printf("    [mod %3i]   %-32s  <%s> %s %s %s\n", i,
			   modName.c_str(), modArchive.c_str(),
			   modShortName.c_str(), modVersion.c_str(), modMutator.c_str());

		UnInit();
		printf("uninitialized\n");
	}

	json.push_back(Pair("mods",mods));

	{
		// keep 'o' out of scope
		ofstream o("lobby/index.js");
		write_formatted( json, o );
		o.close();
	}
}

/******************************************************************************/
/******************************************************************************/
