#ifndef __COB_FILE_H__
#define __COB_FILE_H__

#include <vector>
#include <string>
#include <map>

#include "Lua/LuaHashString.h"
// TODO: use this include in rest of engine directly,
// instead of including it through #include "CobFile.h"
#include "UnitScriptNames.h"

//0 = none
//1 = script calls
//2 = show every instruction
#define COB_DEBUG	0

// Should return true for scripts that should have debug output.
#define COB_DEBUG_FILTER false

class CFileHandler;

class CCobFile
{
public:
	std::vector<std::string> scriptNames;
	std::vector<int> scriptOffsets;
	std::vector<int> scriptLengths;			//Assumes that the scripts are sorted by offset in the file
	std::vector<std::string> pieceNames;
	std::vector<int> scriptIndex;
	std::vector<int> sounds;
	std::map<std::string, int> scriptMap;
	std::vector<LuaHashString> luaScripts;
	int* code;
	int numStaticVars;
	std::string name;
public:
	CCobFile(CFileHandler &in, std::string name);
	~CCobFile();
	int GetFunctionId(const std::string &name);
};

#endif // __COB_FILE_H__
