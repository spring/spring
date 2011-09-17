/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef COB_FILE_H
#define COB_FILE_H

#include <vector>
#include <string>
#include <map>

#include "Lua/LuaHashString.h"
#include "CobScriptNames.h"

class CFileHandler;

class CCobFile
{
public:
	CCobFile(CFileHandler& in, std::string name);
	~CCobFile();

	int GetFunctionId(const std::string& name);


	std::vector<std::string> scriptNames;
	std::vector<int> scriptOffsets;
	/// Assumes that the scripts are sorted by offset in the file
	std::vector<int> scriptLengths;
	std::vector<std::string> pieceNames;
	std::vector<int> scriptIndex;
	std::vector<int> sounds;
	std::map<std::string, int> scriptMap;
	std::vector<LuaHashString> luaScripts;
	int* code;
	int numStaticVars;
	std::string name;
};

#endif // COB_FILE_H
