#if 0
################################################################################
file=lua2php.cpp

g++ -g -I../../../rts/System $file ../../../game/unitsync.so

mod=$1
if [ -z $mod ]; then
  echo "usage:  $0 <ModArchiveName>    {ex: ca.sdd, ba631.sd7}"
  exit 1
fi

echo ./a.out $mod
./a.out $mod

exit

################################################################################
#endif


/******************************************************************************/
/******************************************************************************/
//  Simple file to help test unitsync, compile with:
//
//    g++ -I../../../rts/System lua2php.cxx ../../../game/unitsync.so
//


#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <vector>
using namespace std;

#include "../unitsync_api.h"


/******************************************************************************/
/******************************************************************************/

static void OutputPHPTable(const string& indent);

static FILE* outfile = NULL;

#define OUTF(...) fprintf(outfile, __VA_ARGS__)


/******************************************************************************/
/******************************************************************************/

// from VFSModes.h
#define SPRING_VFS_RAW  "r"
#define SPRING_VFS_MOD  "M"
#define SPRING_VFS_MAP  "m"
#define SPRING_VFS_BASE "b"
#define SPRING_VFS_NONE " "
#define SPRING_VFS_MOD_BASE   SPRING_VFS_MOD  SPRING_VFS_BASE
#define SPRING_VFS_ZIP        SPRING_VFS_MOD  SPRING_VFS_MAP  SPRING_VFS_BASE


int main(int argc, char** argv)
{
  if (argc < 2) {
    printf("usage:  %s <mod>\n", argv[0]);
    exit(1);
  }
  const string mod = argv[1];
  printf("MOD = %s\n", mod.c_str());

  const string outName = mod + ".php";
  printf("outName = %s\n", outName.c_str());
  outfile = fopen(outName.c_str(), "w");
  if (outfile == NULL) {
    perror("fopen");
    return 1;
  }

  Init(false, 0);

  printf("GetSpringVersion() = %s\n", GetSpringVersion());

  // load the mod archives
  AddAllArchives(mod.c_str());

  // print the defs
  lpOpenFile("gamedata/defs.lua", SPRING_VFS_MOD_BASE , SPRING_VFS_ZIP);
  if (!lpExecute()) {
    printf(lpErrorLog());
  }
  else {
    lpRootTable();
    OUTF("<?php\n");
    OUTF("$defs = array(\n");
    OutputPHPTable("\t");
    OUTF(");\n");
    OUTF("?>\n");
  }
  lpClose();

  UnInit();

  fclose(outfile);

  return 0;
}


/******************************************************************************/
/******************************************************************************/

static void GetIntKeys(vector<int>& keys)
{
  const int count = lpGetIntKeyListCount();
  for (int i = 0; i < count; i++) {
    keys.push_back(lpGetIntKeyListEntry(i));
  }
}


static void GetStrKeys(vector<string>& keys)
{
  const int count = lpGetStrKeyListCount();
  for (int i = 0; i < count; i++) {
    keys.push_back(lpGetStrKeyListEntry(i));
  }
}


static inline std::string IntToString(int i, const std::string& format = "%i")
{
	char buf[64];
	snprintf(buf, sizeof(buf), format.c_str(), i);
	return std::string(buf);
}


// from lua.h
#define LUA_TNONE          (-1)
#define LUA_TNIL           0
#define LUA_TBOOLEAN       1
#define LUA_TLIGHTUSERDATA 2
#define LUA_TNUMBER        3
#define LUA_TSTRING        4
#define LUA_TTABLE         5
#define LUA_TFUNCTION      6
#define LUA_TUSERDATA      7
#define LUA_TTHREAD        8


static string SafeStr(const string& str)
{
  string newStr;
  for (unsigned int i = 0; i < str.size(); i++) {
    const char c = str[i];
    if (c == '"')  {
      newStr.push_back('\\');
    }
    newStr.push_back(c);
  }
  return newStr;
}


static void OutputPHPTable(const string& indent)
{
  vector<int>    intKeys; GetIntKeys(intKeys);
  vector<string> strKeys; GetStrKeys(strKeys);

  const char* ind = indent.c_str();

  for (int i = 0; i < intKeys.size(); i++) {
    const int key = intKeys[i];
    const int type = lpGetIntKeyType(key);
    if (type == LUA_TTABLE) {
      lpSubTableInt(key);
      OUTF("%s%i => array(\n", ind, key);
      OutputPHPTable(indent + "\t");
      OUTF("%s),\n", ind);
      lpPopTable();
    } else {
      if (type == LUA_TNUMBER) {
        OUTF("%s%i => %g,\n",   ind, key, lpGetIntKeyFloatVal(key, 0.0f));
      } else if (type == LUA_TBOOLEAN) {
        OUTF("%s%i => %s,\n",   ind, key, lpGetIntKeyBoolVal(key, 0) ? "true" : "false");
      } else if (type == LUA_TSTRING) {
        OUTF("%s%i => \"%s\",\n", ind, key,
               SafeStr(lpGetIntKeyStrVal(key, "BOGUS")).c_str());
      } else {
        OUTF("//%s%i => bad type (%i)\n", ind, key, type);
      }
    }
  }

  for (int i = 0; i < strKeys.size(); i++) {
    const char* key = strKeys[i].c_str();
    const int type = lpGetStrKeyType(key);
    if (type == LUA_TTABLE) {
      lpSubTableStr(key);
      OUTF("%s\"%s\" => array(\n", ind, key);
      OutputPHPTable(indent + "\t");
      OUTF("%s),\n", ind);
      lpPopTable();
    } else {
      if (type == LUA_TNUMBER) {
        OUTF("%s\"%s\" => %g,\n",   ind, key, lpGetStrKeyFloatVal(key, 0.0f));
      } else if (type == LUA_TBOOLEAN) {
        OUTF("%s\"%s\" => %s,\n",   ind, key, lpGetStrKeyBoolVal(key, 0) ? "true" : "false");
      } else if (type == LUA_TSTRING) {
        OUTF("%s\"%s\" => \"%s\",\n", ind, key,
               SafeStr(lpGetStrKeyStrVal(key, "BOGUS")).c_str());
      } else {
        OUTF("//%s\"%s\" => bad type (%i)\n", ind, key, type);
      }
    }
  }
}


/******************************************************************************/
/******************************************************************************/

